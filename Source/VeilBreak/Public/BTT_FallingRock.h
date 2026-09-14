#pragma once

#include "CoreMinimal.h"
#include "BTT_BossPatternBase.h"
#include "BTT_FallingRock.generated.h"

// 낙석 시전 요청·Ultimate Swing 종료 대기용 BT 태스크
UCLASS()
class VEILBREAK_API UBTT_FallingRock : public UBTT_BossPatternBase
{
	GENERATED_BODY()

public:
	// 태스크 Tick과 보스별 실행 인스턴스 활성화
	UBTT_FallingRock();
	// 사거리 안의 TargetActor 좌표로 낙석 시전 시작
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
protected:
	// Ultimate Swing 종료 시 태스크 성공 처리
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
