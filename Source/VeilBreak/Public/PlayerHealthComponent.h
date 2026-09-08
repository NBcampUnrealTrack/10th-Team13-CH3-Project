#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerHealthComponent.generated.h"

// 체력이 변경됐을 때 UI 등에 현재 체력과 최대 체력을 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnHealthChanged,
	float,
	CurrentHealth,
	float,
	MaxHealth
);

// 체력이 0이 되어 플레이어가 사망했을 때 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE(
	FOnPlayerDeath
);

UCLASS(
	ClassGroup = (Custom),
	meta = (BlueprintSpawnableComponent)
)
class VEILBREAK_API UPlayerHealthComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	// 생성자
	UPlayerHealthComponent();

public:
	// 지정한 수치만큼 플레이어에게 피해 적용
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(float DamageAmount);

	// 지정한 수치만큼 플레이어의 체력 회복
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float HealAmount);

	// 플레이어의 체력을 최대치로 초기화
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	// 현재 체력 반환
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const;

	// 최대 체력 반환
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const;

	// 현재 플레이어가 사망 상태인지 반환
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const;

public:
	// 체력이 변경될 때마다 호출되는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged;

	// 체력이 0이 되었을 때 한 번 호출되는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnPlayerDeath OnPlayerDeath;

protected:
	// Unreal Override
	virtual void BeginPlay() override;

private:
	// 체력을 0부터 최대 체력 사이의 값으로 안전하게 변경
	void SetCurrentHealth(float NewHealth);

	// 체력이 0이 됐을 때 사망 상태로 변경
	void HandleDeath();

private:
	// 체력 설정

	// 플레이어가 가질 수 있는 최대 체력
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth = 100.0f;

	// 현재 플레이어가 보유한 체력
	UPROPERTY(
		VisibleInstanceOnly,
		Category = "Health"
	)
	float CurrentHealth = 100.0f;

	// 플레이어가 사망 상태인지 저장
	UPROPERTY(
		VisibleInstanceOnly,
		Category = "Health"
	)
	bool bIsDead = false;
};