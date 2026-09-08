#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatusEffectReceiverComponent.generated.h"

// CC 가능 여부가 변경됐을 때 외부 시스템에 전달하는 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnCrowdControlStateChanged,
	bool,
	bIsCrowdControlled
);

UCLASS(
	ClassGroup = (Custom),
	meta = (BlueprintSpawnableComponent)
)
class VEILBREAK_API UStatusEffectReceiverComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	// 생성자
	UStatusEffectReceiverComponent();

public:
	// 새로운 CC 효과가 시작됐음을 컴포넌트에 전달
	UFUNCTION(BlueprintCallable, Category = "Status Effect")
	void AddCrowdControl();

	// 적용 중인 CC 효과 하나가 종료됐음을 컴포넌트에 전달
	UFUNCTION(BlueprintCallable, Category = "Status Effect")
	void RemoveCrowdControl();

	// 사망이나 상태 초기화 시 모든 CC 효과 제거
	UFUNCTION(BlueprintCallable, Category = "Status Effect")
	void ClearCrowdControl();

	// 현재 하나 이상의 CC 효과를 받고 있는지 반환
	UFUNCTION(BlueprintPure, Category = "Status Effect")
	bool IsCrowdControlled() const;

public:
	// CC 활성 상태가 변경됐을 때 호출되는 이벤트
	UPROPERTY(
		BlueprintAssignable,
		Category = "Status Effect"
	)
	FOnCrowdControlStateChanged OnCrowdControlStateChanged;

protected:
	// Unreal Override
	virtual void BeginPlay() override;

private:
	// 현재 적용 중인 CC 효과의 개수
	UPROPERTY(
		VisibleInstanceOnly,
		Category = "Status Effect"
	)
	int32 ActiveCrowdControlCount = 0;
};