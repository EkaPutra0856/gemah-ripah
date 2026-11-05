#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GRTerrainDetails.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * Everything on the landscape that GRTerrain describes but the level does not hold: the water surface, the bridges
 * (concrete, the steel truss on Jl. Merdeka, the kampung suspension bridge and footbridges), the kali's concrete
 * walls and culvert, goals on the kampung field and the fence and sign of the abandoned project.
 * Built at runtime like GRFacades, so it follows GRTerrain without rebuilding the level.
 * It also brings Bayu back to the bank when he walks into deep water: he cannot swim.
 */
UCLASS()
class GEMAHRIPAH_API AGRTerrainDetails : public AActor
{
	GENERATED_BODY()

public:
	AGRTerrainDetails();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	enum EPaint : uint8
	{
		Concrete, Stone, Steel, Rail, Cable, Plank, White, Zinc, Rust, Board, Dark, Stream,
		PaintCount
	};

	void Build();
	void Water();
	void RoadDeck(const FVector& C, const FVector& F, const FVector& R, float Length, float Width, bool bTruss);
	void Truss(const FVector& C, const FVector& F, const FVector& R, float Length, float Width);
	void Suspension(const FVector& C, const FVector& F, const FVector& R, float Length, float Width);
	void Footbridge(const FVector& C, const FVector& F, const FVector& R, float Length, float Width);
	void Culvert(const FVector& C, const FVector& F, const FVector& R, float Width);
	void CreekWalls();
	void FieldGoals();
	void ProjectSite();
	void Trees();
	void AddText(const FString& Text, const FVector& Where, const FVector& Facing, float Size, const FColor& Color);

	/** A box Size cm (along X, Y, Z of the rotation) centred on Where. */
	void Box(EPaint Paint, const FVector& Where, const FRotator& Rotation, const FVector& Size);
	/** A square beam (or a round one) from A to B. */
	void Beam(EPaint Paint, const FVector& A, const FVector& B, float Thickness, bool bRound = false);
	float GroundZ(const FVector2D& P) const;

	TArray<FTransform> Parts[PaintCount][2];

	UPROPERTY()
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Meshes;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> WaterSurface;

	FVector LastDry = FVector::ZeroVector;
	float DryTimer = 0.f;
	float WetTime = 0.f;
	float LastWarning = -100.f;
	bool bBuilt = false;
	float WaitTime = 0.f;
};
