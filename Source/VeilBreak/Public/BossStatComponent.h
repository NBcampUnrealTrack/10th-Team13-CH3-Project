#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossStatComponent.generated.h"

// 현재 체력·최대 체력 전달용 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBossHealthChanged, float, CurrentHealth, float, MaxHealth);
// 보스 체력이 0이 된 시점 전달용 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossDied);

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
	// 체력 변경 시 전달
	UPROPERTY(BlueprintAssignable, Category="Boss|Stat")
	FOnBossHealthChanged OnHealthChanged;
	// 체력이 0이 된 시점에 전달
	UPROPERTY(BlueprintAssignable, Category="Boss|Stat")
	FOnBossDied OnBossDied;
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
};
