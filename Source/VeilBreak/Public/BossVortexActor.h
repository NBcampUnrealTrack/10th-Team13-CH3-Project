#pragma once

#include "CoreMinimal.h"
#include "BossPatternActorBase.h"
#include "BossVortexActor.generated.h"

// 1페이즈 플레이어 추적·초당 피해·지속 외형 관리, BP_BossVortex의 부모
UCLASS(Blueprintable)
class VEILBREAK_API ABossVortexActor : public ABossPatternActorBase
{
	GENERATED_BODY()

public:
	// 추적 콜리전과 소용돌이 이펙트 기본 구성
	ABossVortexActor();
	// 목표·피해·속도·유지시간을 적용하고 추적 시작
	void Configure(AActor* InTargetActor, float InDamagePerSecond, float InMoveSpeed, float InDuration);
	// 플레이어 추적과 초당 피해 주기 갱신
	virtual void Tick(float DeltaSeconds) override;

protected:
	// 소용돌이 바닥 위치를 유지하는 루트
	UPROPERTY()
	TObjectPtr<class USceneComponent> SceneRoot;
	// 바닥부터 위로 이어지는 소용돌이 범위 판정용 캡슐 콜리전
	UPROPERTY()
	TObjectPtr<class UCapsuleComponent> DamageCollision;
	// Simple Sprite Burst 기반 소용돌이 시각 효과
	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> VortexEffect;

private:
	// 현재 추적할 플레이어 참조
	TWeakObjectPtr<AActor> TargetActor;
	// 범위 안 플레이어에게 1초마다 적용할 피해량
	float DamagePerSecond = 1.f;
	// 소용돌이 이동속도, cm/s
	float MoveSpeed = 300.f;
	// 남은 소용돌이 유지시간, 초
	float RemainingDuration = 10.f;
	// 다음 피해 적용까지 누적 시간, 초
	float DamageAccumulator = 0.f;
	// 현재 범위 안 플레이어에게 한 차례 피해 적용
	void ApplyDamageTick();
};
