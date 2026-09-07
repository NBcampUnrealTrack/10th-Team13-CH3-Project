#include "StatusEffectReceiverComponent.h"

UStatusEffectReceiverComponent::UStatusEffectReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatusEffectReceiverComponent::BeginPlay()
{
	Super::BeginPlay();
}