#include "PlayerCombatComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UPlayerCombatComponent::UPlayerCombatComponent()
{
	// 전투 기능은 입력과 타이머로 처리하므로 Tick 비활성화
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// 잘못된 탄약 설정이 들어오지 않도록 최소값 보정
	CylinderCapacity = FMath::Max(
		CylinderCapacity,
		1
	);

	MaxReserveAmmo = FMath::Max(
		MaxReserveAmmo,
		0
	);

	// 게임 시작 시 실린더와 예비 탄약을 최대치로 설정
	CurrentAmmo = CylinderCapacity;
	ReserveAmmo = MaxReserveAmmo;

	// 게임 시작 시 전투 상태 초기화
	bCanFire = true;
	bIsReloading = false;

	// 초기 탄약 정보를 UI에 전달
	BroadcastAmmoChanged();
}

void UPlayerCombatComponent::TryFire()
{
	if (!bCanFire)
	{
		// 발사 간격이 끝나지 않았다면 사격하지 않음
		return;
	}

	if (bIsReloading)
	{
		// 재장전 도중 사격하면 재장전을 먼저 취소
		CancelReload();
	}

	if (CurrentAmmo <= 0)
	{
		// 실린더가 비어 있으면 자동으로 재장전 시도
		StartReload();
		return;
	}

	// 실린더에서 한 발 소모
	--CurrentAmmo;

	// 변경된 탄약 정보를 UI에 전달
	BroadcastAmmoChanged();

	// 카메라 중앙을 기준으로 명중 판정 실행
	PerformHitScan();

	// 사격 후 카메라에 강한 반동 적용
	ApplyRecoil();

	// 사격 애니메이션과 효과에 발사 사실 전달
	OnWeaponFired.Broadcast();

	// 다음 발사까지 사격 불가능 상태로 변경
	bCanFire = false;

	// 발사 간격이 끝나면 다시 사격할 수 있도록 타이머 실행
	GetWorld()->GetTimerManager().SetTimer(
		FireCooldownTimerHandle,
		this,
		&UPlayerCombatComponent::ResetFireCooldown,
		FireInterval,
		false
	);
}

void UPlayerCombatComponent::StartReload()
{
	if (bIsReloading)
	{
		// 이미 재장전 중이면 중복 실행하지 않음
		return;
	}

	if (CurrentAmmo >= CylinderCapacity)
	{
		// 실린더가 가득 차 있으면 재장전하지 않음
		return;
	}

	if (ReserveAmmo <= 0)
	{
		// 예비 탄약이 없으면 재장전하지 않음
		return;
	}

	// 한 발씩 재장전하는 상태로 변경
	bIsReloading = true;

	// 재장전 시작 상태를 UI와 애니메이션에 전달
	OnReloadStateChanged.Broadcast(true);

	// 설정된 시간이 지나면 총알 한 발 장전
	GetWorld()->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&UPlayerCombatComponent::HandleReloadRound,
		ReloadTimePerRound,
		false
	);
}

void UPlayerCombatComponent::CancelReload()
{
	if (!bIsReloading)
	{
		// 재장전 중이 아니라면 취소하지 않음
		return;
	}

	// 진행 중인 한 발 장전 타이머 제거
	GetWorld()->GetTimerManager().ClearTimer(
		ReloadTimerHandle
	);

	// 재장전 상태를 종료
	FinishReload();
}

int32 UPlayerCombatComponent::AddReserveAmmo(
	int32 AmmoAmount
)
{
	if (AmmoAmount <= 0)
	{
		// 보급량이 0 이하라면 탄약을 추가하지 않음
		return 0;
	}

	// 탄약 추가 전 예비 탄약 수 저장
	const int32 PreviousReserveAmmo =
		ReserveAmmo;

	// 예비 탄약이 최대치를 넘지 않도록 제한
	ReserveAmmo = FMath::Clamp(
		ReserveAmmo + AmmoAmount,
		0,
		MaxReserveAmmo
	);

	// 실제로 추가된 탄약 수 계산
	const int32 AddedAmmo =
		ReserveAmmo - PreviousReserveAmmo;

	if (AddedAmmo > 0)
	{
		// 실제로 탄약이 추가됐을 때만 UI에 전달
		BroadcastAmmoChanged();
	}

	// 보급 아이템이 습득 성공 여부를 판단하도록 반환
	return AddedAmmo;
}

int32 UPlayerCombatComponent::GetCurrentAmmo() const
{
	// UI에서 사용할 현재 실린더 탄약 수 반환
	return CurrentAmmo;
}

int32 UPlayerCombatComponent::GetReserveAmmo() const
{
	// UI에서 사용할 현재 예비 탄약 수 반환
	return ReserveAmmo;
}

int32 UPlayerCombatComponent::
GetCylinderCapacity() const
{
	// UI에서 사용할 최대 실린더 탄약 수 반환
	return CylinderCapacity;
}

bool UPlayerCombatComponent::IsReloading() const
{
	// 현재 재장전 상태 반환
	return bIsReloading;
}

void UPlayerCombatComponent::PerformHitScan()
{
	// 전투 컴포넌트를 소유한 플레이어 확인
	APawn* OwnerPawn =
		Cast<APawn>(GetOwner());

	if (OwnerPawn == nullptr)
	{
		// 소유자가 Pawn이 아니면 사격 판정 중단
		return;
	}

	// 현재 플레이어를 조종하는 컨트롤러 확인
	APlayerController* PlayerController =
		Cast<APlayerController>(
			OwnerPawn->GetController()
		);

	if (PlayerController == nullptr)
	{
		// 플레이어 컨트롤러가 없으면 사격 판정 중단
		return;
	}

	// 플레이어 카메라의 위치와 회전 가져오기
	FVector ViewLocation;
	FRotator ViewRotation;

	PlayerController->GetPlayerViewPoint(
		ViewLocation,
		ViewRotation
	);

	// 카메라가 바라보는 방향으로 사격 종료 위치 계산
	const FVector TraceEnd =
		ViewLocation +
		ViewRotation.Vector() * TraceDistance;

	// 자기 자신이 사격 판정에 걸리지 않도록 제외
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	// 카메라 중앙에서 직선 명중 판정 실행
	FHitResult HitResult;

	const bool bHit =
		GetWorld()->LineTraceSingleByChannel(
			HitResult,
			ViewLocation,
			TraceEnd,
			ECC_Visibility,
			QueryParams
		);

	// 실제 판정이 끝나는 위치 결정
	const FVector DebugTraceEnd =
		bHit
		? HitResult.ImpactPoint
		: TraceEnd;

	if (bDrawDebugTrace)
	{
		// 명중 시 빨간색, 빗나가면 초록색 선 표시
		DrawDebugLine(
			GetWorld(),
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
		// 적이나 지형에 명중하지 않았다면 피해 없음
		return;
	}

	AActor* HitActor =
		HitResult.GetActor();

	if (HitActor == nullptr)
	{
		// 명중한 액터가 없다면 피해 적용 중단
		return;
	}

	// 실제 총알이 진행한 방향 계산
	const FVector ShotDirection =
		(DebugTraceEnd - ViewLocation)
		.GetSafeNormal();

	// 명중한 액터에 기본 공격력 20의 점 피해 적용
	UGameplayStatics::ApplyPointDamage(
		HitActor,
		BaseDamage,
		ShotDirection,
		HitResult,
		PlayerController,
		GetOwner(),
		UDamageType::StaticClass()
	);
}

void UPlayerCombatComponent::ApplyRecoil()
{
	// 전투 컴포넌트를 소유한 플레이어 확인
	APawn* OwnerPawn =
		Cast<APawn>(GetOwner());

	if (OwnerPawn == nullptr)
	{
		// 소유자가 Pawn이 아니면 반동 적용 중단
		return;
	}

	// 현재 플레이어의 컨트롤러 확인
	APlayerController* PlayerController =
		Cast<APlayerController>(
			OwnerPawn->GetController()
		);

	if (PlayerController == nullptr)
	{
		// 플레이어 컨트롤러가 없으면 반동 적용 중단
		return;
	}

	// 카메라를 위쪽으로 올리는 강한 수직 반동 적용
	PlayerController->AddPitchInput(
		-VerticalRecoil
	);

	// 매 발 좌우 방향이 달라지는 무작위 반동 계산
	const float RandomHorizontalRecoil =
		FMath::FRandRange(
			-HorizontalRecoil,
			HorizontalRecoil
		);

	// 계산된 좌우 반동을 카메라에 적용
	PlayerController->AddYawInput(
		RandomHorizontalRecoil
	);
}

void UPlayerCombatComponent::ResetFireCooldown()
{
	// 발사 간격이 끝났으므로 다시 사격 가능
	bCanFire = true;
}

void UPlayerCombatComponent::HandleReloadRound()
{
	if (!bIsReloading)
	{
		// 재장전이 취소됐다면 총알을 추가하지 않음
		return;
	}

	if (
		CurrentAmmo >= CylinderCapacity ||
		ReserveAmmo <= 0
		)
	{
		// 실린더가 가득 찼거나 예비 탄약이 없으면 종료
		FinishReload();
		return;
	}

	// 예비 탄약 한 발을 실린더로 이동
	++CurrentAmmo;
	--ReserveAmmo;

	// 변경된 탄약 정보를 UI에 전달
	BroadcastAmmoChanged();

	if (
		CurrentAmmo >= CylinderCapacity ||
		ReserveAmmo <= 0
		)
	{
		// 더 장전할 수 없다면 재장전 종료
		FinishReload();
		return;
	}

	// 다음 한 발을 장전하기 위한 타이머 실행
	GetWorld()->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&UPlayerCombatComponent::HandleReloadRound,
		ReloadTimePerRound,
		false
	);
}

void UPlayerCombatComponent::FinishReload()
{
	if (!bIsReloading)
	{
		// 이미 재장전이 종료됐다면 중복 처리하지 않음
		return;
	}

	// 재장전 상태 종료
	bIsReloading = false;

	// UI와 애니메이션에 재장전 종료 전달
	OnReloadStateChanged.Broadcast(false);
}

void UPlayerCombatComponent::BroadcastAmmoChanged()
{
	// 현재 실린더와 예비 탄약 수를 UI에 전달
	OnAmmoChanged.Broadcast(
		CurrentAmmo,
		ReserveAmmo
	);
}