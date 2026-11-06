#include "World/GRPlusDressing.h"

#include "World/GRCrossings.h"
#include "World/GRTerrain.h"
#include "Story/GRStoryUtil.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LandscapeProxy.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"

const FName AGRPlusDressing::ReplacedTag(TEXT("GRPlusDiganti"));

namespace
{
	// Every pack mesh the dressing uses; Mesh() finds them by the name after "SM_".
	const TCHAR* const MeshPaths[] = {
		TEXT("/Game/BankBuilding/Meshes/SM_Tree1"),
		TEXT("/Game/BrutalistLevelKit/Models/Nature/Foliage/SM_Fol_Fern1"),
		TEXT("/Game/BrutalistLevelKit/Models/Nature/Foliage/SM_Fol_Grass_Set1"),
		TEXT("/Game/BrutalistLevelKit/Models/Nature/Foliage/SM_Fol_Grass_Set3"),
		TEXT("/Game/PA_UrbanCity/StaticMeshes/SM_BananaTree"),
		TEXT("/Game/PA_UrbanCity/StaticMeshes/SM_Cart_01"),
		TEXT("/Game/PA_UrbanCity/StaticMeshes/SM_ExteriorApartment_02"),
		TEXT("/Game/PA_UrbanCity/StaticMeshes/SM_ExteriorApartment_03"),
		TEXT("/Game/PA_UrbanCity/StaticMeshes/SM_ExteriorApartment_04"),
		TEXT("/Game/PA_UrbanCity/StaticMeshes/SM_Grass"),
		TEXT("/Game/PA_UrbanCity/StaticMeshes/SM_PaperBox_01"),
		TEXT("/Game/PA_UrbanCity/StaticMeshes/SM_PlasticBarrel_Close"),
		TEXT("/Game/PA_UrbanCity/StaticMeshes/SM_SpeedBump"),
		TEXT("/Game/PA_UrbanCity/StaticMeshes/SM_TrafficBarrels"),
		TEXT("/Game/PA_UrbanCity/StaticMeshes/SM_TrashBag_01"),
		TEXT("/Game/PA_UrbanCity/StaticMeshes/SM_TrashBag_03"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Bush01"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Bush02"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Bush03"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Bush08"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_PalmTree01"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Rock001"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Rock004"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_RocksScatter01"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Veg001"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Veg002"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Veg005"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Veg013"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Veg025"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Veg027"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Veg031"),
		TEXT("/Game/ProceduralBiomes/Meshes/SM_Veg033"),
		TEXT("/Game/Scene_Bazaar_Vol1/Assets/MS/3D/His_Baz_Container_Basket_Wicker_S_01/SM_His_Baz_Container_Basket_Wicker_S_01"),
		TEXT("/Game/Scene_Bazaar_Vol1/Assets/MS/3D/His_Baz_Container_Pot_Clay_S_01/SM_His_Baz_Container_Pot_Clay_S_01"),
		TEXT("/Game/Scene_Bazaar_Vol1/Assets/MS/3D/His_Baz_Sack_Jute_Closed_01/SM_His_Baz_Sack_Jute_Closed_01"),
		TEXT("/Game/Scene_Bazaar_Vol1/Assets/MS/3D/His_Baz_Sack_Jute_Open_01/SM_His_Baz_Sack_Jute_Open_01"),
		TEXT("/Game/Scene_Bazaar_Vol1/Assets/MS/3D/His_Baz_Sack_Jute_Stack_01/SM_His_Baz_Sack_Jute_Stack_01"),
		TEXT("/Game/Scene_Bazaar_Vol1/Assets/MS/3D/His_Baz_Stall_Frame_Wood_02/SM_His_Baz_Stall_Frame_Wood_02"),
		TEXT("/Game/Scene_Bazaar_Vol1/Assets/MS/3D/His_Baz_Stand_Wood_Worn_01/SM_His_Baz_Stand_Wood_Worn_01"),
		TEXT("/Game/Scene_Bazaar_Vol1/Assets/MS/3D/His_Baz_Storage_Crate_Wood_M_01/SM_His_Baz_Storage_Crate_Wood_M_01_A"),
		TEXT("/Game/Scene_Bazaar_Vol1/Assets/MS/3D/His_Baz_Tools_Sifter_Wicker_S_01/SM_His_Baz_Tools_Sifter_Wicker_S_01"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Ind_Con_Bag_Cement_Closed_01/SM_Ind_Con_Bag_Cement_Closed_01"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Ind_Con_Concrete_L_02/SM_Ind_Con_Jersey_Concrete_L_02"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Ind_Con_Pile_Gravel_Crushed_01/SM_Ind_Con_Pile_Gravel_Crushed_01"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Ind_Con_Wheelbarrow_Worn_01/SM_Ind_Con_Wheelbarrow_Worn_01"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Ind_Roa_Barrier_Concrete_M_01/SM_Ind_Roa_Barrier_Concrete_M_01"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Ind_Rub_Pile_Concrete_M_01/SM_Ind_Rub_Pile_Concrete_M_01"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Urb_Roa_Barrel_Plastic_Orange_01/SM_Urb_Roa_Barrel_Plastic_Orange_01"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Urb_Roa_Barricade_Metal_Worn_02/SM_Urb_Roa_Barricade_Metal_Worn_02"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Urb_Roa_Delineator_Plastic_01/SM_Urb_Roa_Delineator_Plastic_01"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Urb_Roa_Light_Plastic_Yellow_06/SM_Urb_Roa_Light_Plastic_Yellow_06"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Urb_Roa_Pipe_Concrete_M_03/SM_Urb_Roa_Pipe_Concrete_M_03"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Urb_Roa_Sheet_Metal_Rusty_01/SM_Urb_Roa_Sheet_Metal_Rusty_01"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Urb_Roa_Sign_Post_Plastic_Red_05/SM_Urb_Roa_Sign_Post_Plastic_Red_05"),
		TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/3D/Urb_Roa_TrafficCone_Plastic_Red_03/SM_Urb_Roa_Equipment_TrafficCone_Plastic_Red_03"),
		TEXT("/Game/Scene_Trash/Assets/MS/3D/Ind_Aba_Tarp_Wrinkled_01/SM_Ind_Aba_Tarp_Wrinkled_01"),
		TEXT("/Game/Scene_Trash/Assets/MS/3D/Ind_Tra_Bag_Plastic_Tied_01/SM_Ind_Tra_Bag_Plastic_Tied_01"),
		TEXT("/Game/Scene_Trash/Assets/MS/3D/Ind_Tra_Bags_Plastic_Pack_01/SM_Ind_Tra_Bags_Plastic_Pack_01"),
		TEXT("/Game/Scene_Trash/Assets/MS/3D/Ind_War_Storage_Pallet_Wood_Worn_01/SM_Ind_War_Storage_Pallet_Wood_Worn_01"),
		TEXT("/Game/Scene_Trash/Assets/MS/3D/Ind_War_Storage_Sheet_Cardboard_Flattened_01/SM_Ind_War_Storage_Sheet_Cardboard_Flattened_01"),
		TEXT("/Game/Scene_Trash/Assets/MS/3D/Urb_Str_Bin_Trash_Metal_Worn_02/SM_Urb_Str_Bin_Trash_Metal_Worn_02"),
		TEXT("/Game/Scene_Trash/Assets/MS/3D/Urb_Tra_Debris_Styrofoam_Dirty_01/SM_Urb_Tra_Debris_Styrofoam_Dirty_01"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Custom/Ind_Unf__Wall_03/SM_Ind_Unf_Wall_03"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/3D_Assets/Ind_Con_Beams_Ceiling_Concrete_Rough_01/SM_Ind_Con_Beams_Ceiling_Concrete_Rough_01"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/3D_Assets/Ind_Con_Pile_Rubble_Gravel_Patch_01/SM_Ind_Con_Pile_Rubble_Gravel_Patch_01"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/3D_Assets/Ind_Con_Pillar_Square_Concrete_Stained_01/SM_Ind_Con_Pillar_Square_Concrete_Stained_01"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/3D_Assets/Ind_Con_Stairs_Concrete_Worn_01/SM_Ind_Con_Stairs_Concrete_Worn_01"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/3D_Assets/Ind_Con_Storeage_Sack_Debris_M_01/SM_Ind_Con_Storeage_Sack_Debris_M_01"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/3D_Assets/Ind_Fac_Container_Drum_Metal_Worn_02/SM_Ind_Fac_Container_Drum_Metal_Worn_02"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/3D_Assets/Ind_Rub_Block_Cinder_Concrete_Chipped_02/SM_Ind_Rub_Block_Cinder_Concrete_Chipped_02"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/3D_Assets/Ind_Rub_Block_Cinder_Concrete_Chipped_03/SM_Ind_Rub_Block_Cinder_Concrete_Chipped_03"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/3D_Assets/Ind_Rub_Debris_Concrete_Broken_01/SM_Ind_Rub_Debris_Concrete_Broken_01"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/3D_Assets/Ind_Rub_Debris_Concrete_Broken_03/SM_Ind_Rub_Debris_Concrete_Broken_03"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/3D_Assets/Ind_Rub_Pile_Concrete_M_05/SM_Ind_Rub_Pile_Concrete_M_05"),
		TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/3D_Assets/Ind_Rub_Pile_Debris_Gravel_M_01/SM_Ind_Rub_Pile_Debris_Gravel_M_01"),
		TEXT("/Game/ScifiJungle/Models/Nature/Foliage/Jungle/SM_Jungle_BigLeaf_Bush1"),
		TEXT("/Game/ScifiJungle/Models/Nature/Foliage/Jungle/SM_Jungle_BroadLeaf2"),
		TEXT("/Game/ScifiJungle/Models/Nature/Foliage/Jungle/SM_Jungle_Bush1"),
		TEXT("/Game/ScifiJungle/Models/Nature/Foliage/Jungle/SM_Jungle_Bush2"),
		TEXT("/Game/ScifiJungle/Models/Nature/Foliage/Jungle/SM_Jungle_Grass1"),
		TEXT("/Game/ScifiJungle/Models/Nature/Foliage/Jungle/SM_Jungle_Grass2"),
		TEXT("/Game/ScifiJungle/Models/Nature/Foliage/Jungle/SM_Jungle_Waterlily1"),
		TEXT("/Game/ScifiJungle/Models/Nature/Rocks/SM_Rock_Boulder_1"),
		TEXT("/Game/Warehouse/Assets/Props/WM00217_Pallet/SM_WM00217-Pallet_Standard"),
		TEXT("/Game/Warehouse/Assets/Props/WM00223-Trash_Can/SM_WM00223-Trash_Can_General"),
	};

	const TCHAR* const PlusMaterials = TEXT("/Game/GemahRipah/Materials/Plus/");

	/** "/Game/A/B" -> "/Game/A/B.B", what LoadObject wants. */
	FString ObjectPath(const FString& Path)
	{
		return Path.Contains(TEXT(".")) ? Path : Path + TEXT(".") + FPaths::GetBaseFilename(Path);
	}

	FString RoadDecal(const TCHAR* Name)
	{
		return FString::Printf(TEXT("/Game/Scene_RoadsideConstruction/Assets/MS/Decals/%s/MI_%s"), Name, Name);
	}

	FString TrashDecal(const TCHAR* Folder, const TCHAR* Variant = TEXT(""))
	{
		return FString::Printf(TEXT("/Game/Scene_Trash/Assets/MS/Decals/%s/MI_%s%s"), Folder, Folder, Variant);
	}

	FString UnfinishedDecal(const TCHAR* Name)
	{
		return FString::Printf(TEXT("/Game/Scene_UnfinishedBuilding/Assets/Megascans/Decals/%s/MI_%s"), Name, Name);
	}

	FString MaterialName(const UStaticMeshComponent* Comp)
	{
		const UMaterialInterface* Material = Comp ? Comp->GetMaterial(0) : nullptr;
		return Material ? Material->GetName() : FString();
	}

	/** 0..99, the same every run for the same spot. */
	uint32 Roll(const FVector& P, uint32 Salt)
	{
		return HashCombine(HashCombine(GetTypeHash(FMath::RoundToInt32(P.X / 50.f)), GetTypeHash(FMath::RoundToInt32(P.Y / 50.f))), Salt) % 100;
	}

	/** How far an instanced mesh is drawn: grass and small bits only close by, bushes and props further, the rest always. */
	int32 CullEnd(const FString& Name)
	{
		static const TCHAR* const Near[] = { TEXT("Fol_"), TEXT("Jungle_Grass"), TEXT("Veg0"), TEXT("Grass"), TEXT("Rock00"),
			TEXT("RocksScatter"), TEXT("Ind_Rub_Pile_Debris"), TEXT("Ind_Rub_Debris"), TEXT("Ind_Rub_Block"), TEXT("Urb_Tra_Debris"),
			TEXT("Ind_War_Storage_Sheet"), TEXT("His_Baz_Container"), TEXT("His_Baz_Tools"), TEXT("Urb_Roa_Light") };
		static const TCHAR* const Mid[] = { TEXT("Bush"), TEXT("Jungle_Bush"), TEXT("Jungle_BigLeaf"),
			TEXT("TrashBag"), TEXT("Ind_Tra"), TEXT("Urb_Roa"), TEXT("Urb_Str"), TEXT("His_Baz"), TEXT("Ind_Con_Bag"), TEXT("WM00"),
			TEXT("PaperBox"), TEXT("Jungle_Waterlily"), TEXT("SpeedBump"), TEXT("PlasticBarrel"), TEXT("Cart_"), TEXT("Ind_Aba"),
			TEXT("Ind_War"), TEXT("Ind_Fac"), TEXT("TrafficBarrels") };
		for (const TCHAR* Prefix : Near)
		{
			if (Name.StartsWith(Prefix))
			{
				return 7000;
			}
		}
		for (const TCHAR* Prefix : Mid)
		{
			if (Name.StartsWith(Prefix))
			{
				return 20000;
			}
		}
		return 0;
	}

	const TCHAR* const GrassMeshes[] = { TEXT("Fol_Grass_Set1"), TEXT("Fol_Grass_Set3"), TEXT("Jungle_Grass1"), TEXT("Jungle_Grass2"),
		TEXT("Veg005"), TEXT("Veg013"), TEXT("Veg033"), TEXT("Grass") };
	const TCHAR* const FlowerMeshes[] = { TEXT("Veg031"), TEXT("Veg025"), TEXT("Veg027"), TEXT("Veg001"), TEXT("Veg002"), TEXT("Fol_Fern1") };
	const TCHAR* const BushMeshes[] = { TEXT("Bush01"), TEXT("Bush02"), TEXT("Bush03"), TEXT("Jungle_Bush1"), TEXT("Jungle_Bush2"),
		TEXT("Jungle_BigLeaf_Bush1"), TEXT("Bush08") };
	const TCHAR* const BagMeshes[] = { TEXT("TrashBag_01"), TEXT("TrashBag_03"), TEXT("Ind_Tra_Bag_Plastic_Tied_01"), TEXT("Ind_Tra_Bags_Plastic_Pack_01") };
	const TCHAR* const GoodsMeshes[] = { TEXT("His_Baz_Sack_Jute_Open_01"), TEXT("His_Baz_Container_Basket_Wicker_S_01"),
		TEXT("His_Baz_Storage_Crate_Wood_M_01_A"), TEXT("His_Baz_Tools_Sifter_Wicker_S_01"), TEXT("His_Baz_Sack_Jute_Closed_01"),
		TEXT("His_Baz_Container_Pot_Clay_S_01") };

	/**
	 * Pack meshes whose own materials do not render here (flat grey towers, checkerboard concrete): they take our
	 * world-aligned wall materials instead.
	 */
	struct FOverride
	{
		const TCHAR* Mesh;
		int32 Slot;
		const TCHAR* Material;
	};
	const FOverride Overrides[] = {
		{ TEXT("ExteriorApartment_02"), 0, TEXT("MI_Ruko1_Plus") },
		{ TEXT("ExteriorApartment_03"), 0, TEXT("MI_Rumah4_Plus") },
		{ TEXT("ExteriorApartment_04"), 0, TEXT("MI_Ruko2_Plus") },
		{ TEXT("Ind_Con_Pillar_Square_Concrete_Stained_01"), 0, TEXT("MI_Rumah5_Plus") },
		{ TEXT("Ind_Con_Beams_Ceiling_Concrete_Rough_01"), 0, TEXT("MI_Rumah5_Plus") },
		{ TEXT("Ind_Unf_Wall_03"), 0, TEXT("MI_Rumah5_Plus") },
		{ TEXT("Ind_Con_Stairs_Concrete_Worn_01"), 0, TEXT("MI_Rumah5_Plus") },
	};

	bool IsLand(const FHitResult& Hit)
	{
		return Hit.GetActor() && Hit.GetActor()->IsA<ALandscapeProxy>();
	}

	template <typename T, int32 N>
	const T& Any(const T (&List)[N], FRandomStream& Rng)
	{
		return List[Rng.RandRange(0, N - 1)];
	}
}

AGRPlusDressing::AGRPlusDressing()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
}

bool AGRPlusDressing::IsPlusMap(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World && World->GetMapName().Contains(TEXT("_Plus"));
}

void AGRPlusDressing::BeginPlay()
{
	Super::BeginPlay();
	SwapMaterials();
	ReplaceTowers();
	ReplaceTrees();
	Flush();
}

void AGRPlusDressing::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bBuilt)
	{
		return;
	}
	// Most of it stands on the landscape, whose collision streams in a little after BeginPlay (as in AGRTerrainDetails).
	WaitTime += DeltaSeconds;
	FHitResult Hit;
	const FVector2D Field = GRTerrain::Field().Center;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GRPlusWait), false, this);
	if (WaitTime > 15.f || (GetWorld()->LineTraceSingleByChannel(Hit, FVector(Field, 6000.f), FVector(Field, -2000.f), ECC_WorldStatic, Params)
		&& Hit.GetActor() && Hit.GetActor()->IsA<ALandscapeProxy>()))
	{
		bBuilt = true;
		SetActorTickEnabled(false);
		Build();
	}
}

UStaticMesh* AGRPlusDressing::Mesh(const TCHAR* Name)
{
	if (const TObjectPtr<UStaticMesh>* Found = Meshes.Find(Name))
	{
		return *Found;
	}
	UStaticMesh* Loaded = nullptr;
	const FString Suffix = FString(TEXT("/SM_")) + Name;
	for (const TCHAR* Path : MeshPaths)
	{
		if (FString(Path).EndsWith(Suffix))
		{
			Loaded = LoadObject<UStaticMesh>(nullptr, *ObjectPath(Path), nullptr, LOAD_NoWarn | LOAD_Quiet);
			break;
		}
	}
	if (!Loaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("GRPLUS mesh %s tidak ada"), Name);
	}
	Meshes.Add(Name, Loaded);
	return Loaded;
}

UMaterialInterface* AGRPlusDressing::Material(const FString& Path)
{
	if (const TObjectPtr<UMaterialInterface>* Found = Materials.Find(Path))
	{
		return *Found;
	}
	UMaterialInterface* Loaded = LoadObject<UMaterialInterface>(nullptr, *ObjectPath(Path), nullptr, LOAD_NoWarn | LOAD_Quiet);
	Materials.Add(Path, Loaded);
	return Loaded;
}

bool AGRPlusDressing::Ground(const FVector2D& P, FHitResult& Hit) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GRPlusGround), false, this);
	return GetWorld()->LineTraceSingleByChannel(Hit, FVector(P, 8000.f), FVector(P, -2000.f), ECC_WorldStatic, Params);
}

bool AGRPlusDressing::Land(const FVector2D& P, FVector& Out) const
{
	FHitResult Hit;
	if (!Ground(P, Hit) || !Hit.GetActor() || !Hit.GetActor()->IsA<ALandscapeProxy>())
	{
		return false;
	}
	Out = Hit.ImpactPoint;
	return true;
}

bool AGRPlusDressing::Road(const FVector2D& P, FVector& Out) const
{
	FHitResult Hit;
	if (!Ground(P, Hit) || !MaterialName(Cast<UStaticMeshComponent>(Hit.GetComponent())).StartsWith(TEXT("MI_Jalan")))
	{
		return false;
	}
	Out = Hit.ImpactPoint;
	return true;
}

bool AGRPlusDressing::IsBlocked(const FVector& Where, const FVector& HalfSize, float Yaw) const
{
	// From 20 cm up, so the ground, roads and pavements never count (as AGRDressing::IsBlocked).
	const float HalfZ = FMath::Max(HalfSize.Z - 10.f, 5.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GRPlusBlocked), false);
	return GetWorld()->OverlapBlockingTestByChannel(Where + FVector(0.f, 0.f, 20.f + HalfZ), FRotator(0.f, Yaw, 0.f).Quaternion(),
		ECC_Pawn, FCollisionShape::MakeBox(FVector(HalfSize.X, HalfSize.Y, HalfZ)), Params);
}

void AGRPlusDressing::Put(const TCHAR* Name, const FVector& Where, float Yaw, const FVector& Scale, bool bBlock, bool bAtPivot)
{
	UStaticMesh* Found = Mesh(Name);
	if (!Found)
	{
		return;
	}
	const FBox Box = Found->GetBoundingBox();
	const FRotator Turn(0.f, Yaw, 0.f);
	const FVector Foot = bAtPivot ? FVector::ZeroVector : FVector(Box.GetCenter().X, Box.GetCenter().Y, Box.Min.Z);
	Pending.FindOrAdd(Name).Add(FTransform(Turn, Where - Turn.RotateVector(Foot * Scale), Scale));
	if (bBlock)
	{
		const FVector Half = Box.GetExtent() * Scale * 0.9f;
		Blocker(Where + FVector(0.f, 0.f, Half.Z), Half, Yaw);
	}
}

float AGRPlusDressing::ScaleTo(const TCHAR* Name, float Size, bool bHeight)
{
	const UStaticMesh* Found = Mesh(Name);
	if (!Found)
	{
		return 1.f;
	}
	const FVector Extent = Found->GetBoundingBox().GetSize();
	return Size / FMath::Max(bHeight ? Extent.Z : FMath::Max(Extent.X, Extent.Y), 1.f);
}

void AGRPlusDressing::Blocker(const FVector& Center, const FVector& Half, float Yaw, bool bCamera)
{
	UBoxComponent* Block = NewObject<UBoxComponent>(this);
	Block->SetMobility(EComponentMobility::Static);
	Block->SetupAttachment(RootComponent);
	Block->SetBoxExtent(Half);
	Block->SetWorldLocationAndRotation(Center, FRotator(0.f, Yaw, 0.f));
	Block->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (!bCamera)
	{
		Block->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
	Block->CanCharacterStepUpOn = ECB_No;
	Block->SetCanEverAffectNavigation(false);
	Block->RegisterComponent();
	++BlockerCount;
}

void AGRPlusDressing::Decal(const FString& Path, const FVector& Where, const FVector2D& Size, float Yaw, float Depth)
{
	UMaterialInterface* DecalMaterial = Material(Path);
	if (!DecalMaterial)
	{
		return;
	}
	UDecalComponent* Comp = NewObject<UDecalComponent>(this);
	Comp->SetupAttachment(RootComponent);
	Comp->SetDecalMaterial(DecalMaterial);
	// Projected down its X; with pitch -90 its Z lies along Yaw and its Y across.
	Comp->DecalSize = FVector(Depth, Size.Y * 0.5f, Size.X * 0.5f);
	Comp->SetWorldLocationAndRotation(Where, FRotator(-90.f, Yaw, 0.f));
	Comp->SetFadeScreenSize(0.002f);
	Comp->RegisterComponent();
	++DecalCount;
}

void AGRPlusDressing::WallDecal(const FString& Path, const FVector& Where, const FVector& Normal, const FVector2D& Size)
{
	UMaterialInterface* DecalMaterial = Material(Path);
	if (!DecalMaterial)
	{
		return;
	}
	UDecalComponent* Comp = NewObject<UDecalComponent>(this);
	Comp->SetupAttachment(RootComponent);
	Comp->SetDecalMaterial(DecalMaterial);
	Comp->DecalSize = FVector(15.f, Size.X * 0.5f, Size.Y * 0.5f);
	Comp->SetWorldLocationAndRotation(Where, (-Normal).Rotation());
	Comp->SetFadeScreenSize(0.004f);
	Comp->RegisterComponent();
	++DecalCount;
}

void AGRPlusDressing::Flush()
{
	for (TPair<FString, TArray<FTransform>>& Pair : Pending)
	{
		if (Pair.Value.IsEmpty())
		{
			continue;
		}
		TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Comp = Instances.FindOrAdd(Pair.Key);
		if (!Comp)
		{
			Comp = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
			Comp->SetMobility(EComponentMobility::Static);
			Comp->SetupAttachment(RootComponent);
			Comp->SetStaticMesh(Mesh(*Pair.Key));
			for (const FOverride& Override : Overrides)
			{
				if (Pair.Key == Override.Mesh)
				{
					Comp->SetMaterial(Override.Slot, Material(FString(PlusMaterials) + Override.Material));
				}
			}
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Comp->SetCanEverAffectNavigation(false);
			const int32 Cull = CullEnd(Pair.Key);
			if (Cull > 0)
			{
				Comp->SetCullDistances(Cull * 3 / 4, Cull);
			}
			Comp->SetCastShadow(Cull == 0 || Cull > 7000);
			Comp->RegisterComponent();
		}
		Comp->AddInstances(Pair.Value, false, true);
		Pair.Value.Reset();
	}
}

// ---------------------------------------------------------------------------------------------------------------
// BeginPlay: before AGRFacades and AGRDressing look at the level.

void AGRPlusDressing::SwapMaterials()
{
	const UStaticMesh* Cube = GRStoryUtil::LoadShape(TEXT("cube"));
	int32 Swapped = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		UStaticMeshComponent* Comp = It->GetStaticMeshComponent();
		const FString Name = MaterialName(Comp);
		if (Name.IsEmpty() || Name.Contains(TEXT("_Plus")))
		{
			continue;
		}
		const FBox Bounds = Comp->Bounds.GetBox();
		const FVector Size = Bounds.GetSize();
		const uint32 Pick = Roll(Bounds.GetCenter(), 11);
		FString New = Name + TEXT("_Plus");
		if (Name == TEXT("MI_Jalan"))
		{
			// The kampung lanes are broken; the main roads and Jl. Lingkar (8 m) keep their asphalt, Lingkar with more damage decals.
			New = FMath::Min(Size.X, Size.Y) < 750.f ? TEXT("MI_Jalan_PlusRusak") : TEXT("MI_Jalan_Plus");
		}
		else if (Name == TEXT("MI_Trotoar") && (Comp->GetStaticMesh() != Cube || FMath::Min(Size.X, Size.Y) > 3000.f))
		{
			// The Tugu island and the Dewan plaza are paved, not tiled.
			New = TEXT("MI_Paving_Plus");
		}
		else if (Name.Len() == 9 && Name.StartsWith(TEXT("MI_Rumah")) && FChar::IsDigit(Name[8]))
		{
			// Some houses bare concrete, some bare brick.
			if (Pick < 30)
			{
				New = Pick < 18 ? TEXT("MI_Rumah5_Plus") : TEXT("MI_Rumah6_Plus");
			}
		}
		else if ((Name == TEXT("MI_Ruko1") || Name == TEXT("MI_Ruko2")) && Pick < 20)
		{
			New = TEXT("MI_Ruko3_Plus");
		}
		if (UMaterialInterface* Plus = Material(PlusMaterials + New))
		{
			Comp->SetMaterial(0, Plus);
			++Swapped;
		}
	}
	UE_LOG(LogTemp, Display, TEXT("GRPLUS %d material diganti"), Swapped);
}

void AGRPlusDressing::ReplaceTowers()
{
	// The 14 m Gedung blocks of the north-east and north-west quarters become apartment towers.
	const UStaticMesh* Cube = GRStoryUtil::LoadShape(TEXT("cube"));
	const float Scale = 1400.f / 2176.f * 0.98f;
	int32 Count = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		UStaticMeshComponent* Comp = It->GetStaticMeshComponent();
		const FString Name = MaterialName(Comp);
		if (!Comp || Comp->GetStaticMesh() != Cube || !(Name.StartsWith(TEXT("MI_Rumah")) || Name.StartsWith(TEXT("MI_Ruko"))))
		{
			continue;
		}
		const FBox Bounds = Comp->Bounds.GetBox();
		const FVector Size = Bounds.GetSize();
		const FVector Center = Bounds.GetCenter();
		if (FMath::Abs(Size.X - 1400.f) > 60.f || FMath::Abs(Size.Y - 1400.f) > 60.f || Size.Z < 590.f || Center.X < 3000.f || FMath::Abs(Center.Y) < 3000.f)
		{
			continue;
		}
		const uint32 Pick = Roll(Center, 23);
		const TCHAR* Tower = Size.Z < 750.f ? TEXT("ExteriorApartment_04") : (Pick % 2 ? TEXT("ExteriorApartment_02") : TEXT("ExteriorApartment_03"));
		const UStaticMesh* TowerMesh = Mesh(Tower);
		if (!TowerMesh)
		{
			continue;
		}
		const FVector Base(Center.X, Center.Y, Bounds.Min.Z);
		Put(Tower, Base, (Pick % 4) * 90.f, FVector(Scale));
		const float Height = TowerMesh->GetBoundingBox().GetSize().Z * Scale;
		Blocker(Base + FVector(0.f, 0.f, Height * 0.5f), FVector(690.f, 690.f, Height * 0.5f), 0.f, true);
		It->SetActorHiddenInGame(true);
		It->SetActorEnableCollision(false);
		It->Tags.Add(ReplacedTag);
		++Count;
	}
	UE_LOG(LogTemp, Display, TEXT("GRPLUS %d gedung jadi apartemen"), Count);
}

void AGRPlusDressing::ReplaceTrees()
{
	TArray<FVector> Spots;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		UStaticMeshComponent* Comp = It->GetStaticMeshComponent();
		const FString Name = MaterialName(Comp);
		const bool bTrunk = Name.StartsWith(TEXT("MI_Batang"));
		if (!bTrunk && !Name.StartsWith(TEXT("MI_Daun")))
		{
			continue;
		}
		if (bTrunk)
		{
			const FBox Bounds = Comp->Bounds.GetBox();
			Spots.Add(FVector(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z));
		}
		It->SetActorHiddenInGame(true);
		It->SetActorEnableCollision(false);
		It->Tags.Add(ReplacedTag);
	}
	FRandomStream Rng(808);
	for (const FVector& Spot : Spots)
	{
		const int32 Pick = Rng.RandRange(0, 99);
		const TCHAR* Tree = Pick < 45 ? TEXT("PalmTree01") : Pick < 75 ? TEXT("Jungle_BroadLeaf2") : TEXT("Tree1");
		const float Height = Rng.FRandRange(1050.f, 1400.f);
		Put(Tree, Spot - FVector(0.f, 0.f, 5.f), Rng.FRandRange(0.f, 360.f), FVector(ScaleTo(Tree, Height, true)), false, true);
		Blocker(Spot + FVector(0.f, 0.f, 200.f), FVector(30.f, 30.f, 200.f), 0.f);
	}
	UE_LOG(LogTemp, Display, TEXT("GRPLUS %d pohon diganti"), Spots.Num());
}

// ---------------------------------------------------------------------------------------------------------------
// Once the landscape is in.

void AGRPlusDressing::Build()
{
	FRandomStream Rng(2611);
	RoadMarkings();
	RoadDamage(Rng);
	WallStains(Rng);
	ProjectSite(Rng);
	RoadWorks();
	SpeedBumps();
	Market(Rng);
	Rubbish(Rng);
	Greenery(Rng);
	Flush();
	int32 Total = 0;
	for (const TPair<FString, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& Pair : Instances)
	{
		Total += Pair.Value->GetInstanceCount();
		UE_LOG(LogTemp, Display, TEXT("GRPLUS %s x%d"), *Pair.Key, Pair.Value->GetInstanceCount());
	}
	UE_LOG(LogTemp, Display, TEXT("GRPLUS selesai setelah %.1f detik: %d mesh, %d decal, %d penghalang"), WaitTime, Total, DecalCount, BlockerCount);
}

void AGRPlusDressing::RoadMarkings()
{
	const FString Zebra = RoadDecal(TEXT("Decal_Line_Road_Crossing_Painted_01"));
	// Centre line dashes, 3 m by 15 cm of white paint (the pack's line decals only show a sliver at road scale).
	UInstancedStaticMeshComponent* Paint = NewObject<UInstancedStaticMeshComponent>(this);
	Paint->SetMobility(EComponentMobility::Static);
	Paint->SetupAttachment(RootComponent);
	Paint->SetStaticMesh(GRStoryUtil::LoadShape(TEXT("cube")));
	Paint->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Paint->SetCanEverAffectNavigation(false);
	Paint->SetCastShadow(false);
	Paint->SetCullDistances(15000, 20000);
	Paint->RegisterComponent();
	GRStoryUtil::ApplyColor(Paint, FLinearColor(0.78f, 0.78f, 0.74f));
	TArray<FTransform> Dashes;
	auto Line = [&](const FVector& Where, float RoadYaw)
	{
		Dashes.Add(FTransform(FRotator(0.f, RoadYaw, 0.f), Where + FVector(0.f, 0.f, 0.6f), FVector(3.f, 0.15f, 0.01f)));
	};
	FVector At;
	for (int32 Axis = 0; Axis < 2; ++Axis)
	{
		const float Yaw = Axis == 0 ? 90.f : 0.f;
		// Zebra crossings where the townspeople cross (GRCrossings), four painted squares across the road.
		for (const float A : GRCrossings::Along)
		{
			for (const float Sign : { -1.f, 1.f })
			{
				for (float Across = -600.f; Across <= 600.f; Across += 400.f)
				{
					if (Road(GRCrossings::Point(Axis, Sign * A, Across), At))
					{
						Decal(Zebra, At, FVector2D(GRCrossings::HalfBand * 2.f, 400.f), Yaw);
					}
				}
			}
		}
		// Broken centre line, kept off the crossings and the roundabout.
		for (float Along = 3300.f; Along < 47000.f; Along += 900.f)
		{
			for (const float Sign : { -1.f, 1.f })
			{
				if (FMath::Abs(GRCrossings::Nearest(Sign * Along) - Sign * Along) < GRCrossings::HalfBand + 250.f)
				{
					continue;
				}
				if (Road(GRCrossings::Point(Axis, Sign * Along, 0.f), At))
				{
					Line(At, Yaw);
				}
			}
		}
	}
	// Jl. Lingkar: x = +-27000 along Y, y = +-27000 along X.
	for (int32 i = 0; i < 4; ++i)
	{
		const bool bAlongY = i < 2;
		const float Fixed = (i % 2 ? 1.f : -1.f) * 27000.f;
		for (float Along = -26000.f; Along <= 26000.f; Along += 1200.f)
		{
			if (Road(bAlongY ? FVector2D(Fixed, Along) : FVector2D(Along, Fixed), At))
			{
				Line(At, bAlongY ? 90.f : 0.f);
			}
		}
	}
	Paint->AddInstances(Dashes, false, true);
}

void AGRPlusDressing::RoadDamage(FRandomStream& Rng)
{
	struct FDamage
	{
		const TCHAR* Name;
		FVector2D Size;
		int32 Main;
		int32 Ring;
		bool bAligned;
	};
	const FDamage Kinds[] = {
		{ TEXT("Decal_Crack_Asphalt_01"), FVector2D(260.f, 260.f), 30, 50, false },
		{ TEXT("Decal_Crack_Asphalt_02"), FVector2D(260.f, 260.f), 30, 50, false },
		{ TEXT("Decal_Patch_Asphalt_01"), FVector2D(260.f, 180.f), 16, 40, true },
		{ TEXT("Decal_Patch_Asphalt_02"), FVector2D(260.f, 180.f), 16, 40, true },
		{ TEXT("Decal_Road_Dust_Tileable_01"), FVector2D(500.f, 500.f), 20, 30, false },
		{ TEXT("Decal_Debris_Road_Coarse_01"), FVector2D(200.f, 200.f), 16, 30, false },
	};
	FVector At;
	for (const FDamage& Kind : Kinds)
	{
		const FString Path = RoadDecal(Kind.Name);
		for (int32 n = 0; n < Kind.Main; ++n)
		{
			const int32 Axis = Rng.RandRange(0, 1);
			const float Along = Rng.FRandRange(3600.f, 46000.f) * (Rng.FRand() < 0.5f ? -1.f : 1.f);
			if (FMath::Abs(GRCrossings::Nearest(Along) - Along) < GRCrossings::HalfBand + 200.f)
			{
				continue;
			}
			if (Road(GRCrossings::Point(Axis, Along, Rng.FRandRange(-650.f, 650.f)), At))
			{
				Decal(Path, At, Kind.Size, Kind.bAligned ? (Axis == 0 ? 90.f : 0.f) : Rng.FRandRange(0.f, 360.f));
			}
		}
		for (int32 n = 0; n < Kind.Ring; ++n)
		{
			const int32 Side = Rng.RandRange(0, 3);
			const bool bAlongY = Side < 2;
			const float Fixed = (Side % 2 ? 1.f : -1.f) * 27000.f + Rng.FRandRange(-300.f, 300.f);
			const float Along = Rng.FRandRange(-26500.f, 26500.f);
			if (Road(bAlongY ? FVector2D(Fixed, Along) : FVector2D(Along, Fixed), At))
			{
				Decal(Path, At, Kind.Size, Kind.bAligned ? (bAlongY ? 90.f : 0.f) : Rng.FRandRange(0.f, 360.f));
			}
		}
	}
	// Manhole covers in the lanes and drain grates along the kerb of the main roads.
	const FString Manhole = RoadDecal(TEXT("Decal_Cover_Manhole_Square_01"));
	const FString RoundManhole = TrashDecal(TEXT("Decal_Cover_Manhole_Metal_Round_01"));
	const FString Drain = RoadDecal(TEXT("Decal_Cover_Drain_Metal_01"));
	for (int32 Axis = 0; Axis < 2; ++Axis)
	{
		const float Yaw = Axis == 0 ? 90.f : 0.f;
		for (float Along = 4200.f; Along < 46000.f; Along += 3000.f)
		{
			for (const float Sign : { -1.f, 1.f })
			{
				const float Lane = (FMath::RoundToInt32(Along / 3000.f) % 2 ? 1.f : -1.f) * 400.f;
				if (Road(GRCrossings::Point(Axis, Sign * Along, Lane), At))
				{
					Decal(Rng.FRand() < 0.5f ? Manhole : RoundManhole, At, FVector2D(90.f, 90.f), Yaw);
				}
				for (const float Kerb : { -740.f, 740.f })
				{
					if (Road(GRCrossings::Point(Axis, Sign * (Along + 1500.f), Kerb), At))
					{
						Decal(Drain, At, FVector2D(100.f, 50.f), Yaw);
					}
				}
			}
		}
	}
}

void AGRPlusDressing::WallStains(FRandomStream& Rng)
{
	// Rain streaks down from the roof edge on about a third of the houses and shops.
	const UStaticMesh* Cube = GRStoryUtil::LoadShape(TEXT("cube"));
	const FString Stains[] = { UnfinishedDecal(TEXT("Decal_Leak_Grime_01")), UnfinishedDecal(TEXT("Decal_Leak_Grime_02")),
		UnfinishedDecal(TEXT("Decal_Leak_Grime_03")), UnfinishedDecal(TEXT("Decal_Leak_Grime_04")), TrashDecal(TEXT("Decal_Leak_Dry_01"), TEXT("_A")) };
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		const UStaticMeshComponent* Comp = It->GetStaticMeshComponent();
		const FString Name = MaterialName(Comp);
		if (!Comp || Comp->GetStaticMesh() != Cube || It->IsHidden() || !(Name.StartsWith(TEXT("MI_Rumah")) || Name.StartsWith(TEXT("MI_Ruko"))))
		{
			continue;
		}
		const FBox Bounds = Comp->Bounds.GetBox();
		const FVector Half = Bounds.GetExtent();
		if (Half.Z < 150.f || Roll(Bounds.GetCenter(), 31) >= 35)
		{
			continue;
		}
		const int32 Face = Rng.RandRange(0, 3);
		const FVector Normal = Face == 0 ? FVector(1.f, 0.f, 0.f) : Face == 1 ? FVector(-1.f, 0.f, 0.f) : Face == 2 ? FVector(0.f, 1.f, 0.f) : FVector(0.f, -1.f, 0.f);
		const FVector Side(-Normal.Y, Normal.X, 0.f);
		const float Width = FMath::Abs(Side.X) > 0.5f ? Half.X : Half.Y;
		const FVector Where = Bounds.GetCenter() + Normal * (FMath::Abs(Normal.X) > 0.5f ? Half.X : Half.Y)
			+ Side * Rng.FRandRange(-0.6f, 0.6f) * Width + FVector(0.f, 0.f, Half.Z - 130.f);
		WallDecal(Stains[Rng.RandRange(0, UE_ARRAY_COUNT(Stains) - 1)], Where, Normal, FVector2D(Rng.FRandRange(140.f, 220.f), 260.f));
	}
}

void AGRPlusDressing::Greenery(FRandomStream& Rng)
{
	const GRTerrain::FArea& Field = GRTerrain::Field();
	auto Open = [&](const FVector2D& P, FVector& At)
	{
		return Land(P, At) && !GRTerrain::IsWater(P, 150.f) && !Field.Contains(P, 200.f);
	};
	FCollisionQueryParams Probe(SCENE_QUERY_STAT(GRPlusProbe), false, this);
	auto Clear = [&](const FVector& At, float Radius, float Up)
	{
		return !GetWorld()->OverlapAnyTestByChannel(At + FVector(0.f, 0.f, Up), FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(Radius), Probe);
	};
	FVector At;

	// Clumps of grass and wild flowers on the open ground.
	for (int32 c = 0; c < 2600; ++c)
	{
		const FVector2D Center(Rng.FRandRange(-40000.f, 40000.f), Rng.FRandRange(-40000.f, 40000.f));
		const bool bFlowers = Rng.FRand() < 0.2f;
		const int32 Count = Rng.RandRange(3, 7);
		for (int32 i = 0; i < Count; ++i)
		{
			const FVector2D P = Center + FVector2D(Rng.FRandRange(-220.f, 220.f), Rng.FRandRange(-220.f, 220.f));
			if (Open(P, At))
			{
				const TCHAR* Name = bFlowers && Rng.FRand() < 0.5f ? Any(FlowerMeshes, Rng) : Any(GrassMeshes, Rng);
				Put(Name, At - FVector(0.f, 0.f, 3.f), Rng.FRandRange(0.f, 360.f), FVector(Rng.FRandRange(0.8f, 1.3f)), false, true);
			}
		}
	}
	// Bushes, alone or in twos and threes.
	for (int32 c = 0; c < 900; ++c)
	{
		const FVector2D Center(Rng.FRandRange(-38000.f, 38000.f), Rng.FRandRange(-38000.f, 38000.f));
		const int32 Count = Rng.RandRange(1, 3);
		for (int32 i = 0; i < Count; ++i)
		{
			const FVector2D P = Center + FVector2D(Rng.FRandRange(-300.f, 300.f), Rng.FRandRange(-300.f, 300.f));
			if (Open(P, At) && Clear(At, 80.f, 160.f))
			{
				const TCHAR* Name = Any(BushMeshes, Rng);
				Put(Name, At - FVector(0.f, 0.f, 5.f), Rng.FRandRange(0.f, 360.f), FVector(ScaleTo(Name, Rng.FRandRange(130.f, 260.f))), false, true);
			}
		}
	}
	// Banana trees in the kampung yards and around the pasar.
	int32 Bananas = 0;
	for (int32 Try = 0; Try < 900 && Bananas < 220; ++Try)
	{
		const bool bKampung = Try % 4 != 0;
		const FVector2D P = bKampung ? FVector2D(Rng.FRandRange(-20000.f, -2200.f), Rng.FRandRange(-20000.f, -2200.f))
									 : FVector2D(Rng.FRandRange(-16500.f, -7500.f), Rng.FRandRange(7500.f, 18500.f));
		if (Open(P, At) && Clear(At, 140.f, 260.f))
		{
			Put(TEXT("BananaTree"), At - FVector(0.f, 0.f, 5.f), Rng.FRandRange(0.f, 360.f), FVector(ScaleTo(TEXT("BananaTree"), Rng.FRandRange(380.f, 520.f), true)), false, true);
			++Bananas;
		}
	}
	// More palms and shade trees in the open.
	int32 Trees = 0;
	for (int32 Try = 0; Try < 500 && Trees < 90; ++Try)
	{
		const FVector2D P(Rng.FRandRange(-30000.f, 30000.f), Rng.FRandRange(-30000.f, 30000.f));
		if (Open(P, At) && Clear(At, 350.f, 600.f))
		{
			const TCHAR* Tree = Rng.FRand() < 0.6f ? TEXT("PalmTree01") : TEXT("Jungle_BroadLeaf2");
			Put(Tree, At - FVector(0.f, 0.f, 10.f), Rng.FRandRange(0.f, 360.f), FVector(ScaleTo(Tree, Rng.FRandRange(1000.f, 1500.f), true)), false, true);
			Blocker(At + FVector(0.f, 0.f, 200.f), FVector(30.f, 30.f, 200.f), 0.f);
			++Trees;
		}
	}
	// Rock outcrops on the steep outer hillsides.
	int32 Outcrops = 0;
	for (int32 Try = 0; Try < 3000 && Outcrops < 160; ++Try)
	{
		const FVector2D P(Rng.FRandRange(-60000.f, 60000.f), Rng.FRandRange(-60000.f, 60000.f));
		if (FMath::Max(FMath::Abs(P.X), FMath::Abs(P.Y)) < 30000.f || !Open(P, At))
		{
			continue;
		}
		FVector A;
		FVector B;
		if (!Land(P + FVector2D(250.f, 0.f), A) || !Land(P + FVector2D(0.f, 250.f), B))
		{
			continue;
		}
		const float Slope = FMath::RadiansToDegrees(FMath::Atan(FMath::Sqrt(FMath::Square(A.Z - At.Z) + FMath::Square(B.Z - At.Z)) / 250.f));
		if (Slope < 20.f)
		{
			continue;
		}
		const float Size = Rng.FRandRange(250.f, 750.f);
		Put(TEXT("Rock_Boulder_1"), At - FVector(0.f, 0.f, Size * 0.3f), Rng.FRandRange(0.f, 360.f), FVector(ScaleTo(TEXT("Rock_Boulder_1"), Size)), false, true);
		for (int32 k = Rng.RandRange(1, 3); k > 0; --k)
		{
			const FVector2D Q = P + FVector2D(Rng.FRandRange(-Size, Size), Rng.FRandRange(-Size, Size));
			FVector Near;
			if (Land(Q, Near))
			{
				Put(TEXT("Rock_Boulder_1"), Near - FVector(0.f, 0.f, 20.f), Rng.FRandRange(0.f, 360.f), FVector(ScaleTo(TEXT("Rock_Boulder_1"), Rng.FRandRange(60.f, 200.f))), false, true);
			}
		}
		++Outcrops;
	}
	UE_LOG(LogTemp, Display, TEXT("GRPLUS %d singkapan batu di bukit"), Outcrops);

	// Stones on the river and kali banks, water lilies in the slow river.
	const TCHAR* const Stones[] = { TEXT("Rock001"), TEXT("Rock004"), TEXT("RocksScatter01"), TEXT("Rock_Boulder_1") };
	for (int32 Way = 0; Way < 2; ++Way)
	{
		const GRTerrain::FWaterway& Water = Way == 0 ? GRTerrain::River() : GRTerrain::Creek();
		for (int32 i = 0; i + 1 < Water.Points.Num(); i += 2)
		{
			const FVector2D Dir = (Water.Points[i + 1] - Water.Points[i]).GetSafeNormal();
			const FVector2D Side(-Dir.Y, Dir.X);
			for (const float Sign : { -1.f, 1.f })
			{
				const float Out = Water.HalfWidth + Rng.FRandRange(-40.f, FMath::Max(Water.BankOut - Water.HalfWidth, 150.f));
				const FVector2D P = Water.Points[i] + Side * Sign * Out;
				if (Rng.FRand() < 0.6f && Land(P, At))
				{
					const TCHAR* Stone = Any(Stones, Rng);
					const float Scale = Stone == Stones[3] ? ScaleTo(Stone, Rng.FRandRange(70.f, 180.f)) : Rng.FRandRange(0.8f, 2.2f);
					Put(Stone, At - FVector(0.f, 0.f, 8.f), Rng.FRandRange(0.f, 360.f), FVector(Scale), false, true);
				}
			}
			if (Way == 0 && i % 6 == 0 && Rng.FRand() < 0.35f)
			{
				const FVector2D Patch = Water.Points[i] + Side * Rng.FRandRange(-0.6f, 0.6f) * Water.HalfWidth;
				for (int32 k = Rng.RandRange(2, 5); k > 0; --k)
				{
					const FVector2D P = Patch + FVector2D(Rng.FRandRange(-120.f, 120.f), Rng.FRandRange(-120.f, 120.f));
					Put(TEXT("Jungle_Waterlily1"), FVector(P, GRTerrain::WaterZ), Rng.FRandRange(0.f, 360.f), FVector(Rng.FRandRange(1.5f, 2.5f)), false, true);
				}
			}
		}
	}
}

void AGRPlusDressing::ProjectSite(FRandomStream& Rng)
{
	// Inside AGRTerrainDetails' zinc fence: the concrete frame that was never finished, and what was left around it.
	const GRTerrain::FArea& Site = GRTerrain::Project();
	constexpr int32 Cols = 5;
	constexpr int32 Rows = 3;
	constexpr float Bay = 600.f;
	const FVector2D Origin = Site.Center - FVector2D((Cols - 1) * Bay * 0.5f, (Rows - 1) * Bay * 0.5f);
	FVector Feet[Cols][Rows];
	float Top = -1.e9f;
	FHitResult Hit;
	for (int32 i = 0; i < Cols; ++i)
	{
		for (int32 j = 0; j < Rows; ++j)
		{
			const FVector2D P = Origin + FVector2D(i * Bay, j * Bay);
			Feet[i][j] = Ground(P, Hit) ? Hit.ImpactPoint : FVector(P, 0.f);
			Top = FMath::Max(Top, Feet[i][j].Z);
		}
	}
	Top += 420.f;
	const TCHAR* Pillar = TEXT("Ind_Con_Pillar_Square_Concrete_Stained_01");
	const TCHAR* Beam = TEXT("Ind_Con_Beams_Ceiling_Concrete_Rough_01");
	const TCHAR* Wall = TEXT("Ind_Unf_Wall_03");
	for (int32 i = 0; i < Cols; ++i)
	{
		for (int32 j = 0; j < Rows; ++j)
		{
			const FVector& Foot = Feet[i][j];
			Put(Pillar, Foot - FVector(0.f, 0.f, 10.f), 0.f, FVector(1.f, 2.f, (Top - Foot.Z + 10.f) / 253.f), true);
			// Columns of the floor that never came, cut off at odd heights.
			if (Rng.FRand() < 0.5f)
			{
				Put(Pillar, FVector(Foot.X, Foot.Y, Top), 0.f, FVector(1.f, 2.f, Rng.FRandRange(0.3f, 0.9f)));
			}
			// Beams to the next column along X and along Y, some never cast.
			if (i + 1 < Cols && Rng.FRand() < 0.85f)
			{
				Put(Beam, FVector(Foot.X + Bay * 0.5f, Foot.Y, Top - 50.f), 0.f, FVector(Bay / 351.f, 1.6f, 1.4f));
			}
			if (j + 1 < Rows && Rng.FRand() < 0.85f)
			{
				Put(Beam, FVector(Foot.X, Foot.Y + Bay * 0.5f, Top - 50.f), 90.f, FVector(Bay / 351.f, 1.6f, 1.4f));
			}
		}
	}
	// Brick infill on the outer lines, some only half way up.
	for (int32 i = 0; i + 1 < Cols; ++i)
	{
		for (const int32 j : { 0, Rows - 1 })
		{
			if (Rng.FRand() < 0.55f)
			{
				const float Base = FMath::Min(Feet[i][j].Z, Feet[i + 1][j].Z);
				const float Height = (Top - 50.f - Base) * (Rng.FRand() < 0.4f ? Rng.FRandRange(0.35f, 0.7f) : 1.f);
				Put(Wall, FVector(Feet[i][j].X + Bay * 0.5f, Feet[i][j].Y, Base - 5.f), 0.f, FVector((Bay - 100.f) / 662.f, 1.f, Height / 440.f), true);
			}
		}
	}
	for (const int32 i : { 0, Cols - 1 })
	{
		const float Base = FMath::Min(Feet[i][0].Z, Feet[i][1].Z);
		Put(Wall, FVector(Feet[i][0].X, Feet[i][0].Y + Bay * 0.5f, Base - 5.f), 90.f, FVector((Bay - 100.f) / 662.f, 1.f, (Top - 50.f - Base) / 440.f), true);
	}
	// A flight of stairs at the east end, going nowhere.
	{
		const FVector2D P = Origin + FVector2D((Cols - 1) * Bay * 0.5f, (Rows - 1) * Bay + 250.f);
		if (Ground(P, Hit))
		{
			Put(TEXT("Ind_Con_Stairs_Concrete_Worn_01"), Hit.ImpactPoint, 90.f, FVector(1.f), true);
		}
	}

	// Heaps and leftovers around the frame, inside the fence.
	const FVector2D FrameHalf((Cols - 1) * Bay * 0.5f + 300.f, (Rows - 1) * Bay * 0.5f + 300.f);
	auto Spot = [&](FVector& At)
	{
		for (int32 Try = 0; Try < 30; ++Try)
		{
			const FVector2D P = Site.Center + FVector2D(Rng.FRandRange(-1.f, 1.f) * (Site.Half.X - 350.f), Rng.FRandRange(-1.f, 1.f) * (Site.Half.Y - 350.f));
			if (FMath::Abs(P.X - Site.Center.X) < FrameHalf.X && FMath::Abs(P.Y - Site.Center.Y) < FrameHalf.Y)
			{
				continue;
			}
			if (Ground(P, Hit) && !IsBlocked(Hit.ImpactPoint, FVector(120.f, 120.f, 60.f), 0.f))
			{
				At = Hit.ImpactPoint;
				return true;
			}
		}
		return false;
	};
	struct FHeap
	{
		const TCHAR* Name;
		int32 Count;
		bool bBlock;
	};
	const FHeap Heaps[] = {
		{ TEXT("Ind_Con_Pile_Gravel_Crushed_01"), 2, true },
		{ TEXT("Ind_Rub_Pile_Concrete_M_01"), 3, true },
		{ TEXT("Ind_Rub_Pile_Concrete_M_05"), 2, true },
		{ TEXT("Ind_Con_Pile_Rubble_Gravel_Patch_01"), 4, false },
		{ TEXT("Ind_Rub_Pile_Debris_Gravel_M_01"), 10, false },
		{ TEXT("Ind_Rub_Debris_Concrete_Broken_01"), 8, false },
		{ TEXT("Ind_Rub_Debris_Concrete_Broken_03"), 8, false },
		{ TEXT("Ind_Con_Storeage_Sack_Debris_M_01"), 4, true },
		{ TEXT("Ind_Fac_Container_Drum_Metal_Worn_02"), 3, true },
		{ TEXT("Ind_Con_Wheelbarrow_Worn_01"), 2, true },
		{ TEXT("Urb_Roa_Sheet_Metal_Rusty_01"), 3, false },
		{ TEXT("Ind_War_Storage_Pallet_Wood_Worn_01"), 4, false },
	};
	FVector At;
	for (const FHeap& Heap : Heaps)
	{
		for (int32 n = 0; n < Heap.Count; ++n)
		{
			if (Spot(At))
			{
				Put(Heap.Name, At - FVector(0.f, 0.f, 4.f), Rng.FRandRange(0.f, 360.f), FVector(Rng.FRandRange(0.9f, 1.15f)), Heap.bBlock);
			}
		}
	}
	// Cement bags on pallets, going hard in the rain.
	for (int32 n = 0; n < 3; ++n)
	{
		if (!Spot(At))
		{
			continue;
		}
		const float Yaw = Rng.FRandRange(0.f, 360.f);
		const FRotator Turn(0.f, Yaw, 0.f);
		Put(TEXT("Ind_War_Storage_Pallet_Wood_Worn_01"), At, Yaw, FVector(1.f), true);
		for (const float X : { -28.f, 28.f })
		{
			for (const float Y : { -19.f, 19.f })
			{
				Put(TEXT("Ind_Con_Bag_Cement_Closed_01"), At + Turn.RotateVector(FVector(X, Y, 13.f)), Yaw + Rng.FRandRange(-8.f, 8.f));
			}
		}
	}
	// Stacks of cinder blocks.
	for (int32 n = 0; n < 4; ++n)
	{
		if (!Spot(At))
		{
			continue;
		}
		const float Yaw = Rng.FRandRange(0.f, 360.f);
		const FRotator Turn(0.f, Yaw, 0.f);
		const int32 Courses = Rng.RandRange(2, 5);
		for (int32 Layer = 0; Layer < Courses; ++Layer)
		{
			for (int32 a = 0; a < 3; ++a)
			{
				for (int32 b = 0; b < 2; ++b)
				{
					Put(Layer % 2 ? TEXT("Ind_Rub_Block_Cinder_Concrete_Chipped_02") : TEXT("Ind_Rub_Block_Cinder_Concrete_Chipped_03"),
						At + Turn.RotateVector(FVector(a * 41.f - 41.f, b * 21.f - 10.5f, Layer * 20.f)), Yaw);
				}
			}
		}
		Blocker(At + FVector(0.f, 0.f, Courses * 10.f), FVector(62.f, 22.f, Courses * 10.f), Yaw);
	}
	// Concrete pipes waiting for a drain that was never dug.
	if (Spot(At))
	{
		const float Yaw = Rng.FRandRange(0.f, 360.f);
		const FRotator Turn(0.f, Yaw, 0.f);
		for (int32 k = 0; k < 3; ++k)
		{
			Put(TEXT("Urb_Roa_Pipe_Concrete_M_03"), At + Turn.RotateVector(FVector(0.f, k * 56.f - 56.f, 0.f)), Yaw, FVector(1.f), true);
		}
	}
	// Jersey barriers inside the gate on the north side.
	for (const float Y : { -1400.f, 200.f })
	{
		const FVector2D P = Site.Center + FVector2D(Site.Half.X - 200.f, Y);
		if (Ground(P, Hit))
		{
			Put(TEXT("Ind_Con_Jersey_Concrete_L_02"), Hit.ImpactPoint, 90.f, FVector(1.f), true);
		}
	}
}

void AGRPlusDressing::RoadWorks()
{
	// Jl. Lingkar east (x = 27000, along Y): the outer lane dug up between y 8000 and 10000.
	constexpr float Line = 27000.f;
	constexpr float From = 8000.f;
	constexpr float To = 10000.f;
	FHitResult Hit;
	auto At = [&](float X, float Y, FVector& Out)
	{
		if (!Ground(FVector2D(X, Y), Hit))
		{
			return false;
		}
		Out = Hit.ImpactPoint;
		return true;
	};
	FVector P;
	// Cones on the lane line, tapering in at both ends.
	int32 Step = 0;
	for (float Y = From - 500.f; Y <= To + 500.f; Y += 200.f, ++Step)
	{
		const float Taper = Y < From ? (From - Y) / 500.f : Y > To ? (Y - To) / 500.f : 0.f;
		if (At(Line + 30.f + Taper * 340.f, Y, P))
		{
			Put(Step % 3 == 0 ? TEXT("Urb_Roa_Delineator_Plastic_01") : TEXT("Urb_Roa_Equipment_TrafficCone_Plastic_Red_03"), P, Step * 37.f);
		}
	}
	// Barricades across the lane at both ends, drums with a blinker in front of them.
	for (const float Y : { From, To })
	{
		for (const float X : { Line + 130.f, Line + 290.f })
		{
			if (At(X, Y, P))
			{
				Put(TEXT("Urb_Roa_Barricade_Metal_Worn_02"), P, 90.f, FVector(1.f), true);
			}
		}
		const float Out = Y < 9000.f ? -250.f : 250.f;
		if (At(Line + 210.f, Y + Out, P))
		{
			Put(TEXT("TrafficBarrels"), P, 0.f, FVector(0.8f), true);
		}
	}
	for (float Y = From + 300.f; Y < To; Y += 500.f)
	{
		if (At(Line + 340.f, Y, P))
		{
			Put(TEXT("Urb_Roa_Barrel_Plastic_Orange_01"), P, Y, FVector(1.f), true);
			Put(TEXT("Urb_Roa_Light_Plastic_Yellow_06"), P + FVector(0.f, 0.f, 101.f), 90.f);
		}
	}
	// The sign before the works, facing the cars coming from the south.
	if (At(Line + 330.f, From - 900.f, P))
	{
		Put(TEXT("Urb_Roa_Sign_Post_Plastic_Red_05"), P, 0.f, FVector(1.f), true);
	}
	// The hole: patched asphalt, a heap of broken concrete and gravel.
	for (float Y = From + 250.f; Y < To - 100.f; Y += 420.f)
	{
		if (At(Line + 210.f, Y, P))
		{
			Decal(RoadDecal(Y < 9000.f ? TEXT("Decal_Patch_Asphalt_01") : TEXT("Decal_Patch_Asphalt_02")), P, FVector2D(380.f, 300.f), 90.f);
		}
	}
	if (At(Line + 220.f, 9050.f, P))
	{
		Put(TEXT("Ind_Rub_Pile_Concrete_M_01"), P - FVector(0.f, 0.f, 4.f), 0.f, FVector(0.75f), true);
	}
	if (At(Line + 220.f, 8500.f, P))
	{
		Put(TEXT("Ind_Con_Pile_Rubble_Gravel_Patch_01"), P - FVector(0.f, 0.f, 2.f), 90.f, FVector(0.9f));
	}
	if (At(Line + 230.f, 9550.f, P))
	{
		Put(TEXT("Ind_Con_Wheelbarrow_Worn_01"), P, 75.f, FVector(1.f), true);
	}
}

void AGRPlusDressing::SpeedBumps()
{
	// Polisi tidur across Jl. Merdeka (runs along X, 16 m wide), each made of 1.5 m pieces.
	FVector At;
	for (const float X : { -15500.f, -9500.f, 9500.f, 15500.f })
	{
		for (float Y = -800.f + 77.f; Y < 800.f; Y += 154.f)
		{
			if (Road(FVector2D(X, Y), At))
			{
				Put(TEXT("SpeedBump"), At - FVector(0.f, 0.f, 1.f), 0.f);
			}
		}
	}
}

void AGRPlusDressing::Market(FRandomStream& Rng)
{
	// Goods on every lapak table of Pasar Sentosa.
	int32 Tables = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		const UStaticMeshComponent* Comp = It->GetStaticMeshComponent();
		if (!MaterialName(Comp).StartsWith(TEXT("MI_Lapak")))
		{
			continue;
		}
		const FBox Bounds = Comp->Bounds.GetBox();
		const FVector Size = Bounds.GetSize();
		const bool bLongY = Size.Y > Size.X;
		const float Long = bLongY ? Size.Y : Size.X;
		const float Short = bLongY ? Size.X : Size.Y;
		for (int32 k = 0; k < 4; ++k)
		{
			const float U = (-0.36f + k * 0.24f) * Long;
			const float V = Rng.FRandRange(-0.18f, 0.18f) * Short;
			const FVector Where(Bounds.GetCenter().X + (bLongY ? V : U), Bounds.GetCenter().Y + (bLongY ? U : V), Bounds.Max.Z);
			const TCHAR* Goods = Any(GoodsMeshes, Rng);
			if (FCString::Strstr(Goods, TEXT("Pot_Clay")))
			{
				for (int32 p = 0; p < 3; ++p)
				{
					Put(Goods, Where + FVector(Rng.FRandRange(-25.f, 25.f), Rng.FRandRange(-25.f, 25.f), 0.f), Rng.FRandRange(0.f, 360.f));
				}
			}
			else
			{
				Put(Goods, Where, Rng.FRandRange(0.f, 360.f));
			}
		}
		++Tables;
	}
	// Stalls along the south and north edges of the pasar roof, a stand with goods under each.
	int32 Stalls = 0;
	FHitResult Hit;
	for (float X = -14400.f; X <= -9600.f; X += 800.f)
	{
		for (const float Y : { 8450.f, 17550.f })
		{
			if (!Ground(FVector2D(X, Y), Hit) || (!IsLand(Hit) && Hit.ImpactPoint.Z > 40.f) || IsBlocked(Hit.ImpactPoint, FVector(125.f, 60.f, 100.f), 0.f))
			{
				continue;
			}
			const FVector At = Hit.ImpactPoint;
			const float Yaw = Y < 13000.f ? 180.f : 0.f;
			Put(TEXT("His_Baz_Stall_Frame_Wood_02"), At, Yaw, FVector(1.f), true);
			Put(TEXT("His_Baz_Stand_Wood_Worn_01"), At, Yaw);
			for (const float Off : { -55.f, 0.f, 55.f })
			{
				Put(Any(GoodsMeshes, Rng), At + FVector(Off, Rng.FRandRange(-15.f, 15.f), 48.f), Rng.FRandRange(0.f, 360.f));
			}
			++Stalls;
		}
	}
	// Sacks, carts, blue drums and pallets on the floor between the tables.
	struct FFloor
	{
		const TCHAR* Name;
		int32 Count;
		FVector Half;
	};
	const FFloor Floor[] = {
		{ TEXT("His_Baz_Sack_Jute_Stack_01"), 10, FVector(45.f, 40.f, 40.f) },
		{ TEXT("Cart_01"), 3, FVector(80.f, 40.f, 55.f) },
		{ TEXT("PlasticBarrel_Close"), 5, FVector(35.f, 32.f, 48.f) },
		{ TEXT("WM00217-Pallet_Standard"), 4, FVector(62.f, 52.f, 10.f) },
	};
	for (const FFloor& Item : Floor)
	{
		for (int32 n = 0, Try = 0; n < Item.Count && Try < 60; ++Try)
		{
			const FVector2D P(Rng.FRandRange(-14600.f, -9400.f), Rng.FRandRange(9100.f, 16900.f));
			const float Yaw = Rng.FRandRange(0.f, 360.f);
			if (Ground(P, Hit) && (IsLand(Hit) || Hit.ImpactPoint.Z < 40.f) && !IsBlocked(Hit.ImpactPoint, Item.Half + FVector(20.f, 20.f, 0.f), Yaw))
			{
				Put(Item.Name, Hit.ImpactPoint, Yaw, FVector(1.f), true);
				++n;
			}
		}
	}
	UE_LOG(LogTemp, Display, TEXT("GRPLUS pasar: %d lapak diisi, %d lapak tambahan"), Tables, Stalls);
}

void AGRPlusDressing::Rubbish(FRandomStream& Rng)
{
	const FString Litter = TrashDecal(TEXT("Decal_Scatter_Garbage_Small_01"));
	const FString Garbage = TrashDecal(TEXT("Decal_Garbage_Tileable_01"));
	auto Heap = [&](const FVector& At)
	{
		Decal(Rng.FRand() < 0.6f ? Litter : Garbage, At, FVector2D(Rng.FRandRange(220.f, 340.f), Rng.FRandRange(220.f, 340.f)), Rng.FRandRange(0.f, 360.f));
		for (int32 k = Rng.RandRange(2, 5); k > 0; --k)
		{
			Put(Any(BagMeshes, Rng), At + FVector(Rng.FRandRange(-70.f, 70.f), Rng.FRandRange(-70.f, 70.f), -2.f), Rng.FRandRange(0.f, 360.f));
		}
		if (Rng.FRand() < 0.4f)
		{
			Put(Rng.FRand() < 0.5f ? TEXT("WM00223-Trash_Can_General") : TEXT("Urb_Str_Bin_Trash_Metal_Worn_02"),
				At + FVector(Rng.FRandRange(-110.f, 110.f), Rng.FRandRange(-110.f, 110.f), 0.f), Rng.FRandRange(0.f, 360.f), FVector(1.f), true);
		}
		if (Rng.FRand() < 0.4f)
		{
			Put(TEXT("Ind_War_Storage_Sheet_Cardboard_Flattened_01"), At + FVector(Rng.FRandRange(-120.f, 120.f), Rng.FRandRange(-120.f, 120.f), 0.f), Rng.FRandRange(0.f, 360.f));
		}
		if (Rng.FRand() < 0.25f)
		{
			Put(TEXT("PaperBox_01"), At + FVector(Rng.FRandRange(-100.f, 100.f), Rng.FRandRange(-100.f, 100.f), 0.f), Rng.FRandRange(0.f, 360.f));
		}
		for (int32 k = Rng.RandRange(0, 3); k > 0; --k)
		{
			Put(TEXT("Urb_Tra_Debris_Styrofoam_Dirty_01"), At + FVector(Rng.FRandRange(-160.f, 160.f), Rng.FRandRange(-160.f, 160.f), 0.f), Rng.FRandRange(0.f, 360.f));
		}
	};
	FHitResult Hit;
	auto Try = [&](const FVector2D& P)
	{
		if (!Ground(P, Hit) || (!IsLand(Hit) && FMath::Abs(Hit.ImpactPoint.Z) > 40.f) || GRTerrain::IsWater(P, 100.f)
			|| MaterialName(Cast<UStaticMeshComponent>(Hit.GetComponent())).StartsWith(TEXT("MI_Jalan"))
			|| IsBlocked(Hit.ImpactPoint, FVector(90.f, 90.f, 40.f), 0.f))
		{
			return false;
		}
		Heap(Hit.ImpactPoint);
		return true;
	};
	int32 Heaps = 0;
	// Behind the shop rows of both main roads.
	for (int32 n = 0; n < 90 && Heaps < 40; ++n)
	{
		const float Along = Rng.FRandRange(3500.f, 44000.f) * (Rng.FRand() < 0.5f ? -1.f : 1.f);
		const float Across = Rng.FRandRange(2600.f, 2900.f) * (Rng.FRand() < 0.5f ? -1.f : 1.f);
		Heaps += Try(GRCrossings::Point(n % 2, Along, Across)) ? 1 : 0;
	}
	// Around the pasar and in the kampung.
	for (int32 n = 0; n < 40; ++n)
	{
		Heaps += Try(FVector2D(Rng.FRandRange(-16000.f, -8000.f), Rng.FRandRange(8000.f, 18000.f))) ? 1 : 0;
	}
	for (int32 n = 0; n < 50; ++n)
	{
		Heaps += Try(FVector2D(Rng.FRandRange(-20000.f, -2200.f), Rng.FRandRange(-20000.f, -2200.f))) ? 1 : 0;
	}
	// A tarp blown into a corner here and there.
	for (int32 n = 0; n < 12; ++n)
	{
		const FVector2D P(Rng.FRandRange(-20000.f, -2200.f), Rng.FRandRange(-20000.f, 18000.f));
		if (Ground(P, Hit) && (IsLand(Hit) || FMath::Abs(Hit.ImpactPoint.Z) < 40.f) && !GRTerrain::IsWater(P, 100.f) && !IsBlocked(Hit.ImpactPoint, FVector(90.f, 60.f, 20.f), 0.f))
		{
			Put(TEXT("Ind_Aba_Tarp_Wrinkled_01"), Hit.ImpactPoint, Rng.FRandRange(0.f, 360.f));
		}
	}
	// Mud splashed over the broken lanes of the kampung.
	const FString Mud[] = { TrashDecal(TEXT("Decal_Splash_Mud_01"), TEXT("_A")), TrashDecal(TEXT("Decal_Splash_Mud_01"), TEXT("_B")) };
	FVector At;
	for (int32 n = 0; n < 160; ++n)
	{
		if (Road(FVector2D(Rng.FRandRange(-26000.f, -1500.f), Rng.FRandRange(-26000.f, -1500.f)), At))
		{
			Decal(Mud[n % 2], At, FVector2D(Rng.FRandRange(250.f, 420.f), Rng.FRandRange(200.f, 320.f)), Rng.FRandRange(0.f, 360.f));
		}
	}
	UE_LOG(LogTemp, Display, TEXT("GRPLUS %d tumpukan sampah"), Heaps);
}
