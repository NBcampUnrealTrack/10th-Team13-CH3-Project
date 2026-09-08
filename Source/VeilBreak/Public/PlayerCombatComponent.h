#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerCombatComponent.generated.h"

UCLASS(
	ClassGroup = (Custom),
	meta = (BlueprintSpawnableComponent)
)
class VEILBREAK_API UPlayerCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// »ý¼ºÀÚ
	UPlayerCombatComponent();

protected:
	// Unreal Override
	virtual void BeginPlay() override;
};