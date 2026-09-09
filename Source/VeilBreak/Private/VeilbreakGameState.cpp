#include "VeilBreakGameState.h"

AVeilBreakGameState::AVeilBreakGameState()
{
	Score = 0;
}

int32 AVeilBreakGameState::GetScore() const
{
	return Score;
}

void AVeilBreakGameState::AddScore(int32 Amount)
{
	Score += Amount;
}