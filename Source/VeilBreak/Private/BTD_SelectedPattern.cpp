#include "BTD_SelectedPattern.h"
#include "BehaviorTree/BlackboardComponent.h"

// BT 노드 표시명 설정
UBTD_SelectedPattern::UBTD_SelectedPattern()
{
	NodeName = TEXT("Check Selected Pattern");
}

// Blackboard SelectedPattern과 요구 패턴 비교
bool UBTD_SelectedPattern::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	return Blackboard && Blackboard->GetValueAsInt(TEXT("SelectedPattern")) == static_cast<int32>(RequiredPattern);
}
