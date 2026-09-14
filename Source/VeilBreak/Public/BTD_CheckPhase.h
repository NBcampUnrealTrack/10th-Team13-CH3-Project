#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BossData.h"
#include "BTD_CheckPhase.generated.h"

// Blackboard CurrentPhase와 요구 페이즈 일치 여부 검사 데코레이터
UCLASS()
class VEILBREAK_API UBTD_CheckPhase : public UBTDecorator
{
	GENERATED_BODY()

public:
	// 현재 페이즈 검사 태스크 표시명 설정
	UBTD_CheckPhase();
	// Blackboard CurrentPhase가 RequiredPhase와 같을 때 true 반환
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	// 통과가 필요한 페이즈
	UPROPERTY(EditAnywhere, Category="Boss|Phase")
	EBossPhase RequiredPhase = EBossPhase::Phase2;
};
