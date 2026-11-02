#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "GRProgressSubsystem.generated.h"

class UGRSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGROnStoryFlagAdded, FGameplayTag, Flag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGROnCityUnlocked, FName, CityId);

/** Story flags and city unlock/travel. One city = one map; cities unlock as the story goes on. */
UCLASS()
class GEMAHRIPAH_API UGRProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Progres")
	void AddStoryFlag(FGameplayTag Flag);

	UFUNCTION(BlueprintPure, Category = "Progres")
	bool HasStoryFlag(FGameplayTag Flag) const { return StoryFlags.HasTagExact(Flag); }

	UFUNCTION(BlueprintPure, Category = "Progres")
	const FGameplayTagContainer& GetStoryFlags() const { return StoryFlags; }

	UFUNCTION(BlueprintCallable, Category = "Kota")
	void UnlockCity(FName CityId);

	UFUNCTION(BlueprintPure, Category = "Kota")
	bool IsCityUnlocked(FName CityId) const { return UnlockedCities.Contains(CityId); }

	UFUNCTION(BlueprintPure, Category = "Kota")
	FName GetCurrentCityId() const { return CurrentCityId; }

	/** Opens the city's map. Returns false if the city is unknown or still locked. */
	UFUNCTION(BlueprintCallable, Category = "Kota")
	bool TravelToCity(FName CityId);

	void WriteToSave(UGRSaveGame& Save) const;
	void ReadFromSave(const UGRSaveGame& Save);

	UPROPERTY(BlueprintAssignable, Category = "Progres")
	FGROnStoryFlagAdded OnStoryFlagAdded;

	UPROPERTY(BlueprintAssignable, Category = "Kota")
	FGROnCityUnlocked OnCityUnlocked;

private:
	UPROPERTY()
	FGameplayTagContainer StoryFlags;

	UPROPERTY()
	TSet<FName> UnlockedCities;

	UPROPERTY()
	FName CurrentCityId;
};
