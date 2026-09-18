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

	// 과녁 중복 처리 기록 초기화
	HitUltimateTargets.Empty();

	// 초기 탄약 정보를 UI에 전달
	BroadcastAmmoChanged();

	// 수정한 컴포넌트가 실제로 실행되는지 확인
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("SHOT_CHECK: Combat BeginPlay | Owner=%s"),
		*GetNameSafe(GetOwner())
	);
}

void UPlayerCombatComponent::TryFire()
{
	// 사격 입력이 전투 컴포넌트까지 도달하는지 확인
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("SHOT_CHECK: TryFire | CanFire=%d | Ammo=%d"),
		bCanFire ? 1 : 0,
		CurrentAmmo
	);

	if (!bCanFire)
	{
		// 발사 간격이 끝나지 않았다면 중단
		return;
	}

	if (bIsReloading)
	{
		// 사격 시 진행 중인 재장전 취소
		CancelReload();
	}

	if (CurrentAmmo <= 0)
	{
		// 탄약이 없으면 재장전 시도
		StartReload();
		return;
	}

	// 이벤트 처리 중 중복 사격 방지
	bCanFire = false;

	// 탄약 한 발 소모
	--CurrentAmmo;
	BroadcastAmmoChanged();

	// 명중 판정 및 반동 처리
	PerformHitScan();
	ApplyRecoil();

	// 사격 애니메이션과 효과에 전달
	OnWeaponFired.Broadcast();

	// 발사 간격 이후 사격 가능 상태로 복구
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
	if (bIsReloading)
	{
		// 이미 재장전 중이라면 중단
		return;
	}

	if (CurrentAmmo >= CylinderCapacity)
	{
		// 실린더가 가득 찼다면 중단
		return;
	}

	if (ReserveAmmo <= 0)
	{
		// 예비 탄약이 없다면 중단
		return;
	}

	// 재장전 상태 시작
	bIsReloading = true;

	// 한 발 장전 타이머 시작
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

	// 타이머와 재장전 상태 정리
	FinishReload();
}

int32 UPlayerCombatComponent::AddReserveAmmo(int32 AmmoAmount)
{
	if (AmmoAmount <= 0)
	{
		// 유효하지 않은 보급량은 무시
		return 0;
	}

	// 예비 탄약의 남은 공간 계산
	const int32 AvailableSpace = FMath::Max(
		MaxReserveAmmo - ReserveAmmo,
		0
	);

	// 실제 추가할 수 있는 탄약 계산
	const int32 AddedAmmo = FMath::Min(
		AmmoAmount,
		AvailableSpace
	);

	if (AddedAmmo > 0)
	{
		ReserveAmmo += AddedAmmo;

		// 실제 추가됐을 때만 UI에 전달
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
	// 실린더 최대 탄약 수 반환
	return CylinderCapacity;
}

bool UPlayerCombatComponent::IsReloading() const
{
	// 현재 재장전 상태 반환
	return bIsReloading;
}

void UPlayerCombatComponent::SetUltimateBuffActive(
	bool bEnableUltimateBuff
)
{
	// 궁극기 상태에 따라 공격력 배율 변경
	CurrentDamageMultiplier = bEnableUltimateBuff
		? UltimateDamageMultiplier
		: 1.0f;

	// 궁극기 상태에 따라 재장전 시간 배율 변경
	CurrentReloadTimeMultiplier = bEnableUltimateBuff
		? UltimateReloadTimeMultiplier
		: 1.0f;
}

void UPlayerCombatComponent::PerformHitScan()
{
	// 명중 여부와 관계없이 함수 실행 확인
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("SHOT_CHECK: PerformHitScan called")
	);

	// 컴포넌트를 소유한 플레이어 확인
	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	if (OwnerPawn == nullptr)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SHOT_CHECK: Stopped - Owner is not Pawn")
		);

		return;
	}

	// 플레이어 컨트롤러 확인
	APlayerController* PlayerController =
		Cast<APlayerController>(OwnerPawn->GetController());

	if (PlayerController == nullptr)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SHOT_CHECK: Stopped - PlayerController missing")
		);

		return;
	}

	UWorld* World = GetWorld();

	if (World == nullptr)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SHOT_CHECK: Stopped - World missing")
		);

		return;
	}

	// 카메라 위치와 회전 조회
	FVector ViewLocation;
	FRotator ViewRotation;

	PlayerController->GetPlayerViewPoint(
		ViewLocation,
		ViewRotation
	);

	// 카메라 전방을 기준으로 사격 방향 계산
	const FVector ShotDirection = ViewRotation.Vector();

	const FVector TraceEnd =
		ViewLocation + ShotDirection * TraceDistance;

	// 자기 자신은 명중 대상에서 제외
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerPawn);

	// 보스 전용 피격 채널로 명중 검사
	FHitResult HitResult;

	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		ViewLocation,
		TraceEnd,
		ECC_GameTraceChannel1,
		QueryParams
	);

	// 빗나간 경우에도 검사 결과 출력
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("SHOT_CHECK: Hit=%d | Actor=%s | Component=%s"),
		bHit ? 1 : 0,
		*GetNameSafe(HitResult.GetActor()),
		*GetNameSafe(HitResult.GetComponent())
	);

	// 디버그 선의 끝 위치 결정
	const FVector DebugTraceEnd =
		bHit ? HitResult.ImpactPoint : TraceEnd;

	if (bDrawDebugTrace)
	{
		// 명중은 빨간색, 빗나감은 초록색
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
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SHOT_CHECK: Stopped - HitActor invalid")
		);

		return;
	}

	// 실제 명중한 액터와 클래스 확인
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("SHOT_CHECK: Shot Hit=%s | Class=%s"),
		*GetNameSafe(HitActor),
		*GetNameSafe(HitActor->GetClass())
	);

	// 피해로 과녁이 파괴되기 전에 스택 먼저 처리
	HandleUltimateTargetHit(HitActor);

	if (!IsValid(HitActor))
	{
		// 스택 이벤트 처리 중 액터가 파괴됐다면 중단
		return;
	}

	// 궁극기 배율을 반영한 피해 적용
	UGameplayStatics::ApplyPointDamage(
		HitActor,
		BaseDamage * CurrentDamageMultiplier,
		ShotDirection,
		HitResult,
		PlayerController,
		GetOwner(),
		UDamageType::StaticClass()
	);
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
		// 명중 대상이 과녁 클래스가 아닌 경우 확인
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SHOT_CHECK: Not ultimate target | Actor=%s | Class=%s"),
			*GetNameSafe(HitActor),
			*GetNameSafe(HitActor->GetClass())
		);

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
		// 같은 과녁에서 중복 스택 획득 방지
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SHOT_CHECK: Target already counted | Actor=%s"),
			*GetNameSafe(HitActor)
		);

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
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SHOT_CHECK: PlayerSkillComponent missing | Owner=%s"),
			*GetNameSafe(OwnerActor)
		);

		return;
	}

	// 현재 스킬 상태와 이벤트 바인딩 여부 확인
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("SHOT_CHECK: Skill=%s | Stacks=%d/%d | Active=%d | EventBound=%d"),
		*SkillComponent->GetPathName(),
		SkillComponent->GetCurrentTargetStacks(),
		SkillComponent->GetRequiredTargetStacks(),
		SkillComponent->IsUltimateActive() ? 1 : 0,
		SkillComponent->OnUltimateTargetStackChanged.IsBound() ? 1 : 0
	);

	if (SkillComponent->IsUltimateActive())
	{
		// 궁극기 사용 중에는 스택 획득 불가
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SHOT_CHECK: Stack rejected - Ultimate active")
		);

		return;
	}

	if (
		SkillComponent->GetCurrentTargetStacks() >=
		SkillComponent->GetRequiredTargetStacks()
		)
	{
		// 이미 최대 스택이라면 중단
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SHOT_CHECK: Stack rejected - Stacks full")
		);

		return;
	}

	// 증가 전 스택 저장
	const int32 PreviousStacks =
		SkillComponent->GetCurrentTargetStacks();

	// 이벤트 처리 중 중복 호출되지 않도록 먼저 기록
	HitUltimateTargets.Add(TargetActor);

	// 스택 증가와 UI 이벤트 실행
	SkillComponent->AddUltimateTargetStack();

	// 호출 전후 스택 확인
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("SHOT_CHECK: AddUltimateTargetStack called | Before=%d | After=%d"),
		PreviousStacks,
		SkillComponent->GetCurrentTargetStacks()
	);
}

void UPlayerCombatComponent::ApplyRecoil()
{
	// 컴포넌트를 소유한 플레이어 확인
	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	if (OwnerPawn == nullptr)
	{
		return;
	}

	// 플레이어 컨트롤러 확인
	APlayerController* PlayerController =
		Cast<APlayerController>(OwnerPawn->GetController());

	if (PlayerController == nullptr)
	{
		return;
	}

	// 수직 반동 적용
	PlayerController->AddPitchInput(-VerticalRecoil);

	// 좌우 무작위 반동 적용
	const float RandomHorizontalRecoil = FMath::FRandRange(
		-HorizontalRecoil,
		HorizontalRecoil
	);

	PlayerController->AddYawInput(RandomHorizontalRecoil);
}

void UPlayerCombatComponent::ResetFireCooldown()
{
	// 발사 간격 종료
	bCanFire = true;
}

void UPlayerCombatComponent::HandleReloadRound()
{
	if (!bIsReloading)
	{
		// 취소된 재장전은 처리하지 않음
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

	// 변경된 탄약 전달
	BroadcastAmmoChanged();

	if (!bIsReloading)
	{
		// 이벤트 처리 중 재장전이 취소됐다면 중단
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

	// 남아 있는 장전 타이머 제거
	GetWorld()->GetTimerManager().ClearTimer(
		ReloadTimerHandle
	);

	// 재장전 종료 상태 전달
	bIsReloading = false;
	OnReloadStateChanged.Broadcast(false);
}

void UPlayerCombatComponent::BroadcastAmmoChanged()
{
	// 현재 탄약과 예비 탄약 전달
	OnAmmoChanged.Broadcast(
		CurrentAmmo,
		ReserveAmmo
	);
}