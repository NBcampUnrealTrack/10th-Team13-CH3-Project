#pragma once

#include "CoreMinimal.h"
#include "BossPatternActorBase.h"
#include "BossFallingRockActor.generated.h"

// 낙석 생성·낙하·착지 피해용 액터 골격, BP_BossFallingRock의 부모
UCLASS(Blueprintable)
class VEILBREAK_API ABossFallingRockActor : public ABossPatternActorBase
{
	GENERATED_BODY()
};
