#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossCharacterBase.generated.h"

class UBossStatComponent;

// 보스 캐릭터 공통 부모, Sevarog 메시·idle 반복 재생 기본 설정, BP_BossCharacterBase가 상속
UCLASS()
class VEILBREAK_API ABossCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// 생성자: Sevarog
	ABossCharacterBase();
	// Player 0 숫자 0 입력 감지, 디버그 체력·페이즈 순환 호출
	virtual void Tick(float DeltaSeconds) override;
	// 숫자 0 입력마다 Phase1·2·3 대표 체력 순환
	UFUNCTION(BlueprintCallable, Category="Boss|Debug")
	void CycleDebugHealthPhase();
	// TakeDamage를 BossStatComponent에 전달
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	// 보스 체력 컴포넌트 반환
	UBossStatComponent* GetBossStatComponent() const { return BossStatComponent; }
	
	// 목표 좌표로 MagicAttack 시전, 시작 성공 여부 반환
	bool StartMagicAttack(const FVector& Target);
	// 목표 좌표로 FallingRock 시전, 시작 성공 여부 반환
	bool StartFallingRock(const FVector& Target);
	// MagicAttack 진행 여부
	bool IsMagicAttackRunning() const { return bMagicAttackRunning; }
	// FallingRock 시전 진행 여부
	bool IsFallingRockRunning() const { return bFallingRockRunning; }
	// MagicAttack·FallingRock 중 하나라도 시전 중인지 반환
	bool IsPatternRunning() const { return bMagicAttackRunning || bFallingRockRunning; }
	// 이번 MagicAttack 시전 투사체 생성 성공 여부
	bool DidMagicAttackLaunch() const { return bMagicAttackLaunched; }
	// Magic Attack 시전·추적 전환 거리, cm
	float GetMagicAttackRange() const { return MagicAttackRange; }
	// FallingRock 시전 거리, cm
	float GetFallingRockRange() const { return FallingRockRange; }
	
	// 플레이어 추적 시작 거리, cm
	float GetChaseStartDistance() const { return ChaseStartDistance; }
	// 플레이어 추적 종료 거리, cm
	float GetChaseStopDistance() const { return ChaseStopDistance; }
protected:
	// 보스 체력·무적·사망 상태 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss|Component")
	TObjectPtr<class UBossStatComponent> BossStatComponent;
	// 종료 시 시전 타이머 정리
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// Idle 기본 모션
	UPROPERTY(EditDefaultsOnly, Category="Boss|MagicAttack")
	TObjectPtr<class UAnimSequence> IdleMotion;
	// Sevarog Cast 모션
	UPROPERTY(EditDefaultsOnly, Category="Boss|MagicAttack")
	TObjectPtr<class UAnimSequence> CastMotion;
	// 생성할 마법 투사체 BP 클래스
	UPROPERTY(EditDefaultsOnly, Category="Boss|MagicAttack")
	TSubclassOf<class ABossMagicAttackActor> MagicAttackClass;
	// 생성할 낙석 투사체 BP 클래스
	UPROPERTY(EditDefaultsOnly, Category="Boss|FallingRock")
	TSubclassOf<class ABossFallingRockActor> FallingRockClass;
	// Sevarog Ultimate Swing 모션
	UPROPERTY(EditDefaultsOnly, Category="Boss|FallingRock")
	TObjectPtr<class UAnimSequence> FallingRockMotion;
	// Magic Attack 시전·추적 전환 거리, cm
	UPROPERTY(EditDefaultsOnly, Category="Boss|Distance", meta=(ClampMin="1"))
	float MagicAttackRange = 3100.f;
	// FallingRock 시전 거리, cm
	UPROPERTY(EditDefaultsOnly, Category="Boss|Distance", meta=(ClampMin="1"))
	float FallingRockRange = 3100.f;
	// 플레이어 추적 시작 거리, cm
	UPROPERTY(EditDefaultsOnly, Category="Boss|Distance", meta=(ClampMin="1"))
	float ChaseStartDistance = 3000.f;
	// 플레이어 추적 종료 거리, cm
	UPROPERTY(EditDefaultsOnly, Category="Boss|Distance", meta=(ClampMin="1"))
	float ChaseStopDistance = 2000.f;
	// Cast 시작부터 발사까지의 지연, 초
	UPROPERTY(EditDefaultsOnly, Category="Boss|MagicAttack", meta=(ClampMin="0.0"))
	float MagicReleaseDelay = 0.2f;
	// FallingRock 모션 시작부터 발사까지 지연, 초
	UPROPERTY(EditDefaultsOnly, Category="Boss|FallingRock", meta=(ClampMin="0.0"))
	float FallingRockReleaseDelay = 0.8f;
	// 발사 기준 손 본 또는 소켓
	UPROPERTY(EditDefaultsOnly, Category="Boss|MagicAttack")
	FName MagicSpawnSocket = TEXT("hand_l");
	// 낙석 발사 기준 손 본 또는 소켓
	UPROPERTY(EditDefaultsOnly, Category="Boss|FallingRock")
	FName FallingRockSpawnSocket = TEXT("hand_l");
	// BP Class Defaults에서 숫자 0 체력·페이즈 순환 활성화 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Debug")
	bool bEnablePhaseDebugInput = true;
private:
	// 다음 숫자 0 입력에 적용할 페이즈 순번, 시작 상태 Phase1 다음인 Phase2부터 적용
	int32 DebugPhaseIndex = 1;
	// 시전 중 고정된 목표 좌표
	FVector MagicTarget = FVector::ZeroVector;
	// 현재 Cast 진행 여부
	bool bMagicAttackRunning = false;
	// 이번 시전 투사체 생성 여부
	bool bMagicAttackLaunched = false;
	// 현재 FallingRock 시전 진행 여부
	bool bFallingRockRunning = false;
	// FallingRock 시전 중 고정된 목표 좌표
	FVector FallingRockTarget = FVector::ZeroVector;
	// 발사 예약 타이머
	FTimerHandle MagicReleaseTimer;
	// Idle 복귀 타이머
	FTimerHandle MagicFinishTimer;
	// FallingRock 발사 예약 타이머
	FTimerHandle FallingRockReleaseTimer;
	// FallingRock Idle 복귀 타이머
	FTimerHandle FallingRockFinishTimer;
	// 손 위치에서 목표로 투사체 생성
	void ReleaseMagicAttack();
	// Cast 종료 후 Idle 반복 재생 복귀
	void FinishMagicAttack();
	// 보스 위쪽 위치에서 낙석 액터 생성
	void ReleaseFallingRock();
	// Ultimate Swing 종료 후 Idle 반복 재생 복귀
	void FinishFallingRock();
};
