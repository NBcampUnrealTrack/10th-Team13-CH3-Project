#include "BTT_MagicAttack.h"
#include "BossCharacterBase.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"

// 실행 상태를 보스별로 분리
UBTT_MagicAttack::UBTT_MagicAttack()
{
    NodeName = TEXT("MagicAttack (Cast -> Player Location)");
    bCreateNodeInstance = true;
    bNotifyTick = true;
}

// 시전 순간 플레이어 위치로 시전 요청, Cast 종료까지 대기
EBTNodeResult::Type UBTT_MagicAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 실행 대상 보스
    ABossCharacterBase* Boss = OwnerComp.GetAIOwner() ? Cast<ABossCharacterBase>(OwnerComp.GetAIOwner()->GetPawn()) : nullptr;
    // BB에 저장된 시전 대상
    AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TEXT("TargetActor")));
    if (!Boss || !TargetActor || !Boss->StartMagicAttack(TargetActor->GetActorLocation())) return EBTNodeResult::Failed;
    StartedAt = Boss->GetWorld()->GetTimeSeconds();
    return EBTNodeResult::InProgress;
}

// Cast 소요 시간을 5초 주기에서 차감, 다음 Idle 대기값 설정
void UBTT_MagicAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // 시전 중인 보스
    ABossCharacterBase* Boss = OwnerComp.GetAIOwner() ? Cast<ABossCharacterBase>(OwnerComp.GetAIOwner()->GetPawn()) : nullptr;
    if (!Boss) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }
    if (Boss->IsMagicAttackRunning()) return;
    // 태스크가 실제로 기다린 시전 시간
    const float Elapsed = Boss->GetWorld()->GetTimeSeconds() - StartedAt;
    OwnerComp.GetBlackboardComponent()->SetValueAsFloat(TEXT("MagicAttackWaitTime"), FMath::Max(0.01f, Boss->GetMagicAttackInterval() - Elapsed));
    FinishLatentTask(OwnerComp, Boss->DidMagicAttackLaunch() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
}
