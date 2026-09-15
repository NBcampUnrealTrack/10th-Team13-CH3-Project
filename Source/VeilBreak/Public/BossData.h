#pragma once

#include "CoreMinimal.h"

// 보스 체력 구간 기반 페이즈 식별값
UENUM(BlueprintType)
enum class EBossPhase : uint8
{
	Phase1 UMETA(DisplayName="Phase 1"),
	Phase2 UMETA(DisplayName="Phase 2"),
	Phase3 UMETA(DisplayName="Phase 3")
};

// BT에서 선택하는 보스 패턴 식별값
UENUM(BlueprintType)
enum class EBossPattern : uint8
{
	MagicAttack UMETA(DisplayName="Magic Attack"),
	FallingRock UMETA(DisplayName="Falling Rock"),
	Berserk UMETA(DisplayName="Berserk"),
	BlackHole UMETA(DisplayName="Black Hole"),
	Vortex UMETA(DisplayName="Vortex")
};
