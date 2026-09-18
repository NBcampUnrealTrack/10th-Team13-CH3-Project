
#include "BossCharacterBase.h"
#include "BossAnimInstance.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
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
#include "Sound/SoundBase.h"
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
	// 체력 0 도달 시 재생할 Sevarog 정면 사망 모션
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathAnimation(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Death_front.Death_front"));
	// 사망 시 재생할 Sevarog 보이스 Cue
	static ConstructorHelpers::FObjectFinder<USoundBase> DeathVoiceCue(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sounds/SoundCues/Sevarog_Effort_Death.Sevarog_Effort_Death"));
	// 마법공격 시작 시 재생할 Sevarog Q 능력 보이스 Cue
	static ConstructorHelpers::FObjectFinder<USoundBase> MagicAttackVoiceCue(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sounds/SoundCues/Sevarog_Effort_Ability_Q.Sevarog_Effort_Ability_Q"));
	// 낙석 시작 시 재생할 Sevarog 궁극기 보이스 Cue
	static ConstructorHelpers::FObjectFinder<USoundBase> FallingRockVoiceCue(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sounds/SoundCues/Sevarog_Effort_Ability_Ultimate.Sevarog_Effort_Ability_Ultimate"));
	// 소용돌이 시작 시 재생할 Sevarog E 능력 보이스 Cue
	static ConstructorHelpers::FObjectFinder<USoundBase> VortexVoiceCue(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sounds/SoundCues/Sevarog_Effort_Ability_E.Sevarog_Effort_Ability_E"));
	// 발악 시작 시 재생할 Sevarog 위기 보이스 Cue
	static ConstructorHelpers::FObjectFinder<USoundBase> BerserkVoiceCue(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sounds/SoundCues/Sevarog_Health_Critical.Sevarog_Health_Critical"));
	// Fire 투사체 상속 BP
	static ConstructorHelpers::FClassFinder<ABossMagicAttackActor> MagicBlueprint(
		TEXT("/Game/Boss/Patterns/BP_BossMagicAttack"));
	// 낙석 투사체 상속 BP
	static ConstructorHelpers::FClassFinder<ABossFallingRockActor> FallingRockBlueprint(
		TEXT("/Game/Boss/Patterns/BP_BossFallingRock"));
	// 낙석 시전용 Ultimate Swing 모션
	static ConstructorHelpers::FObjectFinder<UAnimSequence> UltimateSwingAnimation(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Ultimate_Swing_120fps.Ultimate_Swing_120fps"));
	// 사격으로 파괴할 발악 구체 상속 BP
	static ConstructorHelpers::FClassFinder<ABossBerserkActor> BerserkOrbBlueprint(
		TEXT("/Game/Boss/Patterns/BP_BossBerserk"));
	// 플레이어 추적 소용돌이 상속 BP
	static ConstructorHelpers::FClassFinder<ABossVortexActor> VortexBlueprint(
		TEXT("/Game/Boss/Patterns/BP_BossVortex"));
	// 낙석 위험 지점 표시용 Sevarog 지속 타기팅 이펙트
	static ConstructorHelpers::FObjectFinder<UParticleSystem> FallingRockWarning(
		TEXT("/Game/ParagonSevarog/FX/Particles/Abilities/SoulSiphon/FX/P_SiphonTargeting.P_SiphonTargeting"));
	// 사망 모션 종료 후 보스가 사라질 때 재생할 Sevarog 영혼 폭발 이펙트
	static ConstructorHelpers::FObjectFinder<UParticleSystem> DeathDisappearParticle(
		TEXT("/Game/ParagonSevarog/FX/Particles/Abilities/SoulStackPassive/FX/P_SoulStageEmbersBurst.P_SoulStageEmbersBurst"));
	if (CastAnimation.Succeeded()) CastMotion = CastAnimation.Object;
	if (DeathAnimation.Succeeded()) DeathMotion = DeathAnimation.Object;
	if (DeathVoiceCue.Succeeded()) DeathVoice = DeathVoiceCue.Object;
	if (MagicAttackVoiceCue.Succeeded()) MagicAttackVoice = MagicAttackVoiceCue.Object;
	if (FallingRockVoiceCue.Succeeded()) FallingRockVoice = FallingRockVoiceCue.Object;
	if (VortexVoiceCue.Succeeded()) VortexVoice = VortexVoiceCue.Object;
	if (BerserkVoiceCue.Succeeded()) BerserkVoice = BerserkVoiceCue.Object;
	MagicAttackClass = MagicBlueprint.Succeeded() ? MagicBlueprint.Class.Get() : ABossMagicAttackActor::StaticClass();
	FallingRockClass = FallingRockBlueprint.Succeeded() ? FallingRockBlueprint.Class.Get() : ABossFallingRockActor::StaticClass();
	if (UltimateSwingAnimation.Succeeded()) FallingRockMotion = UltimateSwingAnimation.Object;
	BerserkOrbClass = BerserkOrbBlueprint.Succeeded() ? BerserkOrbBlueprint.Class.Get() : ABossBerserkActor::StaticClass();
	VortexClass = VortexBlueprint.Succeeded() ? VortexBlueprint.Class.Get() : ABossVortexActor::StaticClass();
	if (FallingRockWarning.Succeeded()) FallingRockWarningEffect = FallingRockWarning.Object;
	if (DeathDisappearParticle.Succeeded()) DeathDisappearEffect = DeathDisappearParticle.Object;
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

	// 다른 팀원의 Single Node 패턴에서만 사용하는 복귀 포즈
	if (IdleAnimation.Succeeded()) IdleMotion = IdleAnimation.Object;
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
}

// BP PatternSetter의 이동속도를 실제 CharacterMovement에 적용
void ABossCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	BossAnimationClass = GetMesh()->AnimClass;
	if (!BossAnimationClass || !BossAnimationClass->IsChildOf(UBossAnimInstance::StaticClass()))
	{
		BossAnimationClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Boss/Animations/ABP_Boss.ABP_Boss_C"));
	}
	RestoreBossAnimationBlueprint();
	GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.f, BossMovementSpeed);
	// 체력 컴포넌트 사망 이벤트를 캐릭터 연출과 AI 정지 처리에 연결
	if (BossStatComponent) BossStatComponent->OnBossDied.AddUniqueDynamic(this, &ABossCharacterBase::HandleBossDied);
}

// Player 0의 실제 키 입력 상태에서 숫자 0 Pressed 감지
void ABossCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// 팀원 패턴이 끝나 BT의 대기로 돌아오면 Idle/Walk 상태 머신을 복구
	if (bLegacyPatternAnimation)
	{
		const AAIController* AI = Cast<AAIController>(GetController());
		const UBehaviorTreeComponent* Tree = AI ? Cast<UBehaviorTreeComponent>(AI->GetBrainComponent()) : nullptr;
		const UBTNode* ActiveNode = Tree ? Tree->GetActiveNode() : nullptr;
		if (!Tree || !Tree->IsRunning() || (ActiveNode && ActiveNode->IsA<UBTTask_Wait>())) RestoreBossAnimationBlueprint();
	}
	if (!bEnablePhaseDebugInput || !GetWorld()) return;
	// 현재 플레이어 캐릭터를 조작하는 Player 0 Controller
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController) return;
	// 상단 숫자 0 또는 숫자패드 0의 이번 프레임 입력 여부
	const bool bDebugKeyPressed = PlayerController->WasInputKeyJustPressed(EKeys::Zero) || PlayerController->WasInputKeyJustPressed(EKeys::NumPadZero);
	if (bDebugKeyPressed) CycleDebugHealthPhase();
}

// Phase1 최대 체력·Phase2 50%·Phase3 40 체력 순환 적용, 사망 상태면 함께 부활
void ABossCharacterBase::CycleDebugHealthPhase()
{
	if (!BossStatComponent) return;
	// 디버그 체력 적용 전 사망 상태
	const bool bWasDead = BossStatComponent->IsDead();
	// Phase1·Phase2는 최대 체력 비율, Phase3는 요청된 고정 체력 40 사용
	const float DebugHealthValues[] = { BossStatComponent->GetMaxHealth(), BossStatComponent->GetMaxHealth() * 0.5f, 40.f };
	// 현재 순번에 대응하는 디버그 체력
	const float DebugHealth = DebugHealthValues[DebugPhaseIndex];
	BossStatComponent->SetHealthForDebug(DebugHealth);
	if (bWasDead && !BossStatComponent->IsDead()) ReviveBossForDebug();
	DebugPhaseIndex = (DebugPhaseIndex + 1) % UE_ARRAY_COUNT(DebugHealthValues);
	// 화면에 표시할 현재 페이즈 번호
	const int32 PhaseNumber = static_cast<int32>(BossStatComponent->GetCurrentPhase()) + 1;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Boss Debug - Health: %.0f / %.0f, Phase: %d"), BossStatComponent->GetCurrentHealth(), BossStatComponent->GetMaxHealth(), PhaseNumber));
}

// Ultimate Swing 재생과 0.8초 뒤 낙석 발사 예약
bool ABossCharacterBase::StartFallingRock(const FVector& Target)
{
	if (IsPatternRunning() || !FallingRockMotion || !FallingRockClass || !GetWorld()) return false;
	const float Duration = FallingRockMotion->GetPlayLength();
	if (Duration <= 0.f) return false;
	RestoreBossAnimationBlueprint();
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
	// 낙석 시전 시작 보이스 1회 재생
	if (FallingRockVoice) UGameplayStatics::PlaySoundAtLocation(this, FallingRockVoice, GetActorLocation());
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

// 진행 중 행동을 종료하고 이동·BT 정지 후 사망 모션 한 번 재생
void ABossCharacterBase::HandleBossDied()
{
	// 사망 이후 예약된 패턴 생성과 Idle 복귀 차단
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

	// 패턴 실행 상태와 잔여 패턴 액터 정리
	bMagicAttackRunning = false;
	bMagicAttackLaunched = false;
	bFallingRockRunning = false;
	bBerserkRunning = false;
	bVortexRunning = false;
	bVortexCasting = false;
	PendingVortexTarget.Reset();
	if (ActiveVortex.IsValid()) ActiveVortex->Destroy();
	ActiveVortex.Reset();
	for (const TWeakObjectPtr<ABossBerserkActor>& Orb : ActiveBerserkOrbs)
	{
		if (Orb.IsValid()) Orb->Destroy();
	}
	ActiveBerserkOrbs.Reset();
	RemainingBerserkOrbs = 0;
	ClearFallingRockWarning();

	// CharacterMovement와 AIController의 이동·Behavior Tree 실행 정지
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	if (ABossAIController* BossController = Cast<ABossAIController>(GetController()))
	{
		BossController->StopBossBehavior();
	}

	// 액터를 유지한 채 느려진 사망 모션 한 번 재생, 실제 디스폰은 GameMode 담당
	if (DeathVoice) UGameplayStatics::PlaySoundAtLocation(this, DeathVoice, GetActorLocation());
	// 사망 모션의 실제 재생시간, PlayRate 감소분 반영
	const float SafeDeathPlayRate = FMath::Max(DeathAnimationPlayRate, 0.01f);
	const float DeathPresentationDuration = DeathMotion ? DeathMotion->GetPlayLength() / SafeDeathPlayRate : 1.f;
	RestoreBossAnimationBlueprint();
	GetWorldTimerManager().SetTimer(DeathDisappearTimer, this, &ABossCharacterBase::FinishBossDeathPresentation, DeathPresentationDuration, false);
}

// 영혼 폭발 이펙트 생성 후 보스를 화면과 충돌에서 제외
void ABossCharacterBase::FinishBossDeathPresentation()
{
	if (DeathDisappearEffect && GetWorld())
	{
		// 중심 1개와 주변 방향에 배치할 이펙트 개수
		const int32 SafeEffectCount = FMath::Max(1, DeathDisappearEffectCount);
		for (int32 EffectIndex = 0; EffectIndex < SafeEffectCount; ++EffectIndex)
		{
			// 첫 이펙트는 몸 중심, 나머지는 원형으로 균등 배치
			const bool bIsCenterEffect = EffectIndex == 0;
			// 주변 이펙트의 수평 배치 각도
			const float AngleDegrees = bIsCenterEffect ? 0.f : 360.f * static_cast<float>(EffectIndex - 1) / static_cast<float>(SafeEffectCount - 1);
			// 각도를 위치 계산용 라디안으로 변환
			const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
			// 보스의 상체와 주변을 함께 덮도록 높이를 번갈아 적용
			const float HeightOffset = bIsCenterEffect ? 120.f : 70.f + static_cast<float>(EffectIndex % 2) * 100.f;
			// 중심 또는 보스 주변 원형 배치 오프셋
			const FVector EffectOffset = bIsCenterEffect
				? FVector(0.f, 0.f, HeightOffset)
				: FVector(FMath::Cos(AngleRadians) * DeathDisappearEffectRadius, FMath::Sin(AngleRadians) * DeathDisappearEffectRadius, HeightOffset);
			// 방사형으로 퍼져 보이도록 각 이펙트에 서로 다른 회전 적용
			const FRotator EffectRotation(bIsCenterEffect ? 0.f : (EffectIndex % 2 == 0 ? 25.f : -25.f), AngleDegrees, AngleDegrees * 0.5f);
			// 중심 이펙트를 가장 크게, 주변 이펙트는 시야를 가리지 않도록 약간 축소
			const float InstanceScale = DeathDisappearEffectScale * (bIsCenterEffect ? 1.f : 0.8f);
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(), DeathDisappearEffect,
				FTransform(EffectRotation, GetActorLocation() + EffectOffset, FVector(InstanceScale)),
				true, EPSCPoolMethod::AutoRelease, true);
		}
	}
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
}

// 사망 상태에서 숫자 0 입력 시 보스 표시·이동·Idle·Behavior Tree 복구
void ABossCharacterBase::ReviveBossForDebug()
{
	GetWorldTimerManager().ClearTimer(DeathDisappearTimer);
	RestoreBossAnimationBlueprint();
	GetMesh()->InitAnim(true);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	if (BossStatComponent) BossStatComponent->SetInvulnerable(false);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.f, BossMovementSpeed);
	if (ABossAIController* BossController = Cast<ABossAIController>(GetController()))
	{
		BossController->RestartBossBehavior();
	}
}

// Cast 시작과 발사 예약, 애니메이션 상태와 독립된 타이머로 시전 종료 보장
bool ABossCharacterBase::StartMagicAttack(const FVector& Target)
{
	if (IsPatternRunning() || !CastMotion || !MagicAttackClass || !GetWorld()) return false;
	// Cast 재생 길이, 초
	const float Duration = CastMotion->GetPlayLength();
	if (Duration <= 0.f) return false;
	RestoreBossAnimationBlueprint();
	MagicTarget = Target;
	// 목표를 향한 수평 방향으로 보스 회전
	const FVector ToTarget = MagicTarget - GetActorLocation();
	if (!ToTarget.IsNearlyZero()) SetActorRotation(FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f));
	bMagicAttackRunning = true;
	bMagicAttackLaunched = false;
	// 마법공격 시전 시작 보이스 1회 재생
	if (MagicAttackVoice) UGameplayStatics::PlaySoundAtLocation(this, MagicAttackVoice, GetActorLocation());
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
}

// 패턴 실행 상태·거리·무피격 시간·쿨타임 기준 발악 가능 여부 반환
bool ABossCharacterBase::CanStartBerserk(AActor* TargetActor) const
{
	if (!TargetActor || IsPatternRunning() || !BerserkOrbClass || !GetWorld() || !BossStatComponent || BossStatComponent->IsDead()) return false;
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
	RestoreBossAnimationBlueprint();
	bBerserkRunning = true;
	NextBerserkAvailableTime = GetWorld()->GetTimeSeconds() + BerserkCooldown;
	BossStatComponent->SetInvulnerable(true);
	// 발악 시작 보이스 1회 재생
	if (BerserkVoice) UGameplayStatics::PlaySoundAtLocation(this, BerserkVoice, GetActorLocation());
	const FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	if (!ToTarget.IsNearlyZero()) SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
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
}

// Phase1·사거리·쿨타임 확인 후 Cast와 0.3초 생성 예약 시작
bool ABossCharacterBase::StartVortex(AActor* TargetActor)
{
	if (!CanStartVortex(TargetActor)) return false;
	RestoreBossAnimationBlueprint();
	PendingVortexTarget = TargetActor;
	bVortexRunning = true;
	bVortexCasting = true;
	NextVortexAvailableTime = GetWorld()->GetTimeSeconds() + VortexCooldown;
	// 소용돌이 시전 시작 보이스 1회 재생
	if (VortexVoice) UGameplayStatics::PlaySoundAtLocation(this, VortexVoice, GetActorLocation());
	const FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	if (!ToTarget.IsNearlyZero()) SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
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
}

// 종료 시 예약된 마법 시전 타이머 해제
void ABossCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BossStatComponent) BossStatComponent->OnBossDied.RemoveDynamic(this, &ABossCharacterBase::HandleBossDied);
	GetWorldTimerManager().ClearTimer(DeathDisappearTimer);
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

// ABP의 Blend Poses 입력 순서. 사망이 1순위
int32 ABossCharacterBase::GetAnimationMotionIndex() const
{
	if (BossStatComponent && BossStatComponent->IsDead()) return 5;
	if (bBerserkRunning) return 3;
	if (bFallingRockRunning) return 2;
	if (bMagicAttackRunning) return 1;
	if (bVortexCasting) return 4;
	
	
	return 0;
}

// 기존 패턴 모션 그대로 실행
void ABossCharacterBase::PreparePatternAnimation(EBossPattern Pattern)
{
	const bool bLegacy = Pattern == EBossPattern::GroundSmash || Pattern == EBossPattern::CenterProjectile || Pattern == EBossPattern::BlackHole;
	if (bLegacy && !(BossStatComponent && BossStatComponent->IsDead()))
	{
		bLegacyPatternAnimation = true;
		GetMesh()->PlayAnimation(IdleMotion, true);
	}
	else RestoreBossAnimationBlueprint();
}

void ABossCharacterBase::RestoreBossAnimationBlueprint()
{
	bLegacyPatternAnimation = false;
	if (!BossAnimationClass) return;
	if (GetMesh()->AnimClass != BossAnimationClass) GetMesh()->SetAnimInstanceClass(BossAnimationClass);
	if (GetMesh()->GetAnimationMode() != EAnimationMode::AnimationBlueprint)
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	}
}
