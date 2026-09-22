#include "VeilBreakPlayerController.h"
#include "EnhancedInputSubsystems.h"

AVeilBreakPlayerController::AVeilBreakPlayerController()
	:InputMappingContext(nullptr),
	MoveAction(nullptr),
	JumpAction(nullptr), 
	LookAction(nullptr),
	SprintAction(nullptr),
	FireAction(nullptr),
	AimAction(nullptr),
	DashAction(nullptr),
	ReloadAction(nullptr),
	UltimateAction(nullptr)
{
}

void AVeilBreakPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	SetShowMouseCursor(false);
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (InputMappingContext)
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Input] Game Input Restored"));
}