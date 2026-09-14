#pragma once
#include "CoreMinimal.h"
#include "BossPatternActorBase.h"
#include "BossMagicAttackActor.generated.h"

// 지정 좌표로 이동, 플레이어 충돌체와 Dark 반복 이펙트를 가진 마법 투사체
UCLASS(Blueprintable)
class VEILBREAK_API ABossMagicAttackActor : public ABossPatternActorBase
{
    GENERATED_BODY()
public:
    // 이동 및 Dark 이펙트 기본값 생성
    ABossMagicAttackActor();
    // 목표 좌표 설정, 발사 방향 정렬
    void LaunchAt(const FVector& InTarget);
    // 목표까지 이동, 도착 시 AuraFX 이펙트 재생
    virtual void Tick(float DeltaSeconds) override;
protected:
    // 투사체 이동 기준 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MagicAttack")
    TObjectPtr<USceneComponent> SceneRoot;
    // 플레이어 피격 감지용 작은 구형 콜리전
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MagicAttack")
    TObjectPtr<class USphereComponent> HitCollision;
    // 액터와 함께 이동하는 NS_SlashTrail_Dark_Loop 이펙트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MagicAttack")
    TObjectPtr<class UNiagaraComponent> FireEffect;
    // 도착 지점의 일회성 AuraFX Mystic 이펙트
    UPROPERTY(EditDefaultsOnly, Category="MagicAttack")
    TObjectPtr<class UNiagaraSystem> ArrivalEffect;
    // 초당 이동 거리, cm/s
    UPROPERTY(EditDefaultsOnly, Category="MagicAttack", meta=(ClampMin="1"))
    float Speed = 1200.f;
private:
    // 월드 공간 도착 좌표
    FVector TargetLocation = FVector::ZeroVector;
    // 발사 초기화 완료 여부
    bool bLaunched = false;
    // 도착 지점에 AuraFX 생성 후 Actor 삭제
    void FinishProjectile(const FVector& EffectLocation);
};
