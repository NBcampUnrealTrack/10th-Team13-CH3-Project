#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerHealthComponent.generated.h"

// 체력이 변경될 때 UI에 현재 체력과 최대 체력을 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnHealthChanged,
	float,
	CurrentHealth,
	float,
	MaxHealth
);

// 플레이어 체력이 0이 됐을 때 외부 시스템에 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDeath);

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
	// 플레이어에게 피해 적용
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(float DamageAmount);

	// 플레이어 체력 회복
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float HealAmount);

	// 플레이어 체력과 사망 상태 초기화
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	// 현재 체력 반환
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const;

	// 최대 체력 반환
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const;

	// 현재 사망 상태인지 반환
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const;

	// 마지막 피격 이후 20초 이상 지났는지 반환
	UFUNCTION(BlueprintPure, Category = "Health|NoDamage")
	bool HP20Seconds() const;

public:
	// 현재 체력이 변경됐을 때 호출
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged;

	// 플레이어가 사망했을 때 한 번 호출
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnPlayerDeath OnPlayerDeath;

protected:
	// Unreal Override
	virtual void BeginPlay() override;

private:
	// 변경된 체력 정보를 UI에 전달
	void BroadcastHealthChanged();

private:
	// 체력 설정

	// 플레이어의 최대 체력
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth = 100.0f;

	// 플레이어의 현재 체력
	UPROPERTY(VisibleInstanceOnly, Category = "Health")
	float CurrentHealth = 100.0f;

	// 현재 플레이어가 사망했는지 저장
	bool bIsDead = false;

private:
	// 무피격 설정

	// 무피격 상태로 인정되는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Health|NoDamage")
	float NoDamageRequiredTime = 20.0f;

	// 마지막으로 실제 피해를 받은 게임 시간
	float LastDamageReceivedTime = 0.0f;
};
