#include "Systems/GRGameFlowSubsystem.h"
#include "Systems/GRGameSettings.h"
#include "Systems/GRProgressSubsystem.h"
#include "Systems/GRRegulationSubsystem.h"
#include "Systems/GRSaveSubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "Systems/GRClockSubsystem.h"
#include "Story/GRStorySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GemahRipah.h"

void UGRGameFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UGRSaveSubsystem>();
}

void UGRGameFlowSubsystem::StartNewGame()
{
	UGameInstance* GI = GetGameInstance();
	GI->GetSubsystem<UGRStatusSubsystem>()->ResetState();
	GI->GetSubsystem<UGRProgressSubsystem>()->ResetState();
	GI->GetSubsystem<UGRRegulationSubsystem>()->ResetState();
	GI->GetSubsystem<UGRClockSubsystem>()->ResetState();
	GI->GetSubsystem<UGRStorySubsystem>()->ResetState();
	GI->GetSubsystem<UGRSaveSubsystem>()->ResetSession();

	UGRProgressSubsystem* Progress = GI->GetSubsystem<UGRProgressSubsystem>();
	if (!Progress->TravelToCity(GetDefault<UGRGameSettings>()->StartCityId))
	{
		UE_LOG(LogGemahRipah, Error, TEXT("Game baru gagal: kota awal tidak bisa dibuka. Cek StartCityId di DefaultGame.ini."));
	}
}

bool UGRGameFlowSubsystem::CanContinue() const
{
	return !GetGameInstance()->GetSubsystem<UGRSaveSubsystem>()->FindMostRecentSlot().IsEmpty();
}

bool UGRGameFlowSubsystem::ContinueGame()
{
	const FString Slot = GetGameInstance()->GetSubsystem<UGRSaveSubsystem>()->FindMostRecentSlot();
	return !Slot.IsEmpty() && LoadGame(Slot);
}

bool UGRGameFlowSubsystem::LoadGame(const FString& SlotName)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI->GetSubsystem<UGRSaveSubsystem>()->LoadFromSlot(SlotName))
	{
		return false;
	}
	UGRProgressSubsystem* Progress = GI->GetSubsystem<UGRProgressSubsystem>();
	return Progress->TravelToCity(Progress->GetCurrentCityId());
}

void UGRGameFlowSubsystem::ReturnToMainMenu()
{
	const TSoftObjectPtr<UWorld>& Menu = GetDefault<UGRGameSettings>()->MainMenuMap;
	if (Menu.IsNull())
	{
		UE_LOG(LogGemahRipah, Error, TEXT("MainMenuMap belum diatur di DefaultGame.ini."));
		return;
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(GetGameInstance(), Menu);
}

void UGRGameFlowSubsystem::QuitGame()
{
	UGameInstance* GI = GetGameInstance();
	UKismetSystemLibrary::QuitGame(GI, GI->GetFirstLocalPlayerController(), EQuitPreference::Quit, false);
}
