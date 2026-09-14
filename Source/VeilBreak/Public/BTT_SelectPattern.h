#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_SelectPattern.generated.h"

// 페이즈·사거리·쿨타임·가중치 기반 일반 패턴 선택용 BT 태스크 골격, 발악은 별도 판단
UCLASS()
class VEILBREAK_API UBTT_SelectPattern : public UBTTaskNode
{
	GENERATED_BODY()
};
