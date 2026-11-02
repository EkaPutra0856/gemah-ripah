#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UMeshComponent;

/** Small helpers shared by story-spawned actors. */
namespace GRStoryUtil
{
	/** "#RRGGBB" to a linear color (orange on parse failure). */
	FLinearColor ParseColor(const FString& Hex);
	/** "cube", "cylinder", "sphere" (engine basic shapes, 100 cm). */
	UStaticMesh* LoadShape(const FString& Shape);
	/** Flat-colored grid material on every slot of the mesh. */
	void ApplyColor(UMeshComponent* Mesh, const FLinearColor& Color);
	FVector ToLocation(const TArray<float>& Pos, float DefaultZ = 0.f);
}
