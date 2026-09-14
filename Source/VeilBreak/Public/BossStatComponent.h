#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossData.h"
#include "BossStatComponent.generated.h"

// 현재 체력·최대 체력 전달용 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBossHealthChanged, float, CurrentHealth, float, MaxHealth);
// 보스 체력이 0이 된 시점 전달용 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossDied);
// 보스 페이즈 변경 전달용 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossPhaseChanged, EBossPhase, NewPhase);

// 체력 2000·무적·사망 상태 관리용 컴포넌트
UCLASS(ClassGroup=(Boss), meta=(BlueprintSpawnableComponent))
class VEILBREAK_API UBossStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 생성자: 체력 컴포넌트 기본 설정
	UBossStatComponent();
	// 피해량 적용 후 실제 차감량 반환
	float ApplyDamage(float DamageAmount);
	// 최대 체력 반환
	float GetMaxHealth() const { return MaxHealth; }
	// 현재 체력 반환
	float GetCurrentHealth() const { return CurrentHealth; }
	// 현재 체력 백분율 반환
	float GetHealthPercent() const;
	// 무적 상태 설정
	void SetInvulnerable(bool bNewIsInvulnerable);
	// 무적 상태 반환
	bool IsInvulnerable() const { return bIsInvulnerable; }
	// 사망 상태 반환
	bool IsDead() const { return bIsDead; }
	// 현재 체력 구간 페이즈 반환
	EBossPhase GetCurrentPhase() const { return CurrentPhase; }
	// 디버그 체력 강제 설정, 체력·페이즈 변경 이벤트 함께 발생
	UFUNCTION(BlueprintCallable, Category="Boss|Debug")
	void SetHealthForDebug(float NewHealth);
	// 체력 변경 시 전달
	UPROPERTY(BlueprintAssignable, Category="Boss|Stat")
	FOnBossHealthChanged OnHealthChanged;
	// 체력이 0이 된 시점에 전달
	UPROPERTY(BlueprintAssignable, Category="Boss|Stat")
	FOnBossDied OnBossDied;
	// 체력 구간 변경 시 BT·HUD에 전달
	UPROPERTY(BlueprintAssignable, Category="Boss|Stat")
	FOnBossPhaseChanged OnPhaseChanged;
protected:
	// 시작 시 최대 체력으로 현재 체력 초기화
	virtual void BeginPlay() override;
	// 보스 총 체력 기본값 2000
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Stat", meta=(ClampMin="1"))
	float MaxHealth = 2000.f;
	// 피해 처리 뒤 남은 보스 체력
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss|Stat")
	float CurrentHealth = 2000.f;
	// 피해 무시 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Stat")
	bool bIsInvulnerable = false;
	// 체력 0 도달 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss|Stat")
	bool bIsDead = false;
	// 현재 체력 구간 페이즈, 시작값 Phase1
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss|Phase")
	EBossPhase CurrentPhase = EBossPhase::Phase1;
	// Phase2 전환 체력 비율, 60%
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Phase", meta=(ClampMin="0", ClampMax="1"))
	float Phase2HealthThreshold = 0.6f;
	// Phase3 전환 체력 비율, 30%
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss|Phase", meta=(ClampMin="0", ClampMax="1"))
	float Phase3HealthThreshold = 0.3f;
	// 체력 비율로 페이즈 갱신, 변경 시 이벤트 발생
	void EvaluatePhase();
};
