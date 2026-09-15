#include "VeilBreakGameState.h"

AVeilBreakGameState::AVeilBreakGameState()
{
	CurrentGameLoopState = EVeilBreakGameLoopState::Waiting;
	CurrentBossPhase = EBossPhase::Phase1;
	BattleResult = EVeilBreakBattleResult::None;

	Score = 0;
}

EVeilBreakGameLoopState
AVeilBreakGameState::GetCurrentGameLoopState() const
{
	return CurrentGameLoopState;
}

EBossPhase
AVeilBreakGameState::GetCurrentBossPhase() const
{
	return CurrentBossPhase;
}

EVeilBreakBattleResult
AVeilBreakGameState::GetBattleResult() const
{
	return BattleResult;
}

void AVeilBreakGameState::SetCurrentGameLoopState(
	EVeilBreakGameLoopState NewState
)
{
	CurrentGameLoopState = NewState;
}

void AVeilBreakGameState::SetCurrentBossPhase(
	EBossPhase NewPhase
)
{
	CurrentBossPhase = NewPhase;
}

void AVeilBreakGameState::SetBattleResult(
	EVeilBreakBattleResult NewResult
)
{
	BattleResult = NewResult;
}

int32 AVeilBreakGameState::GetScore() const
{
	return Score;
}

void AVeilBreakGameState::AddScore(int32 Amount)
{
	Score += Amount;
}