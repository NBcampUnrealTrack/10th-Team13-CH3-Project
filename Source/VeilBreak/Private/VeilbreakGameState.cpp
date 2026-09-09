#include "VeilBreakGameState.h"

AVeilBreakGameState::AVeilBreakGameState()
{
	CurrentGameLoopState = EVeilBreakGameLoopState::Waiting;
	CurrentBossPhase = EVeilBreakBossPhase::Phase1;
	BattleResult = EVeilBreakBattleResult::None;

	Score = 0;
}

EVeilBreakGameLoopState
AVeilBreakGameState::GetCurrentGameLoopState() const
{
	return CurrentGameLoopState;
}

EVeilBreakBossPhase
AVeilBreakGameState::GetCurrentBossPhase() const
{
	return CurrentBossPhase;
}

EVeilBreakBattleResult
AVeilBreakGameState::GetBattleResult() const
{
	return BattleResult;
}

// GameMode가 전달한 새 게임 진행 상태 저장
void AVeilBreakGameState::SetCurrentGameLoopState(EVeilBreakGameLoopState NewState)
{
	CurrentGameLoopState = NewState;
}

// 보스 체력 조건에 따라 결정된 새 페이즈 저장
void AVeilBreakGameState::SetCurrentBossPhase(EVeilBreakBossPhase NewPhase)
{
	CurrentBossPhase = NewPhase;
}

// 전투 종료 시점에 결정된 승패 결과 저장
void AVeilBreakGameState::SetBattleResult(EVeilBreakBattleResult NewResult)
{
	BattleResult = NewResult;
}

//점수
int32 AVeilBreakGameState::GetScore() const
{
	return Score;
}

void AVeilBreakGameState::AddScore(int32 Amount)
{
	Score += Amount;
}