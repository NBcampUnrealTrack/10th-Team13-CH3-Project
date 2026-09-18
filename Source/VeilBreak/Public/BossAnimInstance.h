#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BossAnimInstance.generated.h"

// ABP_Boss의 상태 머신과 포즈 블렌딩에 필요한 값만 전달
UCLASS(Transient, Blueprintable)
class VEILBREAK_API UBossAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category="Boss|Animation")
	float GroundSpeed = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Boss|Animation")
	bool bMoving = false;
	UPROPERTY(BlueprintReadOnly, Category="Boss|Animation")
	bool bIdle = true;
	// 0: Idle/Walk, 1: 마법, 2: 낙석, 3: 발악, 4: 소용돌이, 5: 사망
	UPROPERTY(BlueprintReadOnly, Category="Boss|Animation")
	int32 MotionIndex = 0;
};
