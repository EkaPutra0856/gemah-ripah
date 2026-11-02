#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GRMapSubsystem.generated.h"

class AGRDistrictVolume;
class AGRMapBounds;
class UGRMapMarkerComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FGROnDistrictChanged, AGRDistrictVolume* /*Current, may be null*/, bool /*bEntered*/);

/** Per-map registry for the map UI: districts, markers and the top-down map image. */
UCLASS()
class GEMAHRIPAH_API UGRMapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterDistrict(AGRDistrictVolume* District);
	void UnregisterDistrict(AGRDistrictVolume* District);
	void EnterDistrict(AGRDistrictVolume* District);
	void LeaveDistrict(AGRDistrictVolume* District);

	void RegisterMarker(UGRMapMarkerComponent* Marker);
	void UnregisterMarker(UGRMapMarkerComponent* Marker);

	void SetMapBounds(AGRMapBounds* Bounds);
	AGRMapBounds* GetMapBounds() const;

	AGRDistrictVolume* GetCurrentDistrict() const;

	/** Enters the districts containing Location when overlap events were missed (e.g. spawning inside one). */
	void UpdateFromLocation(const FVector& Location);
	const TArray<TWeakObjectPtr<AGRDistrictVolume>>& GetDistricts() const { return Districts; }
	const TArray<TWeakObjectPtr<UGRMapMarkerComponent>>& GetMarkers() const { return Markers; }

	/** Square world XY area covered by the map. Falls back to the union of districts. */
	FBox2D GetMapArea() const;

	struct FFootprint
	{
		FBox2D Box;
		FLinearColor Color;
	};

	/** Top-down outlines of static meshes (roads first, tallest last), built once on first use. */
	const TArray<FFootprint>& GetFootprints();

	/** Rebuilds footprints next time (console: GRPetaUlang). */
	void InvalidateFootprints() { Footprints.Reset(); bFootprintsBuilt = false; }

	FGROnDistrictChanged OnDistrictChanged;

private:
	TArray<TWeakObjectPtr<AGRDistrictVolume>> Districts;
	TArray<TWeakObjectPtr<AGRDistrictVolume>> EnteredDistricts;
	TArray<TWeakObjectPtr<UGRMapMarkerComponent>> Markers;
	TWeakObjectPtr<AGRMapBounds> MapBounds;
	TArray<FFootprint> Footprints;
	bool bFootprintsBuilt = false;
};
