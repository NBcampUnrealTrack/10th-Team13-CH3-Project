#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "PlayerSkillComponent.generated.h"

// 과녁 스택이 변경됐을 때 UI에 현재 스택 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnUltimateTargetStackChanged,
	int32,
	CurrentTargetStacks,
	int32,
	RequiredTargetStacks
);

// 궁극기 활성 상태가 변경됐을 때 외부 시스템에 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnUltimateStateChanged,
	bool,
	bIsUltimateActive
);

UCLASS(
	ClassGroup = (Custom),
	meta = (BlueprintSpawnableComponent)
)
class VEILBREAK_API UPlayerSkillComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	// 생성자
	UPlayerSkillComponent();

public:
	// 보스 과녁을 맞혔을 때 궁극기 스택 1 증가
	UFUNCTION(BlueprintCallable, Category = "Skill|Ultimate")
	void AddUltimateTargetStack();

	// 필요한 스택을 모두 채웠다면 궁극기 활성화
	UFUNCTION(BlueprintCallable, Category = "Skill|Ultimate")
	bool ActivateUltimate();

	// 현재 과녁 스택 반환
	UFUNCTION(BlueprintPure, Category = "Skill|Ultimate")
	int32 GetCurrentTargetStacks() const;

	// 궁극기 사용에 필요한 과녁 스택 반환
	UFUNCTION(BlueprintPure, Category = "Skill|Ultimate")
	int32 GetRequiredTargetStacks() const;

	// 현재 궁극기를 사용할 수 있는지 반환
	UFUNCTION(BlueprintPure, Category = "Skill|Ultimate")
	bool CanActivateUltimate() const;

	// 현재 궁극기가 활성화됐는지 반환
	UFUNCTION(BlueprintPure, Category = "Skill|Ultimate")
	bool IsUltimateActive() const;

	// 궁극기의 남은 지속시간 반환
	UFUNCTION(BlueprintPure, Category = "Skill|Ultimate")
	float GetUltimateRemainingTime() const;

public:
	// 과녁 스택 획득 또는 소모 시 호출
	UPROPERTY(BlueprintAssignable, Category = "Skill|Ultimate")
	FOnUltimateTargetStackChanged OnUltimateTargetStackChanged;

	// 궁극기가 시작되거나 종료됐을 때 호출
	UPROPERTY(BlueprintAssignable, Category = "Skill|Ultimate")
	FOnUltimateStateChanged OnUltimateStateChanged;

protected:
	// Unreal Override
	virtual void BeginPlay() override;

private:
	// 궁극기 지속시간이 끝났을 때 활성 상태 종료
	void FinishUltimate();

	// 변경된 과녁 스택을 UI에 전달
	void BroadcastTargetStackChanged();

private:
	// 궁극기 설정

	// 궁극기 사용에 필요한 과녁 적중 횟수
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Ultimate")
	int32 RequiredTargetStacks = 5;

	// 궁극기 강화 효과가 유지되는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Ultimate")
	float UltimateDuration = 8.0f;

private:
	// 궁극기 상태

	// 현재 누적된 과녁 적중 스택
	UPROPERTY(VisibleInstanceOnly, Category = "Skill|Ultimate")
	int32 CurrentTargetStacks = 0;

	// 현재 궁극기가 활성화됐는지 저장
	bool bIsUltimateActive = false;

	// 궁극기 지속시간을 관리하는 타이머
	FTimerHandle UltimateDurationTimerHandle;
};