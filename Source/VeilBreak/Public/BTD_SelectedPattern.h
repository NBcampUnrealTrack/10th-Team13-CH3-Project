#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BossData.h"
#include "BTD_SelectedPattern.generated.h"

// Blackboard SelectedPattern과 요구 패턴 일치 여부 검사 데코레이터
UCLASS()
class VEILBREAK_API UBTD_SelectedPattern : public UBTDecorator
{
	GENERATED_BODY()

public:
	// 선택 패턴 검사 노드 표시명 설정
	UBTD_SelectedPattern();
	// SelectedPattern이 RequiredPattern과 같을 때 true 반환
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	// 통과가 필요한 선택 패턴
	UPROPERTY(EditAnywhere, Category="Boss|Pattern")
	EBossPattern RequiredPattern = EBossPattern::MagicAttack;
};
