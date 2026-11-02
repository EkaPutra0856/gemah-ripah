#include "Core/GRGameMode.h"
#include "Core/GRPlayerController.h"
#include "Player/GRCharacter.h"
#include "UI/GRHUD.h"
#include "Systems/GRSaveSubsystem.h"
#include "Systems/GRClockSubsystem.h"
#include "Engine/GameInstance.h"
#include "Story/GRStoryDirector.h"
#include "World/GRMapBounds.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

AGRGameMode::AGRGameMode()
{
	DefaultPawnClass = AGRCharacter::StaticClass();
	PlayerControllerClass = AGRPlayerController::StaticClass();
	HUDClass = AGRHUD::StaticClass();
}

void AGRGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	APawn* Pawn = NewPlayer ? NewPlayer->GetPawn() : nullptr;
	UGRSaveSubsystem* Saves = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGRSaveSubsystem>() : nullptr;
	FTransform Transform;
	FRotator ControlRotation;
	if (Pawn && Saves && Saves->ConsumePendingPlayerTransform(Transform, ControlRotation))
	{
		Pawn->SetActorLocationAndRotation(Transform.GetLocation(), Transform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
		NewPlayer->SetControlRotation(ControlRotation);
	}
}

void AGRGameMode::BeginPlay()
{
	Super::BeginPlay();
	GetGameInstance()->GetSubsystem<UGRClockSubsystem>()->SetRunning(true);

	// City maps (they have map bounds) run the chapter.
	if (TActorIterator<AGRMapBounds>(GetWorld()))
	{
		GetWorld()->SpawnActor<AGRStoryDirector>();
	}
}

void AGRGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		GI->GetSubsystem<UGRClockSubsystem>()->SetRunning(false);
	}
	Super::EndPlay(EndPlayReason);
}
