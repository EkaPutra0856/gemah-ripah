#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "GRPlusDressing.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * Dressing of L_KotaSentosa_Plus, the copy of Kota Sentosa built from the Epic/Fab packs in the project.
 * Spawned by AGRStoryDirector only in that map, before every other runtime builder:
 *  - BeginPlay: the blockout materials swap to the pack-textured ones in /Game/GemahRipah/Materials/Plus
 *    (Tools/Python/build_plus_materials.py), the 14 m Gedung blocks become apartment towers and the blockout
 *    trees become palms and broadleaf trees. Everything it replaces is hidden and tagged ReplacedTag, so
 *    AGRFacades and AGRDressing leave it alone.
 *  - Once the landscape collision is in: road paint and damage decals, grass, bushes, banana trees and river
 *    rocks, the unfinished building on the proyek mangkrak, road works on Jl. Lingkar, speed bumps, market
 *    goods and rubbish.
 * The original L_KotaSentosa does not use any of this.
 */
UCLASS()
class GEMAHRIPAH_API AGRPlusDressing : public AActor
{
	GENERATED_BODY()

public:
	AGRPlusDressing();

	/** True in L_KotaSentosa_Plus. */
	static bool IsPlusMap(const UObject* WorldContext);
	/** On level actors this dressing has hidden and replaced. */
	static const FName ReplacedTag;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void SwapMaterials();
	void ReplaceTowers();
	void ReplaceTrees();

	void Build();
	void RoadMarkings();
	void RoadDamage(FRandomStream& Rng);
	void WallStains(FRandomStream& Rng);
	void Greenery(FRandomStream& Rng);
	void ProjectSite(FRandomStream& Rng);
	void RoadWorks();
	void SpeedBumps();
	void Market(FRandomStream& Rng);
	void Rubbish(FRandomStream& Rng);
	/** Turns everything Put() collected into instanced meshes. */
	void Flush();

	UStaticMesh* Mesh(const TCHAR* Name);
	UMaterialInterface* Material(const FString& Path);
	/** Highest thing under P (not this actor's own pieces). */
	bool Ground(const FVector2D& P, FHitResult& Hit) const;
	/** Ground() that only counts the bare landscape. */
	bool Land(const FVector2D& P, FVector& Out) const;
	/** Ground() that only counts a road surface. */
	bool Road(const FVector2D& P, FVector& Out) const;
	bool IsBlocked(const FVector& Where, const FVector& HalfSize, float Yaw) const;
	/**
	 * Puts the mesh with the middle of its bottom at Where (plants: their pivot, roots may reach below it).
	 * Optionally a box blocks walking through it.
	 */
	void Put(const TCHAR* Name, const FVector& Where, float Yaw, const FVector& Scale = FVector(1.f), bool bBlock = false, bool bAtPivot = false);
	/** Scale that makes the mesh's larger footprint side (or its height) this many cm. */
	float ScaleTo(const TCHAR* Name, float Size, bool bHeight = false);
	void Blocker(const FVector& Center, const FVector& Half, float Yaw, bool bCamera = false);
	/** Decal on the ground: Size.X along Yaw, Size.Y across. */
	void Decal(const FString& Path, const FVector& Where, const FVector2D& Size, float Yaw, float Depth = 40.f);
	/** Decal on a wall facing Normal: Size.X wide, Size.Y tall. */
	void WallDecal(const FString& Path, const FVector& Where, const FVector& Normal, const FVector2D& Size);

	UPROPERTY()
	TMap<FString, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Instances;
	UPROPERTY()
	TMap<FString, TObjectPtr<UStaticMesh>> Meshes;
	UPROPERTY()
	TMap<FString, TObjectPtr<UMaterialInterface>> Materials;
	TMap<FString, TArray<FTransform>> Pending;
	bool bBuilt = false;
	float WaitTime = 0.f;
	int32 DecalCount = 0;
	int32 BlockerCount = 0;
};
