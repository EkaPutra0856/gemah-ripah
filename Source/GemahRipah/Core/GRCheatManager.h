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

	/** Enacts a test rule that forbids gathering (Aksi.Kumpul). */
	UFUNCTION(Exec)
	void GRUmumkanLarangan();

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

	UFUNCTION(Exec)
	void GRSimpan();

	UFUNCTION(Exec)
	void GRMuat();

private:
	UGameInstance* GetGI() const;
	void Print(const FString& Message) const;
};
