#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_UpdateBossContext.generated.h"

// 플레이어 대상과 보스 거리 갱신용 BT 서비스
UCLASS()
class VEILBREAK_API UBTS_UpdateBossContext : public UBTService
{
	GENERATED_BODY()

public:
	// 0.1초 간격 대상 갱신 기본값
	UBTS_UpdateBossContext();
protected:
	// Player 0을 찾아 TargetActor·TargetDistance 갱신
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
