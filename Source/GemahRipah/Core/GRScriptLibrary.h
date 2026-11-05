#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "GRScriptLibrary.generated.h"

/** Helpers for Blueprint and editor Python scripts (Tools/Python). */
UCLASS()
class GEMAHRIPAH_API UGRScriptLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Python: unreal.GRScriptLibrary.request_tag("Aksi.Kumpul") */
	UFUNCTION(BlueprintPure, Category = "GemahRipah")
	static FGameplayTag RequestTag(FName TagName)
	{
		return FGameplayTag::RequestGameplayTag(TagName, false);
	}

	/**
	 * Editor only (Tools/Python/build_terrain.py): replaces the flat ground of the open level with the GRTerrain
	 * landscape, removes the buildings standing in the river, the kali, the field or the project site, and puts the
	 * buildings on the hills on stone foundations. KeepFlat: story spots that keep level ground. Returns a report.
	 */
	UFUNCTION(BlueprintCallable, Category = "GemahRipah", meta = (DevelopmentOnly))
	static FString BuildTerrain(UObject* WorldContext, const TArray<FVector2D>& KeepFlat, const FString& LandscapeMaterial,
		const FString& FoundationMaterial);
};
