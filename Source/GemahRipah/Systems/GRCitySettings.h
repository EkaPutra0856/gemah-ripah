#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GRCitySettings.generated.h"

USTRUCT(BlueprintType)
struct FGRCityEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kota")
	FName CityId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kota")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kota")
	TSoftObjectPtr<UWorld> Map;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kota")
	bool bUnlockedAtStart = false;
};

/** City list. Edit in Project Settings > Game > Gemah Ripah Kota (stored in DefaultGame.ini). */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Gemah Ripah Kota"))
class GEMAHRIPAH_API UGRCitySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Kota")
	TArray<FGRCityEntry> Cities;

	const FGRCityEntry* FindCity(FName CityId) const
	{
		return Cities.FindByPredicate([CityId](const FGRCityEntry& Entry) { return Entry.CityId == CityId; });
	}

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
