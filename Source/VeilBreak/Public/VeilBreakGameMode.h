#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "VeilBreakGameTypes.h"
#include "VeilBreakGameMode.generated.h"

UCLASS()
class VEILBREAK_API AVeilBreakGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AVeilBreakGameMode();

	virtual void BeginPlay() override;

	// 플레이어 생성 처리 후 사망 이벤트 연결
	virtual void RestartPlayer(AController* NewPlayer) override;

	// 전투 준비 및 상태 초기화
	UFUNCTION(BlueprintCallable, Category = "Game Loop")
	void PrepareBattle();

	// 실제 전투 시작: Waiting -> Combat
	UFUNCTION(BlueprintCallable, Category = "Game Loop")
	void StartBattle();

	// 보스 측에서 결정된 페이즈를 GameMode에 보고
	UFUNCTION(BlueprintCallable, Category = "Game Loop")
	void NotifyBossPhaseChanged(EBossPhase NewPhase);

	// 보스 사망 보고 -> 승리
	UFUNCTION(BlueprintCallable, Category = "Game Loop")
	void NotifyBossDefeated();

	// 플레이어 사망 보고 -> 패배
	UFUNCTION(BlueprintCallable, Category = "Game Loop")
	void NotifyPlayerDefeated();

	// 현재 보스맵 재시작
	UFUNCTION(BlueprintCallable, Category = "Game Loop")
	void RestartBattle();

protected:
	// 지정한 구역의 볼륨에서 탄약 생성
	void SpawnAmmoForArea(FName AreaTag);

	// 모든 탄약 스폰 볼륨의 생성 아이템 정리
	void ClearAllSpawnedAmmo();

	// 페이즈에 맞는 구역으로 플레이어와 보스를 이동
	void MoveActorsToPhaseArea(EBossPhase NewPhase);

	// 플레이어 생성 후 HUD 표시를 BP에 요청
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Loop|UI")
	void OnPlayerReadyForHUD(APlayerController* PlayerController);

	// 승리와 패배의 공통 종료 처리
	void EndBattle(EVeilBreakBattleResult Result);

	// 전투 종료 중복 실행 방지
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Game Loop"
	)
	bool bBattleEnded;

	// true: 맵 시작과 동시에 전투 시작
	// 보스룸 진입 Trigger 완성 후 false로 변경
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Game Loop|Debug"
	)
	bool bAutoStartBattleForTest;

	// 전투 시작을 보스 AI 및 UI에 전달
	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Game Loop|Integration"
	)
	void OnBattleStarted();

	// 보스 측에서 보고받은 페이즈를 UI 및 연출에 전달
	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Game Loop|Integration"
	)
	void OnBossPhaseChanged(EBossPhase NewPhase);

	// 전투 결과 표시 및 보스 AI 정지를 요청
	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Game Loop|Integration"
	)
	void OnBattleEnded(EVeilBreakBattleResult Result);

	// 보스의 페이즈 변경·사망 이벤트를 GameMode에 연결
	void BindBossEvents();
};