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
    float Speed = 800.0f;
    
    UPROPERTY(EditAnywhere, Category = "Tracking")
    float TrackingRange = 300.0f;//감지 거리

    UPROPERTY(EditAnywhere, Category = "Tracking")
    float TrackingDuration = 0.5f;//추적 시간

    UPROPERTY(VisibleAnywhere, Category = "Collision")
    TObjectPtr<class USphereComponent> HitCollision;//충돌

    UFUNCTION()
    void OnProjectileOverlap(
        UPrimitiveComponent* OverlappedComponent,//겹침이 발생한 내 컴포넌트
        AActor* OtherActor,//충돌한 액터
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,//충돌 구분 넘버
        bool bFromSweep,//이동 경로를 검사하다 발생한 겹침인지
        const FHitResult& SweepResult//이동 검사 결과
    );

    UPROPERTY(EditAnywhere, Category = "Attack")
    float Damage = 40.0f;

private:
    FVector MoveDirection = FVector::ZeroVector;

    float TrackingElapsed = 0.0f;

    bool IsTracking = false;
    bool HasTracked = false;
};
