#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GRSaveSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FGRSaveSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Simpan")
	FString SlotName;

	UPROPERTY(BlueprintReadOnly, Category = "Simpan")
	bool bExists = false;

	UPROPERTY(BlueprintReadOnly, Category = "Simpan")
	FDateTime SavedAt;

	UPROPERTY(BlueprintReadOnly, Category = "Simpan")
	FText CityName;

	UPROPERTY(BlueprintReadOnly, Category = "Simpan")
	double PlayTimeSeconds = 0.0;
};

/** Save slots (Auto + 3 manual). Saves all persistent subsystems plus the player's position. */
UCLASS()
class GEMAHRIPAH_API UGRSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static const FString AutoSlot;
	static const TArray<FString>& GetManualSlots();
	static FText GetSlotLabel(const FString& SlotName);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Simpan")
	bool SaveToSlot(const FString& SlotName);

	UFUNCTION(BlueprintCallable, Category = "Simpan")
	bool Autosave() { return SaveToSlot(AutoSlot); }

	/** Restores state and remembers the player position. The caller travels to the saved city. */
	UFUNCTION(BlueprintCallable, Category = "Simpan")
	bool LoadFromSlot(const FString& SlotName);

	UFUNCTION(BlueprintPure, Category = "Simpan")
	bool DoesSlotExist(const FString& SlotName) const;

	FGRSaveSlotInfo GetSlotInfo(const FString& SlotName) const;

	/** Most recently written slot, or empty if there is no save. */
	FString FindMostRecentSlot() const;

	/** New game: zero play time, forget pending position. */
	void ResetSession();

	/** The next city load starts at the player start instead of the loaded save's position. */
	void ClearPendingTransform() { bHasPendingTransform = false; }

	/** Used by the game mode after a map load to place the player where the save left him. */
	bool ConsumePendingPlayerTransform(FTransform& OutTransform, FRotator& OutControlRotation);

	double GetPlayTimeSeconds() const;

	static FString FormatPlayTime(double Seconds);

private:
	double PlayTimeBase = 0.0;
	double SessionStartSeconds = 0.0;

	bool bHasPendingTransform = false;
	FTransform PendingTransform;
	FRotator PendingControlRotation = FRotator::ZeroRotator;
};
