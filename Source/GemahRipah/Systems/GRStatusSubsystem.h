#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GRStatusSubsystem.generated.h"

class UGRSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGROnStatusChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGROnDiawasiMaxed);
DECLARE_MULTICAST_DELEGATE_TwoParams(FGROnSuaraChanged, float /*Old*/, float /*New*/);

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
	static constexpr int32 StartingUang = 50000;
	/** Where a gain in Suara would shrink to nothing. Past MeterMax, so the top can still be reached. */
	static constexpr float SuaraGainEnd = 125.f;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

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

	/** Gains shrink as the meter fills, so the top takes real work; losses count in full. */
	UFUNCTION(BlueprintCallable, Category = "Status")
	void AddSuara(float Delta, bool bQuiet = false);

	UFUNCTION(BlueprintCallable, Category = "Status")
	void AddDiawasi(float Delta, bool bQuiet = false);

	/** New game values. */
	void ResetState();

	void WriteToSave(UGRSaveGame& Save) const;
	void ReadFromSave(const UGRSaveGame& Save);

	/** "Rp 1.250.000" */
	static FString FormatRupiah(int32 Amount);

	UPROPERTY(BlueprintAssignable, Category = "Status")
	FGROnStatusChanged OnStatusChanged;

	/** Fired once each time Diawasi reaches the maximum (arrest event hook). */
	UPROPERTY(BlueprintAssignable, Category = "Status")
	FGROnDiawasiMaxed OnDiawasiMaxed;

	/** Suara moved through play, not through loading a save. */
	FGROnSuaraChanged OnSuaraChanged;

private:
	void NotifyChange(const FString& Message, bool bGood) const;

	UPROPERTY()
	int32 Uang = StartingUang;

	UPROPERTY()
	float Suara = 0.f;

	UPROPERTY()
	float Diawasi = 0.f;
};
