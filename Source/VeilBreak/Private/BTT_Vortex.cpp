#include "BTT_Vortex.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BossCharacterBase.h"

// BT에 표시할 소용돌이 태스크 이름 설정
UBTT_Vortex::UBTT_Vortex()
{
	NodeName = TEXT("Vortex (Chase Target / 10 Seconds)");
}

// BB TargetActor를 대상으로 소용돌이 소환 후 즉시 성공 반환
EBTNodeResult::Type UBTT_Vortex::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	ABossCharacterBase* Boss = Controller ? Cast<ABossCharacterBase>(Controller->GetPawn()) : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;
	if (!Boss || !TargetActor) return EBTNodeResult::Failed;
	Controller->StopMovement();
	return Boss->StartVortex(TargetActor) ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
