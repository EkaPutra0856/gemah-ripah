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
