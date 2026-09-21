#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "PlayerConsumableComponent.generated.h"

class UPlayerHealthComponent;

// 체력 물약 개수가 변경됐을 때 UI에 현재 개수 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnHealthPotionCountChanged,
	int32,
	CurrentPotionCount
);

// 체력 물약의 지속 회복 상태가 변경됐을 때 외부 시스템에 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnHealthPotionStateChanged,
	bool,
	bIsHealing
);

UCLASS(
	ClassGroup = (Custom),
	meta = (BlueprintSpawnableComponent)
)
class VEILBREAK_API UPlayerConsumableComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	// 생성자
	UPlayerConsumableComponent();

public:
	// 체력이 부족하면 물약 한 개를 사용하고 지속 회복 시작
	UFUNCTION(BlueprintCallable, Category = "Consumable|HealthPotion")
	bool UseHealthPotion();

	// 지정한 개수의 체력 물약을 추가하고 실제 추가량 반환
	UFUNCTION(BlueprintCallable, Category = "Consumable|HealthPotion")
	int32 AddHealthPotions(int32 PotionAmount);

	// 현재 보유 중인 체력 물약 개수 반환
	UFUNCTION(BlueprintPure, Category = "Consumable|HealthPotion")
	int32 GetHealthPotionCount() const;

	// 현재 체력 물약으로 지속 회복 중인지 반환
	UFUNCTION(BlueprintPure, Category = "Consumable|HealthPotion")
	bool IsHealingWithPotion() const;

public:
	// 체력 물약 개수가 변경됐을 때 호출
	UPROPERTY(BlueprintAssignable, Category = "Consumable|HealthPotion")
	FOnHealthPotionCountChanged OnHealthPotionCountChanged;

	// 체력 물약 회복이 시작되거나 종료됐을 때 호출
	UPROPERTY(BlueprintAssignable, Category = "Consumable|HealthPotion")
	FOnHealthPotionStateChanged OnHealthPotionStateChanged;

protected:
	// Unreal Override
	virtual void BeginPlay() override;

private:
	// 1초마다 체력을 회복하고 남은 회복 횟수 감소
	void HandlePotionHealTick();

	// 지속 회복을 종료하고 타이머 정리
	UFUNCTION()
	void StopPotionHealing();

	// 변경된 물약 개수를 UI에 전달
	void BroadcastPotionCountChanged();

private:
	// 체력 물약 설정

	// 게임 시작 시 기본으로 지급되는 체력 물약 개수
	UPROPERTY(EditDefaultsOnly, Category = "Consumable|HealthPotion")
	int32 StartingHealthPotionCount = 5;

	// 플레이어가 보유할 수 있는 최대 체력 물약 개수
	UPROPERTY(EditDefaultsOnly, Category = "Consumable|HealthPotion")
	int32 MaxHealthPotionCount = 5;

	// 1초마다 회복되는 체력
	UPROPERTY(EditDefaultsOnly, Category = "Consumable|HealthPotion")
	float HealAmountPerTick = 10.0f;

	// 체력 회복이 실행되는 시간 간격
	UPROPERTY(EditDefaultsOnly, Category = "Consumable|HealthPotion")
	float HealTickInterval = 1.0f;

	// 물약 한 개로 실행되는 총 회복 횟수
	UPROPERTY(EditDefaultsOnly, Category = "Consumable|HealthPotion")
	int32 TotalHealTicks = 5;

private:
	// 체력 물약 상태

	// 현재 보유 중인 체력 물약 개수
	UPROPERTY(VisibleInstanceOnly, Category = "Consumable|HealthPotion")
	int32 CurrentHealthPotionCount = 0;

	// 현재 물약으로 지속 회복 중인지 저장
	bool bIsHealingWithPotion = false;

	// 현재 물약의 남은 회복 횟수
	int32 RemainingHealTicks = 0;

	// 플레이어의 체력 컴포넌트 참조
	UPROPERTY()
	TObjectPtr<UPlayerHealthComponent> PlayerHealthComponent;

	// 1초 간격 지속 회복을 관리하는 타이머
	FTimerHandle PotionHealTimerHandle;
};
