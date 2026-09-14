#include "BTD_CheckPhase.h"
#include "BehaviorTree/BlackboardComponent.h"

// BT 노드 표시명 설정
UBTD_CheckPhase::UBTD_CheckPhase()
{
	NodeName = TEXT("Check Phase");
}

// Blackboard CurrentPhase와 설정된 요구 페이즈 비교
bool UBTD_CheckPhase::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	return Blackboard && Blackboard->GetValueAsInt(TEXT("CurrentPhase")) == static_cast<int32>(RequiredPhase);
}
