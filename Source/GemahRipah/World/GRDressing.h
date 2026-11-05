#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "GRDressing.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * Street furniture of L_KotaSentosa from the Tripo props: the blockout lamp posts and trees swapped for the real
 * ones, and power poles, bins, benches, bus stops, signs, billboards, fences, washing lines and market goods set
 * along the roads and in the districts. Built at runtime like AGRFacades, so it stays separate from the level.
 * Every spot is checked against what is already there (buildings, story props) before anything is put down.
 * Big pieces are instanced with a simple block for collision; small loose pieces (chairs, sacks, gallons, tyres,
 * bins) are single components the scooter can send flying (AGRCharacter::RideIntoThing).
 */
UCLASS()
class GEMAHRIPAH_API AGRDressing : public AActor
{
	GENERATED_BODY()

public:
	AGRDressing();

protected:
	virtual void BeginPlay() override;

private:
	/** Puts Code at Where turned to Yaw, unless something is in the way (bCheck). Returns whether it was placed. */
	bool Place(const TCHAR* Code, const FVector& Where, float Yaw, bool bCheck = true, float Scale = 1.f);
	/** Tries up to Tries random spots inside the rectangle Min..Max (ground level) for Count pieces of Code. */
	int32 Scatter(const TCHAR* Code, int32 Count, const FVector2D& Min, const FVector2D& Max, int32 Tries, FRandomStream& Rng);
	/** True when a box this size at Where (turned to Yaw) would overlap anything solid above the ground. */
	bool IsBlocked(const FVector& Where, const FVector& HalfSize, float Yaw) const;

	void SwapLampsAndTrees();
	void Roads(FRandomStream& Rng);
	void Districts(FRandomStream& Rng);

	UInstancedStaticMeshComponent* InstancesOf(const TCHAR* Code, UStaticMesh* Mesh);

	UPROPERTY()
	TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> Instances;
	int32 Placed = 0;
	/** Where the last Place() put its piece (GRPosters dresses billboards and banners from it). */
	FTransform LastPlaced;
};
