#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerHealthComponent.generated.h"

// 체력 변경을 UI에 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnHealthChanged, float, CurrentHealth, float, MaxHealth
);

// 사망 시 한 번 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDeath);

// 실제 HP 감소가 발생한 피해에만 전달 (회복에는 호출하지 않음)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnPlayerDamageReceived, float, DamageAmount
);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VEILBREAK_API UPlayerHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 생성자
	UPlayerHealthComponent();

	// 지정한 피해 적용
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(float DamageAmount);

	// 지정한 양만큼 회복
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float HealAmount);

	// 체력 및 사망 상태 초기화 (캐릭터 부활 연출은 별도)
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	// 현재 체력 조회
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const;

	// 최대 체력 조회
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const;

	// 사망 상태 조회
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const;

public:
	// 체력 UI 갱신
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged;

	// 사망 처리
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnPlayerDeath OnPlayerDeath;

	// 피격 사운드와 파티클 처리
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnPlayerDamageReceived OnDamageReceived;

protected:
	// 시작 체력 초기화
	virtual void BeginPlay() override;

private:
	// 체력 제한 및 변경 이벤트 전달
	void SetCurrentHealth(float NewHealth);

	// 사망 상태 및 이벤트 처리
	void HandleDeath();

	// 최대 체력
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth = 100.0f;

	// 현재 체력
	UPROPERTY(VisibleInstanceOnly, Category = "Health")
	float CurrentHealth = 100.0f;

	// 사망 여부
	UPROPERTY(VisibleInstanceOnly, Category = "Health")
	bool bIsDead = false;
};
