#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossData.h"
#include "BossCharacterBase.generated.h"

class UBossStatComponent;
class ABossBerserkActor;
class ABossVortexActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossDamageApplied, float, AppliedDamage);

// 보스 캐릭터 공통 부모, Sevarog 메시·idle 반복 재생 기본 설정, BP_BossCharacterBase가 상속
UCLASS()
class VEILBREAK_API ABossCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// 생성자: Sevarog
	ABossCharacterBase();
	// ABP 포즈 선택과 기존 팀원 패턴의 Single Node 호환 연결
	int32 GetAnimationMotionIndex() const;
	void PreparePatternAnimation(EBossPattern Pattern);
	// BP에서 지정한 보스 이동속도를 CharacterMovement에 적용
	virtual void BeginPlay() override;
	// Player 0 숫자 0 입력 감지, 디버그 체력·페이즈 순환 호출
	virtual void Tick(float DeltaSeconds) override;
	// 숫자 0 입력마다 Phase1·2·3 대표 체력 순환
	UFUNCTION(BlueprintCallable, Category="Boss|Debug")
	void CycleDebugHealthPhase();
	// TakeDamage를 BossStatComponent에 전달
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	// 실제 체력이 감소했을 때 데미지 텍스트 등에 적용 피해량 전달
	UPROPERTY(BlueprintAssignable, Category="Boss|Damage")
	FOnBossDamageApplied OnBossDamageApplied;
	// 보스 체력 컴포넌트 반환
	UBossStatComponent* GetBossStatComponent() const { return BossStatComponent; }
	
	// 목표 좌표로 MagicAttack 시전, 시작 성공 여부 반환
	bool StartMagicAttack(const FVector& Target);
	// 목표 좌표로 FallingRock 시전, 시작 성공 여부 반환
	bool StartFallingRock(const FVector& Target);
	// 플레이어 무피격 조건과 쿨타임을 만족하면 발악 시작
	bool StartBerserk(AActor* TargetActor);
	// 현재 조건에서 발악 시작 가능 여부 반환
	bool CanStartBerserk(AActor* TargetActor) const;
	// 플레이어 공개 체력값을 관찰해 마지막 체력 감소 시각 갱신
	void UpdatePlayerNoDamageState(AActor* TargetActor);
	// 사격으로 파괴된 발악 구체 수 반영
	void HandleBerserkOrbDestroyed(ABossBerserkActor* DestroyedOrb);
	// Phase1·사거리·쿨타임 조건을 만족하면 추적 소용돌이 시작
	bool StartVortex(AActor* TargetActor);
	// Phase1·사거리·쿨타임 기준 소용돌이 시작 가능 여부 반환
	bool CanStartVortex(AActor* TargetActor) const;
	// 현재 소용돌이 패턴 진행 여부
	bool IsVortexRunning() const { return bVortexRunning; }
	// MagicAttack 진행 여부
	bool IsMagicAttackRunning() const { return bMagicAttackRunning; }
	// FallingRock 시전 진행 여부
	bool IsFallingRockRunning() const { return bFallingRockRunning; }
	// MagicAttack·FallingRock 중 하나라도 시전 중인지 반환
	bool IsPatternRunning() const { return bMagicAttackRunning || bFallingRockRunning || bBerserkRunning || bVortexCasting; }
	// 발악 패턴 진행 여부
	bool IsBerserkRunning() const { return bBerserkRunning; }
	// 이번 MagicAttack 시전 투사체 생성 성공 여부
	bool DidMagicAttackLaunch() const { return bMagicAttackLaunched; }
	// Magic Attack 시전·추적 전환 거리, cm
	float GetMagicAttackRange() const { return MagicAttackRange; }
	// FallingRock 시전 거리, cm
	float GetFallingRockRange() const { return FallingRockRange; }
	
	// 플레이어 추적 시작 거리, cm
	float GetChaseStartDistance() const { return ChaseStartDistance; }
	// 플레이어 추적 종료 거리, cm
	float GetChaseStopDistance() const { return ChaseStopDistance; }
protected:
	// 보스 체력·무적·사망 상태 컴포넌트
	UPROPERTY()
	TObjectPtr<class UBossStatComponent> BossStatComponent;
	// 종료 시 시전 타이머 정리
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// 팀원 Single Node 패턴의 호환 복귀 포즈
	UPROPERTY()
	TObjectPtr<class UAnimSequence> IdleMotion;
	// ABP Cast 노드와 동일한 시퀀스: 게임 로직 타이머 길이 조회용
	UPROPERTY()
	TObjectPtr<class UAnimSequence> CastMotion;
	// ABP 사망 노드와 동일한 시퀀스: 숨김 타이머 길이 조회용
	UPROPERTY()
	TObjectPtr<class UAnimSequence> DeathMotion;
	// 사망 모션 재생 속도, 기본 속도의 40%
	UPROPERTY()
	float DeathAnimationPlayRate = 0.4f;
	// 사망 모션 종료 후 보스를 감추며 재생할 영혼 폭발 이펙트
	UPROPERTY()
	TObjectPtr<class UParticleSystem> DeathDisappearEffect;
	// 사망 이펙트 월드 크기 배율
	UPROPERTY()
	float DeathDisappearEffectScale = 4.f;
	// 중심 포함 사망 이펙트 동시 생성 개수
	UPROPERTY()
	int32 DeathDisappearEffectCount = 7;
	// 중심 외 사망 이펙트의 보스 주변 배치 반경, cm
	UPROPERTY()
	float DeathDisappearEffectRadius = 140.f;
	// 사망 시 재생할 Sevarog 보이스
	UPROPERTY()
	TObjectPtr<class USoundBase> DeathVoice;
	// 마법공격 시작 시 재생할 Sevarog 보이스
	UPROPERTY()
	TObjectPtr<class USoundBase> MagicAttackVoice;
	// 낙석 시작 시 재생할 Sevarog 보이스
	UPROPERTY()
	TObjectPtr<class USoundBase> FallingRockVoice;
	// 소용돌이 시작 시 재생할 Sevarog 보이스
	UPROPERTY()
	TObjectPtr<class USoundBase> VortexVoice;
	// 발악 시작 시 재생할 Sevarog 보이스
	UPROPERTY()
	TObjectPtr<class USoundBase> BerserkVoice;
	// 생성할 마법 투사체 BP 클래스
	UPROPERTY()
	TSubclassOf<class ABossMagicAttackActor> MagicAttackClass;
	// 생성할 낙석 투사체 BP 클래스
	UPROPERTY()
	TSubclassOf<class ABossFallingRockActor> FallingRockClass;
	// ABP 낙석 노드와 동일한 시퀀스: 게임 로직 타이머 길이 조회용
	UPROPERTY()
	TObjectPtr<class UAnimSequence> FallingRockMotion;
	// 생성할 사격 파괴용 발악 구체 BP 클래스
	UPROPERTY()
	TSubclassOf<ABossBerserkActor> BerserkOrbClass;
	// 생성할 추적 소용돌이 상속 BP 클래스
	UPROPERTY()
	TSubclassOf<ABossVortexActor> VortexClass;
	// 발악 재사용 대기시간, 초
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Berserk", meta=(ClampMin="0"))
	float BerserkCooldown = 120.f;
	// 발악 패턴 발동 가능 거리, cm
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Berserk", meta=(ClampMin="1"))
	float BerserkRange = 3100.f;
	// 발악 최대 유지시간, 초
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Berserk", meta=(ClampMin="0.1"))
	float BerserkMaxDuration = 20.f;
	// 제한시간 실패 시 최대 체력 기준 회복 비율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Berserk", meta=(ClampMin="0", ClampMax="1"))
	float BerserkHealPercent = 0.2f;
	// 발악 시작 시 생성할 구체 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Berserk", meta=(ClampMin="1"))
	int32 BerserkOrbCount = 5;
	// 소용돌이 패턴 발동 가능 거리, cm
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Vortex", meta=(ClampMin="1"))
	float VortexRange = 3100.f;
	// 소용돌이 범위에서 초당 적용할 피해량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Vortex", meta=(ClampMin="0"))
	float VortexDamage = 5.f;
	// 소용돌이 패턴 재사용 대기시간, 초
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Vortex", meta=(ClampMin="0"))
	float VortexCooldown = 30.f;
	// 소용돌이 추적 이동속도, cm/s
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Vortex", meta=(ClampMin="0"))
	float VortexSpeed = 450.f;
	// 소용돌이 추적 유지시간, 초
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Vortex", meta=(ClampMin="0.1"))
	float VortexDuration = 10.f;
	// Cast 시작부터 소용돌이 생성까지 지연, 초
	UPROPERTY()
	float VortexSpawnDelay = 0.3f;
	// 보스 중심에서 구체가 생성되는 최소·최대 반경, cm
	UPROPERTY()
	FVector2D BerserkOrbSpawnRadius = FVector2D(350.f, 750.f);
	// Magic Attack 시전·추적 전환 거리, cm
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|MagicAttack", meta=(ClampMin="1"))
	float MagicAttackRange = 3100.f;
	// 마법 공격 투사체 직접 충돌 피해량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|MagicAttack", meta=(ClampMin="0"))
	float MagicAttackProjectileDamage = 0.f;
	// 마법 공격 도착 원형 범위 피해량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|MagicAttack", meta=(ClampMin="0"))
	float MagicAttackExplosiveDamage = 20.f;
	// 마법 공격 투사체 이동속도, cm/s
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|MagicAttack", meta=(ClampMin="1"))
	float MagicAttackProjectileSpeed = 1200.f;
	// FallingRock 시전 거리, cm
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|FallingRock", meta=(ClampMin="1"))
	float FallingRockRange = 3100.f;
	// 낙석 착지 범위 피해량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|FallingRock", meta=(ClampMin="0"))
	float FallingRockDamage = 30.f;
	// 낙석 투사체 이동속도, cm/s
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|FallingRock", meta=(ClampMin="1"))
	float FallingRockProjectileSpeed = 1200.f;
	// 보스 기본 이동속도, cm/s
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Movement", meta=(ClampMin="0"))
	float BossMovementSpeed = 300.f;
	// 플레이어 추적 시작 거리, cm
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Movement", meta=(ClampMin="1"))
	float ChaseStartDistance = 3000.f;
	// 플레이어 추적 종료 거리, cm
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PatternSetter|Movement", meta=(ClampMin="1"))
	float ChaseStopDistance = 2000.f;
	// Cast 시작부터 발사까지의 지연, 초
	UPROPERTY()
	float MagicReleaseDelay = 0.2f;
	// FallingRock 모션 시작부터 발사까지 지연, 초
	UPROPERTY()
	float FallingRockReleaseDelay = 0.8f;
	// 발사 기준 손 본 또는 소켓
	UPROPERTY()
	FName MagicSpawnSocket = TEXT("hand_l");
	// 낙석 발사 기준 손 본 또는 소켓
	UPROPERTY()
	FName FallingRockSpawnSocket = TEXT("hand_l");
	// 낙석 위험 지점에 지속 표시할 Sevarog 타기팅 이펙트
	UPROPERTY()
	TObjectPtr<class UParticleSystem> FallingRockWarningEffect;
	// 마법 공격 도착 지점에 지속 표시할 타기팅 이펙트
	UPROPERTY()
	TObjectPtr<class UParticleSystem> MagicAttackWarningEffect;
	// 마법 공격 경고 이펙트 월드 크기 배율
	UPROPERTY()
	float MagicAttackWarningScale = 1.1f;
	// 낙석 패턴 시작부터 경고 표시까지 지연, 초
	UPROPERTY()
	float FallingRockWarningDelay = 0.2f;
	// 경고 이펙트 월드 크기 배율
	UPROPERTY()
	float FallingRockWarningScale = 2.f;
	// BP Class Defaults에서 숫자 0 체력·페이즈 순환 활성화 여부
	UPROPERTY()
	bool bEnablePhaseDebugInput = true;
private:
	void RestoreBossAnimationBlueprint();
	bool bLegacyPatternAnimation = false;
	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> BossAnimationClass;
	// 다음 숫자 0 입력에 적용할 페이즈 순번, 시작 상태 Phase1 다음인 Phase2부터 적용
	int32 DebugPhaseIndex = 1;
	// 시전 중 고정된 목표 좌표
	FVector MagicTarget = FVector::ZeroVector;
	
	
	
	
	// 현재 magic attack 패턴 진행 여부
	bool bMagicAttackRunning = false;
	// 이번 시전 투사체 생성 여부
	bool bMagicAttackLaunched = false;
	// 현재 FallingRock 패턴 진행 여부
	bool bFallingRockRunning = false;
	// 현재 발악 패턴 진행 여부
	bool bBerserkRunning = false;
	// 현재 소용돌이 패턴 진행 여부
	bool bVortexRunning = false;
	// 소용돌이 소환 Cast 모션 진행 여부
	bool bVortexCasting = false;
	// 다음 소용돌이 사용 가능 월드 시간
	double NextVortexAvailableTime = 0.0;
	// 현재 플레이어를 추적 중인 소용돌이 액터
	TWeakObjectPtr<ABossVortexActor> ActiveVortex;
	// Cast 시작 시 저장한 소용돌이 추적 대상
	TWeakObjectPtr<AActor> PendingVortexTarget;
	// Cast 이후 소용돌이 생성을 예약하는 타이머
	FTimerHandle VortexSpawnTimer;
	// 소용돌이 패턴 종료 타이머
	FTimerHandle VortexFinishTimer;
	// Cast 모션 종료 후 Idle 복귀 타이머
	FTimerHandle VortexCastFinishTimer;
	// 다음 발악 사용 가능 월드 시간
	double NextBerserkAvailableTime = 0.0;
	// 체력을 관찰 중인 플레이어 참조
	TWeakObjectPtr<AActor> ObservedBerserkTarget;
	// 직전 서비스 갱신에서 확인한 플레이어 체력
	float ObservedPlayerHealth = 0.f;
	// 플레이어 체력 감소를 마지막으로 확인한 월드 시간
	double LastObservedPlayerDamageTime = 0.0;
	// 플레이어 체력 관찰값 초기화 여부
	bool bHasObservedPlayerHealth = false;
	// 아직 파괴되지 않은 발악 구체 수
	int32 RemainingBerserkOrbs = 0;
	// 현재 발악에서 생성된 구체 참조
	TArray<TWeakObjectPtr<ABossBerserkActor>> ActiveBerserkOrbs;
	// 발악 제한시간 타이머
	FTimerHandle BerserkTimeoutTimer;
	// FallingRock 시전 중 고정된 목표 좌표
	FVector FallingRockTarget = FVector::ZeroVector;
	// 발사 예약 타이머
	FTimerHandle MagicReleaseTimer;
	// Idle 복귀 타이머
	FTimerHandle MagicFinishTimer;
	// 마법 공격 위험 지점 경고 제거 타이머
	FTimerHandle MagicWarningClearTimer;
	// FallingRock 발사 예약 타이머
	FTimerHandle FallingRockReleaseTimer;
	// FallingRock Idle 복귀 타이머
	FTimerHandle FallingRockFinishTimer;
	// FallingRock 위험 지점 경고 생성 타이머
	FTimerHandle FallingRockWarningTimer;
	// FallingRock 위험 지점 경고 제거 타이머
	FTimerHandle FallingRockWarningClearTimer;
	// 사망 모션 종료 후 이펙트와 숨김 처리를 예약하는 타이머
	FTimerHandle DeathDisappearTimer;
	// 현재 표시 중인 FallingRock 위험 지점 ParticleSystem 컴포넌트
	TObjectPtr<class UParticleSystemComponent> FallingRockWarningComponent;
	// 현재 표시 중인 마법 공격 위험 지점 ParticleSystem 컴포넌트
	TObjectPtr<class UParticleSystemComponent> MagicWarningComponent;
	// 손 위치에서 목표로 투사체 생성
	void ReleaseMagicAttack();
	// 시전 시 저장한 바닥 목표에 마법 공격 위험 지점 경고 생성
	void ShowMagicAttackWarning();
	// 현재 마법 공격 위험 지점 경고 비활성화·제거
	void ClearMagicAttackWarning();
	// Cast 종료 후 Idle 반복 재생 복귀
	void FinishMagicAttack();
	// 보스 손 위치에서 낙석 액터 생성
	void ReleaseFallingRock();
	// 시전 시 저장한 바닥 목표에 위험 지점 경고 생성
	void ShowFallingRockWarning();
	// 현재 위험 지점 경고 비활성화·제거
	void ClearFallingRockWarning();
	// Ultimate Swing 종료 후 Idle 반복 재생 복귀
	void FinishFallingRock();
	// 보스 주변 임의 위치에 발악 구체 생성
	void SpawnBerserkOrbs();
	// 제한시간 경과 시 체력 회복 후 발악 종료
	void HandleBerserkTimeout();
	// 무적 해제·남은 구체 제거·Idle 복귀
	void FinishBerserk(bool bTimedOut);
	// 소용돌이 제거와 패턴 실행 상태 종료
	void FinishVortex();
	// Cast 시작 0.3초 후 보스 위치 바닥에 소용돌이 생성
	void SpawnVortex();
	// Cast 모션 종료 후 Idle 반복 재생 복귀
	void FinishVortexCast();
	// 체력 0 이벤트 처리, 패턴 중단·이동 및 BT 정지·사망 모션 재생
	UFUNCTION()
	void HandleBossDied();
	// 사망 이펙트 생성 후 메시·콜리전 숨김, 액터는 GameMode 처리를 위해 유지
	void FinishBossDeathPresentation();
	// 숫자 0 디버그용 부활, 사망 연출 취소 후 표시·이동·BT 복구
	void ReviveBossForDebug();
};
