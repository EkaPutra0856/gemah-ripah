#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "GRMapMarkerComponent.generated.h"

/** Shows the owner on the minimap and the big map: a colored circle with a short glyph. */
UCLASS(ClassGroup = (GemahRipah), meta = (BlueprintSpawnableComponent))
class GEMAHRIPAH_API UGRMapMarkerComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peta")
	FText Label;

	/** One or two letters drawn inside the circle, e.g. "R" for Rumah. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peta")
	FString Glyph = TEXT("?");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peta")
	FLinearColor Color = FLinearColor(0.9f, 0.6f, 0.2f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peta")
	bool bShowOnMinimap = true;

	/** Tracked markers (quest targets) stay on the minimap edge when out of range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peta")
	bool bTracked = false;

	/** Listed in the map legend. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peta")
	bool bInLegend = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};

/** Placeable map marker for landmarks. */
UCLASS()
class GEMAHRIPAH_API AGRMapMarker : public AActor
{
	GENERATED_BODY()

public:
	AGRMapMarker();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Peta")
	TObjectPtr<UGRMapMarkerComponent> Marker;
};
