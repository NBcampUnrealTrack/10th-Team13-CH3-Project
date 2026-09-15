#pragma once

#include "CoreMinimal.h"
#include "BTT_BossPatternBase.h"
#include "BTT_Vortex.generated.h"

// 1페이즈 소용돌이 소환 후 다음 행동을 허용하는 BT 태스크
UCLASS()
class VEILBREAK_API UBTT_Vortex : public UBTT_BossPatternBase
{
	GENERATED_BODY()

public:
	// 소용돌이 소환 태스크 표시명 설정
	UBTT_Vortex();
	// 소용돌이를 소환하고 즉시 성공 처리해 다음 패턴 허용
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
