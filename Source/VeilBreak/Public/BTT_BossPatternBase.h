#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_BossPatternBase.generated.h"

// 패턴 태스크 공통 부모 골격, 실행 요청·완료 대기·이벤트 연결 정리 구현 예정
UCLASS()
class VEILBREAK_API UBTT_BossPatternBase : public UBTTaskNode
{
	GENERATED_BODY()
};
