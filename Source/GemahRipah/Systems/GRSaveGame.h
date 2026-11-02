#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "GRSaveGame.generated.h"

/** One save slot. Each subsystem writes and reads its own fields. */
UCLASS()
class GEMAHRIPAH_API UGRSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 3;

	UPROPERTY()
	int32 SaveVersion = CurrentVersion;

	// Slot info shown in the save/load menu
	UPROPERTY()
	FDateTime SavedAt;

	UPROPERTY()
	double PlayTimeSeconds = 0.0;

	// Player
	UPROPERTY()
	bool bHasPlayerTransform = false;

	UPROPERTY()
	FTransform PlayerTransform;

	UPROPERTY()
	FRotator ControlRotation = FRotator::ZeroRotator;

	// Status
	UPROPERTY()
	int32 Uang = 0;

	UPROPERTY()
	float Suara = 0.f;

	UPROPERTY()
	float Diawasi = 0.f;

	// Progress
	UPROPERTY()
	FGameplayTagContainer StoryFlags;

	UPROPERTY()
	TArray<FName> UnlockedCities;

	UPROPERTY()
	FName CurrentCityId;

	// Clock
	UPROPERTY()
	int32 Day = 1;

	UPROPERTY()
	float TimeMinutes = 0.f;

	// Regulations, by tag name
	UPROPERTY()
	TArray<FName> ActiveRegulationIds;
};
