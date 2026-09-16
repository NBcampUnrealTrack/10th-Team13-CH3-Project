#include "BTT_RangeAttack.h"
#include "GroundSmashAttack.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

UBTT_RangeAttack::UBTT_RangeAttack()
{
    NodeName = TEXT("Ground Smash (Animation -> Wave)");//NodeName: BT에서 보이는 이름
    bNotifyTick = true;//Task 실행 중 TickTask 설정
    bCreateNodeInstance = true;//AI마다 Task 객체 사용
}

EBTNodeResult::Type UBTT_RangeAttack::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UE_LOG(LogTemp, Warning, TEXT("ENTER GroundSmash Task"));
    CleanupAttack();
    AAIController* Controller = OwnerComp.GetAIOwner();
    ACharacter* Boss = nullptr;

    if (IsValid(Controller))
    {
        Boss = Cast<ACharacter>(Controller->GetPawn());
    }

    FActorSpawnParameters Params;
    Params.Owner = Boss;
    Params.Instigator = Boss;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ActiveAttack = Boss->GetWorld()->SpawnActor<AGroundSmashAttack>(
        AttackClass, Boss->GetActorLocation(), Boss->GetActorRotation(), Params);

    if (!IsValid(ActiveAttack) || !ActiveAttack->StartAnimatedAttack())//생성, 애니메 실패하면 실패처리
    {
        CleanupAttack();
        return EBTNodeResult::Failed;
    }
    return EBTNodeResult::InProgress;
}

void UBTT_RangeAttack::TickTask(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);//Task 진행중 반복
    if (!IsValid(ActiveAttack) || !IsValid(ActiveAttack->GetOwner()))//공격, 보스가 사라지면 종료
    {
        CleanupAttack();
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    if (ActiveAttack->IsAttacking()) return;
    /*
    공격 중? -> true -> 이번 Tick 종료 -> 다음 Tick에서 재확인
    -> false -> 공격 액터 정리 -> Task 완료
    */
    CleanupAttack();
    FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
}

EBTNodeResult::Type UBTT_RangeAttack::AbortTask(//BT가 공격을 중단하면
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    CleanupAttack();
    return EBTNodeResult::Aborted;
}

void UBTT_RangeAttack::CleanupAttack()//취소 삭제
{
    if (IsValid(ActiveAttack))
    {
        if (ActiveAttack->IsAttacking()) ActiveAttack->CancelAttack();
        ActiveAttack->Destroy();
    }
    ActiveAttack = nullptr;
}