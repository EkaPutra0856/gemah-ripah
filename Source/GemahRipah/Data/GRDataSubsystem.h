#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JsonObjectConverter.h"
#include "Data/GRDataTypes.h"
#include "GRDataSubsystem.generated.h"

/**
 * Loads game content from JSON files in Content/Data (dialog, quests, rules, ...).
 * Text files keep content reviewable and editable without the editor.
 */
UCLASS()
class GEMAHRIPAH_API UGRDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Re-reads every JSON file (console: GRReloadData). */
	void ReloadAll();

	const FGRRegulationDef* FindRegulation(const FString& Id) const;
	const TArray<FGRRegulationDef>& GetRegulations() const { return Regulations.Regulations; }
	const TArray<FString>& GetTips() const { return Tips.Tips; }
	const TArray<FGRCreditEntry>& GetCredits() const { return Credits.Credits; }

	FString GetRandomTip() const;

	static FString GetDataDir();

	/** Parses Content/Data/<FileName> into Out. Logs and returns false on failure. */
	template <typename T>
	static bool LoadJsonFile(const FString& FileName, T& Out)
	{
		FString Text;
		if (!LoadDataText(FileName, Text))
		{
			return false;
		}
		Out = T();
		FText Error;
		if (!FJsonObjectConverter::JsonObjectStringToUStruct(Text, &Out, 0, 0, false, &Error))
		{
			LogParseError(FileName, Error);
			return false;
		}
		return true;
	}

private:
	static bool LoadDataText(const FString& FileName, FString& OutText);
	static void LogParseError(const FString& FileName, const FText& Error);

	FGRRegulationFile Regulations;
	FGRTipsFile Tips;
	FGRCreditsFile Credits;
};
