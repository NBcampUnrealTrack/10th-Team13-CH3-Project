// Fill out your copyright notice in the Description page of Project Settings.


#include "BTT_FindCenterPoint.h"

UBTT_FindCenterPoint::UBTT_FindCenterPoint()//BT 그래프에서 노드 이름
{
    NodeName = TEXT("Find Center Point");
}
EBTNodeResult::Type UBTT_FindCenterPoint::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory
)
{
    // 중앙 태그가 붙은 액터찾기
    TArray<AActor*> CenterPoints;

    UGameplayStatics::GetAllActorsWithTag(
        &OwnerComp,
        FName(TEXT("BossCenterPoint")),
        CenterPoints
    );


    if (CenterPoints.Num() != 1 || !IsValid(CenterPoints[0]))
    {
       

        return EBTNodeResult::Failed;
    }

    AAIController* Controller = OwnerComp.GetAIOwner();

    if (!IsValid(Controller))
    {
        return EBTNodeResult::Failed;
    }

    APawn* Boss = Controller->GetPawn();

    if (!IsValid(Boss))
    {
        return EBTNodeResult::Failed;
    }

    Controller->StopMovement();

    const bool Teleported = Boss->TeleportTo(
        CenterPoints[0]->GetActorLocation(),
        Boss->GetActorRotation()
    );
   

    if (Teleported && TeleportEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            &OwnerComp,
            TeleportEffect,
            Boss->GetActorLocation() + TeleportEffectOffset,
            FRotator::ZeroRotator,
            TeleportEffectScale
        );
    }

    return Teleported
        ? EBTNodeResult::Succeeded
        : EBTNodeResult::Failed;
}