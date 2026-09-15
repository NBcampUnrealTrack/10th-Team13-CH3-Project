// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "BTT_FindCenterPoint.generated.h"


UCLASS()
class VEILBREAK_API UBTT_FindCenterPoint : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTT_FindCenterPoint();

protected:
    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory
    ) override;

    UPROPERTY(EditAnywhere, Category = "Teleport")
    TObjectPtr<class UNiagaraSystem> TeleportEffect;

    UPROPERTY(EditAnywhere, Category = "Teleport")
    FVector TeleportEffectOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "Teleport")
    FVector TeleportEffectScale = FVector(1.0f);
};