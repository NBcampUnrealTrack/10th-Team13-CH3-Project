#include "VeilBreak_GameMode.h"
#include "FPSCharacter.h"
#include "VeilbreakPlayerController.h"

AVeilBreak_GameMode::AVeilBreak_GameMode()
{
	// Set default pawn class to our character
	DefaultPawnClass = AFPSCharacter::StaticClass();
	PlayerControllerClass = AVeilbreakPlayerController::StaticClass();
}

