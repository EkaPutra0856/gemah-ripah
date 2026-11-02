#include "Core/GRGameMode.h"
#include "Core/GRPlayerController.h"
#include "Player/GRCharacter.h"
#include "UI/GRHUD.h"

AGRGameMode::AGRGameMode()
{
	DefaultPawnClass = AGRCharacter::StaticClass();
	PlayerControllerClass = AGRPlayerController::StaticClass();
	HUDClass = AGRHUD::StaticClass();
}
