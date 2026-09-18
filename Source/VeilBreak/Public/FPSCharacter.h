// FPSCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPSCharacter.generated.h"

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

UCLASS()
class VEILBREAK_API AFPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// 생성자
	AFPSCharacter();

protected:
	// 게임 시작 시 호출
	virtual void BeginPlay() override;

	// 매 프레임 호출
	virtual void Tick(float DeltaTime) override;

	// Enhanced Input 입력 바인딩
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

	// E 입력으로 8초 궁극기 사용 시도
	void StartUltimate();

	// F 입력으로 체력 물약 사용
	void UseHealthPotion();

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

	// 우클릭 입력 해제로 조준 종료
	void StopAim();

	// 조준 상태에 따라 카메라 거리와 시야각 변경
	void UpdateAimCamera(float DeltaTime);

	// 체력이 0이 됐을 때 플레이어 행동 정지
	UFUNCTION()
	void HandlePlayerDeath();

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

	// 체력 물약의 개수와 지속 회복을 관리하는 컴포넌트
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UPlayerConsumableComponent> PlayerConsumableComponent;

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

	// F 체력 물약 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PotionAction;
};