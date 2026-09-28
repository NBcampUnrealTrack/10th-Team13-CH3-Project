#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPSCharacter.generated.h"

class UAnimSequence;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class UCameraComponent;
class UInputAction;
class UInputComponent;
class UInputMappingContext;
class UPlayerCombatComponent;
class UPlayerConsumableComponent;
class UPlayerHealthComponent;
class UPlayerSkillComponent;
class UPlayerStaminaComponent;
class USpringArmComponent;
class UStatusEffectReceiverComponent;
struct FInputActionValue;

// 한 번 재생할 이펙트와 사운드 설정
USTRUCT(BlueprintType)
struct FPlayerFeedbackCue
{
	GENERATED_BODY()

	// 한 번 재생하고 종료되는 나이아가라 에셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	TObjectPtr<UNiagaraSystem> Effect = nullptr;

	// 짧은 단발 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	TObjectPtr<USoundBase> Sound = nullptr;

	// 기준 위치에서의 로컬 위치 보정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	FVector LocationOffset = FVector::ZeroVector;

	// 에셋 방향 보정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	FRotator RotationOffset = FRotator::ZeroRotator;

	// 이펙트 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "0.01"))
	float Scale = 1.0f;

	// 사운드 음량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "0.0"))
	float Volume = 1.0f;
};

UCLASS()
class VEILBREAK_API AFPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// 생성자
	AFPSCharacter();

	// 이동 애니메이션의 발 착지 노티파이에서 호출
	UFUNCTION(BlueprintCallable, Category = "Feedback")
	void PlayFootstepFeedback(FName FootSocketName);

	// 재장전 애니메이션의 장전 순간 노티파이에서 호출
	UFUNCTION(BlueprintCallable, Category = "Feedback")
	void PlayReloadFeedback();

protected:
	// Unreal Override
	virtual void BeginPlay() override;

	// 실제 점프가 성공한 순간 호출
	virtual void OnJumped_Implementation() override;

	// 실제 바닥에 착지한 순간 호출
	virtual void Landed(const FHitResult& Hit) override;

	// 캐릭터 제거 시 지속 효과 정리
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(
		UInputComponent* PlayerInputComponent
	) override;

private:
	// 카메라 방향을 기준으로 캐릭터 이동 처리
	void Move(const FInputActionValue& Value);

	// 마우스 입력으로 카메라 회전 처리
	void Look(const FInputActionValue& Value);

	// 점프 입력 시작
	void StartJump();

	// 점프 입력 종료
	void StopJump();

	// 마우스 왼쪽 버튼 입력으로 사격 시도
	void StartFire();

	// R 입력으로 한 발씩 재장전 시작
	void StartReload();

	// F 입력으로 체력 물약 사용 시도
	void UseHealthPotion();

	// E 입력으로 8초 궁극기 사용 시도
	void StartUltimate();

	// 궁극기 상태에 따라 이동, 공격 및 스태미나 효과 적용
	UFUNCTION()
	void HandleUltimateStateChanged(bool bUltimateActive);

	// 스태미나가 충분하면 달리기 시작
	void StartSprint();

	// Shift 해제 또는 스태미나 소진 시 달리기 종료
	UFUNCTION()
	void StopSprint();

	// 조건과 스태미나를 확인하고 대시 실행
	void StartDash();

	// 대시 재사용 대기시간 종료
	void ResetDash();

	// 우클릭 입력으로 조준 시작
	void StartAim();

	// 우클릭 해제로 조준 종료
	void StopAim();

	// 조준 상태에 따라 카메라 거리와 시야각 변경
	void UpdateAimCamera(float DeltaTime);

	// 체력이 0이 됐을 때 플레이어 행동 정지
	UFUNCTION()
	void HandlePlayerDeath();

private:
	// 실제 피해를 받았을 때만 피격 효과 재생
	UFUNCTION()
	void HandleDamageReceived(float DamageAmount);

	// 물약 회복 상태에 맞춰 지속 이펙트 시작 및 종료
	UFUNCTION()
	void HandlePotionStateChanged(bool bHealing);

	// 월드 위치에서 단발 이펙트와 소리 재생
	void PlayFeedbackCue(const FPlayerFeedbackCue& Cue, const FVector& Location, const FRotator& Rotation);

	// 캡슐 아래쪽을 기준으로 발밑 위치 반환
	FVector GetFeetLocation() const;

	// 물약 및 궁극기의 지속 파티클을 강제로 종료
	void StopPersistentFeedback();

private:
	// 발이 닿을 때마다 소리와 먼지 재생
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Movement")
	FPlayerFeedbackCue FootstepFeedback;

	// 블렌딩 중 겹친 발소리 방지를 위한 최소 간격
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Movement", meta = (ClampMin = "0.0"))
	float FootstepMinimumInterval = 0.12f;

	// 실제 이동 중일 때만 발소리 허용
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Movement", meta = (ClampMin = "0.0"))
	float FootstepMinimumSpeed = 10.0f;

	// 발 본 아래에서 지면을 찾을 거리
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Movement", meta = (ClampMin = "1.0"))
	float FootstepTraceDistance = 100.0f;

	// 실제 점프 성공 시 재생
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Movement")
	FPlayerFeedbackCue JumpFeedback;

	// 착지 시 재생
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Movement")
	FPlayerFeedbackCue LandFeedback;

	// 대시 출발 지점에서 재생
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Dash")
	FPlayerFeedbackCue DashStartFeedback;

	// 대시 도착 지점에서 재생
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Dash")
	FPlayerFeedbackCue DashEndFeedback;

	// 실제 HP 감소 시 재생
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Health")
	FPlayerFeedbackCue DamageFeedback;

	// 연속 피해에서 효과가 너무 겹치는 것 방지
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Health", meta = (ClampMin = "0.0"))
	float DamageFeedbackInterval = 0.1f;

	// 사망 시 한 번 재생
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Health")
	FPlayerFeedbackCue DeathFeedback;

	// 사망 시 재생할 애니메이션 시퀀스 (몽타주가 아님)
	// 재생 후 마지막 자세 유지. 캐릭터와 호환되는 스켈레톤 사용
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Health")
	TObjectPtr<UAnimSequence> DeathAnimation;

	// 물약 사용 성공 시 한 번 재생
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Potion")
	FPlayerFeedbackCue PotionStartFeedback;

	// 회복 중에 유지할 루프 나이아가라
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Potion")
	TObjectPtr<UNiagaraSystem> PotionLoopEffect;

	// 캡슐 중심 기준 회복 이펙트 위치
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Potion")
	FVector PotionLoopOffset = FVector::ZeroVector;

	// 회복 이펙트 크기
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Potion", meta = (ClampMin = "0.01"))
	float PotionLoopScale = 1.0f;

	// 궁극기 시작 시 한 번 재생
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Ultimate")
	FPlayerFeedbackCue UltimateStartFeedback;

	// 궁극기 정상 종료 시 한 번 재생
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Ultimate")
	FPlayerFeedbackCue UltimateEndFeedback;

	// 궁극기 활성 중 유지할 루프 나이아가라
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Ultimate")
	TObjectPtr<UNiagaraSystem> UltimateLoopEffect;

	// 캡슐 중심 기준 궁극기 이펙트 위치
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Ultimate")
	FVector UltimateLoopOffset = FVector::ZeroVector;

	// 궁극기 이펙트 크기
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Ultimate", meta = (ClampMin = "0.01"))
	float UltimateLoopScale = 1.0f;

	// 재장전 노티파이에서 재생할 사운드
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Reload")
	TObjectPtr<USoundBase> ReloadSound;

	// 장전 소리 위치로 사용할 총구 소켓
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Reload")
	FName ReloadSoundSocketName = TEXT("FX_Gun_Barrel");

	// 장전음 음량
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Reload", meta = (ClampMin = "0.0"))
	float ReloadSoundVolume = 1.0f;

	// 지속 이펙트는 소유 컴포넌트로 관리해 종료 시 제거
	UPROPERTY(VisibleAnywhere, Category = "Feedback|Runtime")
	TObjectPtr<UNiagaraComponent> PotionLoopComponent;

	UPROPERTY(VisibleAnywhere, Category = "Feedback|Runtime")
	TObjectPtr<UNiagaraComponent> UltimateLoopComponent;

	// 중복 효과 방지용 상태
	double LastFootstepTime = -1000.0;
	double LastDamageFeedbackTime = -1000.0;
	bool bDeathFeedbackPlayed = false;

private:
	// 컴포넌트

	// 3인칭 카메라 거리를 관리하는 스프링암
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<USpringArmComponent> CameraBoom;

	// 플레이어가 바라보는 3인칭 카메라
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UCameraComponent> FollowCamera;

	// 사격, 조준 및 재장전을 담당하는 컴포넌트
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UPlayerCombatComponent> PlayerCombatComponent;

	// 체력 물약의 보유량과 지속 회복을 관리하는 컴포넌트
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UPlayerConsumableComponent>
		PlayerConsumableComponent;

	// 체력, 피해, 회복 및 사망 상태를 관리하는 컴포넌트
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UPlayerHealthComponent> PlayerHealthComponent;

	// 스킬과 궁극기를 담당하는 컴포넌트
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UPlayerSkillComponent> PlayerSkillComponent;

	// 달리기와 대시에 사용하는 스태미나를 관리하는 컴포넌트
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UPlayerStaminaComponent> PlayerStaminaComponent;

	// 넉백과 경직 등의 상태 이상을 관리하는 컴포넌트
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UStatusEffectReceiverComponent>
		StatusEffectReceiverComponent;

private:
	// 이동 설정

	// 평상시 이동 속도
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 400.0f;

	// Shift를 누르고 있을 때의 이동 속도
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 650.0f;

	// 궁극기 중 적용할 이동 속도.
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Ultimate", meta = (ClampMin = "0.0", Units = "cm/s"))
	float UltimateMoveSpeed = 800.0f;

	// 대시로 순간 이동하는 고정 거리
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashDistance = 650.0f;

	// 대시 한 번에 소모되는 스태미나
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashStaminaCost = 30.0f;

	// 다음 대시를 사용할 수 있을 때까지의 시간
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashCooldown = 0.1f;

	// 현재 대시를 사용할 수 있는지 저장
	bool bCanDash = true;

	// 현재 궁극기의 상시 달리기 효과가 적용됐는지 저장
	bool bIsUltimateActive = false;

private:
	// 카메라 설정

	// 평상시 카메라 거리
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float DefaultCameraDistance = 400.0f;

	// 조준 중 카메라 거리
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float AimCameraDistance = 350.0f;

	// 평상시 카메라 시야각
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float DefaultFieldOfView = 90.0f;

	// 조준 중 카메라 시야각
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float AimFieldOfView = 75.0f;

	// 조준 카메라 전환 속도
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float AimInterpolationSpeed = 12.0f;

	// 현재 조준 중인지 저장
	bool bIsAiming = false;

private:
	// 입력 에셋

	// 플레이어 키 매핑 설정
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Input",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	// WASD 이동 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	// 마우스 시점 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	// Space 점프 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	// 마우스 왼쪽 버튼 사격 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	// 마우스 오른쪽 버튼 조준 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	// 왼쪽 Shift 달리기 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	// Q 대시 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DashAction;

	// E 궁극기 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> UltimateAction;

	// R 재장전 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;

	// F 체력 물약 사용 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PotionAction;
};
