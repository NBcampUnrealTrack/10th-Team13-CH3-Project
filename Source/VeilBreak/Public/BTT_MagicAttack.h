#pragma once
#include "CoreMinimal.h"
#include "BTT_BossPatternBase.h"
#include "BTT_MagicAttack.generated.h"

// magic attack 시전
UCLASS()
class VEILBREAK_API UBTT_MagicAttack : public UBTT_BossPatternBase
{
    GENERATED_BODY()
public:
    // 보스별 실행 인스턴스 및 태스크 Tick 활성화
    UBTT_MagicAttack();
    // 시전 순간 플레이어 좌표로 시전 시작, 실패 시 태스크 실패 반환
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
protected:
    // 시전 완료 확인 후 다음 대기 시간 저장
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
private:
    // 이번 시전 시작의 월드 시간, 초
    double StartedAt = 0.0;
};
