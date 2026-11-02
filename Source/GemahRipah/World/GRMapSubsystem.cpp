#include "World/GRMapSubsystem.h"
#include "World/GRDistrictVolume.h"
#include "World/GRMapBounds.h"
#include "World/GRMapMarkerComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"

void UGRMapSubsystem::RegisterDistrict(AGRDistrictVolume* District)
{
	Districts.AddUnique(District);
}

void UGRMapSubsystem::UnregisterDistrict(AGRDistrictVolume* District)
{
	Districts.Remove(District);
	EnteredDistricts.Remove(District);
}

void UGRMapSubsystem::EnterDistrict(AGRDistrictVolume* District)
{
	EnteredDistricts.Remove(District);
	EnteredDistricts.Add(District);
	OnDistrictChanged.Broadcast(District, true);
}

void UGRMapSubsystem::LeaveDistrict(AGRDistrictVolume* District)
{
	const bool bWasCurrent = GetCurrentDistrict() == District;
	EnteredDistricts.Remove(District);
	if (bWasCurrent)
	{
		OnDistrictChanged.Broadcast(GetCurrentDistrict(), false);
	}
}

AGRDistrictVolume* UGRMapSubsystem::GetCurrentDistrict() const
{
	for (int32 Index = EnteredDistricts.Num() - 1; Index >= 0; --Index)
	{
		if (AGRDistrictVolume* District = EnteredDistricts[Index].Get())
		{
			return District;
		}
	}
	return nullptr;
}

void UGRMapSubsystem::RegisterMarker(UGRMapMarkerComponent* Marker)
{
	Markers.AddUnique(Marker);
}

void UGRMapSubsystem::UnregisterMarker(UGRMapMarkerComponent* Marker)
{
	Markers.Remove(Marker);
}

FBox2D UGRMapSubsystem::GetMapArea() const
{
	if (const AGRMapBounds* Bounds = MapBounds.Get())
	{
		return Bounds->GetArea2D();
	}
	FBox2D Area(ForceInit);
	for (const TWeakObjectPtr<AGRDistrictVolume>& District : Districts)
	{
		if (District.IsValid())
		{
			Area += District->GetArea2D();
		}
	}
	if (!Area.bIsValid)
	{
		Area = FBox2D(FVector2D(-50000.0), FVector2D(50000.0));
	}
	return Area;
}

const TArray<UGRMapSubsystem::FFootprint>& UGRMapSubsystem::GetFootprints()
{
	if (bFootprintsBuilt)
	{
		return Footprints;
	}
	bFootprintsBuilt = true;
	Footprints.Reset();

	const FBox2D Area = GetMapArea();
	const double MaxSide = Area.GetSize().X * 0.8;
	static const FName SurfaceColor(TEXT("SurfaceColor"));
	static const FName BaseColor(TEXT("Base Color"));

	TArray<TPair<double, FFootprint>> Sorted;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		const UStaticMeshComponent* Mesh = It->GetStaticMeshComponent();
		if (!Mesh || !Mesh->IsVisible() || !Mesh->GetStaticMesh())
		{
			continue;
		}
		const FBox Bounds = Mesh->Bounds.GetBox();
		const FBox2D Box(FVector2D(Bounds.Min), FVector2D(Bounds.Max));
		if (!Area.Intersect(Box) || Box.GetSize().X > MaxSide || Box.GetSize().Y > MaxSide)
		{
			continue;
		}
		FLinearColor Color(0.55f, 0.55f, 0.55f);
		if (const UMaterialInterface* Material = Mesh->GetMaterial(0))
		{
			if (!Material->GetVectorParameterValue(FHashedMaterialParameterInfo(SurfaceColor), Color))
			{
				Material->GetVectorParameterValue(FHashedMaterialParameterInfo(BaseColor), Color);
			}
		}
		Color.A = 1.f;
		Sorted.Add({ Bounds.Max.Z, FFootprint{ Box, Color } });
	}
	Sorted.Sort([](const TPair<double, FFootprint>& A, const TPair<double, FFootprint>& B) { return A.Key < B.Key; });
	for (const TPair<double, FFootprint>& Entry : Sorted)
	{
		Footprints.Add(Entry.Value);
	}
	return Footprints;
}

void UGRMapSubsystem::SetMapBounds(AGRMapBounds* Bounds)
{
	MapBounds = Bounds;
}

AGRMapBounds* UGRMapSubsystem::GetMapBounds() const
{
	return MapBounds.Get();
}

void UGRMapSubsystem::UpdateFromLocation(const FVector& Location)
{
	TArray<AGRDistrictVolume*> Containing;
	for (const TWeakObjectPtr<AGRDistrictVolume>& Weak : Districts)
	{
		AGRDistrictVolume* District = Weak.Get();
		if (District && District->GetArea2D().IsInside(FVector2D(Location)) && !EnteredDistricts.Contains(District))
		{
			Containing.Add(District);
		}
	}
	// Larger areas first, so the smallest (innermost) one ends up current.
	Containing.Sort([](const AGRDistrictVolume& A, const AGRDistrictVolume& B) { return A.GetArea2D().GetArea() > B.GetArea2D().GetArea(); });
	for (AGRDistrictVolume* District : Containing)
	{
		EnterDistrict(District);
	}
}
