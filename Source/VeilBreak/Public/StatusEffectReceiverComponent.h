#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatusEffectReceiverComponent.generated.h"

UCLASS(
	ClassGroup = (Custom),
	meta = (BlueprintSpawnableComponent)
)
class VEILBREAK_API UStatusEffectReceiverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// »ý¼ºÀÚ
	UStatusEffectReceiverComponent();

protected:
	// Unreal Override
	virtual void BeginPlay() override;
};