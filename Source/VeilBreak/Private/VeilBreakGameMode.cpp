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

	AVeilBreakGameState* VeilBreakGameState =
		GetGameState<AVeilBreakGameState>();

	if (!VeilBreakGameState)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("VeilBreakGameState를 찾을 수 없음")
		);
		return;
	}

	VeilBreakGameState->SetCurrentGameLoopState(
		EVeilBreakGameLoopState::Waiting
	);

	VeilBreakGameState->SetCurrentBossPhase(
		EBossPhase::Phase1
	);

	VeilBreakGameState->SetBattleResult(
		EVeilBreakBattleResult::None
	);

	UE_LOG(LogTemp, Log, TEXT("보스 전투 준비 완료"));
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

	VeilBreakGameState->SetCurrentGameLoopState(
		EVeilBreakGameLoopState::Combat
	);

	UE_LOG(LogTemp, Log, TEXT("보스 전투 시작"));

	// BP_GameMode에 전투 시작 사실 전달
	OnBattleStarted();
}

void AVeilBreakGameMode::NotifyBossPhaseChanged(
	EBossPhase NewPhase
)
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

	if (VeilBreakGameState->GetCurrentBossPhase() == NewPhase)
	{
		return;
	}

	VeilBreakGameState->SetCurrentBossPhase(NewPhase);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("보스 페이즈 변경: %d"),
		static_cast<int32>(NewPhase)
	);

	// BP_GameMode에 페이즈 변경 사실 전달
	OnBossPhaseChanged(NewPhase);
}

void AVeilBreakGameMode::NotifyBossDefeated()
{
	EndBattle(EVeilBreakBattleResult::Victory);
}

void AVeilBreakGameMode::NotifyPlayerDefeated()
{
	EndBattle(EVeilBreakBattleResult::Defeat);
}

void AVeilBreakGameMode::EndBattle(
	EVeilBreakBattleResult Result
)
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

	// 잘못된 종료 결과 방지
	if (Result == EVeilBreakBattleResult::None)
	{
		return;
	}

	bBattleEnded = true;

	VeilBreakGameState->SetBattleResult(Result);

	if (Result == EVeilBreakBattleResult::Victory)
	{
		VeilBreakGameState->SetCurrentGameLoopState(
			EVeilBreakGameLoopState::Victory
		);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Victory: 보스 처치")
		);
	}
	else
	{
		// 플레이어가 사망해도 보스 페이즈는 유지
		VeilBreakGameState->SetCurrentGameLoopState(
			EVeilBreakGameLoopState::Defeat
		);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Defeat: 플레이어 사망")
		);
	}

	// BP_GameMode에 전투 종료 사실 전달
	OnBattleEnded(Result);
}

void AVeilBreakGameMode::RestartBattle()
{
	const FString CurrentLevelName =
		UGameplayStatics::GetCurrentLevelName(this, true);

	// 맵을 다시 열어 플레이어, 보스, GameMode, GameState 재생성
	UGameplayStatics::OpenLevel(
		this,
		FName(*CurrentLevelName)
	);
}