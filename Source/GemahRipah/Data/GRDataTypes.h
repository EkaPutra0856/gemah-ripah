#pragma once

#include "CoreMinimal.h"
#include "GRDataTypes.generated.h"

// Structs mirrored by the JSON files in Content/Data. Field names match JSON keys (case-insensitive).
// Tags are plain strings in JSON ("Aksi.Kumpul") and converted when used.

USTRUCT()
struct FGRRegulationDef
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Announcement;

	UPROPERTY()
	TArray<FString> ForbiddenActions;

	UPROPERTY()
	float DiawasiPenalty = 15.f;

	UPROPERTY()
	int32 FineUang = 0;
};

USTRUCT()
struct FGRRegulationFile
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGRRegulationDef> Regulations;
};

USTRUCT()
struct FGRTipsFile
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FString> Tips;
};

USTRUCT()
struct FGRCreditEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FString Role;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Detail;
};

USTRUCT()
struct FGRCreditsFile
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGRCreditEntry> Credits;
};

USTRUCT()
struct FGRSmokeDay
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Day = 1;

	/** Air quality index (ISPU). 0-50 baik, 51-100 sedang, 101-200 tidak sehat, 201-300 sangat tidak sehat, 301+ berbahaya. */
	UPROPERTY()
	float Ispu = 100.f;

	/** What the government says about the air that day (shown in Berita). */
	UPROPERTY()
	FString OfficialClaim;
};

/** world.json: clock and smoke settings. */
USTRUCT()
struct FGRWorldConfig
{
	GENERATED_BODY()

	/** Real minutes for one game day. */
	UPROPERTY()
	float DayLengthMinutes = 24.f;

	UPROPERTY()
	int32 StartDay = 1;

	UPROPERTY()
	float StartHour = 6.5f;

	UPROPERTY()
	float WakeHour = 6.f;

	/** Bed works from this hour until SleepUntilHour (next morning). */
	UPROPERTY()
	float SleepFromHour = 18.f;

	UPROPERTY()
	float SleepUntilHour = 4.f;

	UPROPERTY()
	float SmokeDefault = 150.f;

	UPROPERTY()
	TArray<FGRSmokeDay> SmokeByDay;
};
