#include "BTT_Berserk.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BossCharacterBase.h"

// 보스별 실행 상태와 발악 종료 감지 Tick 활성화
UBTT_Berserk::UBTT_Berserk()
{
	NodeName = TEXT("Berserk (5 Orbs / 20 Seconds)");
	bCreateNodeInstance = true;
	bNotifyTick = true;
}

// BB TargetActor의 발악 조건 확인 후 패턴 시작 요청
EBTNodeResult::Type UBTT_Berserk::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	ABossCharacterBase* Boss = Controller ? Cast<ABossCharacterBase>(Controller->GetPawn()) : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;
	if (!Boss || !TargetActor) return EBTNodeResult::Failed;
	Controller->StopMovement();
	return Boss->StartBerserk(TargetActor) ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

// 구체 전부 파괴 또는 시간초과로 발악이 끝나면 태스크 완료
void UBTT_Berserk::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	ABossCharacterBase* Boss = Controller ? Cast<ABossCharacterBase>(Controller->GetPawn()) : nullptr;
	if (!Boss) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }
	if (!Boss->IsBerserkRunning()) FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
}
