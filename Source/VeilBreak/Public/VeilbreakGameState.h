#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "VeilBreakGameTypes.h"
#include "VeilBreakGameState.generated.h"

UCLASS()
class VEILBREAK_API AVeilBreakGameState : public AGameState
{
	GENERATED_BODY()

public:
	AVeilBreakGameState();


	// 현재 게임 진행 상태
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Game Loop"
	)
	EVeilBreakGameLoopState CurrentGameLoopState;

	// 현재 보스 페이즈
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Game Loop"
	)
	EBossPhase CurrentBossPhase;

	// 현재 전투 결과
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Game Loop"
	)
	EVeilBreakBattleResult BattleResult;

	UFUNCTION(BlueprintPure, Category = "Game Loop")
	EVeilBreakGameLoopState GetCurrentGameLoopState() const;

	UFUNCTION(BlueprintPure, Category = "Game Loop")
	EBossPhase GetCurrentBossPhase() const;

	UFUNCTION(BlueprintPure, Category = "Game Loop")
	EVeilBreakBattleResult GetBattleResult() const;

	void SetCurrentGameLoopState(
		EVeilBreakGameLoopState NewState
	);

	void SetCurrentBossPhase(
		EBossPhase NewPhase
	);

	void SetBattleResult(
		EVeilBreakBattleResult NewResult
	);

	// 점수
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Score")
	int32 Score;

	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetScore() const;

	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddScore(int32 Amount);

	// 전투 종료 시 확정되는 경과 시간(초)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle Record")
	float BattleDurationSeconds = 0.0f;

	// 전투 중 실제 소비한 탄환 수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle Record")
	int32 AmmoSpent = 0;

	// 보스에게 명중한 탄환 수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle Record")
	int32 BossHitCount = 0;

	// 명중률: 0~100 범위의 백분율
	UFUNCTION(BlueprintPure, Category = "Battle Record")
	float GetAccuracyPercent() const
	{
		if (AmmoSpent <= 0)
		{
			return 0.0f;
		}

		return static_cast<float>(BossHitCount)
			/ static_cast<float>(AmmoSpent) * 100.0f;
	}
	// 새 전투를 위해 기록과 점수를 초기화
	void ResetBattleRecord();
};