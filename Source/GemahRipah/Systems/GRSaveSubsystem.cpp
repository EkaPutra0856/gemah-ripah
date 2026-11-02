#include "Systems/GRSaveSubsystem.h"
#include "Systems/GRSaveGame.h"
#include "Systems/GRStatusSubsystem.h"
#include "Systems/GRProgressSubsystem.h"
#include "Systems/GRRegulationSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GemahRipah.h"

const FString UGRSaveSubsystem::DefaultSlot = TEXT("Slot0");

void UGRSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UGRStatusSubsystem>();
	Collection.InitializeDependency<UGRProgressSubsystem>();
	Collection.InitializeDependency<UGRRegulationSubsystem>();
}

bool UGRSaveSubsystem::SaveToSlot(const FString& SlotName)
{
	UGRSaveGame* Save = Cast<UGRSaveGame>(UGameplayStatics::CreateSaveGameObject(UGRSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}

	UGameInstance* GI = GetGameInstance();
	GI->GetSubsystem<UGRStatusSubsystem>()->WriteToSave(*Save);
	GI->GetSubsystem<UGRProgressSubsystem>()->WriteToSave(*Save);
	GI->GetSubsystem<UGRRegulationSubsystem>()->WriteToSave(*Save);

	const bool bOk = UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
	UE_LOG(LogGemahRipah, Log, TEXT("Simpan ke %s: %s"), *SlotName, bOk ? TEXT("berhasil") : TEXT("gagal"));
	return bOk;
}

bool UGRSaveSubsystem::LoadFromSlot(const FString& SlotName)
{
	const UGRSaveGame* Save = Cast<UGRSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Save)
	{
		UE_LOG(LogGemahRipah, Warning, TEXT("Save %s tidak ada atau rusak."), *SlotName);
		return false;
	}

	UGameInstance* GI = GetGameInstance();
	GI->GetSubsystem<UGRStatusSubsystem>()->ReadFromSave(*Save);
	GI->GetSubsystem<UGRProgressSubsystem>()->ReadFromSave(*Save);
	GI->GetSubsystem<UGRRegulationSubsystem>()->ReadFromSave(*Save);
	return true;
}

bool UGRSaveSubsystem::DoesSlotExist(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}
