
#include "BossCharacterBase.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/ConstructorHelpers.h"

// 생성자: 로드 성공한 에셋을 Mesh 기본값에 적용, 상속 BP에서 변경 가능
ABossCharacterBase::ABossCharacterBase()
{
	// Sevarog 메시
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BossMesh(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog.Sevarog"));
	// Sevarog 스켈레톤용 idle 모션
	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimation(
		TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Idle.Idle"));
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
		// 생성된 인스턴스가 idle을 자동 반복 재생
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		GetMesh()->AnimationData.AnimToPlay = IdleAnimation.Object;
		GetMesh()->AnimationData.bSavedLooping = true;
		GetMesh()->AnimationData.bSavedPlaying = true;
		GetMesh()->AnimationData.SavedPlayRate = 1.0f;
	}
}
