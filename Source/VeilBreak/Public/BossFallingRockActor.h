#pragma once

#include "CoreMinimal.h"
#include "BossPatternActorBase.h"
#include "BossFallingRockActor.generated.h"

// 포물선 비행·회전·착지 이펙트용 낙석 액터, BP_BossFallingRock의 부모
UCLASS(Blueprintable)
class VEILBREAK_API ABossFallingRockActor : public ABossPatternActorBase
{
	GENERATED_BODY()

public:
	// DragonCave Rock 메시·AuraFX Sand 기본값 생성
	ABossFallingRockActor();
	// 시작 위치에서 목표 위치까지 포물선 비행 시작
	void LaunchAt(const FVector& InTarget);
	// 비행 시간·회전 갱신, 목표 도달 시 착지 이펙트 생성
	virtual void Tick(float DeltaSeconds) override;
protected:
	// 낙석 시각화용 DragonCave Static Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="FallingRock")
	TObjectPtr<class UStaticMeshComponent> RockMesh;
	// 목표 도달 지점의 AuraFX Sand 이펙트
	UPROPERTY(EditDefaultsOnly, Category="FallingRock")
	TObjectPtr<class UNiagaraSystem> ArrivalEffect;
	// 포물선 비행 시간, 초
	UPROPERTY(EditDefaultsOnly, Category="FallingRock", meta=(ClampMin="0.1"))
	float FlightDuration = 1.2f;
	// 비행 중 추가 최고 높이, cm
	UPROPERTY(EditDefaultsOnly, Category="FallingRock", meta=(ClampMin="0"))
	float ArcHeight = 700.f;
	// 초당 회전 각도, deg/s
	UPROPERTY(EditDefaultsOnly, Category="FallingRock")
	FRotator RotationRate = FRotator(540.f, 360.f, 180.f);
private:
	// 발사 순간 월드 시작 위치
	FVector StartLocation = FVector::ZeroVector;
	// 목표 월드 위치
	FVector TargetLocation = FVector::ZeroVector;
	// 누적 비행 시간, 초
	float ElapsedFlightTime = 0.f;
	// 포물선 비행 활성 상태
	bool bLaunched = false;
	// 목표 위치의 AuraFX Sand 생성 후 액터 삭제
	void FinishFallingRock();
};
