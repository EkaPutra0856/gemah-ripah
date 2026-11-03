#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UMeshComponent;
class UMaterialInstanceDynamic;

/** Small helpers shared by story-spawned actors. */
namespace GRStoryUtil
{
	/** "#RRGGBB" to a linear color (orange on parse failure). */
	FLinearColor ParseColor(const FString& Hex);
	/** "cube", "cylinder", "sphere" (engine basic shapes, 100 cm). */
	UStaticMesh* LoadShape(const FString& Shape);
	/** Flat-colored grid material; LineFactor darkens the grid lines (1 = no lines). */
	UMaterialInstanceDynamic* MakeColorMaterial(UObject* Outer, const FLinearColor& Color, float LineFactor = 0.75f);
	/** Flat-colored grid material on every slot of the mesh. */
	void ApplyColor(UMeshComponent* Mesh, const FLinearColor& Color);
	FVector ToLocation(const TArray<float>& Pos, float DefaultZ = 0.f);
}
