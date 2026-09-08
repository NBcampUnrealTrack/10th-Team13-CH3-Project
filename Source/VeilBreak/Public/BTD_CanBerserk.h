#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTD_CanBerserk.generated.h"

// 플레이어 20초 이상 무피격·발악 120초 쿨타임 검사용 데코레이터 골격
UCLASS()
class VEILBREAK_API UBTD_CanBerserk : public UBTDecorator
{
	GENERATED_BODY()
};
