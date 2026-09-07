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
#include "StatusEffectReceiverComponent.h"

AFPSCharacter::AFPSCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(
		0.0,
		500.0,
		0.0
	);
	GetCharacterMovement()->MaxWalkSpeed = 400.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(
		TEXT("CameraBoom")
	);
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(
		TEXT("FollowCamera")
	);
	FollowCamera->SetupAttachment(
		CameraBoom,
		USpringArmComponent::SocketName
	);
	FollowCamera->bUsePawnControlRotation = false;

	PlayerCombatComponent =
		CreateDefaultSubobject<UPlayerCombatComponent>(
			TEXT("PlayerCombatComponent")
		);

	StatusEffectReceiverComponent =
		CreateDefaultSubobject<UStatusEffectReceiverComponent>(
			TEXT("StatusEffectReceiverComponent")
		);

	PlayerSkillComponent =
		CreateDefaultSubobject<UPlayerSkillComponent>(
			TEXT("PlayerSkillComponent")
		);
}

void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController =
		Cast<APlayerController>(Controller);

	if (PlayerController == nullptr)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();

	if (LocalPlayer == nullptr)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<
		UEnhancedInputLocalPlayerSubsystem
		>(LocalPlayer);

	if (
		InputSubsystem != nullptr &&
		PlayerMappingContext != nullptr
		)
	{
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

	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (EnhancedInputComponent == nullptr)
	{
		return;
	}

	if (MoveAction != nullptr)
	{
		EnhancedInputComponent->BindAction(
			MoveAction,
			ETriggerEvent::Triggered,
			this,
			&AFPSCharacter::Move
		);
	}

	if (LookAction != nullptr)
	{
		EnhancedInputComponent->BindAction(
			LookAction,
			ETriggerEvent::Triggered,
			this,
			&AFPSCharacter::Look
		);
	}

	if (JumpAction != nullptr)
	{
		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Started,
			this,
			&AFPSCharacter::StartJump
		);

		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Completed,
			this,
			&AFPSCharacter::StopJump
		);
	}
}

void AFPSCharacter::Move(const FInputActionValue& Value)
{
	if (Controller == nullptr)
	{
		return;
	}

	const FVector2D MovementInput =
		Value.Get<FVector2D>();

	const FRotator ControlRotation =
		Controller->GetControlRotation();

	const FRotator YawRotation(
		0.0,
		ControlRotation.Yaw,
		0.0
	);

	const FVector ForwardDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	const FVector RightDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(
		ForwardDirection,
		MovementInput.Y
	);

	AddMovementInput(
		RightDirection,
		MovementInput.X
	);
}

void AFPSCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookInput =
		Value.Get<FVector2D>();

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void AFPSCharacter::StartJump()
{
	Jump();
}

void AFPSCharacter::StopJump()
{
	StopJumping();
}