#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GRStatusSubsystem.generated.h"

class UGRSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGROnStatusChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGROnDiawasiMaxed);

/**
 * Player status that survives map travel: money and the two meters.
 * Suara = courage and support from citizens. Diawasi = risk of being arrested.
 */
UCLASS()
class GEMAHRIPAH_API UGRStatusSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr float MeterMax = 100.f;

	UFUNCTION(BlueprintPure, Category = "Status")
	int32 GetUang() const { return Uang; }

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetSuara() const { return Suara; }

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetDiawasi() const { return Diawasi; }

	UFUNCTION(BlueprintCallable, Category = "Status")
	void AddUang(int32 Delta);

	/** Returns false (and spends nothing) when money is not enough. */
	UFUNCTION(BlueprintCallable, Category = "Status")
	bool SpendUang(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Status")
	void AddSuara(float Delta);

	UFUNCTION(BlueprintCallable, Category = "Status")
	void AddDiawasi(float Delta);

	void WriteToSave(UGRSaveGame& Save) const;
	void ReadFromSave(const UGRSaveGame& Save);

	UPROPERTY(BlueprintAssignable, Category = "Status")
	FGROnStatusChanged OnStatusChanged;

	/** Fired once each time Diawasi reaches the maximum (arrest event hook). */
	UPROPERTY(BlueprintAssignable, Category = "Status")
	FGROnDiawasiMaxed OnDiawasiMaxed;

private:
	UPROPERTY()
	int32 Uang = 50000;

	UPROPERTY()
	float Suara = 0.f;

	UPROPERTY()
	float Diawasi = 0.f;
};
