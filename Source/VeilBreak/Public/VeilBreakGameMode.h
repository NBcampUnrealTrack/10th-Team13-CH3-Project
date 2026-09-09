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

	// 전투 준비 (초기화)
	UFUNCTION(BlueprintCallable, Category = "Game Loop")
	void PrepareBattle();

	// 실제 전투 시작(Waiting->Combat)
	UFUNCTION(BlueprintCallable, Category = "Game Loop")
	void StartBattle();

	// 보스 페이즈 변경 보고
	UFUNCTION(BlueprintCallable, Category = "Game Loop")
	void NotifyBossPhaseChanged(EVeilBreakBossPhase NewPhase);

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
	// 승리와 패배의 공통 종료 처리
	void EndBattle(
		EVeilBreakBattleResult Result
	);

	// 전투 종료 중복 실행 방지
	// true -> EndBattle을 다시 실행하지 않음
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Game Loop")
	bool bBattleEnded;

	//True -> 맵 시작과 동시에 전투 시작
	//보스룸 진입 Trigger가 완성 시 False로 변경
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Game Loop|Debug")
	bool bAutoStartBattleForTest;

	//[보스 AI 연결 지점]
	UFUNCTION(BlueprintImplementableEvent,Category = "Game Loop|Integration")
	void OnBattleStarted();

	//[보스 AI/UI 연결 지점]
	//페이즈 전환 시 AI Blackboard, 페이즈 UI에 같은 값 전달
	UFUNCTION(BlueprintImplementableEvent,Category = "Game Loop|Integration")
	void OnBossPhaseChanged(EVeilBreakBossPhase NewPhase);

	//[UI·보스 AI 연결]
	//결과 UI 표시, 보스 AI를 정지 이벤트
	UFUNCTION(BlueprintImplementableEvent,Category = "Game Loop|Integration")
	void OnBattleEnded(EVeilBreakBattleResult Result);
};