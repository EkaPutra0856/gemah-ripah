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
};
