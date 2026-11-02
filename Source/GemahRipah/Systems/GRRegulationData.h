#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "GRRegulationData.generated.h"

struct FGRRegulationDef;

/**
 * One active "Pengumuman Pemerintah": a rule that can appear suddenly and forbid player actions.
 * Definitions live in Content/Data/regulations.json.
 */
UCLASS(BlueprintType)
class GEMAHRIPAH_API UGRRegulationData : public UObject
{
	GENERATED_BODY()

public:
	static UGRRegulationData* CreateFromDef(UObject* Outer, const FGRRegulationDef& Def);

	UPROPERTY(BlueprintReadOnly, Category = "Aturan")
	FGameplayTag RegulationId;

	UPROPERTY(BlueprintReadOnly, Category = "Aturan")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "Aturan")
	FText Announcement;

	/** Actions (and their child tags) that break this rule. */
	UPROPERTY(BlueprintReadOnly, Category = "Aturan")
	FGameplayTagContainer ForbiddenActions;

	UPROPERTY(BlueprintReadOnly, Category = "Sanksi")
	float DiawasiPenalty = 15.f;

	UPROPERTY(BlueprintReadOnly, Category = "Sanksi")
	int32 FineUang = 0;
};
