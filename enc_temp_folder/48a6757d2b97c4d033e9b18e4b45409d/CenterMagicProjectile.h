// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BossPatternActorBase.h"
#include "CenterMagicProjectile.generated.h"

/**
 * 
 */
UCLASS()
class VEILBREAK_API ACenterMagicProjectile : public ABossPatternActorBase
{
	GENERATED_BODY()
public:
    ACenterMagicProjectile();

    void Launch(FVector Direction);

    virtual void Tick(float DeltaTime) override;

protected:
    UPROPERTY(EditAnywhere, Category = "Projectile")
    float Speed = 1200.0f;

private:
    FVector MoveDirection = FVector::ZeroVector;
};
