#include "Systems/GRProgressSubsystem.h"
#include "Systems/GRCitySettings.h"
#include "Systems/GRSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "GemahRipah.h"

void UGRProgressSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	for (const FGRCityEntry& City : GetDefault<UGRCitySettings>()->Cities)
	{
		if (City.bUnlockedAtStart)
		{
			UnlockedCities.Add(City.CityId);
			if (CurrentCityId.IsNone())
			{
				CurrentCityId = City.CityId;
			}
		}
	}
}

void UGRProgressSubsystem::AddStoryFlag(FGameplayTag Flag)
{
	if (Flag.IsValid() && !StoryFlags.HasTagExact(Flag))
	{
		StoryFlags.AddTag(Flag);
		OnStoryFlagAdded.Broadcast(Flag);
	}
}

void UGRProgressSubsystem::UnlockCity(FName CityId)
{
	if (!GetDefault<UGRCitySettings>()->FindCity(CityId))
	{
		UE_LOG(LogGemahRipah, Warning, TEXT("Kota tidak dikenal: %s"), *CityId.ToString());
		return;
	}

	bool bAlreadyUnlocked = false;
	UnlockedCities.Add(CityId, &bAlreadyUnlocked);
	if (!bAlreadyUnlocked)
	{
		OnCityUnlocked.Broadcast(CityId);
	}
}

bool UGRProgressSubsystem::TravelToCity(FName CityId)
{
	const FGRCityEntry* City = GetDefault<UGRCitySettings>()->FindCity(CityId);
	if (!City || City->Map.IsNull())
	{
		UE_LOG(LogGemahRipah, Warning, TEXT("Kota atau map tidak ada: %s"), *CityId.ToString());
		return false;
	}
	if (!IsCityUnlocked(CityId))
	{
		UE_LOG(LogGemahRipah, Log, TEXT("Kota masih terkunci: %s"), *CityId.ToString());
		return false;
	}

	CurrentCityId = CityId;
	UGameplayStatics::OpenLevelBySoftObjectPtr(GetGameInstance(), City->Map);
	return true;
}

void UGRProgressSubsystem::WriteToSave(UGRSaveGame& Save) const
{
	Save.StoryFlags = StoryFlags;
	Save.UnlockedCities = UnlockedCities.Array();
	Save.CurrentCityId = CurrentCityId;
}

void UGRProgressSubsystem::ReadFromSave(const UGRSaveGame& Save)
{
	StoryFlags = Save.StoryFlags;
	UnlockedCities = TSet<FName>(Save.UnlockedCities);
	CurrentCityId = Save.CurrentCityId;
}
