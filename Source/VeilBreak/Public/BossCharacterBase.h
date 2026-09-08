#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossCharacterBase.generated.h"

// 보스 캐릭터 공통 부모, Sevarog 메시·idle 반복 재생 기본 설정, BP_BossCharacterBase가 상속
UCLASS()
class VEILBREAK_API ABossCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// 생성자: Sevarog
	ABossCharacterBase();
	// 목표 좌표 방향으로 Cast 시전, 시작 성공 여부 반환
	bool StartMagicAttack(const FVector& Target);
	// Cast 진행 여부
	bool IsMagicAttackRunning() const { return bMagicAttackRunning; }
	// 이번 시전 투사체 생성 성공 여부
	bool DidMagicAttackLaunch() const { return bMagicAttackLaunched; }
	// 시전 시작 간격, 초
	float GetMagicAttackInterval() const { return MagicAttackInterval; }
protected:
	// 종료 시 시전 타이머 정리
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// Idle 기본 모션
	UPROPERTY(EditDefaultsOnly, Category="Boss|MagicAttack")
	TObjectPtr<class UAnimSequence> IdleMotion;
	// Sevarog Cast 모션
	UPROPERTY(EditDefaultsOnly, Category="Boss|MagicAttack")
	TObjectPtr<class UAnimSequence> CastMotion;
	// 생성할 마법 투사체 BP 클래스
	UPROPERTY(EditDefaultsOnly, Category="Boss|MagicAttack")
	TSubclassOf<class ABossMagicAttackActor> MagicAttackClass;
	// 시전 시작 간격, 초
	UPROPERTY(EditDefaultsOnly, Category="Boss|MagicAttack", meta=(ClampMin="0.1"))
	float MagicAttackInterval = 5.f;
	// Cast 시작부터 발사까지의 지연, 초
	UPROPERTY(EditDefaultsOnly, Category="Boss|MagicAttack", meta=(ClampMin="0.0"))
	float MagicReleaseDelay = 0.2f;
	// 발사 기준 손 본 또는 소켓
	UPROPERTY(EditDefaultsOnly, Category="Boss|MagicAttack")
	FName MagicSpawnSocket = TEXT("hand_l");
private:
	// 시전 중 고정된 목표 좌표
	FVector MagicTarget = FVector::ZeroVector;
	// 현재 Cast 진행 여부
	bool bMagicAttackRunning = false;
	// 이번 시전 투사체 생성 여부
	bool bMagicAttackLaunched = false;
	// 발사 예약 타이머
	FTimerHandle MagicReleaseTimer;
	// Idle 복귀 타이머
	FTimerHandle MagicFinishTimer;
	// 손 위치에서 목표로 투사체 생성
	void ReleaseMagicAttack();
	// Cast 종료 후 Idle 반복 재생 복귀
	void FinishMagicAttack();
};
