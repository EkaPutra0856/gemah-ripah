#include "World/GRDressing.h"

#include "World/GRPlusDressing.h"
#include "World/GRTerrain.h"
#include "LandscapeProxy.h"
#include "World/GRPosters.h"
#include "World/GRTripoProps.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"

namespace
{
	enum class EBlock : uint8
	{
		None,
		/** The whole bounding box. */
		Full,
		/** A post at the pivot, full height. */
		Post,
		/** A slab along the back (+Y side of the mesh): bus stop walls. */
		Back,
	};

	struct FPiece
	{
		const TCHAR* Code;
		/** Added to the yaw asked for, so that yaw is the way the piece's front looks (+X = 0). */
		float FrontYaw;
		/** A single movable component with its own collision: the scooter knocks it over. */
		bool bLoose;
		EBlock Block;
	};

	// The Tripo sheets were shot from the front, so the front of each piece looks down -Y.
	constexpr float FacingMinusY = 90.f;

	const FPiece Pieces[] = {
		{ TEXT("P01"), 0.f, false, EBlock::Post },
		{ TEXT("P02"), 0.f, true, EBlock::None },
		{ TEXT("P03"), 0.f, true, EBlock::None },
		{ TEXT("P04"), FacingMinusY, false, EBlock::Full },
		{ TEXT("P05"), 0.f, true, EBlock::None },
		{ TEXT("P06"), 0.f, true, EBlock::None },
		{ TEXT("P07"), FacingMinusY, false, EBlock::Full },
		{ TEXT("P08"), FacingMinusY, false, EBlock::Full },
		{ TEXT("P09"), FacingMinusY, false, EBlock::Full },
		{ TEXT("P10"), FacingMinusY, false, EBlock::Full },
		{ TEXT("P11"), FacingMinusY, false, EBlock::Full },
		{ TEXT("P12"), 0.f, true, EBlock::None },
		{ TEXT("P18"), 0.f, false, EBlock::None },
		{ TEXT("P19"), 0.f, false, EBlock::None },
		{ TEXT("P20"), FacingMinusY, false, EBlock::Full },
		{ TEXT("P21"), FacingMinusY, false, EBlock::Back },
		{ TEXT("P22"), FacingMinusY, false, EBlock::Full },
		{ TEXT("P24"), FacingMinusY, false, EBlock::Full },
		{ TEXT("R20"), 0.f, true, EBlock::None },
	};

	const FPiece* FindPiece(const TCHAR* Code)
	{
		for (const FPiece& Piece : Pieces)
		{
			if (FCString::Strcmp(Piece.Code, Code) == 0)
			{
				return &Piece;
			}
		}
		return nullptr;
	}

	FString MaterialOf(const UStaticMeshComponent* Comp)
	{
		const UMaterialInterface* Material = Comp ? Comp->GetMaterial(0) : nullptr;
		return Material ? Material->GetName() : FString();
	}

	// Main roads: Jl. Raya runs along Y at X = 0, Jl. Merdeka along X at Y = 0; both 16 m wide with a 4 m
	// pavement each side (Z top 12) out to 12 m, the roundabout in the middle (12 m radius).
	constexpr float PavementMid = 1000.f;
	constexpr float PavementOuter = 1200.f;
	constexpr float PavementTop = 12.f;
}

AGRDressing::AGRDressing()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
	// Its small pieces can be knocked over by the scooter.
	Tags.Add(TEXT("GRLepas"));
}

void AGRDressing::BeginPlay()
{
	Super::BeginPlay();
	FRandomStream Rng(5150);
	SwapLampsAndTrees();
	Roads(Rng);
	Districts(Rng);
	UE_LOG(LogTemp, Display, TEXT("GRDressing: %d pieces"), Placed);
	for (const TPair<FString, TObjectPtr<UInstancedStaticMeshComponent>>& Pair : Instances)
	{
		UE_LOG(LogTemp, Display, TEXT("GRDressing: %s x%d"), *Pair.Key, Pair.Value->GetInstanceCount());
	}
}

UInstancedStaticMeshComponent* AGRDressing::InstancesOf(const TCHAR* Code, UStaticMesh* Mesh)
{
	TObjectPtr<UInstancedStaticMeshComponent>& Found = Instances.FindOrAdd(Code);
	if (!Found)
	{
		Found = NewObject<UInstancedStaticMeshComponent>(this);
		Found->SetMobility(EComponentMobility::Static);
		Found->SetupAttachment(RootComponent);
		Found->SetStaticMesh(Mesh);
		Found->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Found->SetCanEverAffectNavigation(false);
		Found->RegisterComponent();
	}
	return Found;
}

bool AGRDressing::IsBlocked(const FVector& Where, const FVector& HalfSize, float Yaw) const
{
	// From 20 cm up, so the ground, roads and pavements never count.
	const float HalfZ = FMath::Max(HalfSize.Z - 10.f, 5.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GRDressing), false);
	return GetWorld()->OverlapBlockingTestByChannel(Where + FVector(0.f, 0.f, 20.f + HalfZ), FRotator(0.f, Yaw, 0.f).Quaternion(),
		ECC_Pawn, FCollisionShape::MakeBox(FVector(HalfSize.X, HalfSize.Y, HalfZ)), Params);
}

bool AGRDressing::Place(const TCHAR* Code, const FVector& Where, float Yaw, bool bCheck, float Scale)
{
	const FPiece* Piece = FindPiece(Code);
	UStaticMesh* Mesh = GRTripoProps::Load(Code);
	if (!Piece || !Mesh)
	{
		return false;
	}
	const FBox Box = Mesh->GetBoundingBox();
	const FRotator Turn(0.f, Yaw + Piece->FrontYaw, 0.f);
	const FVector Middle = Turn.RotateVector(Box.GetCenter() * Scale);
	const FVector Base(Where.X + Middle.X, Where.Y + Middle.Y, Where.Z);
	if (bCheck && IsBlocked(Base, Box.GetExtent() * Scale * 0.9f, Turn.Yaw))
	{
		return false;
	}
	const FTransform Transform(Turn, Where - FVector(0.f, 0.f, Box.Min.Z * Scale), FVector(Scale));
	LastPlaced = Transform;
	if (Piece->bLoose)
	{
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetupAttachment(RootComponent);
		Comp->SetStaticMesh(Mesh);
		Comp->SetWorldTransform(Transform);
		Comp->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		Comp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Comp->CanCharacterStepUpOn = ECB_No;
		Comp->SetCanEverAffectNavigation(false);
		Comp->RegisterComponent();
	}
	else
	{
		InstancesOf(Code, Mesh)->AddInstance(Transform, true);
		FVector BlockCenter = Box.GetCenter();
		FVector BlockHalf = Box.GetExtent();
		if (Piece->Block == EBlock::Post)
		{
			BlockCenter = FVector(0.f, 0.f, Box.GetCenter().Z);
			BlockHalf = FVector(15.f, 15.f, BlockHalf.Z);
		}
		else if (Piece->Block == EBlock::Back)
		{
			BlockCenter.Y = Box.Max.Y - Box.GetSize().Y * 0.1f;
			BlockHalf.Y = Box.GetSize().Y * 0.1f;
		}
		if (Piece->Block != EBlock::None)
		{
			UBoxComponent* Block = NewObject<UBoxComponent>(this);
			Block->SetMobility(EComponentMobility::Static);
			Block->SetupAttachment(RootComponent);
			Block->SetBoxExtent(BlockHalf * Scale);
			Block->SetWorldLocationAndRotation(Transform.TransformPosition(BlockCenter), Turn);
			Block->SetCollisionProfileName(TEXT("BlockAllDynamic"));
			Block->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			Block->CanCharacterStepUpOn = ECB_No;
			Block->SetCanEverAffectNavigation(false);
			Block->RegisterComponent();
		}
	}
	++Placed;
	return true;
}

int32 AGRDressing::Scatter(const TCHAR* Code, int32 Count, const FVector2D& Min, const FVector2D& Max, int32 Tries, FRandomStream& Rng)
{
	int32 Done = 0;
	for (int32 Try = 0; Try < Tries && Done < Count; ++Try)
	{
		const FVector Top(Rng.FRandRange(Min.X, Max.X), Rng.FRandRange(Min.Y, Max.Y), 1500.f);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GRDressingGround), false);
		if (!GetWorld()->LineTraceSingleByChannel(Hit, Top, Top - FVector(0.f, 0.f, 2100.f), ECC_Visibility, Params))
		{
			continue;
		}
		// Only on open ground or pavement: not on a road, a roof, a floor inside or anything standing.
		const UStaticMeshComponent* Under = Cast<UStaticMeshComponent>(Hit.GetComponent());
		const FString Material = MaterialOf(Under);
		// The landscape rolls up and down; anything else above 20 cm is a roof or a table. Not in the kali or the river.
		const bool bLand = Hit.GetActor() && Hit.GetActor()->IsA<ALandscapeProxy>();
		if ((!bLand && (Hit.ImpactPoint.Z > 20.f || Hit.ImpactPoint.Z < -40.f)) || GRTerrain::IsWater(FVector2D(Top), 150.f)
			|| Material.StartsWith(TEXT("MI_Jalan")) || Material.StartsWith(TEXT("MI_Lantai")))
		{
			continue;
		}
		if (Place(Code, FVector(Top.X, Top.Y, Hit.ImpactPoint.Z), Rng.FRandRange(0.f, 360.f), true, Rng.FRandRange(0.92f, 1.08f)))
		{
			++Done;
		}
	}
	return Done;
}

void AGRDressing::SwapLampsAndTrees()
{
	UStaticMesh* Lamp = GRTripoProps::Load(TEXT("P18"));
	UStaticMesh* Tree = GRTripoProps::Load(TEXT("P19"));
	TArray<FVector> LampSpots;
	TArray<FVector> TreeSpots;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		UStaticMeshComponent* Comp = It->GetStaticMeshComponent();
		if (It->Tags.Contains(AGRPlusDressing::ReplacedTag))
		{
			continue;
		}
		const FString Material = MaterialOf(Comp);
		const FBox Bounds = Comp ? Comp->Bounds.GetBox() : FBox(ForceInit);
		const FVector Size = Bounds.GetSize();
		const bool bLampPost = Lamp && Material.StartsWith(TEXT("MI_Tiang")) && Size.Z > 450.f && Size.Z < 550.f && Size.X < 40.f;
		const bool bTrunk = Tree && Material.StartsWith(TEXT("MI_Batang"));
		const bool bLeaves = Tree && Material.StartsWith(TEXT("MI_Daun"));
		if (!bLampPost && !bTrunk && !bLeaves)
		{
			continue;
		}
		(bLampPost ? LampSpots : TreeSpots).Add(FVector(Bounds.GetCenter().X, Bounds.GetCenter().Y, 0.f));
		if (bLeaves)
		{
			TreeSpots.Pop();
		}
		It->SetActorHiddenInGame(true);
		It->SetActorEnableCollision(false);
	}

	UE_LOG(LogTemp, Display, TEXT("GRDressing: %d lamp posts, %d trees in the level"), LampSpots.Num(), TreeSpots.Num());
	if (Lamp)
	{
		// The lamp's pole stands at its back end (-X), the arm reaches out along +X to the head.
		const FBox Box = Lamp->GetBoundingBox();
		const float PoleX = Box.Min.X + Box.GetSize().X * 0.18f;
		TArray<APointLight*> Lights;
		for (TActorIterator<APointLight> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TEXT("LampuJalan")))
			{
				Lights.Add(*It);
			}
		}
		for (const FVector& Spot : LampSpots)
		{
			// Arm out over the road it stands beside.
			const FVector Out = FMath::Abs(Spot.X) < FMath::Abs(Spot.Y) ? FVector(-FMath::Sign(Spot.X), 0.f, 0.f) : FVector(0.f, -FMath::Sign(Spot.Y), 0.f);
			const FRotator Turn = Out.Rotation();
			const FVector Pivot = Spot - Turn.RotateVector(FVector(PoleX, 0.f, 0.f));
			InstancesOf(TEXT("P18"), Lamp)->AddInstance(FTransform(Turn, Pivot - FVector(0.f, 0.f, Box.Min.Z)), true);
			UBoxComponent* Block = NewObject<UBoxComponent>(this);
			Block->SetMobility(EComponentMobility::Static);
			Block->SetupAttachment(RootComponent);
			Block->SetBoxExtent(FVector(18.f, 18.f, 300.f));
			Block->SetWorldLocation(Spot + FVector(0.f, 0.f, 300.f));
			Block->SetCollisionProfileName(TEXT("BlockAllDynamic"));
			Block->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			Block->SetCanEverAffectNavigation(false);
			Block->RegisterComponent();
			++Placed;
			// The street light moves up to the lamp head.
			const FVector Head = Pivot + Turn.RotateVector(FVector(Box.Max.X - 25.f, 0.f, 0.f)) + FVector(0.f, 0.f, Box.GetSize().Z - 40.f);
			for (APointLight* Light : Lights)
			{
				if (FVector::Dist2D(Light->GetActorLocation(), Spot) < 100.f)
				{
					Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
					Light->SetActorLocation(Head);
				}
			}
		}
	}
	if (Tree)
	{
		FRandomStream Rng(77);
		for (const FVector& Spot : TreeSpots)
		{
			const float Scale = Rng.FRandRange(0.85f, 1.2f);
			InstancesOf(TEXT("P19"), Tree)->AddInstance(FTransform(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f),
				Spot - FVector(0.f, 0.f, Tree->GetBoundingBox().Min.Z * Scale), FVector(Scale)), true);
			UBoxComponent* Block = NewObject<UBoxComponent>(this);
			Block->SetMobility(EComponentMobility::Static);
			Block->SetupAttachment(RootComponent);
			Block->SetBoxExtent(FVector(30.f, 30.f, 200.f));
			Block->SetWorldLocation(Spot + FVector(0.f, 0.f, 200.f));
			Block->SetCollisionProfileName(TEXT("BlockAllDynamic"));
			Block->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			Block->SetCanEverAffectNavigation(false);
			Block->RegisterComponent();
			++Placed;
		}
	}
}

void AGRDressing::Roads(FRandomStream& Rng)
{
	int32 Billboards = 0;
	// Along both main roads, both sides. Raya: X across, Y along; Merdeka the other way round.
	for (int32 Road = 0; Road < 2; ++Road)
	{
		auto At = [Road](float Across, float Along, float Z)
		{
			return Road == 0 ? FVector(Across, Along, Z) : FVector(Along, Across, Z);
		};
		// Yaw that looks across the road from the side Across is on.
		auto FaceRoad = [Road](float Across)
		{
			return Road == 0 ? (Across > 0.f ? 180.f : 0.f) : (Across > 0.f ? -90.f : 90.f);
		};
		const float AlongYaw = Road == 0 ? 90.f : 0.f;
		for (float Along = -46000.f; Along <= 46000.f; Along += 2000.f)
		{
			if (FMath::Abs(Along) < 3500.f)
			{
				continue;
			}
			// Counted from the far end, so the pattern is the same on both sides of the roundabout.
			const int32 Step = FMath::RoundToInt((Along + 48000.f) / 2000.f);
			// Bins at the outer edge of the pavement, swapping sides.
			if (Step % 3 == 0)
			{
				const float Side = (Step / 3) % 2 == 0 ? PavementOuter - 70.f : -(PavementOuter - 70.f);
				Place(TEXT("P02"), At(Side, Along + 300.f, PavementTop), Rng.FRandRange(0.f, 360.f));
			}
			// Benches facing the road.
			if (Step % 4 == 1)
			{
				const float Side = (Step / 4) % 2 == 0 ? PavementOuter - 55.f : -(PavementOuter - 55.f);
				Place(TEXT("P20"), At(Side, Along, PavementTop), FaceRoad(Side));
			}
		}
		// Bus stops on the pavement.
		for (const float Along : { -30000.f, -14000.f, 14000.f, 26000.f })
		{
			const float Side = Along > 0.f ? -(PavementOuter - 90.f) : PavementOuter - 90.f;
			for (const float Shift : { 0.f, 800.f, -800.f, 1600.f })
			{
				if (Place(TEXT("P21"), At(Side, Along + Shift, PavementTop), FaceRoad(Side)))
				{
					break;
				}
			}
		}
		// Billboards just behind the pavement, a printed picture on each face.
		for (const float Along : { -36000.f, -21000.f, 9000.f, 22000.f, 38000.f })
		{
			const float Side = Along > 0.f ? 1800.f : -1800.f;
			for (const float Shift : { 0.f, 1200.f, -1200.f, 2400.f, -2400.f })
			{
				if (Place(TEXT("P07"), At(Side, Along + Shift, 0.f), FaceRoad(Side)))
				{
					GRPosters::DressProp(this, TEXT("P07"), LastPlaced, Billboards++);
					break;
				}
			}
		}
		// Traffic signs where the road meets the roundabout.
		for (const float Along : { -3200.f, 3200.f })
		{
			const float Side = Along > 0.f ? -(PavementOuter - 60.f) : PavementOuter - 60.f;
			Place(TEXT("P22"), At(Side, Along, PavementTop), Road == 0 ? (Along > 0.f ? 90.f : -90.f) : (Along > 0.f ? 0.f : 180.f));
		}
		// Tyre stacks outside the shops along the road (repair stalls).
		for (int32 i = 0; i < 4; ++i)
		{
			const float Side = i % 2 == 0 ? 1500.f : -1500.f;
			const float Along = Rng.FRandRange(-44000.f, 44000.f);
			if (FMath::Abs(Along) > 4000.f)
			{
				Place(TEXT("P12"), At(Side, Along, 0.f), Rng.FRandRange(0.f, 360.f));
			}
		}
	}

	// Banners outside the Dewan fence, where Bab 2 gathers, and a few burnt tyres.
	int32 Banner = 0;
	for (const float Y : { -3600.f, -1500.f, 1500.f, 3600.f })
	{
		if (Place(TEXT("P08"), FVector(36450.f, Y, 0.f), 180.f))
		{
			GRPosters::DressProp(this, TEXT("P08"), LastPlaced, Banner++);
		}
	}
	Place(TEXT("P12"), FVector(35700.f, -2600.f, 0.f), 30.f);
	Place(TEXT("P12"), FVector(35800.f, 2700.f, 0.f), 200.f);
}

void AGRDressing::Districts(FRandomStream& Rng)
{
	// Kampung Gang around Bayu's house: washing lines, zinc fences, bins, chairs and gallons out front.
	const FVector2D KampungMin(-20000.f, -20000.f);
	const FVector2D KampungMax(-2200.f, -2200.f);
	Scatter(TEXT("P10"), 12, KampungMin, KampungMax, 120, Rng);
	Scatter(TEXT("P09"), 10, KampungMin, KampungMax, 120, Rng);
	Scatter(TEXT("P02"), 6, KampungMin, KampungMax, 60, Rng);
	Scatter(TEXT("P03"), 6, KampungMin, KampungMax, 60, Rng);
	Scatter(TEXT("P06"), 3, KampungMin, KampungMax, 40, Rng);
	Scatter(TEXT("P12"), 3, KampungMin, KampungMax, 40, Rng);
	Scatter(TEXT("R20"), 8, KampungMin, KampungMax, 80, Rng);

	// Pasar Sentosa under its roof: rice sacks, stall tables, plastic chairs, stalls, gallons.
	const FVector2D PasarMin(-14600.f, 9100.f);
	const FVector2D PasarMax(-9400.f, 16900.f);
	Scatter(TEXT("P24"), 4, PasarMin, PasarMax, 60, Rng);
	Scatter(TEXT("P04"), 6, PasarMin, PasarMax, 80, Rng);
	Scatter(TEXT("P05"), 14, PasarMin, PasarMax, 120, Rng);
	Scatter(TEXT("P03"), 8, PasarMin, PasarMax, 80, Rng);
	Scatter(TEXT("P06"), 2, PasarMin, PasarMax, 30, Rng);

	// Dapur PPR: the food carts waiting outside.
	Scatter(TEXT("P11"), 3, FVector2D(17500.f, 17500.f), FVector2D(22500.f, 22500.f), 60, Rng);
	Scatter(TEXT("P06"), 2, FVector2D(17500.f, 17500.f), FVector2D(22500.f, 22500.f), 30, Rng);

	// SDN Sentosa 1: benches and potted plants in the yard.
	Scatter(TEXT("P20"), 4, FVector2D(9500.f, 10500.f), FVector2D(14500.f, 15500.f), 60, Rng);
	Scatter(TEXT("R20"), 6, FVector2D(9500.f, 10500.f), FVector2D(14500.f, 15500.f), 60, Rng);

	// Kantor Kelurahan and the government blocks: potted plants and bins.
	Scatter(TEXT("R20"), 5, FVector2D(6000.f, -11000.f), FVector2D(11000.f, -7000.f), 60, Rng);
	Scatter(TEXT("P02"), 3, FVector2D(4000.f, -20000.f), FVector2D(20000.f, -2500.f), 40, Rng);
}
