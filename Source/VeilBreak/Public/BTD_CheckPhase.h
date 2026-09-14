#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTD_CheckPhase.generated.h"

// 현재 페이즈와 분기 요구 페이즈의 일치 여부 검사용 데코레이터 골격
UCLASS()
class VEILBREAK_API UBTD_CheckPhase : public UBTDecorator
{
	GENERATED_BODY()
};
