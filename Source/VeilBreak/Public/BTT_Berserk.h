#pragma once

#include "CoreMinimal.h"
#include "BTT_BossPatternBase.h"
#include "BTT_Berserk.generated.h"

// 발악 실행 요청 후 구체 파괴 또는 시간초과까지 기다리는 BT 태스크
UCLASS()
class VEILBREAK_API UBTT_Berserk : public UBTT_BossPatternBase
{
	GENERATED_BODY()

public:
	// 태스크 Tick과 보스별 실행 인스턴스 활성화
	UBTT_Berserk();
	// TargetActor의 무피격 조건 확인 후 발악 시작
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	// 구체 전부 파괴 또는 제한시간 종료 시 태스크 성공 처리
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
