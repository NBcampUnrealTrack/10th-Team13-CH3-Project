#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_CenterProjectile.generated.h"

class ACenterProjectile;

UCLASS()
class VEILBREAK_API UBTT_CenterProjectile : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTT_CenterProjectile();

protected:
    virtual EBTNodeResult::Type ExecuteTask(//공격 시작
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory
    ) override;

    virtual void TickTask(//공격이 끝났는지 확인중
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        float DeltaSeconds
    ) override;

    virtual EBTNodeResult::Type AbortTask(//다른 행동으로 바꾸면 정리
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory
    ) override;

    UPROPERTY(EditAnywhere, Category = "Attack")
    TSubclassOf<ACenterProjectile> AttackClass;//선택할 공격
    
    UPROPERTY(EditDefaultsOnly, Category = "Projectile")
    TSubclassOf<class ABossMagicAttackActor> ProjectileClass;

private:
    UPROPERTY()
    TObjectPtr<ACenterProjectile> ActiveAttack;

    void CleanupAttack();//생성한 오브젝트 제거



};