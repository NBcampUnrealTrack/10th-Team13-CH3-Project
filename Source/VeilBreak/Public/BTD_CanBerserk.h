#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTD_CanBerserk.generated.h"

// 플레이어 20초 이상 무피격·발악 쿨타임·사거리 검사용 데코레이터
UCLASS()
class VEILBREAK_API UBTD_CanBerserk : public UBTDecorator
{
	GENERATED_BODY()

public:
	// 발악 조건 검사 노드 표시명 설정
	UBTD_CanBerserk();
	// 플레이어 20초 무피격·120초 쿨타임·보스 상태 검사
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
