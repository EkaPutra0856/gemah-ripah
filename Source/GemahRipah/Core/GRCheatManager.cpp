#include "Core/GRCheatManager.h"
#include "Core/GRPlayerController.h"
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
#include "UI/GRMapScreen.h"
#include "UI/GRUISubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Systems/GRClockSubsystem.h"
#include "World/GRBed.h"
#include "World/GRMapBounds.h"
#include "World/GRMapSubsystem.h"
#include "Engine/Engine.h"
#include "Story/GRStorySubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GemahRipah.h"
#include "Framework/Application/SlateApplication.h"
#include "Containers/Ticker.h"

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
	else if (Name == TEXT("peta")) { UI->PushScreen<UGRMapScreen>(); }
	else if (Name == TEXT("tutup")) { UI->PopAll(); }
	else { Print(TEXT("Layar: menu, jeda, simpan, muat, pengaturan, kredit, tutup")); }
}

void UGRCheatManager::GRReloadData()
{
	if (UGameInstance* GI = GetGI())
	{
		GI->GetSubsystem<UGRDataSubsystem>()->ReloadAll();
		GI->GetSubsystem<UGRStorySubsystem>()->Reload();
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

void UGRCheatManager::GRBab(const FString& ChapterId)
{
	UGameInstance* GI = GetGI();
	if (!GI)
	{
		return;
	}
	UGRStorySubsystem* Story = GI->GetSubsystem<UGRStorySubsystem>();
	for (const FGRChapterDef& Chapter : Story->GetChapters())
	{
		if (Chapter.Id == ChapterId)
		{
			break;
		}
		Story->SetChapterDone(Chapter.Id);
	}
	if (!GI->GetSubsystem<UGRGameFlowSubsystem>()->StartChapter(ChapterId))
	{
		Print(FString::Printf(TEXT("Bab tidak bisa dimulai: %s"), *ChapterId));
	}
}

void UGRCheatManager::GRNotif(const FString& Message)
{
	if (UGameInstance* GI = GetGI())
	{
		GI->GetSubsystem<UGRNotificationSubsystem>()->Notify(FText::FromString(Message.IsEmpty() ? TEXT("Tes notifikasi") : Message));
	}
}

void UGRCheatManager::GRJam(float Hour)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRClockSubsystem>()->SetHour(Hour); }
}

void UGRCheatManager::GRAsap(float Ispu)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRClockSubsystem>()->SetIspuOverride(Ispu); }
}

void UGRCheatManager::GRTidur()
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC)
	{
		return;
	}
	// A temporary bed does the fade, time jump and autosave.
	AGRBed* Bed = PC->GetWorld()->SpawnActor<AGRBed>(FVector(0.f, 0.f, -100000.f), FRotator::ZeroRotator);
	Bed->Sleep(PC);
	Bed->SetLifeSpan(5.f);
}

void UGRCheatManager::GRMotor()
{
	if (AGRPlayerController* PC = Cast<AGRPlayerController>(GetOuterAPlayerController()))
	{
		PC->HandleMotor();
	}
}

void UGRCheatManager::GRPos(float X, float Y, float Yaw)
{
	APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	Pawn->TeleportTo(FVector(X, Y, 110.f), FRotator(0.f, Yaw, 0.f));
	PC->SetControlRotation(FRotator(-15.f, Yaw, 0.f));
}

void UGRCheatManager::GRTekan(const FString& KeyName, float Seconds)
{
	const FKey Key(*KeyName);
	if (!Key.IsValid() || !FSlateApplication::IsInitialized())
	{
		UE_LOG(LogTemp, Warning, TEXT("GRTekan: tombol %s tidak dikenal"), *KeyName);
		return;
	}
	FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
	// Core ticker, not the world timer: menus and minigames pause the world.
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Key](float)
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
		}
		return false;
	}), FMath::Max(Seconds, 0.05f));
}

void UGRCheatManager::GRPetaUlang()
{
	const APlayerController* PC = GetOuterAPlayerController();
	const UGRMapSubsystem* Map = PC ? PC->GetWorld()->GetSubsystem<UGRMapSubsystem>() : nullptr;
	if (UGRMapSubsystem* Mutable = const_cast<UGRMapSubsystem*>(Map))
	{
		Mutable->InvalidateFootprints();
	}
}

void UGRCheatManager::GRCerita(const FString& Actions)
{
	if (UGRStorySubsystem* Story = GetWorld()->GetGameInstance()->GetSubsystem<UGRStorySubsystem>())
	{
		Story->Run(Actions);
	}
}

void UGRCheatManager::GRCek(const FString& Condition)
{
	if (const UGRStorySubsystem* Story = GetWorld()->GetGameInstance()->GetSubsystem<UGRStorySubsystem>())
	{
		UE_LOG(LogTemp, Warning, TEXT("GRCek %s = %s | objektif: %s | energi %.0f"), *Condition, Story->Check(Condition) ? TEXT("benar") : TEXT("salah"),
			*Story->GetObjectiveText(), Story->GetEnergi());
	}
}
