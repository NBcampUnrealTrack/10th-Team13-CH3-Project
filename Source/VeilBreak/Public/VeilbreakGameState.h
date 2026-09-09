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
	EVeilBreakBossPhase CurrentBossPhase;

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
	EVeilBreakBossPhase GetCurrentBossPhase() const;

	UFUNCTION(BlueprintPure, Category = "Game Loop")
	EVeilBreakBattleResult GetBattleResult() const;

	void SetCurrentGameLoopState(
		EVeilBreakGameLoopState NewState
	);

	void SetCurrentBossPhase(
		EVeilBreakBossPhase NewPhase
	);

	void SetBattleResult(
		EVeilBreakBattleResult NewResult
	);

	//점수 치환은 추후 진행
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Score")
	int32 Score;
	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetScore() const;
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddScore(int32 Amount);
};
	