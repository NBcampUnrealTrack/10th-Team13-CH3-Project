#include "BossAnimInstance.h"
#include "BossCharacterBase.h"

void UBossAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const ABossCharacterBase* Boss = Cast<ABossCharacterBase>(TryGetPawnOwner());
	GroundSpeed = Boss ? Boss->GetVelocity().Size2D() : 0.f;
	// 애매한 속도일때 Idle/Walk 전환이 매 프레임 이뤄지지 않게
	bMoving = bMoving ? GroundSpeed > 3.f : GroundSpeed > 10.f;
	bIdle = !bMoving;
	MotionIndex = Boss ? Boss->GetAnimationMotionIndex() : 0;
}
