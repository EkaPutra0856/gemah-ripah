#include "Systems/GRRegulationData.h"
#include "Data/GRDataTypes.h"
#include "GemahRipah.h"

UGRRegulationData* UGRRegulationData::CreateFromDef(UObject* Outer, const FGRRegulationDef& Def)
{
	UGRRegulationData* Rule = NewObject<UGRRegulationData>(Outer);
	Rule->RegulationId = FGameplayTag::RequestGameplayTag(FName(*Def.Id), false);
	Rule->Title = FText::FromString(Def.Title);
	Rule->Announcement = FText::FromString(Def.Announcement);
	Rule->DiawasiPenalty = Def.DiawasiPenalty;
	Rule->FineUang = Def.FineUang;

	for (const FString& Action : Def.ForbiddenActions)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Action), false);
		if (Tag.IsValid())
		{
			Rule->ForbiddenActions.AddTag(Tag);
		}
		else
		{
			UE_LOG(LogGemahRipah, Warning, TEXT("Aturan %s: tag aksi tidak dikenal '%s'."), *Def.Id, *Action);
		}
	}

	if (!Rule->RegulationId.IsValid())
	{
		UE_LOG(LogGemahRipah, Warning, TEXT("Id aturan bukan tag terdaftar: '%s'."), *Def.Id);
	}
	return Rule;
}
