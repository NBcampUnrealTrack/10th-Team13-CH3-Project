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
#include "GameFramework/PlayerController.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AmmoSpawnVolume.h"
#include "Engine/World.h"

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
	// 생성된 플레이어의 컨트롤러로 HUD 표시 요청
	APlayerController* PlayerController =
	Cast<APlayerController>(NewPlayer);

	if (IsValid(PlayerController) && PlayerController->IsLocalController())
	{
		OnPlayerReadyForHUD(PlayerController);
	}
}

void AVeilBreakGameMode::SpawnAmmoForArea(FName AreaTag)
{
	TArray<AActor*> FoundVolumes;

	UGameplayStatics::GetAllActorsOfClassWithTag(
		this,
		AAmmoSpawnVolume::StaticClass(),
		AreaTag,
		FoundVolumes
	);

	if (FoundVolumes.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AmmoSpawn] 구역 볼륨 없음: %s"),
			*AreaTag.ToString()
		);
		return;
	}

	for (AActor* Actor : FoundVolumes)
	{
		AAmmoSpawnVolume* Volume = Cast<AAmmoSpawnVolume>(Actor);

		if (IsValid(Volume))
		{
			Volume->SpawnAmmo();
		}
	}
}

void AVeilBreakGameMode::ClearAllSpawnedAmmo()
{
	TArray<AActor*> FoundVolumes;

	UGameplayStatics::GetAllActorsOfClass(
		this,
		AAmmoSpawnVolume::StaticClass(),
		FoundVolumes
	);

	for (AActor* Actor : FoundVolumes)
	{
		AAmmoSpawnVolume* Volume = Cast<AAmmoSpawnVolume>(Actor);

		if (IsValid(Volume))
		{
			Volume->ClearSpawnedAmmo();
		}
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
	
	//UI가 이전 기록을 읽지 않도록 초기화
	VeilBreakGameState->ResetBattleRecord();
	BattleStartTimeSeconds = 0.0;

	VeilBreakGameState->SetCurrentGameLoopState(
		EVeilBreakGameLoopState::Waiting);

	VeilBreakGameState->SetCurrentBossPhase(
		EBossPhase::Phase1);

	VeilBreakGameState->SetBattleResult(
		EVeilBreakBattleResult::None);

	UE_LOG(LogTemp, Log, TEXT("황금돼지 전투 준비 완료"));
}

void AVeilBreakGameMode::StartBattle()
{
	AVeilBreakGameState* VeilBreakGameState =
		GetGameState<AVeilBreakGameState>();

	UWorld* World = GetWorld();

	if (!VeilBreakGameState || !World)
	{
		return;
	}

	// 대기 상태에서만 전투 시작 가능
	if (VeilBreakGameState->GetCurrentGameLoopState()
		!= EVeilBreakGameLoopState::Waiting)
	{
		return;
	}

	// 실제 전투 시작을 기준으로 기록 초기화
	VeilBreakGameState->ResetBattleRecord();
	BattleStartTimeSeconds = World->GetTimeSeconds();
	bBattleEnded = false;

	VeilBreakGameState->SetCurrentGameLoopState(
		EVeilBreakGameLoopState::Combat);

	SpawnAmmoForArea(FName(TEXT("AmmoSpawn_Phase1")));

	UE_LOG(LogTemp, Log, TEXT("황금돼지 전투 시작"));

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
	// 플레이어와 보스를 해당 페이즈 구역으로 이동
	MoveActorsToPhaseArea(NewPhase);

	// UI 및 연출에 페이즈 변경 알림
	OnBossPhaseChanged(NewPhase);
}

void AVeilBreakGameMode::NotifyBossDefeated()
{
	UE_LOG(LogTemp, Warning, TEXT("황금돼지 사망!"));
	EndBattle(EVeilBreakBattleResult::Victory);
}

void AVeilBreakGameMode::NotifyPlayerDefeated()
{
	UE_LOG(LogTemp, Warning, TEXT("플레이어 사망!"));
	EndBattle(EVeilBreakBattleResult::Defeat);
}

void AVeilBreakGameMode::EndBattle(
	EVeilBreakBattleResult Result)
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

	// 먼저 집계를 닫아 이후 보고를 차단
	bBattleEnded = true;

	// 전투 종료 시 시간을 확정
	if (const UWorld* World = GetWorld())
	{
		const double ElapsedSeconds =
			World->GetTimeSeconds() - BattleStartTimeSeconds;

		VeilBreakGameState->BattleDurationSeconds =
			static_cast<float>(FMath::Max(0.0, ElapsedSeconds));
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleRecord] 시간=%.2f초 / 사용=%d발 / 명중=%d발 / 명중률=%.1f%%"),
		VeilBreakGameState->BattleDurationSeconds,
		VeilBreakGameState->AmmoSpent,
		VeilBreakGameState->BossHitCount,
		VeilBreakGameState->GetAccuracyPercent()
	);

	ClearAllSpawnedAmmo();

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

void AVeilBreakGameMode::MoveActorsToPhaseArea(EBossPhase NewPhase)
{
	if (NewPhase != EBossPhase::Phase2)
	{
		return;
	}

	const FName PlayerTag = TEXT("Phase2_Player");
	const FName BossTag = TEXT("Phase2_Boss");


	// 2. 태그가 지정된 Target Point 검색
	TArray<AActor*> PlayerPoints;
	TArray<AActor*> BossPoints;

	UGameplayStatics::GetAllActorsOfClassWithTag(
		this, ATargetPoint::StaticClass(), PlayerTag, PlayerPoints
	);

	UGameplayStatics::GetAllActorsOfClassWithTag(
		this, ATargetPoint::StaticClass(), BossTag, BossPoints
	);

	// 누락 또는 중복 태그가 있으면 이동하지 않음
	if (PlayerPoints.Num() != 1 || BossPoints.Num() != 1)
	{
		UE_LOG(
			LogTemp, Warning,
			TEXT("페이즈 이동 실패: %s=%d개, %s=%d개"),
			*PlayerTag.ToString(), PlayerPoints.Num(),
			*BossTag.ToString(), BossPoints.Num()
		);
		return;
	}

	// 3. 이동할 플레이어와 보스 확인
	ACharacter* Player =
		UGameplayStatics::GetPlayerCharacter(this, 0);

	ABossCharacterBase* Boss = Cast<ABossCharacterBase>(
		UGameplayStatics::GetActorOfClass(
			this, ABossCharacterBase::StaticClass()
		)
	);

	if (!IsValid(Player) || !IsValid(Boss))
	{
		UE_LOG(LogTemp, Warning, TEXT("페이즈 이동 실패: 캐릭터 없음"));
		return;
	}

	// 4. 이동 직전의 속도를 정리
	Player->GetCharacterMovement()->StopMovementImmediately();
	Boss->GetCharacterMovement()->StopMovementImmediately();

	// 5. 충돌을 확인하며 각 목적지로 이동
	//플레이어 이동에 성공했는지
	const bool bPlayerMoved = Player->TeleportTo(
		PlayerPoints[0]->GetActorLocation(),
		PlayerPoints[0]->GetActorRotation()
	);
	//보스 이동에 성공했는지
	const bool bBossMoved = Boss->TeleportTo(
		BossPoints[0]->GetActorLocation(),
		BossPoints[0]->GetActorRotation()
	);

	if (bPlayerMoved && bBossMoved)
	{
		// 이전 구역에 남은 탄약 제거
		ClearAllSpawnedAmmo();

		// 새 구역에 탄약 생성
		SpawnAmmoForArea(FName(TEXT("AmmoSpawn_Phase2")));
	}

	UE_LOG(
		LogTemp, Log,
		TEXT("페이즈 이동 결과: Player=%s, Boss=%s"),
		bPlayerMoved ? TEXT("성공") : TEXT("실패"),
		bBossMoved ? TEXT("성공") : TEXT("실패")
	);
}

void AVeilBreakGameMode::ReportAmmoSpent(int32 Amount)
{
	if (bBattleEnded || Amount <= 0)
	{
		return;
	}

	AVeilBreakGameState* GS =
		GetGameState<AVeilBreakGameState>();

	if (!GS ||GS->GetCurrentGameLoopState()
		!= EVeilBreakGameLoopState::Combat)
	{
		return;
	}

	GS->AmmoSpent += Amount;
}

void AVeilBreakGameMode::ReportBossHit()
{
	if (bBattleEnded)
	{
		return;
	}

	AVeilBreakGameState* GS =
		GetGameState<AVeilBreakGameState>();

	if (!GS ||GS->GetCurrentGameLoopState()
		!= EVeilBreakGameLoopState::Combat)
	{
		return;
	}

	++GS->BossHitCount;
}