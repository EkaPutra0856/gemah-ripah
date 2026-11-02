#include "Systems/GRClockSubsystem.h"
#include "Systems/GRSaveGame.h"
#include "Data/GRDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
	const FGRWorldConfig& Config(const UGameInstance* GI)
	{
		static const FGRWorldConfig Fallback;
		const UGRDataSubsystem* Data = GI ? GI->GetSubsystem<UGRDataSubsystem>() : nullptr;
		return Data ? Data->GetWorldConfig() : Fallback;
	}
}

void UGRClockSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UGRDataSubsystem>();
	ResetState();
}

void UGRClockSubsystem::ResetState()
{
	const FGRWorldConfig& Cfg = Config(GetGameInstance());
	Day = Cfg.StartDay;
	Minutes = Cfg.StartHour * 60.f;
	IspuOverride = -1.f;
}

void UGRClockSubsystem::Tick(float DeltaTime)
{
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World || World->IsPaused())
	{
		return;
	}
	const float DayLength = FMath::Max(1.f, Config(GetGameInstance()).DayLengthMinutes);
	const float GameMinutesPerSecond = 1440.f / (DayLength * 60.f);
	float NewMinutes = Minutes + DeltaTime * GameMinutesPerSecond;
	int32 NewDay = Day;
	if (NewMinutes >= 1440.f)
	{
		NewMinutes -= 1440.f;
		++NewDay;
	}
	SetMinutes(NewDay, NewMinutes);
}

void UGRClockSubsystem::SetMinutes(int32 NewDay, float NewMinutes)
{
	const int32 OldHour = GetHour();
	const int32 OldDay = Day;
	Day = NewDay;
	Minutes = FMath::Clamp(NewMinutes, 0.f, 1439.99f);
	if (Day != OldDay)
	{
		OnNewDay.Broadcast(Day);
	}
	if (GetHour() != OldHour || Day != OldDay)
	{
		OnHourChanged.Broadcast(GetHour());
	}
}

bool UGRClockSubsystem::IsNight() const
{
	const float H = GetHours();
	return H >= 18.5f || H < 5.5f;
}

FText UGRClockSubsystem::GetClockText() const
{
	const int32 Total = FMath::FloorToInt(Minutes);
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60));
}

void UGRClockSubsystem::SetHour(float Hour, bool bNextDay)
{
	const float Target = FMath::Fmod(FMath::Max(Hour, 0.f), 24.f) * 60.f;
	SetMinutes(bNextDay && Target <= Minutes ? Day + 1 : Day, Target);
}

bool UGRClockSubsystem::CanSleepNow() const
{
	const FGRWorldConfig& Cfg = Config(GetGameInstance());
	const float H = GetHours();
	return H >= Cfg.SleepFromHour || H < Cfg.SleepUntilHour;
}

void UGRClockSubsystem::SleepUntilMorning()
{
	const FGRWorldConfig& Cfg = Config(GetGameInstance());
	// After midnight we are already on the "next" day.
	const int32 WakeDay = GetHours() < Cfg.SleepUntilHour ? Day : Day + 1;
	SetMinutes(WakeDay, Cfg.WakeHour * 60.f);
}

float UGRClockSubsystem::GetIspuForDay(int32 InDay) const
{
	const FGRWorldConfig& Cfg = Config(GetGameInstance());
	for (const FGRSmokeDay& Entry : Cfg.SmokeByDay)
	{
		if (Entry.Day == InDay)
		{
			return Entry.Ispu;
		}
	}
	return Cfg.SmokeDefault;
}

float UGRClockSubsystem::GetIspu() const
{
	if (IspuOverride >= 0.f)
	{
		return IspuOverride;
	}
	// The new day's smoke rolls in between midnight and 06:00.
	const float Alpha = FMath::Clamp(GetHours() / 6.f, 0.f, 1.f);
	return FMath::Lerp(GetIspuForDay(Day - 1), GetIspuForDay(Day), Alpha);
}

FText UGRClockSubsystem::GetSmokeCategory() const
{
	const float Ispu = GetIspu();
	if (Ispu <= 50.f) { return NSLOCTEXT("GemahRipah", "AirGood", "Baik"); }
	if (Ispu <= 100.f) { return NSLOCTEXT("GemahRipah", "AirModerate", "Sedang"); }
	if (Ispu <= 200.f) { return NSLOCTEXT("GemahRipah", "AirUnhealthy", "Tidak Sehat"); }
	if (Ispu <= 300.f) { return NSLOCTEXT("GemahRipah", "AirVeryUnhealthy", "Sangat Tidak Sehat"); }
	return NSLOCTEXT("GemahRipah", "AirHazard", "Berbahaya");
}

FLinearColor UGRClockSubsystem::GetSmokeColor() const
{
	const float Ispu = GetIspu();
	if (Ispu <= 50.f) { return FLinearColor(FColor(110, 200, 120)); }
	if (Ispu <= 100.f) { return FLinearColor(FColor(90, 160, 235)); }
	if (Ispu <= 200.f) { return FLinearColor(FColor(240, 200, 70)); }
	if (Ispu <= 300.f) { return FLinearColor(FColor(232, 120, 50)); }
	return FLinearColor(FColor(220, 60, 60));
}

FString UGRClockSubsystem::GetOfficialClaim() const
{
	for (const FGRSmokeDay& Entry : Config(GetGameInstance()).SmokeByDay)
	{
		if (Entry.Day == Day)
		{
			return Entry.OfficialClaim;
		}
	}
	return TEXT("Situasi terkendali.");
}

void UGRClockSubsystem::WriteToSave(UGRSaveGame& Save) const
{
	Save.Day = Day;
	Save.TimeMinutes = Minutes;
}

void UGRClockSubsystem::ReadFromSave(const UGRSaveGame& Save)
{
	Day = FMath::Max(1, Save.Day);
	Minutes = Save.TimeMinutes;
	IspuOverride = -1.f;
}
