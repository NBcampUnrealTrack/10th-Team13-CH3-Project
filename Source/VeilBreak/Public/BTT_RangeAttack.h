#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_RangeAttack.generated.h"

class AGroundSmashAttack;

UCLASS()
class VEILBREAK_API UBTT_RangeAttack : public UBTTaskNode
{
    GENERATED_BODY()
public:
    UBTT_RangeAttack();

protected:
    virtual EBTNodeResult::Type ExecuteTask(//BT가 이 Task를 선택하면 실행댐
        UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(
        UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual EBTNodeResult::Type AbortTask(
        UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    UPROPERTY(EditAnywhere, Category="Attack")
    TSubclassOf<AGroundSmashAttack> AttackClass;//어떤 공격 클래스를 생성할지 저장

private:
    UPROPERTY()
    TObjectPtr<AGroundSmashAttack> ActiveAttack;//실제로 생성된 공격 액터
    void CleanupAttack();//남은 공격 정리
    /*
    AttackClass  → 생성할 프리팹에 가까움
    ActiveAttack → Instantiate로 생성한 오브젝트에 가까움
    */

};