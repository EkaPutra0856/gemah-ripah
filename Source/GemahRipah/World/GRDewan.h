#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GRDewan.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;

/**
 * Turns the blockout box of the Gedung Dewan into a monument: podium, grand stairs, a colonnade with a pediment, rows of
 * tall windows and a gold dome on a drum, plus a gate in the fence and the baliho of the opening cutscene. Built at runtime
 * from engine shapes, so the real building can replace it by dropping this actor. One draw call per color and shape.
 */
UCLASS()
class GEMAHRIPAH_API AGRDewan : public AActor
{
	GENERATED_BODY()

public:
	AGRDewan();

protected:
	virtual void BeginPlay() override;

private:
	enum EPaint : uint8 { Wall, Stone, Shade, Base, Glass, Door, Gold, GoldDark, Iron, Flag, Board, PaintCount };
	enum EShape : uint8 { Cube, Cylinder, Sphere, ShapeCount };

	/** Finds the level's box and dome, repaints the box and hides the dome. False when the level has no Gedung Dewan. */
	bool ClaimBlockout();
	void Podium();
	void Portico();
	void Windows();
	void Dome();
	void Gate();
	void Baliho();
	/** One row of tall windows between pilasters on a wall: U runs along Along from First, Out points away from the wall. */
	void Bays(const FVector& Origin, const FVector& Out, const FVector& Along, float First, int32 Count);
	void Text(const FString& Words, const FVector& Spot, const FVector& Out, float Size, const FColor& Color);

	void Add(EPaint Paint, EShape Shape, const FVector& Center, const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator);
	void Flush();

	TArray<FTransform> Parts[PaintCount][ShapeCount];

	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Meshes;
};
