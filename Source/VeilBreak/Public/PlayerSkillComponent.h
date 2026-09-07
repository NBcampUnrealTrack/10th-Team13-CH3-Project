#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerSkillComponent.generated.h"

UCLASS(
	ClassGroup = (Custom),
	meta = (BlueprintSpawnableComponent)
)
class VEILBREAK_API UPlayerSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// »ý¼ºÀÚ
	UPlayerSkillComponent();

protected:
	// Unreal Override
	virtual void BeginPlay() override;
};