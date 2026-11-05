#pragma once

#include "CoreMinimal.h"

class AActor;
class UStaticMeshComponent;

/**
 * Printed pictures laid flat on things (Tools/Python/import_baliho.py: /Game/GemahRipah/Props/Baliho/MI_Poster_<Code>):
 * BL01..BL09 on both faces of the P07 billboards, SP01..SP06 on the P08 cloth banners, PS01..PS04 on walls.
 */
namespace GRPosters
{
	/**
	 * A flat picture attached to Owner's root. Center in world space, Out = the way it faces, Up = the picture's top
	 * (made square to Out), Width and Height in cm. Null when the picture is not imported.
	 */
	GEMAHRIPAH_API UStaticMeshComponent* Add(AActor* Owner, const FString& Image, const FVector& Center, const FVector& Out,
		const FVector& Up, float Width, float Height);

	/** Pictures on both faces of a placed P07 billboard or P08 banner; Transform is the prop's. Index picks them. */
	GEMAHRIPAH_API void DressProp(AActor* Owner, const FString& Code, const FTransform& Transform, int32 Index);

	/** One of the wall posters PS01..PS04, any index. */
	GEMAHRIPAH_API FString WallPoster(int32 Index);
}
