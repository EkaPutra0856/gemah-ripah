#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GRRegulationData.generated.h"

/**
 * One "Pengumuman Pemerintah": a rule that can appear suddenly and forbid player actions.
 * Create assets in /Game/GemahRipah/Data/Regulations.
 */
UCLASS(BlueprintType)
class GEMAHRIPAH_API UGRRegulationData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aturan", meta = (Categories = "Aturan"))
	FGameplayTag RegulationId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aturan")
	FText Title;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aturan", meta = (MultiLine = true))
	FText Announcement;

	/** Actions (and their child tags) that break this rule. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aturan", meta = (Categories = "Aksi"))
	FGameplayTagContainer ForbiddenActions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sanksi")
	float DiawasiPenalty = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sanksi")
	int32 FineUang = 0;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("GRRegulationData"), GetFName());
	}
};
