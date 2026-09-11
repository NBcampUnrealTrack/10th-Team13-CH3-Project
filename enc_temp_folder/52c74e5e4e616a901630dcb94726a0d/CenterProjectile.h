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
protected:
	virtual void ActivateAttack() override;

	//발사할 투사체 선택
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class ABossMagicAttackActor> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	int ProjectileCount = 8;//발사 개수

};
