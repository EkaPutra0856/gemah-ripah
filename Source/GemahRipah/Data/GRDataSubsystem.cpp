#include "Data/GRDataSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GemahRipah.h"

void UGRDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReloadAll();
}

void UGRDataSubsystem::ReloadAll()
{
	LoadJsonFile(TEXT("regulations.json"), Regulations);
	LoadJsonFile(TEXT("tips.json"), Tips);
	LoadJsonFile(TEXT("credits.json"), Credits);
	LoadJsonFile(TEXT("world.json"), WorldConfig);

	UE_LOG(LogGemahRipah, Log, TEXT("Data dimuat: %d aturan, %d tips, %d kredit."),
		Regulations.Regulations.Num(), Tips.Tips.Num(), Credits.Credits.Num());
}

const FGRRegulationDef* UGRDataSubsystem::FindRegulation(const FString& Id) const
{
	return Regulations.Regulations.FindByPredicate([&Id](const FGRRegulationDef& Def) { return Def.Id == Id; });
}

FString UGRDataSubsystem::GetRandomTip() const
{
	return Tips.Tips.Num() > 0 ? Tips.Tips[FMath::RandRange(0, Tips.Tips.Num() - 1)] : FString();
}

FString UGRDataSubsystem::GetDataDir()
{
	return FPaths::ProjectContentDir() / TEXT("Data");
}

bool UGRDataSubsystem::LoadDataText(const FString& FileName, FString& OutText)
{
	const FString Path = GetDataDir() / FileName;
	if (!FFileHelper::LoadFileToString(OutText, *Path))
	{
		UE_LOG(LogGemahRipah, Warning, TEXT("File data tidak ditemukan: %s"), *Path);
		return false;
	}
	return true;
}

void UGRDataSubsystem::LogParseError(const FString& FileName, const FText& Error)
{
	UE_LOG(LogGemahRipah, Error, TEXT("Gagal membaca %s: %s"), *FileName, *Error.ToString());
}
