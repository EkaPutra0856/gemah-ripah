#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "GRFacades.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

/**
 * Placeholder facade details on the blockout buildings of L_KotaSentosa: doors, windows,
 * shop fronts with rolling doors, awnings and name boards, house roofs and water tanks.
 * Built at runtime from the level's building boxes (engine cube standing on the ground), so
 * the real buildings can replace the boxes later without touching the level script.
 * Every part is instanced without collision: one draw call per color.
 */
UCLASS()
class GEMAHRIPAH_API AGRFacades : public AActor
{
	GENERATED_BODY()

public:
	AGRFacades();

protected:
	virtual void BeginPlay() override;

private:
	enum EPaint : uint8
	{
		Wood, Trim, Glass, Sill, Shutter, Slat, Dark, Roof, RoofTin, Gable, Cap, Tank, Ac, Pot, Leaf,
		Awning0, Awning1, Awning2, Awning3,
		Sign0, Sign1, Sign2, Sign3, Sign4,
		Goods0, Goods1, Goods2, Goods3,
		PaintCount
	};

	/** One wall of a building: Origin is the bottom middle of the wall surface. */
	struct FFace
	{
		FVector Origin;
		FVector Out;
		FVector Along;
		float Width = 0.f;
	};

	void Decorate(const FVector& Center, const FVector& Half, const FString& Material);
	FFace MakeFace(const FVector& Center, const FVector& Half, const FVector& Out) const;
	void ShopFront(const FFace& Face, int32 Bays, FRandomStream& Rng);
	void Sign(const FFace& Face, float Z, float Width, FRandomStream& Rng);
	void Door(const FFace& Face, float U, float Width);
	void Window(const FFace& Face, float U, float Z, float W, float H);
	void WindowRows(const FFace& Face, float FirstZ, float Top, float DoorU = 0.f, float DoorHalf = 0.f);
	void PitchedRoof(const FVector& Center, const FVector& Half, const FFace& Front, FRandomStream& Rng);
	void FlatRoof(const FVector& Center, const FVector& Half, FRandomStream& Rng);
	/** A box on a wall: U along it, Z up, D out from it; Size is (depth, width, height). */
	void Box(EPaint Paint, const FFace& Face, float U, float Z, float D, const FVector& Size);
	void Add(EPaint Paint, const FTransform& Transform, bool bCylinder = false);
	/** Paper posters (GRPosters) near a corner of the side walls, up to MaxCount in a row. */
	void SidePosters(const FFace& Left, const FFace& Right, const FVector& Center, int32 MaxCount);

	TArray<FTransform> Parts[PaintCount][2];
	int32 SignCount = 0;
	int32 PosterCount = 0;

	UPROPERTY()
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Meshes;
};
