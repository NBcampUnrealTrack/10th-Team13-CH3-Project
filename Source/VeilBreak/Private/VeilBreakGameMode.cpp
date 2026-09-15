#include "VeilBreakGameMode.h"
#include "FPSCharacter.h"
#include "VeilBreakPlayerController.h"
#include "VeilBreakGameState.h"
#include "Kismet/GameplayStatics.h"
#include "BossCharacterBase.h"
#include "BossStatComponent.h"
#include "PlayerHealthComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

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
	BindBossEvents();

	if (bAutoStartBattleForTest)
	{
		StartBattle();
	}
}

void AVeilBreakGameMode::RestartPlayer(AController* NewPlayer)
{
	//기존 플레이어 생성 처리를 먼저 실행
	Super::RestartPlayer(NewPlayer);

	if (!IsValid(NewPlayer))
	{
		UE_LOG(LogTemp, Warning, TEXT("플레이어 컨트롤러 없음"));
		return;
	}

	//컨트롤러가 조종하는 Pawn을 가져옴
	APawn* PlayerPawn = NewPlayer->GetPawn();

	if (!IsValid(PlayerPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("플레이어 Pawn 생성 확인 실패"));
		return;
	}

	//플레이어의 체력 컴포넌트를 찾음
	UPlayerHealthComponent* PlayerHealth =
		PlayerPawn->FindComponentByClass<UPlayerHealthComponent>();

	if (!IsValid(PlayerHealth))
	{
		UE_LOG(LogTemp, Warning, TEXT("플레이어 체력 컴포넌트 없음"));
		return;
	}

	//플레이어 사망 시 GameMode의 패배 처리 함수를 실행
	PlayerHealth->OnPlayerDeath.AddUniqueDynamic(
		this,
		&AVeilBreakGameMode::NotifyPlayerDefeated
	);

	UE_LOG(LogTemp, Log, TEXT("플레이어 사망 이벤트 연결 완료"));
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

void AVeilBreakGameMode::BindBossEvents()
{
	// 레벨에 배치된 보스를 찾기
	ABossCharacterBase* Boss = Cast<ABossCharacterBase>(
		UGameplayStatics::GetActorOfClass(
			this,
			ABossCharacterBase::StaticClass()
		)
	);

	if (!IsValid(Boss))
	{
		UE_LOG(LogTemp, Warning, TEXT("보스를 찾을 수 없음"));
		return;
	}

	// 보스의 체력 컴포넌트를 가져오기
	UBossStatComponent* BossStats = Boss->GetBossStatComponent();

	if (!IsValid(BossStats))
	{
		UE_LOG(LogTemp, Warning, TEXT("체력 컴포넌트 없음"));
		return;
	}

	BossStats->OnPhaseChanged.AddUniqueDynamic(
		this,
		&AVeilBreakGameMode::NotifyBossPhaseChanged
	);

	BossStats->OnBossDied.AddUniqueDynamic(
		this,
		&AVeilBreakGameMode::NotifyBossDefeated
	);
	UE_LOG(LogTemp, Log, TEXT("보스 페이즈·사망 이벤트 연결 완료"));
}