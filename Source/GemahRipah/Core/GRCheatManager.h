#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "GRCheatManager.generated.h"

/** Debug console commands (open the console with ~). Only available in non-shipping builds. */
UCLASS()
class GEMAHRIPAH_API UGRCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** Prints money, meters, rules, flags and cities. */
	UFUNCTION(Exec)
	void GRStatus();

	UFUNCTION(Exec)
	void GRUang(int32 Delta);

	UFUNCTION(Exec)
	void GRSuara(float Delta);

	UFUNCTION(Exec)
	void GRDiawasi(float Delta);

	/** Enacts the "Larangan Berkumpul" rule (Aksi.Kumpul). */
	UFUNCTION(Exec)
	void GRUmumkanLarangan();

	/** Enacts a rule from regulations.json, e.g. GRUmumkan Aturan.PasalKaretKritik */
	UFUNCTION(Exec)
	void GRUmumkan(const FString& RegulationId);

	UFUNCTION(Exec)
	void GRCabutSemuaAturan();

	/** Reports an action tag, e.g. GRAksi Aksi.Kumpul.Demo */
	UFUNCTION(Exec)
	void GRAksi(const FString& ActionTag);

	UFUNCTION(Exec)
	void GRFlag(const FString& FlagTag);

	UFUNCTION(Exec)
	void GRBukaKota(FName CityId);

	UFUNCTION(Exec)
	void GRKota(FName CityId);

	/** GRSimpan [Slot1|Slot2|Slot3|Auto], default Slot1. */
	UFUNCTION(Exec)
	void GRSimpan(const FString& SlotName);

	UFUNCTION(Exec)
	void GRMuat(const FString& SlotName);

	/** Opens a screen: menu, jeda, simpan, muat, pengaturan, kredit. */
	UFUNCTION(Exec)
	void GRUI(const FString& Screen);

	/** Re-reads Content/Data/*.json without restarting. */
	UFUNCTION(Exec)
	void GRReloadData();

	UFUNCTION(Exec)
	void GRNewGame();

	UFUNCTION(Exec)
	void GRMenu();

	/** Sends a test toast. */
	UFUNCTION(Exec)
	void GRNotif(const FString& Message);

private:
	UGameInstance* GetGI() const;
	void Print(const FString& Message) const;
	static FString SlotOrDefault(const FString& SlotName);
};
