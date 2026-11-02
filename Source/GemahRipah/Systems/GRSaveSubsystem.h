#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GRSaveSubsystem.generated.h"

/** Saves and loads all persistent subsystems into one slot. */
UCLASS()
class GEMAHRIPAH_API UGRSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static const FString DefaultSlot;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Simpan")
	bool SaveToSlot(const FString& SlotName);

	/** Restores state only. The caller decides whether to travel to the saved city. */
	UFUNCTION(BlueprintCallable, Category = "Simpan")
	bool LoadFromSlot(const FString& SlotName);

	UFUNCTION(BlueprintPure, Category = "Simpan")
	bool DoesSlotExist(const FString& SlotName) const;
};
