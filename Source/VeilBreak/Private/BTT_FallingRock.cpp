#include "BTT_FallingRock.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BossCharacterBase.h"

// 낙석 태스크 실행 인스턴스와 Tick 활성화
UBTT_FallingRock::UBTT_FallingRock()
{
	NodeName = TEXT("FallingRock (Ultimate Swing -> Target Location)");
	bCreateNodeInstance = true;
	bNotifyTick = true;
}

// TargetActor가 FallingRock 사거리 안이면 낙석 시전 시작
EBTNodeResult::Type UBTT_FallingRock::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	ABossCharacterBase* Boss = Controller ? Cast<ABossCharacterBase>(Controller->GetPawn()) : nullptr;
	AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TEXT("TargetActor")));
	if (!Boss || !TargetActor || FVector::DistSquared(Boss->GetActorLocation(), TargetActor->GetActorLocation()) > FMath::Square(Boss->GetFallingRockRange())) return EBTNodeResult::Failed;
	Controller->StopMovement();
	return Boss->StartFallingRock(TargetActor->GetActorLocation()) ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

// 낙석 시전 종료 확인 후 태스크 성공 처리
void UBTT_FallingRock::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	ABossCharacterBase* Boss = Controller ? Cast<ABossCharacterBase>(Controller->GetPawn()) : nullptr;
	if (!Boss) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }
	if (!Boss->IsFallingRockRunning()) FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
}
