#pragma once

#include "CoreMinimal.h"
#include "BossPatternActorBase.h"
#include "AttackRangeBase.generated.h"

UCLASS()
class VEILBREAK_API AAttackRangeBase : public ABossPatternActorBase
{
    GENERATED_BODY()

public:
    //공격 시작
    void StartAttack();

    //공격 중인지?
    bool IsAttacking() const { return isAttacking; }

    //공격 스탑
    void CancelAttack();

protected:
    //지금 공격
    virtual void ActivateAttack();

    // 공격 종료
    void FinishAttack();

    UPROPERTY(EditAnywhere, Category = "Attack")
    float Damage = 80.0f;

    UPROPERTY(EditAnywhere, Category = "Attack")
    float WarningDuration = 1.0f;

private:
    bool isAttacking = false;

    FTimerHandle WarningTimer;
};

//설정				    적용되는 곳	    의미
//VisibleAnywhere	    에디터 Details	보기만 가능
//EditAnywhere		    에디터 Details	수정 가능
//BlueprintReadOnly	    BP 그래프		Get 가능
//BlueprintReadWrite	BP 그래프		Get·Set 가능