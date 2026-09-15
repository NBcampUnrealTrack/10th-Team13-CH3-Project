#include "BTD_CanBerserk.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BossCharacterBase.h"

// BT에 표시할 발악 조건 데코레이터 이름 설정
UBTD_CanBerserk::UBTD_CanBerserk()
{
	NodeName = TEXT("Can Berserk (No Damage 20s / Cooldown 120s)");
}

// BB TargetActor와 보스 내부 발악 가능 조건 비교
bool UBTD_CanBerserk::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* Controller = OwnerComp.GetAIOwner();
	const ABossCharacterBase* Boss = Controller ? Cast<ABossCharacterBase>(Controller->GetPawn()) : nullptr;
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;
	return Boss && Boss->CanStartBerserk(TargetActor);
}
