#include "UI/GRUISubsystem.h"
#include "UI/GRScreenWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UGRUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UGRUISubsystem::HandleWorldCleanup);
}

void UGRUISubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	Super::Deinitialize();
}

void UGRUISubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	// The viewport drops every widget on map change; forget them too.
	if (World && World->IsGameWorld())
	{
		Stack.Reset();
		bPausedByUI = false;
	}
}

UGRScreenWidget* UGRUISubsystem::PushScreen(TSubclassOf<UGRScreenWidget> ScreenClass)
{
	APlayerController* PC = GetLocalPlayer()->PlayerController;
	if (!PC || !ScreenClass)
	{
		return nullptr;
	}

	UGRScreenWidget* Screen = CreateWidget<UGRScreenWidget>(PC, ScreenClass);
	if (!Screen)
	{
		return nullptr;
	}

	Screen->AddToViewport(100 + Stack.Num());
	Stack.Add(Screen);
	Refresh();
	Screen->OnShown();
	return Screen;
}

void UGRUISubsystem::PopScreen(UGRScreenWidget* Screen)
{
	if (!Screen || !Stack.Contains(Screen))
	{
		return;
	}

	Screen->RemoveFromParent();
	Stack.Remove(Screen);
	Refresh();

	if (UGRScreenWidget* Top = GetTopScreen())
	{
		Top->OnShown();
	}
}

void UGRUISubsystem::PopAll()
{
	for (UGRScreenWidget* Screen : Stack)
	{
		if (Screen)
		{
			Screen->RemoveFromParent();
		}
	}
	Stack.Reset();
	Refresh();
}

void UGRUISubsystem::Refresh()
{
	APlayerController* PC = GetLocalPlayer()->PlayerController;
	if (!PC)
	{
		return;
	}

	bool bWantsPause = false;
	for (const UGRScreenWidget* Screen : Stack)
	{
		bWantsPause |= Screen && Screen->bPausesGame;
	}

	if (UGRScreenWidget* Top = GetTopScreen())
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(Top->TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}

	if (bWantsPause != bPausedByUI)
	{
		UGameplayStatics::SetGamePaused(PC, bWantsPause);
		bPausedByUI = bWantsPause;
	}
}
