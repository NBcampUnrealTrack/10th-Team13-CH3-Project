#include "VeilbreakPlayerController.h"
#include "EnhancedInputSubsystems.h"

AVeilbreakPlayerController::AVeilbreakPlayerController()
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

void AVeilbreakPlayerController::BeginPlay()
{
	Super::BeginPlay();
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
}