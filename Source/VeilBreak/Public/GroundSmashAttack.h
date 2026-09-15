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
    float ImpactDelay = 0.7f;// 애니메이션 시작 -> 0.7초 뒤 ActivateAttack() 발동됨

    UPROPERTY(EditAnywhere, Category = "Wave")
    float StartRadius = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Wave")
    float MaxRadius = 1500.0f;

    // 초당 퍼지는 거리
    UPROPERTY(EditAnywhere, Category = "Wave")
    float WaveSpeed = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Wave")
    float WaveWidth = 80.0f;
    // 파장 높이
    UPROPERTY(EditAnywhere, Category = "Wave")
    float WaveHeight = 40.0f;

    // 공격 시작부터 첫 타격까지의 준비 시간
    UPROPERTY(EditAnywhere, Category = "Smash|Pattern")
    float PreparationTime = 2.0f;

    // 준비가 끝난 뒤 패턴이 진행되는 시간
    UPROPERTY(EditAnywhere, Category = "Smash|Pattern")
    float PatternDuration = 20.0f;

    // 타격과 다음 타격 사이의 시간
    UPROPERTY(EditAnywhere, Category = "Smash|Pattern")
    float StrikeInterval = 6.0f;

    // 총 타격 횟수
    UPROPERTY(EditAnywhere, Category = "Smash|Pattern")
    int StrikeCount = 3;

    UPROPERTY(EditDefaultsOnly, Category = "Smash|Sound")
    TObjectPtr<class USoundBase> SmashSound;

    UPROPERTY(EditDefaultsOnly, Category = "Smash|Sound")
    TObjectPtr<class USoundBase> VoiceSound;
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
    void PlaySmashAnimation();
    void FinishWave();
    float CurrentRadius = 0.0f;
    bool waveActive = false;
    // 이번 파장에 이미 맞았는지
    bool playerHit = false;

    // 플레이어 피격 확인
    void CheckPlayerHit(float PreviousRadius);
    UPROPERTY()
    TArray<class UNiagaraComponent*> WaveEffects;

    // 이번 패턴에서 몇 번 타격했는지
    int CurrentStrikeCount = 0;

    // 패턴 실행 중 흐른 시간
    float PatternElapsedTime = 0.0f;
    // 두 번째·세 번째 타격 예약용
    FTimerHandle StrikeTimer;
    FTimerHandle PreparationTimer;
};