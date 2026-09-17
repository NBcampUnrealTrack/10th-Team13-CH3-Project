// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmmoItem.generated.h"

UCLASS()
class VEILBREAK_API AAmmoItem : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAmmoItem();

protected:
    virtual void BeginPlay() override;

    // 접촉 감지용 SphereComponent
    // 외형 표시용 StaticMeshComponent

    // BP에서 정할 지급 탄약 수
    // 중복 습득 방지 상태

    // 오버랩 콜백: 접촉한 액터를 TryPickup에 전달

    void TryPickup(AActor* OtherActor); rtual void Tick(float DeltaTime) override;

};
