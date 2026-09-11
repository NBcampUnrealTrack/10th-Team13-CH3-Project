#include "BTT_MagicAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BossCharacterBase.h"

// MagicAttack 태스크 실행 인스턴스와 Tick 활성화
UBTT_MagicAttack::UBTT_MagicAttack()
{
    NodeName = TEXT("MagicAttack (Cast -> Target Location)");
    bCreateNodeInstance = true;
    bNotifyTick = true;
}

// TargetActor가 MagicAttack 사거리 안이면 마법 시전 시작
EBTNodeResult::Type UBTT_MagicAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 보스 AIController 참조
    AAIController* Controller = OwnerComp.GetAIOwner();
    // 현재 제어 중인 보스 Pawn 참조
    ABossCharacterBase* Boss = Controller ? Cast<ABossCharacterBase>(Controller->GetPawn()) : nullptr;
    // Blackboard가 유지하는 시전 대상
    AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TEXT("TargetActor")));
    // 보스·대상 참조 유효성 검사
    if (!Boss || !TargetActor) return EBTNodeResult::Failed;
    // 다른 패턴 시전 상태 확인
    if (Boss->IsPatternRunning()) return EBTNodeResult::Failed;
    // MagicAttack 전용 사거리 검사
    if (FVector::DistSquared(Boss->GetActorLocation(), TargetActor->GetActorLocation()) > FMath::Square(Boss->GetMagicAttackRange())) return EBTNodeResult::Failed;
    // 시전 동안 NavMesh 이동 중지
    Controller->StopMovement();
    // 시전 대상의 현재 위치로 MagicAttack 시작
    return Boss->StartMagicAttack(TargetActor->GetActorLocation()) ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

// MagicAttack 시전 종료 확인 후 태스크 성공 처리
void UBTT_MagicAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // 보스 AIController 참조
    AAIController* Controller = OwnerComp.GetAIOwner();
    // 현재 제어 중인 보스 Pawn 참조
    ABossCharacterBase* Boss = Controller ? Cast<ABossCharacterBase>(Controller->GetPawn()) : nullptr;
    // 보스가 사라진 경우 태스크 실패 처리
    if (!Boss) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }
    // MagicAttack 시전 중이면 종료 대기
    if (Boss->IsMagicAttackRunning()) return;
    // MagicAttack 완료 후 Selector에 성공 반환
    FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
}
