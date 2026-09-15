#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerStaminaComponent.generated.h"

// 현재 스태미나가 변경됐을 때 UI 등에 전달하는 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnStaminaChanged,
	float,
	CurrentStamina,
	float,
	MaxStamina
);

// 스태미나가 완전히 소진됐을 때 전달하는 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaDepleted);

UCLASS(
	ClassGroup = (Custom),
	meta = (BlueprintSpawnableComponent)
)
class VEILBREAK_API UPlayerStaminaComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	// 생성자
	UPlayerStaminaComponent();

public:
	// 지정한 양의 스태미나를 사용할 수 있는지 확인
	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool HasEnoughStamina(float Amount) const;

	// 스태미나가 충분하면 지정한 양을 소모
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool TryConsumeStamina(float Amount);

	// 달리기용 지속 스태미나 소모를 시작
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool StartSprintConsumption();

	// 달리기용 지속 스태미나 소모를 종료
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void StopSprintConsumption();

	// 궁극기 활성 상태에 따라 무한 스태미나 적용
	UFUNCTION(BlueprintCallable, Category = "Stamina|Ultimate")
	void SetInfiniteStamina(bool bEnableInfiniteStamina);

	// 현재 스태미나 반환
	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetCurrentStamina() const;

	// 최대 스태미나 반환
	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetMaxStamina() const;

	// 현재 무한 스태미나 상태인지 반환
	UFUNCTION(BlueprintPure, Category = "Stamina|Ultimate")
	bool IsInfiniteStamina() const;

public:
	// 스태미나 수치가 변경됐을 때 호출
	UPROPERTY(BlueprintAssignable, Category = "Stamina")
	FOnStaminaChanged OnStaminaChanged;

	// 스태미나가 완전히 소진됐을 때 호출
	UPROPERTY(BlueprintAssignable, Category = "Stamina")
	FOnStaminaDepleted OnStaminaDepleted;

protected:
	// Unreal Override
	virtual void BeginPlay() override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

private:
	// 현재 스태미나 값을 안전한 범위로 변경
	void SetCurrentStamina(float NewStamina);

	// 달리기 중 스태미나를 지속해서 소모
	void ConsumeSprintStamina(float DeltaTime);

	// 스태미나를 최대치까지 자동 회복
	void RecoverStamina(float DeltaTime);

private:
	// 스태미나 설정

	// 캐릭터가 가질 수 있는 최대 스태미나
	UPROPERTY(EditDefaultsOnly, Category = "Stamina")
	float MaxStamina = 100.0f;

	// 현재 보유 중인 스태미나
	UPROPERTY(VisibleInstanceOnly, Category = "Stamina")
	float CurrentStamina = 100.0f;

	// 달리기 중 1초마다 소모되는 스태미나
	UPROPERTY(EditDefaultsOnly, Category = "Stamina")
	float SprintCostPerSecond = 15.0f;

	// 스태미나가 1초마다 회복되는 양
	UPROPERTY(EditDefaultsOnly, Category = "Stamina")
	float RecoveryPerSecond = 20.0f;

	// 마지막 소모 이후 회복을 시작하기까지의 대기시간
	UPROPERTY(EditDefaultsOnly, Category = "Stamina")
	float RecoveryDelay = 1.0f;

	// 현재 달리기 스태미나를 소모하고 있는지 저장
	bool bIsConsumingSprintStamina = false;

	// 현재 궁극기로 무한 스태미나가 적용됐는지 저장
	bool bInfiniteStamina = false;

	// 마지막 스태미나 소모 이후 지난 시간
	float TimeSinceLastStaminaUse = 0.0f;
};
