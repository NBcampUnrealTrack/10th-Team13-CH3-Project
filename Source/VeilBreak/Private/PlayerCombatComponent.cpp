#include "PlayerCombatComponent.h"

#include "BossBerserkActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PlayerSkillComponent.h"
#include "Sound/SoundBase.h"

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

	// 초기 탄약 정보 전달
	BroadcastAmmoChanged();

	// 총구 이펙트를 지정했다면 소켓 확인
	if (MuzzleEffect != nullptr)
	{
		ACharacter* OwnerCharacter =
			Cast<ACharacter>(GetOwner());

		USkeletalMeshComponent* CharacterMesh =
			OwnerCharacter != nullptr
			? OwnerCharacter->GetMesh()
			: nullptr;

		if (
			CharacterMesh == nullptr ||
			!CharacterMesh->DoesSocketExist(MuzzleSocketName)
			)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Combat: Muzzle socket '%s' missing on %s"),
				*MuzzleSocketName.ToString(),
				*GetNameSafe(GetOwner())
			);
		}
	}
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
		// 빈 총에서는 탄환 소모 이벤트를 호출하지 않음
		StartReload();
		return;
	}

	// 이벤트 처리 중 중복 사격 방지
	bCanFire = false;

	// 탄약 한 발 소모
	--CurrentAmmo;

	// 요청한 위치: 탄약 차감 직후 소모량 전달
	OnAmmoSpent.Broadcast(1);

	// 기존 탄약 UI 갱신
	BroadcastAmmoChanged();

	// 실제 발사 성공 시 총구 효과 재생
	PlayFireEffects();

	// 명중 검사 및 반동 처리
	PerformHitScan();
	ApplyRecoil();

	// 기존 발사 애니메이션 연결 유지
	OnWeaponFired.Broadcast();

	// 다음 발사까지 대기
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

	// UI 및 재장전 애니메이션에 시작 전달
	OnReloadStateChanged.Broadcast(true);
}

void UPlayerCombatComponent::CancelReload()
{
	if (!bIsReloading)
	{
		return;
	}

	// 상태와 타이머 정리
	FinishReload();
}

int32 UPlayerCombatComponent::AddReserveAmmo(int32 AmmoAmount)
{
	if (AmmoAmount <= 0)
	{
		return 0;
	}

	// 남은 공간 안에서만 탄약 추가
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
	// 현재 재장전 상태 반환
	return bIsReloading;
}

void UPlayerCombatComponent::SetUltimateBuffActive(
	bool bEnableUltimateBuff
)
{
	// 궁극기 공격력 배율 적용
	CurrentDamageMultiplier = bEnableUltimateBuff
		? UltimateDamageMultiplier
		: 1.0f;

	// 궁극기 재장전 시간 배율 적용
	CurrentReloadTimeMultiplier = bEnableUltimateBuff
		? UltimateReloadTimeMultiplier
		: 1.0f;
}

void UPlayerCombatComponent::PlayFireEffects()
{
	ACharacter* OwnerCharacter =
		Cast<ACharacter>(GetOwner());

	if (!IsValid(OwnerCharacter))
	{
		return;
	}

	// 총이 포함된 캐릭터 메시 사용
	USkeletalMeshComponent* CharacterMesh =
		OwnerCharacter->GetMesh();

	if (!IsValid(CharacterMesh))
	{
		return;
	}

	const bool bHasMuzzleSocket =
		CharacterMesh->DoesSocketExist(MuzzleSocketName);

	if (bHasMuzzleSocket && MuzzleEffect != nullptr)
	{
		// 크기를 먼저 설정하고 활성화하기 위해 자동 활성화 해제
		UNiagaraComponent* SpawnedEffect =
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				MuzzleEffect.Get(),
				CharacterMesh,
				MuzzleSocketName,
				MuzzleEffectLocationOffset,
				MuzzleEffectRotationOffset,
				EAttachLocation::KeepRelativeOffset,
				true,
				false
			);

		if (SpawnedEffect != nullptr)
		{
			// 총구 소켓에 붙은 이펙트 크기 설정
			SpawnedEffect->SetRelativeScale3D(
				FVector(FMath::Max(MuzzleEffectScale, 0.01f))
			);

			SpawnedEffect->Activate(true);
		}
	}

	if (FireSound != nullptr)
	{
		// 소켓이 없으면 발사음만 캐릭터 위치에서 재생
		const FVector SoundLocation = bHasMuzzleSocket
			? CharacterMesh->GetSocketLocation(MuzzleSocketName)
			: OwnerCharacter->GetActorLocation();

		UGameplayStatics::PlaySoundAtLocation(
			this,
			FireSound.Get(),
			SoundLocation,
			FMath::Max(FireSoundVolume, 0.0f)
		);
	}
}

void UPlayerCombatComponent::PlayImpactEffects(
	const FHitResult& HitResult,
	bool bHitBoss
)
{
	// 기본 명중 효과 선택
	UNiagaraSystem* SelectedEffect = DefaultImpactEffect.Get();
	USoundBase* SelectedSound = DefaultImpactSound.Get();

	if (bHitBoss)
	{
		// 보스 전용 에셋이 지정됐으면 대체
		if (BossImpactEffect != nullptr)
		{
			SelectedEffect = BossImpactEffect.Get();
		}

		if (BossImpactSound != nullptr)
		{
			SelectedSound = BossImpactSound.Get();
		}
	}

	// 명중 표면 바깥 방향 계산
	const FVector SurfaceNormal =
		HitResult.ImpactNormal.GetSafeNormal();

	// 표면에 묻히지 않도록 약간 띄워 생성
	const FVector EffectLocation =
		HitResult.ImpactPoint +
		SurfaceNormal * FMath::Max(ImpactEffectSurfaceOffset, 0.0f);

	// 이펙트의 X축이 표면 바깥 방향을 향하도록 회전
	const FQuat EffectRotation =
		SurfaceNormal.Rotation().Quaternion() *
		ImpactEffectRotationOffset.Quaternion();

	if (SelectedEffect != nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			SelectedEffect,
			EffectLocation,
			EffectRotation.Rotator(),
			FVector(FMath::Max(ImpactEffectScale, 0.01f)),
			true,
			true
		);
	}

	if (SelectedSound != nullptr)
	{
		// 실제 명중 위치에서 소리 재생
		UGameplayStatics::PlaySoundAtLocation(
			this,
			SelectedSound,
			HitResult.ImpactPoint,
			FMath::Max(ImpactSoundVolume, 0.0f)
		);
	}
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

	// 기존 보스 전용 피격 채널 유지
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
		// 디버그 옵션을 켰을 때만 사격 선 표시
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

	// 이벤트 처리 전에 이번 발사의 데미지와 명중 위치 확정
	const float ShotDamage =
		BaseDamage * CurrentDamageMultiplier;

	const FVector HitLocation = HitResult.ImpactPoint;

	// 지정한 보스 본체 BP 및 그 자식 클래스 판별
	// 궁극기 과녁은 보스 본체 명중에서 제외
	const bool bHitBoss =
		BossActorClass.Get() != nullptr &&
		HitActor->IsA(BossActorClass.Get()) &&
		!HitActor->IsA<ABossBerserkActor>();

	// 피해로 대상이 파괴되기 전에 명중 효과 재생
	PlayImpactEffects(HitResult, bHitBoss);

	// 기존 과녁 스택 처리
	HandleUltimateTargetHit(HitActor);

	if (!IsValid(HitActor))
	{
		return;
	}

	if (bHitBoss)
	{
		// 요청한 위치: 보스 명중 시 피해 적용 전에 호출
		OnBossShotLanded.Broadcast();
	}

	if (!IsValid(HitActor))
	{
		// 이벤트 수신 측에서 대상을 제거했다면 피해 적용 중단
		return;
	}

	// 명중 대상에 공격 데미지 적용
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
		// 기존 히트마커 및 데미지 텍스트 이벤트 유지
		// 실제 HP 감소량이 아닌 이번 공격 데미지 전달
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

	// 파괴된 과녁 기록 제거
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
		// 같은 과녁에서는 스택을 중복 획득하지 않음
		return;
	}

	AActor* OwnerActor = GetOwner();

	if (!IsValid(OwnerActor))
	{
		return;
	}

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
		// 최대 스택에서는 추가 획득하지 않음
		return;
	}

	// 이벤트 처리 중 중복 진입 방지
	HitUltimateTargets.Add(TargetActor);

	// 스택 증가 및 스택 UI 이벤트 실행
	SkillComponent->AddUltimateTargetStack();
}

void UPlayerCombatComponent::ApplyRecoil()
{
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

	// 장전은 탄환 사용이 아니므로 OnAmmoSpent를 호출하지 않음
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
	// 기존 탄약 UI 이벤트
	OnAmmoChanged.Broadcast(
		CurrentAmmo,
		ReserveAmmo
	);
}