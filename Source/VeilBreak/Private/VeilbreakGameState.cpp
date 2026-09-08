#include "VeilbreakGameState.h"

void AVeilbreakGameState::AVeilbreakGameState()
{
	Score = 0;
}

int32 AVeilbreakGameState::GetScore() const
{
	return Score;
}

void AVeilbreakGameState::AddScore(int32 Amount)
{
	Score += Amount;
}