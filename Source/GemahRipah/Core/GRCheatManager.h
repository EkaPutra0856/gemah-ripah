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

	/** Runs story actions, e.g. GRCerita "quest Pagi; item beras 2". */
	UFUNCTION(Exec)
	void GRCerita(const FString& Actions);

	/** Logs whether a story condition holds, e.g. GRCek "q.Pagi>=2". */
	UFUNCTION(Exec)
	void GRCek(const FString& Condition);

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

	/** Jumps to a chapter, marking every chapter before it done, e.g. GRBab bab2 */
	UFUNCTION(Exec)
	void GRBab(const FString& ChapterId);

	/** Sets the clock, e.g. GRJam 19.5 */
	UFUNCTION(Exec)
	void GRJam(float Hour);

	/** Overrides the air index, e.g. GRAsap 300 (negative = back to daily value). */
	UFUNCTION(Exec)
	void GRAsap(float Ispu);

	/** Sleeps until morning anywhere (fade + autosave). */
	UFUNCTION(Exec)
	void GRTidur();

	/** Re-captures the map image. */
	UFUNCTION(Exec)
	void GRPetaUlang();

	/** Gets on or off the motor, same as the F key. */
	UFUNCTION(Exec)
	void GRMotor();

	/** Holds a key for a while, as if pressed by the player (for scripted tests). Example: GRTekan W 1.5 */
	UFUNCTION(Exec)
	void GRTekan(const FString& KeyName, float Seconds = 0.1f);

	/** Sends a test toast. */
	UFUNCTION(Exec)
	void GRNotif(const FString& Message);

	/** Moves the player to x, y (cm) facing yaw degrees, for scripted screenshots. Example: GRPos 8740 -9000 0 */
	UFUNCTION(Exec)
	void GRPos(float X, float Y, float Yaw = 0.f);

private:
	UGameInstance* GetGI() const;
	void Print(const FString& Message) const;
	static FString SlotOrDefault(const FString& SlotName);
};
