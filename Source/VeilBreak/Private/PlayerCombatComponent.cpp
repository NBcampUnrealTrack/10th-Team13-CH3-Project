#include "PlayerCombatComponent.h"

#include "BossBerserkActor.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerSkillComponent.h"

UPlayerCombatComponent::UPlayerCombatComponent()
{
	// 입력과 타이머로 처리하므로 Tick 비활성화
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// 탄약 설정 보정
	CylinderCapacity = FMath::Max(CylinderCapacity, 1);
	MaxReserveAmmo = FMath::Max(MaxReserveAmmo, 0);

	// 시작 탄약 설정
	CurrentAmmo = CylinderCapacity;
	ReserveAmmo = MaxReserveAmmo;

	// 전투 상태 초기화
	bCanFire = true;
	bIsReloading = false;
	CurrentDamageMultiplier = 1.0f;
	CurrentReloadTimeMultiplier = 1.0f;

	// 과녁 중복 기록 초기화
	HitUltimateTargets.Empty();

	// 초기 탄약 정보 전달
	BroadcastAmmoChanged();
}

void UPlayerCombatComponent::TryFire()
{
	if (!bCanFire || GetWorld() == nullptr)
	{
		return;
	}

	if (bIsReloading)
	{
		// 사격 시 재장전 취소
		CancelReload();
	}

	if (CurrentAmmo <= 0)
	{
		// 탄약이 없으면 재장전 시도
		StartReload();
		return;
	}

	// 이벤트 처리 도중 중복 사격 방지
	bCanFire = false;

	// 탄약 소모 및 UI 갱신
	--CurrentAmmo;
	BroadcastAmmoChanged();

	// 명중 검사와 반동 처리
	PerformHitScan();
	ApplyRecoil();

	// 발사 애니메이션 및 효과 이벤트
	OnWeaponFired.Broadcast();

	// 발사 간격 이후 사격 가능 상태 복구
	GetWorld()->GetTimerManager().SetTimer(
		FireCooldownTimerHandle,
		this,
		&UPlayerCombatComponent::ResetFireCooldown,
		FMath::Max(FireInterval, 0.01f),
		false
	);
}

void UPlayerCombatComponent::StartReload()
{
	if (
		bIsReloading ||
		CurrentAmmo >= CylinderCapacity ||
		ReserveAmmo <= 0 ||
		GetWorld() == nullptr
		)
	{
		return;
	}

	// 재장전 시작
	bIsReloading = true;

	GetWorld()->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&UPlayerCombatComponent::HandleReloadRound,
		FMath::Max(
			ReloadTimePerRound * CurrentReloadTimeMultiplier,
			0.01f
		),
		false
	);

	// UI와 애니메이션에 시작 전달
	OnReloadStateChanged.Broadcast(true);
}

void UPlayerCombatComponent::CancelReload()
{
	if (!bIsReloading)
	{
		return;
	}

	// 재장전 상태 및 타이머 정리
	FinishReload();
}

int32 UPlayerCombatComponent::AddReserveAmmo(int32 AmmoAmount)
{
	if (AmmoAmount <= 0)
	{
		return 0;
	}

	// 예비 탄약의 남은 공간만큼 추가
	const int32 AvailableSpace = FMath::Max(
		MaxReserveAmmo - ReserveAmmo,
		0
	);

	const int32 AddedAmmo = FMath::Min(
		AmmoAmount,
		AvailableSpace
	);

	if (AddedAmmo > 0)
	{
		ReserveAmmo += AddedAmmo;
		BroadcastAmmoChanged();
	}

	return AddedAmmo;
}

int32 UPlayerCombatComponent::GetCurrentAmmo() const
{
	// 현재 장전된 탄약 반환
	return CurrentAmmo;
}

int32 UPlayerCombatComponent::GetReserveAmmo() const
{
	// 현재 예비 탄약 반환
	return ReserveAmmo;
}

int32 UPlayerCombatComponent::GetCylinderCapacity() const
{
	// 최대 장전 수 반환
	return CylinderCapacity;
}

bool UPlayerCombatComponent::IsReloading() const
{
	// 재장전 상태 반환
	return bIsReloading;
}

void UPlayerCombatComponent::SetUltimateBuffActive(
	bool bEnableUltimateBuff
)
{
	// 공격력 배율 변경
	CurrentDamageMultiplier = bEnableUltimateBuff
		? UltimateDamageMultiplier
		: 1.0f;

	// 재장전 시간 배율 변경
	CurrentReloadTimeMultiplier = bEnableUltimateBuff
		? UltimateReloadTimeMultiplier
		: 1.0f;
}

void UPlayerCombatComponent::PerformHitScan()
{
	// 사격한 플레이어 확인
	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	if (!IsValid(OwnerPawn))
	{
		return;
	}

	APlayerController* PlayerController =
		Cast<APlayerController>(OwnerPawn->GetController());

	UWorld* World = GetWorld();

	if (!IsValid(PlayerController) || World == nullptr)
	{
		return;
	}

	// 카메라 위치와 방향 조회
	FVector ViewLocation;
	FRotator ViewRotation;

	PlayerController->GetPlayerViewPoint(
		ViewLocation,
		ViewRotation
	);

	const FVector ShotDirection = ViewRotation.Vector();

	const FVector TraceEnd =
		ViewLocation + ShotDirection * TraceDistance;

	// 자기 자신은 명중 검사에서 제외
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerPawn);

	FHitResult HitResult;

	// 기존 보스 피격 채널 유지
	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		ViewLocation,
		TraceEnd,
		ECC_GameTraceChannel1,
		QueryParams
	);

	const FVector DebugTraceEnd =
		bHit ? HitResult.ImpactPoint : TraceEnd;

	if (bDrawDebugTrace)
	{
		// 명중 여부에 따라 디버그 선 색상 변경
		DrawDebugLine(
			World,
			ViewLocation,
			DebugTraceEnd,
			bHit ? FColor::Red : FColor::Green,
			false,
			1.0f,
			0,
			2.0f
		);
	}

	if (!bHit)
	{
		return;
	}

	AActor* HitActor = HitResult.GetActor();

	if (!IsValid(HitActor))
	{
		return;
	}

	// 이번 발사에 사용할 공격 데미지를 미리 확정
	const float ShotDamage = BaseDamage * CurrentDamageMultiplier;

	// 피해로 보스가 파괴되더라도 사용할 수 있도록 위치 저장
	const FVector HitLocation = HitResult.ImpactPoint;

	// 지정한 보스 BP와 그 자식 클래스인지 검사
	// 과녁은 보스 본체 히트마커 대상에서 제외
	const bool bHitBoss =
		BossActorClass.Get() != nullptr &&
		HitActor->IsA(BossActorClass.Get()) &&
		!HitActor->IsA<ABossBerserkActor>();

	// 과녁이 피해로 파괴되기 전에 스택 처리
	HandleUltimateTargetHit(HitActor);

	if (!IsValid(HitActor))
	{
		return;
	}

	// 명중 대상에 공격 데미지 전달
	UGameplayStatics::ApplyPointDamage(
		HitActor,
		ShotDamage,
		ShotDirection,
		HitResult,
		PlayerController,
		OwnerPawn,
		UDamageType::StaticClass()
	);

	if (bHitBoss)
	{
		// 보스 명중 시 UI에 공격 데미지와 월드 위치 전달
		// 실제 HP 감소량이나 피해 승인 여부를 의미하지 않음
		OnBossHitConfirmed.Broadcast(
			ShotDamage,
			HitLocation
		);
	}
}

void UPlayerCombatComponent::HandleUltimateTargetHit(
	AActor* HitActor
)
{
	if (!IsValid(HitActor))
	{
		return;
	}

	if (!HitActor->IsA<ABossBerserkActor>())
	{
		// 과녁만 스택 획득 대상으로 인정
		return;
	}

	// 파괴된 과녁의 기록 제거
	for (
		auto TargetIterator = HitUltimateTargets.CreateIterator();
		TargetIterator;
		++TargetIterator
		)
	{
		if (!TargetIterator->IsValid())
		{
			TargetIterator.RemoveCurrent();
		}
	}

	const TWeakObjectPtr<AActor> TargetActor(HitActor);

	if (HitUltimateTargets.Contains(TargetActor))
	{
		// 같은 과녁의 중복 스택 획득 방지
		return;
	}

	AActor* OwnerActor = GetOwner();

	if (!IsValid(OwnerActor))
	{
		return;
	}

	// 실제 사격한 플레이어의 스킬 컴포넌트 조회
	UPlayerSkillComponent* SkillComponent =
		OwnerActor->FindComponentByClass<UPlayerSkillComponent>();

	if (!IsValid(SkillComponent))
	{
		return;
	}

	if (SkillComponent->IsUltimateActive())
	{
		// 궁극기 활성 중에는 스택 획득 불가
		return;
	}

	if (
		SkillComponent->GetCurrentTargetStacks() >=
		SkillComponent->GetRequiredTargetStacks()
		)
	{
		// 최대 스택이면 추가하지 않음
		return;
	}

	// 이벤트 처리 중 동일 과녁이 중복 처리되지 않도록 기록
	HitUltimateTargets.Add(TargetActor);

	// 스택 증가 및 기존 스택 UI 이벤트 전달
	SkillComponent->AddUltimateTargetStack();
}

void UPlayerCombatComponent::ApplyRecoil()
{
	// 플레이어와 컨트롤러 확인
	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	if (!IsValid(OwnerPawn))
	{
		return;
	}

	APlayerController* PlayerController =
		Cast<APlayerController>(OwnerPawn->GetController());

	if (!IsValid(PlayerController))
	{
		return;
	}

	// 수직 반동
	PlayerController->AddPitchInput(-VerticalRecoil);

	// 좌우 무작위 반동
	const float RandomHorizontalRecoil = FMath::FRandRange(
		-HorizontalRecoil,
		HorizontalRecoil
	);

	PlayerController->AddYawInput(RandomHorizontalRecoil);
}

void UPlayerCombatComponent::ResetFireCooldown()
{
	// 사격 가능 상태 복구
	bCanFire = true;
}

void UPlayerCombatComponent::HandleReloadRound()
{
	if (!bIsReloading)
	{
		return;
	}

	if (CurrentAmmo >= CylinderCapacity || ReserveAmmo <= 0)
	{
		FinishReload();
		return;
	}

	// 예비 탄약 한 발을 실린더로 이동
	++CurrentAmmo;
	--ReserveAmmo;
	BroadcastAmmoChanged();

	if (!bIsReloading)
	{
		// 이벤트 처리 중 취소됐다면 중단
		return;
	}

	if (CurrentAmmo >= CylinderCapacity || ReserveAmmo <= 0)
	{
		FinishReload();
		return;
	}

	// 다음 한 발 장전 예약
	GetWorld()->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&UPlayerCombatComponent::HandleReloadRound,
		FMath::Max(
			ReloadTimePerRound * CurrentReloadTimeMultiplier,
			0.01f
		),
		false
	);
}

void UPlayerCombatComponent::FinishReload()
{
	if (!bIsReloading)
	{
		return;
	}

	// 남아 있는 재장전 타이머 제거
	if (GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ReloadTimerHandle
		);
	}

	// 재장전 종료 전달
	bIsReloading = false;
	OnReloadStateChanged.Broadcast(false);
}

void UPlayerCombatComponent::BroadcastAmmoChanged()
{
	// 현재 탄약 상태 전달
	OnAmmoChanged.Broadcast(
		CurrentAmmo,
		ReserveAmmo
	);
}