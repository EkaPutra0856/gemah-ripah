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

	/** OjekKu online with an order offer at once (and a motor if Bayu has none). Then Y takes it. */
	UFUNCTION(Exec)
	void GROjol();

	/** Jumps Bayu to a few metres before the OjekKu pickup or drop-off (scripted tests). */
	UFUNCTION(Exec)
	void GROjolLompat();

	/** Holds a key for a while, as if pressed by the player (for scripted tests). Example: GRTekan W 1.5 */
	UFUNCTION(Exec)
	void GRTekan(const FString& KeyName, float Seconds = 0.1f);

	/** Sends a test toast. */
	UFUNCTION(Exec)
	void GRNotif(const FString& Message);

	/** Moves the player to x, y (cm) facing yaw degrees, for scripted screenshots. Example: GRPos 8740 -9000 0 */
	UFUNCTION(Exec)
	void GRPos(float X, float Y, float Yaw = 0.f);
	/** Logs the player's position and any seated NPC that drifted off its seat (pushed by furniture). */
	UFUNCTION(Exec)
	void GRPosisi();
	/** Points the camera from yaw/pitch degrees relative to Bayu, at Distance cm, for scripted screenshots. */
	UFUNCTION(Exec)
	void GRKamera(float Yaw, float Pitch = -10.f, float Distance = 450.f);
	/** Puts the nearest traffic car Distance cm ahead of Bayu, turned Yaw degrees from his heading, standing still for Hold s. */
	UFUNCTION(Exec)
	void GRMobil(float Distance = 1500.f, float Yaw = 90.f, float Hold = 10.f);
	/** Logs every traffic car (spot, speed, knocked) and any two that overlap. */
	UFUNCTION(Exec)
	void GRLalin();

	/** Logs the townspeople: how many are out, what they are doing, and who is near Bayu. */
	UFUNCTION(Exec)
	void GRWarga();

	/** Lines up townspeople in front of Bayu, standing still, for screenshots of clothes and headwear. */
	UFUNCTION(Exec)
	void GRWargaBaris();

	/** Puts an SPN pair and a lone officer on patrol close to Bayu. */
	UFUNCTION(Exec)
	void GRPolisi();

	/** Logs every Tripo body near Bayu (model, shown, head and foot bone heights) to check the retargeting. */
	UFUNCTION(Exec)
	void GRTubuh();

	/**
	 * Test for riding into things: loose props in a line ahead of Bayu, 5 m apart. Models from GRPropModels, space
	 * separated; default "kursi_plastik pot kotak_suara karpet sembako meja_kecil".
	 */
	UFUNCTION(Exec)
	void GRBarang(const FString& Models);

	/**
	 * Asset gallery for picking store packs: reads a text file of object paths (one per line, e.g.
	 * /Game/PA_UrbanCity/StaticMeshes/SM_Road_01), lays the meshes out in rows on a grey floor far from town, labelled,
	 * materials on tiles and decals on tiles, and moves Bayu to the front middle. A new call clears the old gallery.
	 */
	UFUNCTION(Exec)
	void GRGaleri(const FString& ListFile);

	/** Close-up of gallery item N (1-based): hides the other items and frames this one. 0 shows all again. */
	UFUNCTION(Exec)
	void GRGaleriFokus(int32 Index);

	/** Puts Bayu 2.5 m beside the nearest cart vendor, facing them (check the hands on the handlebar). */
	UFUNCTION(Exec)
	void GRGerobak();

	/**
	 * Cart food (UGRJajanScreen). With the screen closed: puts Bayu in front of the nearest vendor and opens it. Then
	 * words: amounts in menu order ("2 1 0"), "bungkus" or "sini", "bayar". Example: GRJajan "3 1 0 bungkus bayar"
	 */
	UFUNCTION(Exec)
	void GRJajan(const FString& Order);

private:
	UGameInstance* GetGI() const;
	void Print(const FString& Message) const;
	static FString SlotOrDefault(const FString& SlotName);
};
