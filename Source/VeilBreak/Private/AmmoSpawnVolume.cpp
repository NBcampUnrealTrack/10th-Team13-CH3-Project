#include "AmmoSpawnVolume.h"

#include "AmmoItem.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

AAmmoSpawnVolume::AAmmoSpawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnBox = CreateDefaultSubobject<UBoxComponent>(
		TEXT("SpawnBox")
	);
	SetRootComponent(SpawnBox);

	// 기본 크기: 가로 1000, 세로 1000, 높이 600
	SpawnBox->InitBoxExtent(FVector(500.0f, 500.0f, 300.0f));

	// 박스 자체는 생성 범위 표시용
	SpawnBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBox->SetGenerateOverlapEvents(false);
	SpawnBox->SetCanEverAffectNavigation(false);
}

void AAmmoSpawnVolume::SpawnAmmo()
{
	if (bSpawnRequested)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World || !AmmoItemClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AmmoSpawn] %s: World 또는 AmmoItemClass 없음"),
			*GetName()
		);
		return;
	}

	bSpawnRequested = true;

	const int32 TargetCount = FMath::Max(SpawnCount, 1);
	const int32 AttemptsPerItem = FMath::Max(MaxAttemptsPerItem, 1);

	int32 CreatedCount = 0;

	for (int32 ItemIndex = 0; ItemIndex < TargetCount; ++ItemIndex)
	{
		for (int32 Attempt = 0; Attempt < AttemptsPerItem; ++Attempt)
		{
			FVector SpawnLocation;

			if (!FindSpawnLocation(SpawnLocation))
			{
				continue;
			}

			const FRotator SpawnRotation(
				0.0f,
				FMath::FRandRange(0.0f, 360.0f),
				0.0f
			);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;

			// 위치는 위에서 검사했으며, 생성 시 추가 충돌도 확인
			SpawnParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

			AAmmoItem* Item = World->SpawnActor<AAmmoItem>(
				AmmoItemClass,
				SpawnLocation,
				SpawnRotation,
				SpawnParams
			);

			if (!IsValid(Item))
			{
				continue;
			}

			SpawnedItems.Add(TWeakObjectPtr<AAmmoItem>(Item));
			++CreatedCount;
			break;
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[AmmoSpawn] %s: 목표=%d / 실제 생성=%d"),
		*GetName(),
		TargetCount,
		CreatedCount
	);

	if (CreatedCount < TargetCount)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AmmoSpawn] 바닥 충돌, 박스 위치, 생성 간격 및 확보 공간을 확인하세요.")
		);
	}

	// 생성 이후 0.5초마다 남은 아이템 확인
	GetWorldTimerManager().SetTimer(
		AmmoCheckTimerHandle,
		this,
		&AAmmoSpawnVolume::CheckRemainingAmmo,
		0.5f,
		true
	);
}

bool AAmmoSpawnVolume::FindSpawnLocation(
	FVector& OutLocation
) const
{
	UWorld* World = GetWorld();

	if (!World || !IsValid(SpawnBox))
	{
		return false;
	}

	const FVector Extent = SpawnBox->GetUnscaledBoxExtent();
	const FTransform BoxTransform = SpawnBox->GetComponentTransform();

	// 박스 내부에서 가로·세로 좌표 선택
	const float LocalX = FMath::FRandRange(-Extent.X, Extent.X);
	const float LocalY = FMath::FRandRange(-Extent.Y, Extent.Y);

	// 선택한 위치의 박스 상단에서 하단으로 탐색
	const FVector TraceStart = BoxTransform.TransformPosition(
		FVector(LocalX, LocalY, Extent.Z)
	);

	const FVector TraceEnd = BoxTransform.TransformPosition(
		FVector(LocalX, LocalY, -Extent.Z)
	);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// 고정된 지형과 구조물을 바닥 후보로 사용
	FCollisionObjectQueryParams GroundObjects;
	GroundObjects.AddObjectTypesToQuery(ECC_WorldStatic);

	FHitResult GroundHit;

	const bool bFoundGround = World->LineTraceSingleByObjectType(
		GroundHit,
		TraceStart,
		TraceEnd,
		GroundObjects,
		QueryParams
	);

	if (!bFoundGround || !GroundHit.bBlockingHit)
	{
		return false;
	}

	// 급경사와 벽은 제외
	const float SafeSlope = FMath::Clamp(
		MaxGroundSlopeDegrees,
		0.0f,
		60.0f
	);

	const float MinNormalZ = FMath::Cos(
		FMath::DegreesToRadians(SafeSlope)
	);

	if (GroundHit.ImpactNormal.Z < MinNormalZ)
	{
		return false;
	}

	const float SafeRadius = FMath::Max(ClearanceRadius, 1.0f);
	const float SafeGap = FMath::Max(GroundGap, 1.0f);

	// 경사면에서도 바닥과 겹치지 않도록 표면 법선 방향으로 띄움
	const FVector Candidate =
		GroundHit.ImpactPoint
		+ GroundHit.ImpactNormal * (SafeRadius + SafeGap);

	// 최종 위치도 박스 내부인지 확인
	const FVector LocalCandidate =
		BoxTransform.InverseTransformPosition(Candidate);

	if (FMath::Abs(LocalCandidate.X) > Extent.X
		|| FMath::Abs(LocalCandidate.Y) > Extent.Y
		|| FMath::Abs(LocalCandidate.Z) > Extent.Z)
	{
		return false;
	}

	// 같은 볼륨에서 생성한 아이템과 간격 확보
	const float SpacingSquared =
		FMath::Square(FMath::Max(MinSpacing, 0.0f));

	for (const TWeakObjectPtr<AAmmoItem>& ItemReference : SpawnedItems)
	{
		const AAmmoItem* Item = ItemReference.Get();

		if (IsValid(Item)
			&& FVector::DistSquared(
				Candidate,
				Item->GetActorLocation()
			) < SpacingSquared)
		{
			return false;
		}
	}

	// 주변 지형·물체·캐릭터와 공간이 겹치는지 검사
	FCollisionObjectQueryParams ObstacleObjects;
	ObstacleObjects.AddObjectTypesToQuery(ECC_WorldStatic);
	ObstacleObjects.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObstacleObjects.AddObjectTypesToQuery(ECC_Pawn);

	const bool bOccupied = World->OverlapAnyTestByObjectType(
		Candidate,
		FQuat::Identity,
		ObstacleObjects,
		FCollisionShape::MakeSphere(SafeRadius),
		QueryParams
	);

	if (bOccupied)
	{
		return false;
	}

	OutLocation = Candidate;
	return true;
}

void AAmmoSpawnVolume::ClearSpawnedAmmo()
{
	// 먼저 재생성 중단
	bSpawnRequested = false;

	GetWorldTimerManager().ClearTimer(AmmoCheckTimerHandle);
	GetWorldTimerManager().ClearTimer(AmmoRespawnTimerHandle);

	// 이 볼륨에서 생성한 아이템 제거
	for (const TWeakObjectPtr<AAmmoItem>& ItemReference : SpawnedItems)
	{
		AAmmoItem* Item = ItemReference.Get();

		if (IsValid(Item))
		{
			Item->Destroy();
		}
	}

	SpawnedItems.Empty();
}

void AAmmoSpawnVolume::CheckRemainingAmmo()
{
	// 생성이 시작되지 않았거나 정리된 볼륨은 처리하지 않음
	if (!bSpawnRequested)
	{
		return;
	}

	// 이미 제거된 아이템을 목록에서 제외
	SpawnedItems.RemoveAll(
		[](const TWeakObjectPtr<AAmmoItem>& ItemReference)
		{
			return !ItemReference.IsValid();
		}
	);

	// 하나라도 남아 있으면 재생성하지 않음
	if (!SpawnedItems.IsEmpty())
	{
		return;
	}

	// 이미 재생성을 예약했다면 중복 예약하지 않음
	if (GetWorldTimerManager().IsTimerActive(AmmoRespawnTimerHandle))
	{
		return;
	}

	const float SafeDelay = FMath::Max(RespawnDelay, 0.1f);

	GetWorldTimerManager().SetTimer(
		AmmoRespawnTimerHandle,
		this,
		&AAmmoSpawnVolume::RespawnAmmo,
		SafeDelay,
		false
	);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[AmmoSpawn] %s: 남은 아이템 없음. %.1f초 후 재생성"),
		*GetName(),
		SafeDelay
	);
}

void AAmmoSpawnVolume::RespawnAmmo()
{
	if (!bSpawnRequested)
	{
		return;
	}

	// 재생성 직전에도 남은 아이템 확인
	SpawnedItems.RemoveAll(
		[](const TWeakObjectPtr<AAmmoItem>& ItemReference)
		{
			return !ItemReference.IsValid();
		}
	);

	if (!SpawnedItems.IsEmpty())
	{
		return;
	}

	// 기존 확인 타이머를 정리하고 생성 제한 해제
	GetWorldTimerManager().ClearTimer(AmmoCheckTimerHandle);
	GetWorldTimerManager().ClearTimer(AmmoRespawnTimerHandle);

	bSpawnRequested = false;

	// 기존 생성 함수를 재사용
	// 이 함수 마지막에서 확인 타이머도 다시 시작됨
	SpawnAmmo();
}

void AAmmoSpawnVolume::EndPlay(
	const EEndPlayReason::Type EndPlayReason
)
{
	bSpawnRequested = false;

	GetWorldTimerManager().ClearTimer(AmmoCheckTimerHandle);
	GetWorldTimerManager().ClearTimer(AmmoRespawnTimerHandle);

	Super::EndPlay(EndPlayReason);
}