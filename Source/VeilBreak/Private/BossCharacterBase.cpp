
#include "BossCharacterBase.h"
#include "BossAIController.h"
#include "BossMagicAttackActor.h"
#include "BossFallingRockActor.h"
#include "BossBerserkActor.h"
#include "BossVortexActor.h"
#include "BossStatComponent.h"
#include "PlayerHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/ConstructorHelpers.h"

// 생성자: 로드 성공한 에셋을 Mesh 기본값에 적용, 상속 BP에서 변경 가능
ABossCharacterBase::ABossCharacterBase()
{
	// Player 0 디버그 키 상태 확인용 Tick 활성화
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = ABossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	// 체력 2000·사망 이벤트 제공 컴포넌트 생성
	BossStatComponent = CreateDefaultSubobject<UBossStatComponent>(TEXT("BossStatComponent"));
	// 보스 고정 이동속도 300cm/s 적용
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	// Sevarog 메시
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BossMesh(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog.Sevarog"));
	// Sevarog 스켈레톤용 idle 모션
	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimation(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Idle.Idle"));
	// 마법 시전용 Cast 모션
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CastAnimation(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Cast.Cast"));
	// Fire 투사체 상속 BP
	static ConstructorHelpers::FClassFinder<ABossMagicAttackActor> MagicBlueprint(
		TEXT("/Game/Boss/Patterns/BP_BossMagicAttack"));
	// 낙석 투사체 상속 BP
	static ConstructorHelpers::FClassFinder<ABossFallingRockActor> FallingRockBlueprint(
		TEXT("/Game/Boss/Patterns/BP_BossFallingRock"));
	// 낙석 시전용 Ultimate Swing 모션
	static ConstructorHelpers::FObjectFinder<UAnimSequence> UltimateSwingAnimation(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Ultimate_Swing_120fps.Ultimate_Swing_120fps"));
	// 발악 유지 중 반복할 Knock Back 모션
	static ConstructorHelpers::FObjectFinder<UAnimSequence> BerserkAnimation(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Knock_back_bwd.Knock_back_bwd"));
	// 사격으로 파괴할 발악 구체 상속 BP
	static ConstructorHelpers::FClassFinder<ABossBerserkActor> BerserkOrbBlueprint(
		TEXT("/Game/Boss/Patterns/BP_BossBerserk"));
	// 플레이어 추적 소용돌이 상속 BP
	static ConstructorHelpers::FClassFinder<ABossVortexActor> VortexBlueprint(
		TEXT("/Game/Boss/Patterns/BP_BossVortex"));
	// 낙석 위험 지점 표시용 Sevarog 지속 타기팅 이펙트
	static ConstructorHelpers::FObjectFinder<UParticleSystem> FallingRockWarning(
		TEXT("/Game/ParagonSevarog/FX/Particles/Abilities/SoulSiphon/FX/P_SiphonTargeting.P_SiphonTargeting"));
	if (CastAnimation.Succeeded()) CastMotion = CastAnimation.Object;
	MagicAttackClass = MagicBlueprint.Succeeded() ? MagicBlueprint.Class.Get() : ABossMagicAttackActor::StaticClass();
	FallingRockClass = FallingRockBlueprint.Succeeded() ? FallingRockBlueprint.Class.Get() : ABossFallingRockActor::StaticClass();
	if (UltimateSwingAnimation.Succeeded()) FallingRockMotion = UltimateSwingAnimation.Object;
	if (BerserkAnimation.Succeeded()) BerserkMotion = BerserkAnimation.Object;
	BerserkOrbClass = BerserkOrbBlueprint.Succeeded() ? BerserkOrbBlueprint.Class.Get() : ABossBerserkActor::StaticClass();
	VortexClass = VortexBlueprint.Succeeded() ? VortexBlueprint.Class.Get() : ABossVortexActor::StaticClass();
	if (FallingRockWarning.Succeeded()) FallingRockWarningEffect = FallingRockWarning.Object;
	// 피격용 Physics Asset
	static ConstructorHelpers::FObjectFinder<UPhysicsAsset> BossPhysicsAsset(
		TEXT("/Game/Boss/Physics/PA_BossSevarog_ShadowCyl.PA_BossSevarog_ShadowCyl"));

	if (BossMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(BossMesh.Object);
	}

	// 메시의 Z축 회전 -90도, 캐릭터 전방 정렬
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	// 메시의 상대 Z 위치 -70cm, Capsule 기준 높이 정렬
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -70.0f));

	// Physics Asset의 단순 충돌체 사용
	GetMesh()->bEnablePerPolyCollision = false;
	if (BossPhysicsAsset.Succeeded())
	{
		GetMesh()->SetPhysicsAsset(BossPhysicsAsset.Object);
	}
	// 화면에 렌더링될 때만 애니메이션 갱신
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	// 임시 WeaponTrace 채널 - 테스트 필요
	constexpr ECollisionChannel WeaponTraceChannel = ECC_GameTraceChannel1;
	// Capsule은 이동 충돌 유지, 사격만 무시
	GetCapsuleComponent()->SetCollisionResponseToChannel(WeaponTraceChannel, ECR_Ignore);
	// Mesh는 피격만 처리
	GetMesh()->SetCollisionProfileName(TEXT("Custom"));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(WeaponTraceChannel, ECR_Block);
	GetMesh()->SetSimulatePhysics(false);

	if (IdleAnimation.Succeeded())
	{
		IdleMotion = IdleAnimation.Object;
		// 생성된 인스턴스가 idle을 자동 반복 재생
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		GetMesh()->AnimationData.AnimToPlay = IdleAnimation.Object;
		GetMesh()->AnimationData.bSavedLooping = true;
		GetMesh()->AnimationData.bSavedPlaying = true;
		GetMesh()->AnimationData.SavedPlayRate = 1.0f;
	}
}

// BP PatternSetter의 이동속도를 실제 CharacterMovement에 적용
void ABossCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.f, BossMovementSpeed);
}

// Player 0의 실제 키 입력 상태에서 숫자 0 Pressed 감지
void ABossCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bEnablePhaseDebugInput || !GetWorld()) return;
	// 현재 플레이어 캐릭터를 조작하는 Player 0 Controller
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController) return;
	// 상단 숫자 0 또는 숫자패드 0의 이번 프레임 입력 여부
	const bool bDebugKeyPressed = PlayerController->WasInputKeyJustPressed(EKeys::Zero) || PlayerController->WasInputKeyJustPressed(EKeys::NumPadZero);
	if (bDebugKeyPressed) CycleDebugHealthPhase();
}

// Phase1 2000·Phase2 1000·Phase3 400 체력 순환 적용
void ABossCharacterBase::CycleDebugHealthPhase()
{
	if (!BossStatComponent) return;
	// 각 페이즈 내부에 확실히 포함되는 대표 체력 비율
	constexpr float DebugHealthPercents[] = { 1.f, 0.5f, 0.2f };
	// 현재 순번에 대응하는 디버그 체력
	const float DebugHealth = BossStatComponent->GetMaxHealth() * DebugHealthPercents[DebugPhaseIndex];
	BossStatComponent->SetHealthForDebug(DebugHealth);
	DebugPhaseIndex = (DebugPhaseIndex + 1) % UE_ARRAY_COUNT(DebugHealthPercents);
	// 화면에 표시할 현재 페이즈 번호
	const int32 PhaseNumber = static_cast<int32>(BossStatComponent->GetCurrentPhase()) + 1;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Boss Debug - Health: %.0f / %.0f, Phase: %d"), BossStatComponent->GetCurrentHealth(), BossStatComponent->GetMaxHealth(), PhaseNumber));
}

// Ultimate Swing 재생과 0.8초 뒤 낙석 발사 예약
bool ABossCharacterBase::StartFallingRock(const FVector& Target)
{
	if (IsPatternRunning() || !FallingRockMotion || !IdleMotion || !FallingRockClass || !GetWorld()) return false;
	const float Duration = FallingRockMotion->GetPlayLength();
	if (Duration <= 0.f) return false;
	// 플레이어 위치 위쪽에서 아래로 검사할 지면 Trace 시작점
	const FVector GroundTraceStart = Target + FVector(0.f, 0.f, 5000.f);
	// 플레이어 위치 아래쪽까지 검사할 지면 Trace 끝점
	const FVector GroundTraceEnd = Target - FVector(0.f, 0.f, 10000.f);
	// 보스 자신을 제외하는 지면 Trace 조건
	FCollisionQueryParams GroundTraceParams(SCENE_QUERY_STAT(FallingRockGroundTrace), false, this);
	// WorldStatic 바닥만 찾는 지면 Trace 대상
	FCollisionObjectQueryParams GroundObjectParams(ECC_WorldStatic);
	// 지면 Trace 충돌 결과
	FHitResult GroundHit;
	// 점프 중인 플레이어를 건너뛰고 시전 위치 아래 바닥을 목표로 저장
	FallingRockTarget = GetWorld()->LineTraceSingleByObjectType(GroundHit, GroundTraceStart, GroundTraceEnd, GroundObjectParams, GroundTraceParams) ? GroundHit.ImpactPoint : Target;
	const FVector ToTarget = FallingRockTarget - GetActorLocation();
	if (!ToTarget.IsNearlyZero()) SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
	bFallingRockRunning = true;
	GetMesh()->PlayAnimation(FallingRockMotion, false);
	GetWorldTimerManager().SetTimer(FallingRockWarningTimer, this, &ABossCharacterBase::ShowFallingRockWarning, FallingRockWarningDelay, false);
	GetWorldTimerManager().SetTimer(FallingRockReleaseTimer, this, &ABossCharacterBase::ReleaseFallingRock, FMath::Clamp(FallingRockReleaseDelay, 0.01f, Duration * 0.95f), false);
	GetWorldTimerManager().SetTimer(FallingRockWarningClearTimer, this, &ABossCharacterBase::ClearFallingRockWarning, FallingRockWarningDelay + FallingRockWarningDuration, false);
	GetWorldTimerManager().SetTimer(FallingRockFinishTimer, this, &ABossCharacterBase::FinishFallingRock, Duration, false);
	return true;
}

// 시전 0.2초 뒤 바닥 목표 위치에 Sevarog 타기팅 경고 표시
void ABossCharacterBase::ShowFallingRockWarning()
{
	if (!bFallingRockRunning || !FallingRockWarningEffect || !GetWorld()) return;
	ClearFallingRockWarning();
	// 지면 겹침 방지용으로 2cm 올린 경고 표시 위치
	const FVector WarningLocation = FallingRockTarget + FVector(0.f, 0.f, 2.f);
	// 낙석 도착 위치를 알리는 지속 ParticleSystem 경고 컴포넌트
	FallingRockWarningComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FallingRockWarningEffect, FTransform(FRotator::ZeroRotator, WarningLocation, FVector(FallingRockWarningScale)), true, EPSCPoolMethod::None, true);
}

// 표시 중인 낙석 경고를 즉시 종료하고 참조 해제
void ABossCharacterBase::ClearFallingRockWarning()
{
	if (!FallingRockWarningComponent) return;
	FallingRockWarningComponent->DeactivateSystem();
	FallingRockWarningComponent->DestroyComponent();
	FallingRockWarningComponent = nullptr;
}

// 표준 피해 처리와 보스 체력 컴포넌트 연결
float ABossCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!BossStatComponent) return 0.f;
	const float AppliedDamage = BossStatComponent->ApplyDamage(DamageAmount);
	if (AppliedDamage > 0.f) Super::TakeDamage(AppliedDamage, DamageEvent, EventInstigator, DamageCauser);
	return AppliedDamage;
}

// Cast 시작과 발사 예약, 애니메이션 상태와 독립된 타이머로 시전 종료 보장
bool ABossCharacterBase::StartMagicAttack(const FVector& Target)
{
	if (bMagicAttackRunning || !CastMotion || !IdleMotion || !MagicAttackClass || !GetWorld()) return false;
	// Cast 재생 길이, 초
	const float Duration = CastMotion->GetPlayLength();
	if (Duration <= 0.f) return false;
	MagicTarget = Target;
	// 목표를 향한 수평 방향으로 보스 회전
	const FVector ToTarget = MagicTarget - GetActorLocation();
	if (!ToTarget.IsNearlyZero()) SetActorRotation(FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f));
	bMagicAttackRunning = true;
	bMagicAttackLaunched = false;
	GetMesh()->PlayAnimation(CastMotion, false);
	GetWorldTimerManager().SetTimer(MagicReleaseTimer, this, &ABossCharacterBase::ReleaseMagicAttack, FMath::Clamp(MagicReleaseDelay, 0.01f, Duration * 0.95f), false);
	GetWorldTimerManager().SetTimer(MagicFinishTimer, this, &ABossCharacterBase::FinishMagicAttack, Duration, false);
	return true;
}

// 손 본 기준 발사 위치에서 월드 목표 좌표로 이동 시작
void ABossCharacterBase::ReleaseMagicAttack()
{
	if (!bMagicAttackRunning || !GetWorld()) return;
	// 소켓이 없을 경우 캐릭터 위치 위쪽을 발사점으로 사용
	const FVector SpawnLocation = GetMesh()->DoesSocketExist(MagicSpawnSocket) ? GetMesh()->GetSocketLocation(MagicSpawnSocket) : GetActorLocation() + FVector(0, 0, 100);
	// 투사체 소유자와 충돌 무관 생성 정책
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// 실제 생성된 마법 투사체
	ABossMagicAttackActor* Projectile = GetWorld()->SpawnActor<ABossMagicAttackActor>(MagicAttackClass, SpawnLocation, (MagicTarget - SpawnLocation).Rotation(), Params);
	if (Projectile)
	{
		Projectile->Configure(MagicAttackProjectileDamage, MagicAttackExplosiveDamage, MagicAttackProjectileSpeed);
		Projectile->LaunchAt(MagicTarget);
		bMagicAttackLaunched = true;
	}
}

// 시전 상태 종료, 다음 BT 대기 동안 Idle 유지
void ABossCharacterBase::FinishMagicAttack()
{
	bMagicAttackRunning = false;
	if (IdleMotion) GetMesh()->PlayAnimation(IdleMotion, true);
}

// 시전 시 저장한 목표를 향해 보스 손에서 포물선 낙석 생성
void ABossCharacterBase::ReleaseFallingRock()
{
	if (!bFallingRockRunning || !GetWorld()) return;
	// 손 소켓이 없을 경우 캐릭터 위치 위쪽을 발사점으로 사용
	const FVector SpawnLocation = GetMesh()->DoesSocketExist(FallingRockSpawnSocket) ? GetMesh()->GetSocketLocation(FallingRockSpawnSocket) : GetActorLocation() + FVector(0.f, 0.f, 100.f);
	// 낙석 소유자와 충돌 무관 생성 정책
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// 손에서 생성돼 시전 순간 목표 위치로 비행하는 낙석
	if (ABossFallingRockActor* Rock = GetWorld()->SpawnActor<ABossFallingRockActor>(FallingRockClass, SpawnLocation, FRotator::ZeroRotator, Params))
	{
		Rock->Configure(FallingRockDamage, FallingRockProjectileSpeed);
		Rock->LaunchAt(FallingRockTarget);
	}
}

// 낙석 시전 상태 종료와 Idle 복귀
void ABossCharacterBase::FinishFallingRock()
{
	bFallingRockRunning = false;
	if (IdleMotion) GetMesh()->PlayAnimation(IdleMotion, true);
}

// 패턴 실행 상태·거리·무피격 시간·쿨타임 기준 발악 가능 여부 반환
bool ABossCharacterBase::CanStartBerserk(AActor* TargetActor) const
{
	if (!TargetActor || IsPatternRunning() || !BerserkMotion || !BerserkOrbClass || !GetWorld() || !BossStatComponent || BossStatComponent->IsDead()) return false;
	if (FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation()) > FMath::Square(BerserkRange)) return false;
	return bHasObservedPlayerHealth && ObservedBerserkTarget.Get() == TargetActor && GetWorld()->GetTimeSeconds() - LastObservedPlayerDamageTime >= 20.0 && GetWorld()->GetTimeSeconds() >= NextBerserkAvailableTime;
}

// 플레이어 공개 체력을 관찰해 마지막 체력 감소 시간 갱신
void ABossCharacterBase::UpdatePlayerNoDamageState(AActor* TargetActor)
{
	if (!GetWorld() || !TargetActor)
	{
		ObservedBerserkTarget.Reset();
		bHasObservedPlayerHealth = false;
		return;
	}
	const UPlayerHealthComponent* PlayerHealth = TargetActor->FindComponentByClass<UPlayerHealthComponent>();
	if (!PlayerHealth)
	{
		ObservedBerserkTarget.Reset();
		bHasObservedPlayerHealth = false;
		return;
	}
	const float CurrentPlayerHealth = PlayerHealth->GetCurrentHealth();
	if (!bHasObservedPlayerHealth || ObservedBerserkTarget.Get() != TargetActor)
	{
		ObservedBerserkTarget = TargetActor;
		ObservedPlayerHealth = CurrentPlayerHealth;
		LastObservedPlayerDamageTime = GetWorld()->GetTimeSeconds();
		bHasObservedPlayerHealth = true;
		return;
	}
	if (CurrentPlayerHealth < ObservedPlayerHealth) LastObservedPlayerDamageTime = GetWorld()->GetTimeSeconds();
	ObservedPlayerHealth = CurrentPlayerHealth;
}

// 보스 무적 설정·Knock Back 모션 재생·파괴 구체 생성 후 발악 시작
bool ABossCharacterBase::StartBerserk(AActor* TargetActor)
{
	if (!CanStartBerserk(TargetActor)) return false;
	bBerserkRunning = true;
	NextBerserkAvailableTime = GetWorld()->GetTimeSeconds() + BerserkCooldown;
	BossStatComponent->SetInvulnerable(true);
	const FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	if (!ToTarget.IsNearlyZero()) SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
	GetMesh()->PlayAnimation(BerserkMotion, true);
	SpawnBerserkOrbs();
	if (RemainingBerserkOrbs <= 0)
	{
		FinishBerserk(false);
		return false;
	}
	GetWorldTimerManager().SetTimer(BerserkTimeoutTimer, this, &ABossCharacterBase::HandleBerserkTimeout, BerserkMaxDuration, false);
	return true;
}

// 보스 주변 임의 각도·거리·높이에 발악 구체 생성
void ABossCharacterBase::SpawnBerserkOrbs()
{
	ActiveBerserkOrbs.Reset();
	RemainingBerserkOrbs = 0;
	for (int32 Index = 0; Index < BerserkOrbCount; ++Index)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::FRandRange(BerserkOrbSpawnRadius.X, BerserkOrbSpawnRadius.Y);
		const FVector SpawnLocation = GetActorLocation() + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, FMath::FRandRange(120.f, 320.f));
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABossBerserkActor* Orb = GetWorld()->SpawnActor<ABossBerserkActor>(BerserkOrbClass, SpawnLocation, FRotator::ZeroRotator, Params))
		{
			ActiveBerserkOrbs.Add(Orb);
			++RemainingBerserkOrbs;
		}
	}
}

// 파괴된 구체를 목록에서 제외하고 전부 파괴되면 발악 종료
void ABossCharacterBase::HandleBerserkOrbDestroyed(ABossBerserkActor* DestroyedOrb)
{
	if (!bBerserkRunning || !DestroyedOrb) return;
	ActiveBerserkOrbs.RemoveAll([DestroyedOrb](const TWeakObjectPtr<ABossBerserkActor>& Orb) { return Orb.Get() == DestroyedOrb; });
	RemainingBerserkOrbs = FMath::Max(0, RemainingBerserkOrbs - 1);
	if (RemainingBerserkOrbs == 0) FinishBerserk(false);
}

// 제한시간 안에 구체가 남아 있으면 실패 처리 요청
void ABossCharacterBase::HandleBerserkTimeout()
{
	if (!bBerserkRunning) return;
	FinishBerserk(true);
}

// 시간초과 시 최대 체력 비율 회복·무적 해제·남은 구체 제거
void ABossCharacterBase::FinishBerserk(bool bTimedOut)
{
	if (!bBerserkRunning) return;
	bBerserkRunning = false;
	GetWorldTimerManager().ClearTimer(BerserkTimeoutTimer);
	if (bTimedOut && RemainingBerserkOrbs > 0 && BossStatComponent)
	{
		BossStatComponent->HealWithoutPhaseRegression(BossStatComponent->GetMaxHealth() * BerserkHealPercent);
	}
	if (BossStatComponent) BossStatComponent->SetInvulnerable(false);
	for (const TWeakObjectPtr<ABossBerserkActor>& Orb : ActiveBerserkOrbs)
	{
		if (Orb.IsValid()) Orb->Destroy();
	}
	ActiveBerserkOrbs.Reset();
	RemainingBerserkOrbs = 0;
	if (IdleMotion) GetMesh()->PlayAnimation(IdleMotion, true);
}

// Phase1·사거리·쿨타임 확인 후 Cast와 0.3초 생성 예약 시작
bool ABossCharacterBase::StartVortex(AActor* TargetActor)
{
	if (!CanStartVortex(TargetActor)) return false;
	PendingVortexTarget = TargetActor;
	bVortexRunning = true;
	bVortexCasting = true;
	NextVortexAvailableTime = GetWorld()->GetTimeSeconds() + VortexCooldown;
	const FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	if (!ToTarget.IsNearlyZero()) SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
	GetMesh()->PlayAnimation(CastMotion, false);
	GetWorldTimerManager().SetTimer(VortexSpawnTimer, this, &ABossCharacterBase::SpawnVortex, VortexSpawnDelay, false);
	GetWorldTimerManager().SetTimer(VortexCastFinishTimer, this, &ABossCharacterBase::FinishVortexCast, CastMotion->GetPlayLength(), false);
	return true;
}

// 예약된 시점의 보스 위치 바닥에서 소용돌이 생성·10초 유지 시작
void ABossCharacterBase::SpawnVortex()
{
	if (!bVortexRunning || !PendingVortexTarget.IsValid() || !GetWorld()) { FinishVortex(); return; }
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector TraceStart = GetActorLocation() + FVector(0.f, 0.f, 1000.f);
	const FVector TraceEnd = GetActorLocation() - FVector(0.f, 0.f, 5000.f);
	FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(VortexGroundTrace), false, this);
	FHitResult GroundHit;
	const FVector SpawnLocation = GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, GroundParams) ? GroundHit.ImpactPoint : GetActorLocation();
	ABossVortexActor* Vortex = GetWorld()->SpawnActor<ABossVortexActor>(VortexClass, SpawnLocation, FRotator::ZeroRotator, Params);
	if (!Vortex) { FinishVortex(); return; }
	Vortex->Configure(PendingVortexTarget.Get(), VortexDamage, VortexSpeed, VortexDuration);
	ActiveVortex = Vortex;
	PendingVortexTarget.Reset();
	GetWorldTimerManager().SetTimer(VortexFinishTimer, this, &ABossCharacterBase::FinishVortex, VortexDuration, false);
}

// Phase1·패턴 상태·사거리·쿨타임 기준 소용돌이 가능 여부 반환
bool ABossCharacterBase::CanStartVortex(AActor* TargetActor) const
{
	if (!TargetActor || IsPatternRunning() || !GetWorld() || !CastMotion || !VortexClass || !BossStatComponent || BossStatComponent->IsDead()) return false;
	if (BossStatComponent->GetCurrentPhase() != EBossPhase::Phase1 || GetWorld()->GetTimeSeconds() < NextVortexAvailableTime) return false;
	return FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation()) <= FMath::Square(VortexRange);
}

// 소용돌이 Cast 상태 해제 후 다른 패턴이 없으면 idle 복귀
void ABossCharacterBase::FinishVortexCast()
{
	bVortexCasting = false;
	if (!IsPatternRunning() && IdleMotion) GetMesh()->PlayAnimation(IdleMotion, true);
}

// 유지시간이 끝난 소용돌이 제거·실행 상태 초기화
void ABossCharacterBase::FinishVortex()
{
	if (!bVortexRunning) return;
	bVortexRunning = false;
	bVortexCasting = false;
	GetWorldTimerManager().ClearTimer(VortexSpawnTimer);
	GetWorldTimerManager().ClearTimer(VortexFinishTimer);
	GetWorldTimerManager().ClearTimer(VortexCastFinishTimer);
	if (ActiveVortex.IsValid()) ActiveVortex->Destroy();
	ActiveVortex.Reset();
	PendingVortexTarget.Reset();
	if (!IsPatternRunning() && IdleMotion) GetMesh()->PlayAnimation(IdleMotion, true);
}

// 종료 시 예약된 마법 시전 타이머 해제
void ABossCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(MagicReleaseTimer);
	GetWorldTimerManager().ClearTimer(MagicFinishTimer);
	GetWorldTimerManager().ClearTimer(FallingRockReleaseTimer);
	GetWorldTimerManager().ClearTimer(FallingRockFinishTimer);
	GetWorldTimerManager().ClearTimer(FallingRockWarningTimer);
	GetWorldTimerManager().ClearTimer(FallingRockWarningClearTimer);
	GetWorldTimerManager().ClearTimer(BerserkTimeoutTimer);
	GetWorldTimerManager().ClearTimer(VortexFinishTimer);
	GetWorldTimerManager().ClearTimer(VortexSpawnTimer);
	GetWorldTimerManager().ClearTimer(VortexCastFinishTimer);
	if (ActiveVortex.IsValid()) ActiveVortex->Destroy();
	if (BossStatComponent) BossStatComponent->SetInvulnerable(false);
	for (const TWeakObjectPtr<ABossBerserkActor>& Orb : ActiveBerserkOrbs)
	{
		if (Orb.IsValid()) Orb->Destroy();
	}
	ClearFallingRockWarning();
	Super::EndPlay(EndPlayReason);
}
