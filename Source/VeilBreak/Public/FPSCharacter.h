#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPSCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputComponent;
class UInputMappingContext;
class UPlayerCombatComponent;
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
	// Unreal Override
	virtual void BeginPlay() override;

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

	// 스태미나가 충분하면 달리기 시작
	void StartSprint();

	// Shift 해제 또는 스태미나 소진 시 달리기 종료
	UFUNCTION()
	void StopSprint();

	// 조건과 스태미나를 확인하고 대시 실행
	void StartDash();

	// 대시 재사용 대기시간 종료
	void ResetDash();

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

	// 사격, 조준 및 재장전 담당
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UPlayerCombatComponent> PlayerCombatComponent;

	// 넉백 및 경직 상태 담당
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UStatusEffectReceiverComponent>
		StatusEffectReceiverComponent;

	// 스킬 및 궁극기 담당
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UPlayerSkillComponent> PlayerSkillComponent;

	// 달리기와 대시에 사용하는 스태미나 관리
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UPlayerStaminaComponent> PlayerStaminaComponent;

private:
	// 이동 설정

	// 기본 걷기 속도
	UPROPERTY(EditDefaultsOnly, Category = "Movement") 
	float WalkSpeed = 400.0f;

	// Shift 달리기 속도
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 650.0f;

	// 대시에 적용되는 순간 수평 속도
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashStrength = 1200.0f;

	// 대시 한 번에 소모되는 스태미나
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashStaminaCost = 30.0f;

	// 다음 대시를 사용할 수 있을 때까지의 시간
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash")
	float DashCooldown = 0.1f;

	// 현재 대시를 다시 사용할 수 있는지 저장
	bool bCanDash = true;

private:
	// 입력 에셋

	// 플레이어의 키 매핑 설정
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
};