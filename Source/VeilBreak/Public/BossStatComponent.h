#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossStatComponent.generated.h"

// 체력·무적·사망 상태 및 알림 관리용 컴포넌트 골격
UCLASS(ClassGroup=(Boss), meta=(BlueprintSpawnableComponent))
class VEILBREAK_API UBossStatComponent : public UActorComponent
{
	GENERATED_BODY()
};
