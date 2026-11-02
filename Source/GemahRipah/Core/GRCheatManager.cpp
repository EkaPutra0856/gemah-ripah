#include "Core/GRCheatManager.h"
#include "Systems/GRStatusSubsystem.h"
#include "Systems/GRRegulationSubsystem.h"
#include "Systems/GRRegulationData.h"
#include "Systems/GRProgressSubsystem.h"
#include "Systems/GRSaveSubsystem.h"
#include "Systems/GRGameFlowSubsystem.h"
#include "Systems/GRNotificationSubsystem.h"
#include "Data/GRDataSubsystem.h"
#include "UI/GRMenuScreens.h"
#include "UI/GRSaveLoadScreen.h"
#include "UI/GRSettingsScreen.h"
#include "UI/GRUISubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GemahRipah.h"

UGameInstance* UGRCheatManager::GetGI() const
{
	const APlayerController* PC = GetOuterAPlayerController();
	return PC ? PC->GetGameInstance() : nullptr;
}

void UGRCheatManager::Print(const FString& Message) const
{
	UE_LOG(LogGemahRipah, Display, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Cyan, Message);
	}
}

void UGRCheatManager::GRStatus()
{
	UGameInstance* GI = GetGI();
	if (!GI)
	{
		return;
	}

	const UGRStatusSubsystem* Status = GI->GetSubsystem<UGRStatusSubsystem>();
	const UGRProgressSubsystem* Progress = GI->GetSubsystem<UGRProgressSubsystem>();
	const UGRRegulationSubsystem* Regulations = GI->GetSubsystem<UGRRegulationSubsystem>();

	FString Rules;
	for (const UGRRegulationData* Regulation : Regulations->GetActiveRegulations())
	{
		Rules += Regulation->Title.ToString() + TEXT("; ");
	}

	Print(FString::Printf(TEXT("Uang=%d Suara=%.0f Diawasi=%.0f | Kota=%s | Aturan: %s | Flag: %s"),
		Status->GetUang(), Status->GetSuara(), Status->GetDiawasi(),
		*Progress->GetCurrentCityId().ToString(),
		Rules.IsEmpty() ? TEXT("-") : *Rules,
		*Progress->GetStoryFlags().ToStringSimple()));
}

void UGRCheatManager::GRUang(int32 Delta)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRStatusSubsystem>()->AddUang(Delta); }
}

void UGRCheatManager::GRSuara(float Delta)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRStatusSubsystem>()->AddSuara(Delta); }
}

void UGRCheatManager::GRDiawasi(float Delta)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRStatusSubsystem>()->AddDiawasi(Delta); }
}

void UGRCheatManager::GRUmumkanLarangan()
{
	GRUmumkan(TEXT("Aturan.LaranganBerkumpul"));
}

void UGRCheatManager::GRUmumkan(const FString& RegulationId)
{
	UGameInstance* GI = GetGI();
	if (GI && !GI->GetSubsystem<UGRRegulationSubsystem>()->EnactById(RegulationId))
	{
		Print(FString::Printf(TEXT("Aturan tidak ada di regulations.json: %s"), *RegulationId));
	}
}

void UGRCheatManager::GRCabutSemuaAturan()
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRRegulationSubsystem>()->RevokeAll(); }
}

void UGRCheatManager::GRAksi(const FString& ActionTag)
{
	UGameInstance* GI = GetGI();
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*ActionTag), false);
	if (!GI || !Tag.IsValid())
	{
		Print(FString::Printf(TEXT("Tag tidak dikenal: %s"), *ActionTag));
		return;
	}
	const bool bBroke = GI->GetSubsystem<UGRRegulationSubsystem>()->ReportAction(Tag);
	Print(bBroke ? TEXT("Aksi melanggar aturan.") : TEXT("Aksi aman."));
}

void UGRCheatManager::GRFlag(const FString& FlagTag)
{
	UGameInstance* GI = GetGI();
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*FlagTag), false);
	if (!GI || !Tag.IsValid())
	{
		Print(FString::Printf(TEXT("Tag tidak dikenal: %s"), *FlagTag));
		return;
	}
	GI->GetSubsystem<UGRProgressSubsystem>()->AddStoryFlag(Tag);
}

void UGRCheatManager::GRBukaKota(FName CityId)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRProgressSubsystem>()->UnlockCity(CityId); }
}

void UGRCheatManager::GRKota(FName CityId)
{
	if (UGameInstance* GI = GetGI())
	{
		if (!GI->GetSubsystem<UGRProgressSubsystem>()->TravelToCity(CityId))
		{
			Print(FString::Printf(TEXT("Tidak bisa pindah ke %s (tidak ada atau terkunci)."), *CityId.ToString()));
		}
	}
}

FString UGRCheatManager::SlotOrDefault(const FString& SlotName)
{
	return SlotName.IsEmpty() ? UGRSaveSubsystem::GetManualSlots()[0] : SlotName;
}

void UGRCheatManager::GRSimpan(const FString& SlotName)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRSaveSubsystem>()->SaveToSlot(SlotOrDefault(SlotName)); }
}

void UGRCheatManager::GRMuat(const FString& SlotName)
{
	if (UGameInstance* GI = GetGI())
	{
		const bool bOk = GI->GetSubsystem<UGRGameFlowSubsystem>()->LoadGame(SlotOrDefault(SlotName));
		Print(bOk ? TEXT("Save dimuat.") : TEXT("Save tidak ada."));
	}
}

void UGRCheatManager::GRUI(const FString& Screen)
{
	const APlayerController* PC = GetOuterAPlayerController();
	UGRUISubsystem* UI = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UGRUISubsystem>() : nullptr;
	if (!UI)
	{
		return;
	}
	const FString Name = Screen.ToLower();
	if (Name == TEXT("menu")) { UI->PushScreen<UGRMainMenuScreen>(); }
	else if (Name == TEXT("jeda")) { UI->PushScreen<UGRPauseScreen>(); }
	else if (Name == TEXT("simpan")) { UI->PushScreen<UGRSaveScreen>(); }
	else if (Name == TEXT("muat")) { UI->PushScreen<UGRLoadScreen>(); }
	else if (Name == TEXT("pengaturan")) { UI->PushScreen<UGRSettingsScreen>(); }
	else if (Name == TEXT("kredit")) { UI->PushScreen<UGRCreditsScreen>(); }
	else if (Name == TEXT("tutup")) { UI->PopAll(); }
	else { Print(TEXT("Layar: menu, jeda, simpan, muat, pengaturan, kredit, tutup")); }
}

void UGRCheatManager::GRReloadData()
{
	if (UGameInstance* GI = GetGI())
	{
		GI->GetSubsystem<UGRDataSubsystem>()->ReloadAll();
		Print(TEXT("Data JSON dimuat ulang."));
	}
}

void UGRCheatManager::GRNewGame()
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRGameFlowSubsystem>()->StartNewGame(); }
}

void UGRCheatManager::GRMenu()
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRGameFlowSubsystem>()->ReturnToMainMenu(); }
}

void UGRCheatManager::GRNotif(const FString& Message)
{
	if (UGameInstance* GI = GetGI())
	{
		GI->GetSubsystem<UGRNotificationSubsystem>()->Notify(FText::FromString(Message.IsEmpty() ? TEXT("Tes notifikasi") : Message));
	}
}
