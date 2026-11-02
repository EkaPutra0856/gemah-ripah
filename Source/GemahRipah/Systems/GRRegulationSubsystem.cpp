#include "Systems/GRRegulationSubsystem.h"
#include "Systems/GRRegulationData.h"
#include "Systems/GRStatusSubsystem.h"
#include "Systems/GRSaveGame.h"
#include "Data/GRDataSubsystem.h"
#include "GemahRipah.h"

void UGRRegulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UGRStatusSubsystem>();
	Collection.InitializeDependency<UGRDataSubsystem>();
}

UGRRegulationData* UGRRegulationSubsystem::CreateById(const FString& RegulationId)
{
	const FGRRegulationDef* Def = GetGameInstance()->GetSubsystem<UGRDataSubsystem>()->FindRegulation(RegulationId);
	if (!Def)
	{
		UE_LOG(LogGemahRipah, Warning, TEXT("Aturan tidak ada di regulations.json: %s"), *RegulationId);
		return nullptr;
	}
	return UGRRegulationData::CreateFromDef(this, *Def);
}

bool UGRRegulationSubsystem::EnactById(const FString& RegulationId)
{
	UGRRegulationData* Rule = CreateById(RegulationId);
	if (!Rule)
	{
		return false;
	}
	EnactRegulation(Rule);
	return true;
}

void UGRRegulationSubsystem::EnactRegulation(UGRRegulationData* Regulation)
{
	if (!Regulation || ActiveRegulations.Contains(Regulation))
	{
		return;
	}

	// A newer rule with the same id replaces the old one.
	if (Regulation->RegulationId.IsValid())
	{
		ActiveRegulations.RemoveAll([Regulation](const UGRRegulationData* Existing)
		{
			return Existing && Existing->RegulationId == Regulation->RegulationId;
		});
	}

	ActiveRegulations.Add(Regulation);
	UE_LOG(LogGemahRipah, Log, TEXT("Aturan baru: %s"), *Regulation->Title.ToString());
	OnRegulationEnacted.Broadcast(Regulation);
}

void UGRRegulationSubsystem::RevokeRegulation(FGameplayTag RegulationId)
{
	const int32 Removed = ActiveRegulations.RemoveAll([RegulationId](const UGRRegulationData* Existing)
	{
		return Existing && Existing->RegulationId == RegulationId;
	});

	if (Removed > 0)
	{
		OnRegulationRevoked.Broadcast(RegulationId);
	}
}

void UGRRegulationSubsystem::RevokeAll()
{
	TArray<TObjectPtr<UGRRegulationData>> Old = MoveTemp(ActiveRegulations);
	ActiveRegulations.Reset();
	for (const UGRRegulationData* Regulation : Old)
	{
		if (Regulation)
		{
			OnRegulationRevoked.Broadcast(Regulation->RegulationId);
		}
	}
}

bool UGRRegulationSubsystem::IsActive(FGameplayTag RegulationId) const
{
	return ActiveRegulations.ContainsByPredicate([RegulationId](const UGRRegulationData* Existing)
	{
		return Existing && Existing->RegulationId == RegulationId;
	});
}

TArray<UGRRegulationData*> UGRRegulationSubsystem::GetActiveRegulations() const
{
	TArray<UGRRegulationData*> Result;
	Result.Reserve(ActiveRegulations.Num());
	for (UGRRegulationData* Regulation : ActiveRegulations)
	{
		Result.Add(Regulation);
	}
	return Result;
}

UGRRegulationData* UGRRegulationSubsystem::FindViolatedRegulation(FGameplayTag Action) const
{
	if (!Action.IsValid())
	{
		return nullptr;
	}

	for (UGRRegulationData* Regulation : ActiveRegulations)
	{
		// MatchesAny: "Aksi.Kumpul.Demo" breaks a rule that forbids "Aksi.Kumpul".
		if (Regulation && Action.MatchesAny(Regulation->ForbiddenActions))
		{
			return Regulation;
		}
	}
	return nullptr;
}

bool UGRRegulationSubsystem::ReportAction(FGameplayTag Action)
{
	UGRRegulationData* Violated = FindViolatedRegulation(Action);
	if (!Violated)
	{
		return false;
	}

	if (UGRStatusSubsystem* Status = GetGameInstance()->GetSubsystem<UGRStatusSubsystem>())
	{
		Status->AddDiawasi(Violated->DiawasiPenalty);
		if (Violated->FineUang > 0)
		{
			Status->AddUang(-Violated->FineUang);
		}
	}

	OnRegulationViolated.Broadcast(Violated, Action);
	return true;
}

void UGRRegulationSubsystem::ResetState()
{
	ActiveRegulations.Reset();
}

void UGRRegulationSubsystem::WriteToSave(UGRSaveGame& Save) const
{
	Save.ActiveRegulationIds.Reset();
	for (const UGRRegulationData* Regulation : ActiveRegulations)
	{
		if (Regulation && Regulation->RegulationId.IsValid())
		{
			Save.ActiveRegulationIds.Add(Regulation->RegulationId.GetTagName());
		}
	}
}

void UGRRegulationSubsystem::ReadFromSave(const UGRSaveGame& Save)
{
	ActiveRegulations.Reset();
	for (const FName& Id : Save.ActiveRegulationIds)
	{
		if (UGRRegulationData* Rule = CreateById(Id.ToString()))
		{
			ActiveRegulations.Add(Rule);
		}
	}
}
