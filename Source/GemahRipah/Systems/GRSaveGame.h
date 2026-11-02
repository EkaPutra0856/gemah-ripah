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
	UPROPERTY()
	int32 SaveVersion = 1;

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

	// Regulations (only asset-backed ones are saved)
	UPROPERTY()
	TArray<FSoftObjectPath> ActiveRegulations;
};
