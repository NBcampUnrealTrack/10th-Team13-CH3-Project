// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttackRangeBase.h"
#include "CenterProjectile.generated.h"

/**
 * 
 */
UCLASS()
class VEILBREAK_API ACenterProjectile : public AAttackRangeBase
{
	GENERATED_BODY()

private:
	void FireProjectiles();
	void EndPattern();

	FTimerHandle FireTimer;
	FTimerHandle PatternTimer;
	int FireCount = 0;//몇번째 발사인지



protected:
	virtual void ActivateAttack() override;

	//발사할 투사체 선택
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class ACenterMagicProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	int ProjectileCount = 4;//발사 개수
	
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float FireInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float PatternDuration = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float RotationPerShot = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float VerticalAngle = 10.0f;

	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason
	) override;


};

