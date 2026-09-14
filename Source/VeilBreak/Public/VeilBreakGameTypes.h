#pragma once

#include "CoreMinimal.h"
#include "VeilBreakGameTypes.generated.h"

// 현재 게임 진행 상태
UENUM(BlueprintType)
enum class EVeilBreakGameLoopState : uint8
{
	Waiting,
	Combat,
	Victory,
	Defeat
};
//Waiting : 전투준비
//Combat : 전투 중
//Victory : 보스 사망, PC 승리
//Defeat : 플레이어 사망, PC 패배

// 현재 보스 페이즈
UENUM(BlueprintType)
enum class EVeilBreakBossPhase : uint8
{
	Phase1,
	Phase2,
	Phase3,
	Dead
};
//보스 AI의 Blackboard에서 사용

// 최종 전투 결과(UI에 전달)
UENUM(BlueprintType)
enum class EVeilBreakBattleResult : uint8
{
	None,
	Victory,
	Defeat
};