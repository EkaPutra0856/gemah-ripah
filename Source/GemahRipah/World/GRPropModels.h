#pragma once

#include "CoreMinimal.h"

/** One basic shape of a prop model, in the prop's local space (cm, origin on the ground, +X = front). */
struct FGRPropPart
{
	FVector Loc = FVector::ZeroVector;
	FVector Size = FVector(10.f);
	FLinearColor Color = FLinearColor::White;
	FRotator Rot = FRotator::ZeroRotator;
	const TCHAR* Shape = TEXT("cube");
	bool bCollide = true;
	/** Hidden while the player stands underneath (tent roofs, awnings), so the camera can see inside. */
	bool bRoof = false;
};

/**
 * Furniture and set dressing assembled from engine basic shapes, so story props read as tables, chairs,
 * tents or motorbikes instead of plain boxes. Seats face +X and have their origin where the sitter's
 * feet are; desks have the user on the -X side.
 */
namespace GRPropModels
{
	/** Size (x, y, z cm) overrides the default footprint of models that scale; Color overrides the main color. */
	bool Build(const FString& Model, const TArray<float>& Size, const FString& Color, uint32 Seed, TArray<FGRPropPart>& Out);
}
