#include "Core/GRMenuPlayerController.h"
#include "Core/GRCheatManager.h"
#include "UI/GRMenuScreens.h"
#include "UI/GRUISubsystem.h"
#include "Camera/CameraActor.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"

AGRMenuPlayerController::AGRMenuPlayerController()
{
	CheatClass = UGRCheatManager::StaticClass();
}

void AGRMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		SetViewTarget(*It);
		break;
	}

	if (UGRUISubsystem* UI = GetLocalPlayer()->GetSubsystem<UGRUISubsystem>())
	{
		UI->PushScreen<UGRMainMenuScreen>();
	}
}
