#include "FPSCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "PlayerCombatComponent.h"
#include "PlayerSkillComponent.h"
#include "PlayerStaminaComponent.h"
#include "StatusEffectReceiverComponent.h"
#include "TimerManager.h"

AFPSCharacter::AFPSCharacter()
{
	// 캐릭터 자체에서는 매 프레임 처리하지 않음
	PrimaryActorTick.bCanEverTick = false;

	// 카메라 회전이 캐릭터 회전에 직접 적용되지 않도록 설정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 캐릭터가 이동하는 방향을 바라보도록 설정
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// 이동 방향이 바뀔 때의 회전 속도 설정
	GetCharacterMovement()->RotationRate = FRotator(
		0.0,
		500.0,
		0.0
	);

	// 기본 이동 속도를 걷기 속도로 설정
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// 3인칭 카메라 스프링암 생성
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(
		TEXT("CameraBoom")
	);
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// 스프링암 끝에 3인칭 카메라 생성
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(
		TEXT("FollowCamera")
	);
	FollowCamera->SetupAttachment(
		CameraBoom,
		USpringArmComponent::SocketName
	);
	FollowCamera->bUsePawnControlRotation = false;

	// 사격, 조준 및 재장전 컴포넌트 생성
	PlayerCombatComponent =
		CreateDefaultSubobject<UPlayerCombatComponent>(
			TEXT("PlayerCombatComponent")
		);

	// 넉백 및 경직 상태 컴포넌트 생성
	StatusEffectReceiverComponent =
		CreateDefaultSubobject<UStatusEffectReceiverComponent>(
			TEXT("StatusEffectReceiverComponent")
		);

	// 스킬 및 궁극기 컴포넌트 생성
	PlayerSkillComponent =
		CreateDefaultSubobject<UPlayerSkillComponent>(
			TEXT("PlayerSkillComponent")
		);

	// 달리기와 대시용 스태미나 컴포넌트 생성
	PlayerStaminaComponent =
		CreateDefaultSubobject<UPlayerStaminaComponent>(
			TEXT("PlayerStaminaComponent")
		);
}

void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 블루프린트에서 설정한 걷기 속도를 적용
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	if (PlayerStaminaComponent != nullptr)
	{
		// 스태미나가 소진되면 달리기를 강제로 종료
		PlayerStaminaComponent->OnStaminaDepleted.AddUniqueDynamic(
			this,
			&AFPSCharacter::StopSprint
		);
	}

	// 현재 캐릭터를 조종하는 플레이어 컨트롤러 확인
	APlayerController* PlayerController =
		Cast<APlayerController>(Controller);

	if (PlayerController == nullptr)
	{
		return;
	}

	// Enhanced Input을 사용하는 로컬 플레이어 확인
	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();

	if (LocalPlayer == nullptr)
	{
		return;
	}

	// 플레이어의 Enhanced Input Subsystem 가져오기
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<
		UEnhancedInputLocalPlayerSubsystem
		>(LocalPlayer);

	if (
		InputSubsystem != nullptr &&
		PlayerMappingContext != nullptr
		)
	{
		// 플레이어 입력 매핑 활성화
		InputSubsystem->AddMappingContext(
			PlayerMappingContext,
			0
		);
	}
}

void AFPSCharacter::SetupPlayerInputComponent(
	UInputComponent* PlayerInputComponent
)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 기본 입력 컴포넌트를 Enhanced Input 형식으로 변환
	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (EnhancedInputComponent == nullptr)
	{
		return;
	}

	if (MoveAction != nullptr)
	{
		// WASD 입력이 들어오는 동안 이동 처리
		EnhancedInputComponent->BindAction(
			MoveAction,
			ETriggerEvent::Triggered,
			this,
			&AFPSCharacter::Move
		);
	}

	if (LookAction != nullptr)
	{
		// 마우스 입력이 들어오는 동안 카메라 회전 처리
		EnhancedInputComponent->BindAction(
			LookAction,
			ETriggerEvent::Triggered,
			this,
			&AFPSCharacter::Look
		);
	}

	if (JumpAction != nullptr)
	{
		// Space를 처음 누른 순간 점프 실행
		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Started,
			this,
			&AFPSCharacter::StartJump
		);

		// Space를 뗐을 때 점프 입력 종료
		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Completed,
			this,
			&AFPSCharacter::StopJump
		);
	}

	if (SprintAction != nullptr)
	{
		// Shift를 처음 누른 순간 달리기 시작
		EnhancedInputComponent->BindAction(
			SprintAction,
			ETriggerEvent::Started,
			this,
			&AFPSCharacter::StartSprint
		);

		// Shift를 뗐을 때 달리기 종료
		EnhancedInputComponent->BindAction(
			SprintAction,
			ETriggerEvent::Completed,
			this,
			&AFPSCharacter::StopSprint
		);

		// 입력이 취소된 경우에도 달리기 종료
		EnhancedInputComponent->BindAction(
			SprintAction,
			ETriggerEvent::Canceled,
			this,
			&AFPSCharacter::StopSprint
		);
	}

	if (DashAction != nullptr)
	{
		// Q를 처음 누른 순간 대시 시도
		EnhancedInputComponent->BindAction(
			DashAction,
			ETriggerEvent::Started,
			this,
			&AFPSCharacter::StartDash
		);
	}
}

void AFPSCharacter::Move(const FInputActionValue& Value)
{
	if (Controller == nullptr)
	{
		// 조종 중인 컨트롤러가 없으면 이동하지 않음
		return;
	}

	// IA_Move가 전달한 좌우 및 전후 입력값
	const FVector2D MovementInput =
		Value.Get<FVector2D>();

	// 현재 카메라의 회전값 가져오기
	const FRotator ControlRotation =
		Controller->GetControlRotation();

	// 위아래 각도를 제외하고 좌우 회전만 사용
	const FRotator YawRotation(
		0.0,
		ControlRotation.Yaw,
		0.0
	);

	// 카메라 기준의 전방 방향 계산
	const FVector ForwardDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	// 카메라 기준의 오른쪽 방향 계산
	const FVector RightDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// W와 S 입력으로 전진 및 후진 처리
	AddMovementInput(
		ForwardDirection,
		MovementInput.Y
	);

	// A와 D 입력으로 좌우 이동 처리
	AddMovementInput(
		RightDirection,
		MovementInput.X
	);
}

void AFPSCharacter::Look(const FInputActionValue& Value)
{
	// IA_Look이 전달한 마우스 이동값 가져오기
	const FVector2D LookInput =
		Value.Get<FVector2D>();

	// 마우스 가로 입력으로 좌우 회전
	AddControllerYawInput(LookInput.X);

	// 마우스 세로 입력으로 상하 회전
	AddControllerPitchInput(LookInput.Y);
}

void AFPSCharacter::StartJump()
{
	// ACharacter가 제공하는 기본 점프 실행
	Jump();
}

void AFPSCharacter::StopJump()
{
	// 점프 입력이 끝났음을 ACharacter에 전달
	StopJumping();
}

void AFPSCharacter::StartSprint()
{
	if (PlayerStaminaComponent == nullptr)
	{
		// 스태미나 컴포넌트가 없으면 달리기 불가
		return;
	}

	if (!PlayerStaminaComponent->StartSprintConsumption())
	{
		// 스태미나가 없으면 달리기를 시작하지 않음
		return;
	}

	// 스태미나 소모 시작에 성공하면 달리기 속도 적용
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AFPSCharacter::StopSprint()
{
	if (PlayerStaminaComponent != nullptr)
	{
		// 달리기에 사용되는 지속 스태미나 소모 중단
		PlayerStaminaComponent->StopSprintConsumption();
	}

	// 기본 걷기 속도로 복구
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AFPSCharacter::StartDash()
{
	if (!bCanDash)
	{
		// 대시 재사용 대기시간 중이면 실행하지 않음
		return;
	}

	if (
		StatusEffectReceiverComponent != nullptr &&
		StatusEffectReceiverComponent->IsCrowdControlled()
		)
	{
		// 경직이나 넉백 등 CC 상태에서는 대시 불가
		return;
	}

	if (PlayerStaminaComponent == nullptr)
	{
		// 스태미나 컴포넌트가 없으면 대시 불가
		return;
	}

	if (
		!PlayerStaminaComponent->TryConsumeStamina(
			DashStaminaCost
		)
		)
	{
		// 스태미나가 30 미만이면 대시를 실행하지 않음
		return;
	}

	// 대시 재사용 대기시간 시작
	bCanDash = false;

	// 현재 캐릭터의 이동 방향 가져오기
	FVector DashDirection = GetVelocity();

	// 낙하 속도는 대시 방향 계산에서 제외
	DashDirection.Z = 0.0f;

	if (DashDirection.IsNearlyZero())
	{
		// 정지 중이면 캐릭터가 바라보는 방향으로 대시
		DashDirection = GetActorForwardVector();
	}

	// 대시 거리가 현재 속도의 영향을 받지 않도록 방향 정규화
	DashDirection.Normalize();

	// 지상과 공중 모두에서 일정한 수평 대시 속도 적용
	LaunchCharacter(
		DashDirection * DashStrength,
		true,
		true
	);

	// 설정한 대기시간 후 다시 대시할 수 있도록 타이머 실행
	FTimerHandle DashCooldownTimerHandle;

	GetWorldTimerManager().SetTimer(
		DashCooldownTimerHandle,
		this,
		&AFPSCharacter::ResetDash,
		DashCooldown,
		false
	);
}

void AFPSCharacter::ResetDash()
{
	// 대시 재사용 대기시간이 끝났으므로 사용 가능 상태로 변경
	bCanDash = true;
}