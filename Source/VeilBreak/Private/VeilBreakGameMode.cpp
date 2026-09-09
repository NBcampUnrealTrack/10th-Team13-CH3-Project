#include "VeilBreakGameMode.h"
#include "FPSCharacter.h"
#include "VeilBreakPlayerController.h"
#include "VeilBreakGameState.h"
#include "Kismet/GameplayStatics.h"

AVeilBreakGameMode::AVeilBreakGameMode()
{
	DefaultPawnClass = AFPSCharacter::StaticClass();
	PlayerControllerClass = AVeilBreakPlayerController::StaticClass();
	GameStateClass = AVeilBreakGameState::StaticClass();
	bBattleEnded = false;
	bAutoStartBattleForTest = true;
}

void AVeilBreakGameMode::BeginPlay()
{
	Super::BeginPlay();

	PrepareBattle();

	if (bAutoStartBattleForTest)
	{
		StartBattle();
	}
}

void AVeilBreakGameMode::PrepareBattle()
{
	bBattleEnded = false;

	AVeilBreakGameState* VeilBreakGameState =GetGameState<AVeilBreakGameState>();

	if (!VeilBreakGameState)
	{
		UE_LOG(LogTemp,Error,TEXT("VeilBreakGameState를 찾을 수 없음"));
		return;
	}

	VeilBreakGameState->SetCurrentGameLoopState(EVeilBreakGameLoopState::Waiting);

	VeilBreakGameState->SetCurrentBossPhase(EVeilBreakBossPhase::Phase1);

	VeilBreakGameState->SetBattleResult(EVeilBreakBattleResult::None);

	UE_LOG(LogTemp,Log,TEXT("보스 전투 준비 완료"));
}

void AVeilBreakGameMode::StartBattle()
{
	AVeilBreakGameState* VeilBreakGameState =
		GetGameState<AVeilBreakGameState>();

	if (!VeilBreakGameState)
	{
		return;
	}

	if (VeilBreakGameState->GetCurrentGameLoopState()
		!= EVeilBreakGameLoopState::Waiting)
	{
		return;
	}

	VeilBreakGameState->SetCurrentGameLoopState(EVeilBreakGameLoopState::Combat);

	UE_LOG(LogTemp,Log,TEXT("보스 전투 시작"));
}

void AVeilBreakGameMode::NotifyBossPhaseChanged(EVeilBreakBossPhase NewPhase)
{
	if (bBattleEnded)
	{
		return;
	}

	AVeilBreakGameState* VeilBreakGameState =GetGameState<AVeilBreakGameState>();

	if (!VeilBreakGameState)
	{
		return;
	}

	if (VeilBreakGameState->GetCurrentGameLoopState()
		!= EVeilBreakGameLoopState::Combat)
	{
		return;
	}

	if (VeilBreakGameState->GetCurrentBossPhase()== NewPhase)
	{
		return;
	}

	VeilBreakGameState->SetCurrentBossPhase(NewPhase);

	UE_LOG(LogTemp,Log,TEXT("보스 페이즈 변경: %d"),static_cast<int32>(NewPhase));
}

void AVeilBreakGameMode::NotifyBossDefeated()
{
	EndBattle(EVeilBreakBattleResult::Victory);
}

void AVeilBreakGameMode::NotifyPlayerDefeated()
{
	EndBattle(EVeilBreakBattleResult::Defeat);
}

void AVeilBreakGameMode::EndBattle(EVeilBreakBattleResult Result)
{
	if (bBattleEnded)
	{
		return;
	}

	AVeilBreakGameState* VeilBreakGameState =
		GetGameState<AVeilBreakGameState>();

	if (!VeilBreakGameState)
	{
		return;
	}

	if (VeilBreakGameState->GetCurrentGameLoopState()
		!= EVeilBreakGameLoopState::Combat)
	{
		return;
	}

	bBattleEnded = true;

	VeilBreakGameState->SetBattleResult(Result);
	VeilBreakGameState->SetCurrentBossPhase(EVeilBreakBossPhase::Dead);

	if (Result == EVeilBreakBattleResult::Victory)
	{
		VeilBreakGameState->SetCurrentGameLoopState(
			EVeilBreakGameLoopState::Victory);

		UE_LOG(LogTemp,Warning,TEXT("Victory: 보스 처치"));
	}
	else
	{
		VeilBreakGameState->SetCurrentGameLoopState(
			EVeilBreakGameLoopState::Defeat
		);

		UE_LOG(LogTemp,Warning,TEXT("Defeat: 사망"));
	}

	OnBattleEnded(Result);
}

void AVeilBreakGameMode::RestartBattle()
{
	const FString CurrentLevelName =UGameplayStatics::GetCurrentLevelName(this,true);
	//맵을 새로 열어 플레이어, 보스, GameMode, GameState를 생성
	UGameplayStatics::OpenLevel(this,FName(*CurrentLevelName));
}
