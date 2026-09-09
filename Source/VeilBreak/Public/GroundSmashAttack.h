#pragma once

#include "CoreMinimal.h"
#include "AttackRangeBase.h"
#include "GroundSmashAttack.generated.h"

UCLASS()
class VEILBREAK_API AGroundSmashAttack : public AAttackRangeBase
{
    GENERATED_BODY()

public:
    AGroundSmashAttack();

    bool StartAnimatedAttack();

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void ActivateAttack() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditDefaultsOnly, Category="Smash|Animation")
    TObjectPtr<class UAnimSequence> GroundSmashMotion;

    UPROPERTY(EditDefaultsOnly, Category="Smash|Animation", meta=(ClampMin="0.0"))
    float ImpactDelay = 0.7f;// 대충 애니메이션 시작 → 0.7초 뒤 ActivateAttack() 발동되는거임

    UPROPERTY(EditAnywhere, Category = "Wave")
    float StartRadius = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Wave")
    float MaxRadius = 1500.0f;

    // 초당 퍼지는 거리
    UPROPERTY(EditAnywhere, Category = "Wave")
    float WaveSpeed = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Wave")
    float WaveWidth = 80.0f;
    // 파장 높이. 발이 이 높이보다 올라가면 회피
    UPROPERTY(EditAnywhere, Category = "Wave")
    float WaveHeight = 40.0f;
private:
    bool animationRunning = false;
    bool waveFinished = false;
    FTimerHandle AnimationTimer;

    UPROPERTY()
    TObjectPtr<class USkeletalMeshComponent> BossMesh;
    UPROPERTY()
    TObjectPtr<class UAnimationAsset> PreviousAnimation;
    bool previousLooping = true;
    bool previousPlaying = true;
    float previousRate = 1.f;
    float previousTime = 0.f;

    void FinishAnimation();
    void RestoreAnimation();
    void FinishWave();
    float CurrentRadius = 0.0f;
    bool waveActive = false;
    // 이번 파장에 이미 맞았는지
    bool playerHit = false;

    // 플레이어 피격 확인
    void CheckPlayerHit(float PreviousRadius);
    UPROPERTY()
    TArray<class UNiagaraComponent*> WaveEffects;
};