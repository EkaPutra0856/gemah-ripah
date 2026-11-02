#include "Core/GRMenuGameMode.h"
#include "Core/GRMenuPlayerController.h"

AGRMenuGameMode::AGRMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AGRMenuPlayerController::StaticClass();
	HUDClass = nullptr;
}
