#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_SelectPattern.generated.h"

// 현재 페이즈에서 허용된 패턴 하나를 Blackboard에 저장하는 BT 태스크
UCLASS()
class VEILBREAK_API UBTT_SelectPattern : public UBTTaskNode
{
	GENERATED_BODY()

public:
	// 패턴 선택 태스크 표시명 설정
	UBTT_SelectPattern();
	// Phase1·3 일반 패턴, Phase2 블랙홀 포함 후보 중 하나 선택
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
