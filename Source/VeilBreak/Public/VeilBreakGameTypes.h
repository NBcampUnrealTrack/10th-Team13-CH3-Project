#pragma once

#include "CoreMinimal.h"
#include "BossData.h"
#include "VeilBreakGameTypes.generated.h"

// 현재 게임 진행 상태
UENUM(BlueprintType)
enum class EVeilBreakGameLoopState : uint8
{
	Waiting UMETA(DisplayName = "Waiting"),
	Combat  UMETA(DisplayName = "Combat"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat  UMETA(DisplayName = "Defeat")
};

// 최종 전투 결과
UENUM(BlueprintType)
enum class EVeilBreakBattleResult : uint8
{
	None    UMETA(DisplayName = "None"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat  UMETA(DisplayName = "Defeat")
};