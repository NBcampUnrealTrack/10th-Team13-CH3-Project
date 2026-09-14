#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "VeilBreakGameTypes.h"
#include "BossLoopDummy.generated.h"

class UStaticMeshComponent;

/*
 * 게임 루프 검증용 임시 보스입니다.
 *
 * 실제 AI는 없으며 다음 기능만 갖습니다.
 * 1. 화면에 보일 Static Mesh
 * 2. 체력
 * 3. 체력에 따른 3페이즈 변경
 * 4. 사망 시 GameMode에 승리 보고
 * 5. 일정 시간마다 자동으로 테스트 데미지 적용
 */
UCLASS()
class VEILBREAK_API ABossLoopDummy : public AActor
{
	GENERATED_BODY()

public:
	ABossLoopDummy();

	UFUNCTION(BlueprintCallable, Category = "Dummy Boss")
	void ApplyTestDamage(float DamageAmount);

protected:
	virtual void BeginPlay() override;
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Dummy Boss"
	)
	TObjectPtr<UStaticMeshComponent> DummyMesh;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Dummy Boss",
		meta = (ClampMin = "1.0")
	)
	float MaxHealth;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Dummy Boss"
	)
	float CurrentHealth;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Dummy Boss"
	)
	EVeilBreakBossPhase CurrentPhase;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Dummy Boss|Test"
	)
	bool bRunAutomaticTest;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Dummy Boss|Test",
		meta = (ClampMin = "1.0")
	)
	float AutomaticDamageAmount;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Dummy Boss|Test",
		meta = (ClampMin = "0.1")
	)
	float AutomaticDamageInterval;

private:
	void ApplyAutomaticTestDamage();
	void UpdatePhase();
	void ShowStatusOnScreen() const;
	FString GetPhaseText() const;
	FTimerHandle AutomaticDamageTimerHandle;
};