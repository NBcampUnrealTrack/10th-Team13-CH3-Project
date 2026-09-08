#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossCombatComponent.generated.h"

// 페이즈·패턴 상태·쿨타임 관리용 컴포넌트 골격, 체력 관리는 BossStatComponent 담당
UCLASS(ClassGroup=(Boss), meta=(BlueprintSpawnableComponent))
class VEILBREAK_API UBossCombatComponent : public UActorComponent
{
	GENERATED_BODY()
};
