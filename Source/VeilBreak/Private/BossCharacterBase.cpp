
#include "BossCharacterBase.h"
#include "BossAIController.h"
#include "BossMagicAttackActor.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/ConstructorHelpers.h"

// 생성자: 로드 성공한 에셋을 Mesh 기본값에 적용, 상속 BP에서 변경 가능
ABossCharacterBase::ABossCharacterBase()
{
	AIControllerClass = ABossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
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
	if (CastAnimation.Succeeded()) CastMotion = CastAnimation.Object;
	MagicAttackClass = MagicBlueprint.Succeeded() ? MagicBlueprint.Class.Get() : ABossMagicAttackActor::StaticClass();
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
	if (Projectile) { Projectile->LaunchAt(MagicTarget); bMagicAttackLaunched = true; }
}

// 시전 상태 종료, 다음 BT 대기 동안 Idle 유지
void ABossCharacterBase::FinishMagicAttack()
{
	bMagicAttackRunning = false;
	if (IdleMotion) GetMesh()->PlayAnimation(IdleMotion, true);
}

// 어 음.. BT루프에서 예약된 시전을 캔슬하는 용도입니다 , 이번바퀴가 돌고있는중에 보스가 죽는다던가하는
// 정상작동여부는 체크되지 않았습니다, 이렇게 넣으면 좋다 해서 넣음
void ABossCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(MagicReleaseTimer);
	GetWorldTimerManager().ClearTimer(MagicFinishTimer);
	Super::EndPlay(EndPlayReason);
}

