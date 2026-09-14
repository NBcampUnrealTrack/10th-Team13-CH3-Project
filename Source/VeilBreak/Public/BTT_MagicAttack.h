#pragma once
#include "CoreMinimal.h"
#include "BTT_BossPatternBase.h"
#include "BTT_MagicAttack.generated.h"

// MagicAttack 시전 요청과 Cast 모션 종료 대기용 BT 태스크
UCLASS()
class VEILBREAK_API UBTT_MagicAttack : public UBTT_BossPatternBase
{
    GENERATED_BODY()
public:
    // 보스별 실행 인스턴스 및 태스크 Tick 활성화
    UBTT_MagicAttack();
    // 사거리 안의 TargetActor 좌표로 MagicAttack 시전 시작
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
protected:
    // Cast 모션 종료 시 태스크 성공 처리
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
