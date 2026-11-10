#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GRRoomShells.generated.h"

class AGRTopDownRoomVolume;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;
class UProceduralMeshComponent;
class UStaticMeshComponent;

/** One room's roof: the slopes go see-through when seen from above, the rest of it goes away. */
USTRUCT()
struct FGRRoofParts
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UMeshComponent>> Slopes;

	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> Rest;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SeeThrough;

	/** 0 solid, 1 fully see-through. */
	float Fade = 0.f;
};

/**
 * Makes the walled rooms (rumah Bayu, warnet, kelurahan, dapur PPR) look like buildings from outside: a pitched roof,
 * windows, a lintel over the door and a name board. The roof turns see-through only while the room's top-down camera is
 * the view; from every other camera it stays solid. Built at runtime from the level's walls.
 */
UCLASS()
class GEMAHRIPAH_API AGRRoomShells : public AActor
{
	GENERATED_BODY()

public:
	AGRRoomShells();

	virtual void Tick(float DeltaSeconds) override;

	/** A solid triangular prism: Width wide across its base, Height tall at the ridge, Length long. BaseCenter is the middle of its floor. */
	static UProceduralMeshComponent* MakePrism(AActor* Owner, USceneComponent* Parent, const FVector& BaseCenter, const FVector& Along, const FVector& Across,
		float Length, float Width, float Height, UMaterialInterface* Material);

protected:
	virtual void BeginPlay() override;

private:
	/** What the level's walls say about a room. */
	struct FShell
	{
		float Height = 0.f;
		int32 Walls = 0;
		FVector Door = FVector::ZeroVector;
		UMaterialInterface* Material = nullptr;
	};

	FShell Inspect(const AGRTopDownRoomVolume& Room, const FVector& Half) const;
	void Build(AGRTopDownRoomVolume& Room);
	void Roof(USceneComponent* Group, const FVector& Center, const FVector& Half, const FShell& Shell);
	void Front(const FVector& Center, const FVector& Half, const FShell& Shell, const FVector& Out);
	void NameBoard(USceneComponent* Group, const FVector& Center, const FVector& Half, const FShell& Shell);
	UStaticMeshComponent* Part(USceneComponent* Parent, UMaterialInterface* Material, const FTransform& Transform, bool bShadow);

	UPROPERTY()
	TArray<TObjectPtr<AGRTopDownRoomVolume>> Rooms;

	/** Per room, in the order of Rooms. */
	UPROPERTY()
	TArray<FGRRoofParts> Roofs;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SeeThroughParent;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> RoofPaint;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> TrimPaint;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlassPaint;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BoardPaint;
};
