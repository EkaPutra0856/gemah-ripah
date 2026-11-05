#pragma once

#include "CoreMinimal.h"

class UStaticMesh;

/**
 * The Tripo prop meshes (Tools/Python/import_tripo_props.py): /Game/GemahRipah/Props/<Category>/SM_<Code>_<Name>.
 * Each stands on its pivot (bottom middle) at real size, its long side along X. Vehicles and buildings are left
 * out on purpose: vehicles still need their moving parts, buildings come from the owner's own pipeline.
 */
namespace GRTripoProps
{
	/** The mesh for a code such as "P18", or null when it is not imported. */
	UStaticMesh* Load(const FString& Code);

	/**
	 * The Tripo mesh standing in for a GRPropModels model ("kursi_plastik" -> P03), or an empty string when the
	 * model keeps its basic shapes. YawOffset turns the mesh to face the way the basic-shape model does.
	 */
	FString CodeForModel(const FString& Model, float& YawOffset);
}
