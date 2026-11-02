#include "Systems/GRSaveSubsystem.h"
#include "Systems/GRSaveGame.h"
#include "Systems/GRStatusSubsystem.h"
#include "Systems/GRProgressSubsystem.h"
#include "Systems/GRRegulationSubsystem.h"
#include "Systems/GRClockSubsystem.h"
#include "Systems/GRNotificationSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "GemahRipah.h"

const FString UGRSaveSubsystem::AutoSlot = TEXT("Auto");

const TArray<FString>& UGRSaveSubsystem::GetManualSlots()
{
	static const TArray<FString> Slots = { TEXT("Slot1"), TEXT("Slot2"), TEXT("Slot3") };
	return Slots;
}

FText UGRSaveSubsystem::GetSlotLabel(const FString& SlotName)
{
	if (SlotName == AutoSlot)
	{
		return NSLOCTEXT("GemahRipah", "AutoSlot", "Simpan Otomatis");
	}
	const int32 Index = GetManualSlots().IndexOfByKey(SlotName);
	return FText::Format(NSLOCTEXT("GemahRipah", "ManualSlot", "Slot {0}"), FText::AsNumber(Index + 1));
}

void UGRSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UGRStatusSubsystem>();
	Collection.InitializeDependency<UGRProgressSubsystem>();
	Collection.InitializeDependency<UGRRegulationSubsystem>();
	Collection.InitializeDependency<UGRNotificationSubsystem>();
	Collection.InitializeDependency<UGRClockSubsystem>();
	ResetSession();
}

void UGRSaveSubsystem::ResetSession()
{
	PlayTimeBase = 0.0;
	SessionStartSeconds = FPlatformTime::Seconds();
	bHasPendingTransform = false;
}

double UGRSaveSubsystem::GetPlayTimeSeconds() const
{
	return PlayTimeBase + (FPlatformTime::Seconds() - SessionStartSeconds);
}

FString UGRSaveSubsystem::FormatPlayTime(double Seconds)
{
	const int32 Total = FMath::FloorToInt(Seconds);
	return FString::Printf(TEXT("%d jam %02d menit"), Total / 3600, (Total / 60) % 60);
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
	GI->GetSubsystem<UGRClockSubsystem>()->WriteToSave(*Save);

	Save->SavedAt = FDateTime::Now();
	Save->PlayTimeSeconds = GetPlayTimeSeconds();

	if (const APlayerController* PC = GI->GetFirstLocalPlayerController())
	{
		if (const APawn* Pawn = PC->GetPawn())
		{
			Save->bHasPlayerTransform = true;
			Save->PlayerTransform = Pawn->GetActorTransform();
			Save->ControlRotation = PC->GetControlRotation();
		}
	}

	const bool bOk = UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
	UE_LOG(LogGemahRipah, Log, TEXT("Simpan ke %s: %s"), *SlotName, bOk ? TEXT("berhasil") : TEXT("gagal"));

	if (UGRNotificationSubsystem* Notifications = GI->GetSubsystem<UGRNotificationSubsystem>())
	{
		Notifications->Notify(bOk
			? FText::Format(NSLOCTEXT("GemahRipah", "Saved", "Tersimpan: {0}"), GetSlotLabel(SlotName))
			: NSLOCTEXT("GemahRipah", "SaveFailed", "Gagal menyimpan"),
			bOk ? EGRNotifyType::Info : EGRNotifyType::Bad);
	}
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
	if (Save->SaveVersion != UGRSaveGame::CurrentVersion)
	{
		UE_LOG(LogGemahRipah, Warning, TEXT("Save %s versi %d, game versi %d. Dimuat sebisanya."), *SlotName, Save->SaveVersion, UGRSaveGame::CurrentVersion);
	}

	UGameInstance* GI = GetGameInstance();
	GI->GetSubsystem<UGRStatusSubsystem>()->ReadFromSave(*Save);
	GI->GetSubsystem<UGRProgressSubsystem>()->ReadFromSave(*Save);
	GI->GetSubsystem<UGRRegulationSubsystem>()->ReadFromSave(*Save);
	GI->GetSubsystem<UGRClockSubsystem>()->ReadFromSave(*Save);

	PlayTimeBase = Save->PlayTimeSeconds;
	SessionStartSeconds = FPlatformTime::Seconds();

	bHasPendingTransform = Save->bHasPlayerTransform;
	PendingTransform = Save->PlayerTransform;
	PendingControlRotation = Save->ControlRotation;
	return true;
}

bool UGRSaveSubsystem::DoesSlotExist(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

FGRSaveSlotInfo UGRSaveSubsystem::GetSlotInfo(const FString& SlotName) const
{
	FGRSaveSlotInfo Info;
	Info.SlotName = SlotName;
	if (!DoesSlotExist(SlotName))
	{
		return Info;
	}

	if (const UGRSaveGame* Save = Cast<UGRSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
	{
		Info.bExists = true;
		Info.SavedAt = Save->SavedAt;
		Info.PlayTimeSeconds = Save->PlayTimeSeconds;
		Info.CityName = GetGameInstance()->GetSubsystem<UGRProgressSubsystem>()->GetCityDisplayName(Save->CurrentCityId);
	}
	return Info;
}

FString UGRSaveSubsystem::FindMostRecentSlot() const
{
	TArray<FString> Slots = GetManualSlots();
	Slots.Add(AutoSlot);

	FString Best;
	FDateTime BestTime = FDateTime::MinValue();
	for (const FString& Slot : Slots)
	{
		const FGRSaveSlotInfo Info = GetSlotInfo(Slot);
		if (Info.bExists && Info.SavedAt > BestTime)
		{
			BestTime = Info.SavedAt;
			Best = Slot;
		}
	}
	return Best;
}

bool UGRSaveSubsystem::ConsumePendingPlayerTransform(FTransform& OutTransform, FRotator& OutControlRotation)
{
	if (!bHasPendingTransform)
	{
		return false;
	}
	bHasPendingTransform = false;
	OutTransform = PendingTransform;
	OutControlRotation = PendingControlRotation;
	return true;
}
