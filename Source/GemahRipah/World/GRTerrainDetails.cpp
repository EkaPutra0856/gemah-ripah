#include "World/GRTerrainDetails.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "LandscapeProxy.h"
#include "Materials/MaterialInterface.h"
#include "Story/GRStoryUtil.h"
#include "Systems/GRNotificationSubsystem.h"
#include "World/GRTerrain.h"
#include "World/GRTripoProps.h"
#include "Engine/StaticMeshActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogGRTerrain, Log, All);

#define LOCTEXT_NAMESPACE "GRTerrain"

namespace
{
	struct FPaintDef
	{
		const TCHAR* Asset;
		FLinearColor Color;
		bool bCollide;
		bool bShadow;
	};
	// Concrete and stone use the Poly Haven materials made by Tools/Python/build_terrain.py; the rest are flat colors.
	const FPaintDef Paints[] = {
		{ TEXT("/Game/GemahRipah/Landscape/M_GR_Beton.M_GR_Beton"), FLinearColor(0.42f, 0.41f, 0.38f), true, true },
		{ TEXT("/Game/GemahRipah/Landscape/M_GR_Pondasi.M_GR_Pondasi"), FLinearColor(0.35f, 0.33f, 0.30f), true, true },
		{ nullptr, FLinearColor(0.10f, 0.36f, 0.20f), true, true },   // Steel: green truss
		{ nullptr, FLinearColor(0.16f, 0.30f, 0.52f), true, true },   // Rail: painted railings
		{ nullptr, FLinearColor(0.05f, 0.05f, 0.05f), false, false }, // Cable
		{ nullptr, FLinearColor(0.36f, 0.24f, 0.14f), true, true },   // Plank
		{ nullptr, FLinearColor(0.92f, 0.92f, 0.90f), true, true },   // White
		{ nullptr, FLinearColor(0.55f, 0.57f, 0.58f), true, true },   // Zinc
		{ nullptr, FLinearColor(0.42f, 0.27f, 0.17f), true, true },   // Rust
		{ nullptr, FLinearColor(0.95f, 0.95f, 0.92f), true, true },   // Board
		{ nullptr, FLinearColor(0.03f, 0.03f, 0.03f), true, false },  // Dark
		{ TEXT("/Game/GemahRipah/Landscape/M_GR_Kali.M_GR_Kali"), FLinearColor(0.16f, 0.15f, 0.085f), false, false }, // Stream: the kali
	};

	FVector Up(float Z)
	{
		return FVector(0.f, 0.f, Z);
	}
}

AGRTerrainDetails::AGRTerrainDetails()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	// The parts are static meshes; a movable root would refuse them as children.
	RootComponent->SetMobility(EComponentMobility::Static);
}

void AGRTerrainDetails::BeginPlay()
{
	Super::BeginPlay();
	// Only on a map that has the landscape (L_KotaSentosa after Tools/Python/build_terrain.py).
	TActorIterator<ALandscapeProxy> Land(GetWorld());
	if (!Land)
	{
		UE_LOG(LogGRTerrain, Display, TEXT("GRTANAH tidak ada landscape, dilewati"));
		SetActorTickEnabled(false);
		return;
	}
	const FVector2D Open = GRTerrain::Bridges().Last().Center;
	const TOptional<float> AtOpen = Land->GetHeightAtLocation(FVector(Open, 0.f));
	const TOptional<float> AtTugu = Land->GetHeightAtLocation(FVector(0.f, 0.f, 0.f));
	UE_LOG(LogGRTerrain, Display, TEXT("GRTANAH landscape %s di %s skala %s; tinggi sungai %.0f, tugu %.0f"), *Land->GetName(),
		*Land->GetActorLocation().ToString(), *Land->GetActorScale3D().ToString(), AtOpen.Get(-99999.f), AtTugu.Get(-99999.f));
}

void AGRTerrainDetails::Build()
{
	bBuilt = true;
	Water();
	for (const GRTerrain::FBridge& Spec : GRTerrain::Bridges())
	{
		const FVector C(Spec.Center, 0.f);
		const FVector F(Spec.Along.GetSafeNormal(), 0.f);
		const FVector R(-F.Y, F.X, 0.f);
		switch (Spec.Kind)
		{
		case GRTerrain::EBridge::Beton: RoadDeck(C, F, R, Spec.Length, Spec.Width, false); break;
		case GRTerrain::EBridge::Rangka: RoadDeck(C, F, R, Spec.Length, Spec.Width, true); Truss(C, F, R, Spec.Length, Spec.Width); break;
		case GRTerrain::EBridge::Gantung: Suspension(C, F, R, Spec.Length, Spec.Width); break;
		case GRTerrain::EBridge::Kampung: Footbridge(C, F, R, Spec.Length, Spec.Width); break;
		case GRTerrain::EBridge::Gorong: Culvert(C, F, R, Spec.Width); break;
		}
	}
	CreekWalls();
	FieldGoals();
	ProjectSite();
	Trees();
	int32 Logged = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It && Logged < 4; ++It)
	{
		if (It->Tags.Contains(TEXT("GRPondasi")))
		{
			const FBox Box = It->GetComponentsBoundingBox();
			UE_LOG(LogGRTerrain, Display, TEXT("GRTANAH pondasi %s di %.0f %.0f, %.0f..%.0f"), *It->GetName(), Box.GetCenter().X, Box.GetCenter().Y, Box.Min.Z, Box.Max.Z);
			++Logged;
		}
	}

	int32 Instances = 0;
	for (int32 Paint = 0; Paint < PaintCount; ++Paint)
	{
		const FPaintDef& Def = Paints[Paint];
		UMaterialInterface* Material = Def.Asset ? LoadObject<UMaterialInterface>(nullptr, Def.Asset, nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
		for (int32 Shape = 0; Shape < 2; ++Shape)
		{
			TArray<FTransform>& Items = Parts[Paint][Shape];
			if (Items.IsEmpty())
			{
				continue;
			}
			UHierarchicalInstancedStaticMeshComponent* Mesh = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
			Mesh->SetMobility(EComponentMobility::Static);
			Mesh->SetupAttachment(RootComponent);
			Mesh->SetStaticMesh(GRStoryUtil::LoadShape(Shape ? TEXT("cylinder") : TEXT("cube")));
			Mesh->SetMaterial(0, Material ? Material : static_cast<UMaterialInterface*>(GRStoryUtil::MakeColorMaterial(this, Def.Color, 0.92f)));
			Mesh->SetCollisionEnabled(Def.bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
			Mesh->SetCollisionResponseToAllChannels(ECR_Block);
			Mesh->SetCanEverAffectNavigation(false);
			Mesh->SetCastShadow(Def.bShadow);
			Mesh->RegisterComponent();
			Mesh->AddInstances(Items, false, true);
			Meshes.Add(Mesh);
			Instances += Items.Num();
			Items.Empty();
		}
	}
	UE_LOG(LogGRTerrain, Display, TEXT("GRTANAH %d jembatan, %d bagian, %d mesh"), GRTerrain::Bridges().Num(), Instances, Meshes.Num());
}

void AGRTerrainDetails::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// The details stand on the landscape's collision, which streams in a little after BeginPlay.
	if (!bBuilt)
	{
		WaitTime += DeltaSeconds;
		const FVector2D Field = GRTerrain::Field().Center;
		FHitResult Hit;
		if (WaitTime > 15.f || (GetWorld()->LineTraceSingleByChannel(Hit, FVector(Field, 6000.f), FVector(Field, -2000.f), ECC_WorldStatic)
			&& Hit.GetActor() && Hit.GetActor()->IsA<ALandscapeProxy>()))
		{
			UE_LOG(LogGRTerrain, Display, TEXT("GRTANAH dibangun setelah %.1f detik, tanah lapangan %.0f"), WaitTime, Hit.ImpactPoint.Z);
			Build();
		}
		return;
	}
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
		return;
	}
	// Bayu cannot swim: wading in is fine, but once the water is over his chest he is put back on the bank.
	const FVector At = Pawn->GetActorLocation();
	const FVector2D P(At);
	if (At.Z > GRTerrain::WaterZ + 120.f && !GRTerrain::IsWater(P, 500.f))
	{
		DryTimer += DeltaSeconds;
		if (DryTimer > 0.5f)
		{
			LastDry = At;
			DryTimer = 0.f;
		}
		WetTime = 0.f;
		return;
	}
	if (At.Z > GRTerrain::WaterZ + 20.f || LastDry.IsZero())
	{
		WetTime = 0.f;
		return;
	}
	WetTime += DeltaSeconds;
	if (WetTime < 0.75f)
	{
		return;
	}
	WetTime = 0.f;
	Pawn->TeleportTo(LastDry + Up(20.f), Pawn->GetActorRotation());
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastWarning > 10.f)
	{
		LastWarning = Now;
		if (UGRNotificationSubsystem* Notify = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGRNotificationSubsystem>() : nullptr)
		{
			Notify->Notify(LOCTEXT("NoSwim", "Bayu tidak bisa berenang. Balik ke darat dulu."), EGRNotifyType::Warning);
		}
	}
	UE_LOG(LogGRTerrain, Display, TEXT("GRTANAH Bayu tenggelam di %.0f %.0f, dikembalikan ke %.0f %.0f"), At.X, At.Y, LastDry.X, LastDry.Y);
}

void AGRTerrainDetails::Water()
{
	WaterSurface = NewObject<UStaticMeshComponent>(this);
	WaterSurface->SetMobility(EComponentMobility::Static);
	WaterSurface->SetupAttachment(RootComponent);
	WaterSurface->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/GemahRipah/Landscape/M_GR_Air.M_GR_Air"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	WaterSurface->SetMaterial(0, Material ? Material : static_cast<UMaterialInterface*>(GRStoryUtil::MakeColorMaterial(this, FLinearColor(0.08f, 0.12f, 0.09f), 1.f)));
	// One sheet under the whole map: the land is above it everywhere except in the river and the kali.
	WaterSurface->SetWorldLocationAndRotation(Up(GRTerrain::WaterZ), FRotator::ZeroRotator);
	WaterSurface->SetWorldScale3D(FVector(1210.f, 1210.f, 1.f));
	WaterSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterSurface->SetCastShadow(false);
	WaterSurface->SetCanEverAffectNavigation(false);
	WaterSurface->RegisterComponent();
}

void AGRTerrainDetails::RoadDeck(const FVector& C, const FVector& F, const FVector& R, float Length, float Width, bool bTruss)
{
	// The road's own slab is the riding surface; underneath it a concrete deck on girders, abutments and two piers.
	const FRotator Turn = FRotationMatrix::MakeFromXY(F, R).Rotator();
	Box(Concrete, C + Up(-51.f), Turn, FVector(Length, Width + 60.f, 100.f));
	const int32 Girders = Width > 1500.f ? 4 : 2;
	for (int32 g = 0; g < Girders; ++g)
	{
		const float Across = (g - (Girders - 1) * 0.5f) * (Width / Girders);
		Box(Concrete, C + R * Across + Up(-141.f), Turn, FVector(Length - 100.f, 60.f, 80.f));
	}
	for (const float Side : { -1.f, 1.f })
	{
		// Abutment walls into the banks.
		Box(Concrete, C + F * Side * (Length * 0.5f - 125.f) + Up(-250.f), Turn, FVector(250.f, Width + 100.f, 500.f));
		// Piers standing in the water near either bank.
		Box(Concrete, C + F * Side * Length * 0.2f + Up(-300.f), Turn, FVector(140.f, Width * 0.85f, 400.f));
		Box(Concrete, C + F * Side * Length * 0.2f + Up(-150.f), Turn, FVector(200.f, Width + 20.f, 60.f));
		if (bTruss)
		{
			continue;
		}
		// Parapet: a concrete curb with a painted steel railing.
		const FVector Edge = C + R * Side * (Width * 0.5f + 20.f);
		Box(Concrete, Edge + Up(25.f), Turn, FVector(Length, 40.f, 50.f));
		const int32 Posts = FMath::Max(2, FMath::RoundToInt32(Length / 250.f));
		for (int32 p = 0; p <= Posts; ++p)
		{
			const float Along = -Length * 0.5f + Length * p / Posts;
			Box(Rail, Edge + F * Along + Up(80.f), Turn, FVector(8.f, 8.f, 60.f));
		}
		Box(Rail, Edge + Up(110.f), Turn, FVector(Length, 8.f, 6.f));
		Box(Rail, Edge + Up(80.f), Turn, FVector(Length, 5.f, 5.f));
	}
}

void AGRTerrainDetails::Truss(const FVector& C, const FVector& F, const FVector& R, float Length, float Width)
{
	// A through truss with sloped end posts, green like the old city bridges; the Merdeka landmark.
	constexpr int32 Panels = 8;
	constexpr float Height = 650.f;
	const float Panel = Length / Panels;
	const float Base = 40.f;
	for (const float Side : { -1.f, 1.f })
	{
		const FVector S = C + R * Side * (Width * 0.5f + 60.f);
		auto Bottom = [&](int32 i) { return S + F * (-Length * 0.5f + Panel * i) + Up(Base); };
		auto Top = [&](int32 i) { return S + F * (-Length * 0.5f + Panel * i) + Up(Base + Height); };
		Box(Concrete, S + Up(20.f), FRotationMatrix::MakeFromXY(F, R).Rotator(), FVector(Length, 50.f, 40.f));
		Beam(Steel, Bottom(0), Bottom(Panels), 50.f);
		Beam(Steel, Top(1), Top(Panels - 1), 55.f);
		Beam(Steel, Bottom(0), Top(1), 55.f);
		Beam(Steel, Bottom(Panels), Top(Panels - 1), 55.f);
		for (int32 i = 1; i < Panels; ++i)
		{
			Beam(Steel, Bottom(i), Top(i), 30.f);
			// Diagonals lean towards the middle (a Pratt truss).
			if (i < Panels / 2)
			{
				Beam(Steel, Top(i), Bottom(i + 1), 25.f);
			}
			else if (i > Panels / 2)
			{
				Beam(Steel, Top(i), Bottom(i - 1), 25.f);
			}
		}
		// Railing between the verticals at walking height.
		Beam(Rail, Bottom(0) + Up(60.f), Bottom(Panels) + Up(60.f), 6.f);
	}
	// Top bracing over the road.
	const FVector L = C - R * (Width * 0.5f + 60.f);
	const FVector Rr = C + R * (Width * 0.5f + 60.f);
	for (int32 i = 1; i < Panels; ++i)
	{
		const FVector At = F * (-Length * 0.5f + Panel * i) + Up(Base + Height);
		Beam(Steel, L + At, Rr + At, 30.f);
		if (i + 1 < Panels)
		{
			const FVector Next = F * (-Length * 0.5f + Panel * (i + 1)) + Up(Base + Height);
			Beam(Steel, L + At, Rr + Next, 18.f);
		}
	}
	// Name plates over both portals.
	for (const float End : { -1.f, 1.f })
	{
		const FVector Plate = C + F * End * (Length * 0.5f - Panel) + Up(Base + Height - 90.f);
		Box(Steel, Plate, FRotationMatrix::MakeFromXY(F, R).Rotator(), FVector(12.f, Width * 0.6f, 110.f));
		AddText(TEXT("JEMBATAN MERDEKA"), Plate + F * End * 8.f, F * End, 70.f, FColor(245, 240, 220));
	}
}

void AGRTerrainDetails::Suspension(const FVector& C, const FVector& F, const FVector& R, float Length, float Width)
{
	// A kampung suspension bridge: two concrete towers, steel cables, a plank deck that sags in the middle.
	constexpr int32 Segments = 24;
	const FVector E0 = C - F * Length * 0.5f;
	const FVector E1 = C + F * Length * 0.5f;
	const float Z0 = GroundZ(FVector2D(E0)) + 25.f;
	const float Z1 = GroundZ(FVector2D(E1)) + 25.f;
	constexpr float Sag = 80.f;
	auto Deck = [&](float T) { return FMath::Lerp(E0, E1, T) + Up(FMath::Lerp(Z0, Z1, T) - Sag * 4.f * T * (1.f - T)); };
	const float TowerTop = 520.f;
	const FRotator Turn = FRotationMatrix::MakeFromXY(F, R).Rotator();
	for (int32 End = 0; End < 2; ++End)
	{
		const FVector Foot = End == 0 ? E0 + F * 60.f : E1 - F * 60.f;
		const float Z = End == 0 ? Z0 : Z1;
		for (const float Side : { -1.f, 1.f })
		{
			const FVector Post = Foot + R * Side * (Width * 0.5f + 45.f);
			Box(Concrete, Post + Up(Z - 80.f + (TowerTop + 80.f) * 0.5f), Turn, FVector(45.f, 45.f, TowerTop + 80.f));
			// Back stays down to an anchor block in the ground.
			const FVector Anchor = Post - F * (End == 0 ? 1.f : -1.f) * 450.f;
			const float AnchorZ = GroundZ(FVector2D(Anchor));
			Beam(Cable, Post + Up(Z + TowerTop - 20.f), Anchor + Up(AnchorZ + 20.f), 5.f, true);
			Box(Concrete, Anchor + Up(AnchorZ), Turn, FVector(90.f, 90.f, 70.f));
		}
		Box(Concrete, Foot + Up(Z + TowerTop + 20.f), Turn, FVector(50.f, Width + 140.f, 40.f));
		Box(Concrete, Foot + Up(Z - 20.f), Turn, FVector(140.f, Width + 40.f, 50.f));
	}
	const float MidCable = (Deck(0.5f).Z + 120.f);
	const float CableSag = FMath::Lerp(Z0, Z1, 0.5f) + TowerTop - 20.f - MidCable;
	for (int32 s = 0; s < Segments; ++s)
	{
		const float T0 = float(s) / Segments;
		const float T1 = float(s + 1) / Segments;
		const FVector A = Deck(T0);
		const FVector B = Deck(T1);
		const FVector Mid = (A + B) * 0.5f;
		Box(Plank, Mid, FRotationMatrix::MakeFromXY(B - A, R).Rotator(), FVector(FVector::Distance(A, B) + 4.f, Width, 10.f));
		for (const float Side : { -1.f, 1.f })
		{
			const FVector Off = R * Side * (Width * 0.5f + 45.f);
			auto CableAt = [&](float T)
			{
				return FMath::Lerp(E0 + F * 60.f, E1 - F * 60.f, T) + Off + Up(FMath::Lerp(Z0, Z1, T) + TowerTop - 20.f - CableSag * 4.f * T * (1.f - T));
			};
			Beam(Cable, CableAt(T0), CableAt(T1), 6.f, true);
			const FVector Hand = R * Side * (Width * 0.5f + 5.f);
			Beam(Rail, A + Hand + Up(100.f), B + Hand + Up(100.f), 4.f);
			Beam(Cable, A + Hand + Up(5.f), CableAt(T0) - Off + Hand, 2.f, true);
		}
	}
}

void AGRTerrainDetails::Footbridge(const FVector& C, const FVector& F, const FVector& R, float Length, float Width)
{
	const float Z = FMath::Max(GroundZ(FVector2D(C - F * Length * 0.5f)), GroundZ(FVector2D(C + F * Length * 0.5f)));
	const FRotator Turn = FRotationMatrix::MakeFromXY(F, R).Rotator();
	Box(Concrete, C + Up(Z + 7.f), Turn, FVector(Length, Width, 16.f));
	Box(Concrete, C + Up(Z - 30.f), Turn, FVector(Length - 40.f, Width * 0.6f, 60.f));
	for (const float Side : { -1.f, 1.f })
	{
		const FVector Edge = C + R * Side * (Width * 0.5f - 4.f);
		for (const float Along : { -0.5f, -0.17f, 0.17f, 0.5f })
		{
			Box(Rail, Edge + F * Along * (Length - 20.f) + Up(Z + 60.f), Turn, FVector(6.f, 6.f, 90.f));
		}
		Box(Rail, Edge + Up(Z + 103.f), Turn, FVector(Length - 10.f, 6.f, 5.f));
		Box(Rail, Edge + Up(Z + 60.f), Turn, FVector(Length - 10.f, 4.f, 4.f));
	}
}

void AGRTerrainDetails::Culvert(const FVector& C, const FVector& F, const FVector& R, float Width)
{
	// The concrete headwall where the kali comes out from under Jl. Merdeka, with its dark mouth.
	const FRotator Turn = FRotationMatrix::MakeFromXY(F, R).Rotator();
	const float Bed = GRTerrain::Creek().BedZ;
	Box(Concrete, C + Up((Bed - 20.f + 40.f) * 0.5f), Turn, FVector(80.f, Width, 40.f - (Bed - 20.f)));
	Box(Dark, C - F * 41.f + Up(Bed + 60.f), Turn, FVector(4.f, 170.f, 120.f));
}

void AGRTerrainDetails::CreekWalls()
{
	// Talud: the concrete walls of the kali between the kampung houses, a little above the ground so nobody falls in.
	// They stop at each footbridge so the gang runs through.
	const GRTerrain::FWaterway& Kali = GRTerrain::Creek();
	const float Bottom = Kali.BedZ - 20.f;
	constexpr float Top = 60.f;
	for (int32 i = 0; i + 1 < Kali.Points.Num(); ++i)
	{
		const FVector2D A = Kali.Points[i];
		const FVector2D B = Kali.Points[i + 1];
		if (!GRTerrain::IsWalledCreek(A) || !GRTerrain::IsWalledCreek(B))
		{
			continue;
		}
		const FVector2D Mid = (A + B) * 0.5;
		bool bGap = false;
		for (const GRTerrain::FBridge& Spec : GRTerrain::Bridges())
		{
			if (Spec.Kind == GRTerrain::EBridge::Kampung && FVector2D::Distance(Spec.Center, Mid) < 140.f)
			{
				bGap = true;
			}
		}
		if (bGap)
		{
			// The water runs on under the footbridge.
			const FVector Dir = FVector(B - A, 0.f).GetSafeNormal();
			Box(Stream, FVector(Mid, GRTerrain::CreekWaterZ - 1.f), FRotationMatrix::MakeFromXY(Dir, FVector(-Dir.Y, Dir.X, 0.f)).Rotator(),
				FVector(float(FVector2D::Distance(A, B)) + 2.f, Kali.HalfWidth * 2.f + 4.f, 2.f));
			continue;
		}
		const FVector Dir = FVector(B - A, 0.f).GetSafeNormal();
		const FVector Side(-Dir.Y, Dir.X, 0.f);
		const FRotator Turn = FRotationMatrix::MakeFromXY(Dir, Side).Rotator();
		const float Len = float(FVector2D::Distance(A, B)) + 2.f;
		Box(Stream, FVector(Mid, GRTerrain::CreekWaterZ - 1.f), Turn, FVector(Len, Kali.HalfWidth * 2.f + 4.f, 2.f));
		for (const float S : { -1.f, 1.f })
		{
			Box(Concrete, FVector(Mid, 0.f) + Side * S * (Kali.HalfWidth + 15.f) + Up((Bottom + Top) * 0.5f), Turn, FVector(Len, 30.f, Top - Bottom));
		}
	}
}

void AGRTerrainDetails::Trees()
{
	// Trees on the hills and along the river banks, where nothing else stands.
	UStaticMesh* Tree = GRTripoProps::Load(TEXT("P19"));
	if (!Tree)
	{
		return;
	}
	UHierarchicalInstancedStaticMeshComponent* Mesh = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
	Mesh->SetMobility(EComponentMobility::Static);
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetStaticMesh(Tree);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCullDistances(30000, 40000);
	Mesh->RegisterComponent();
	Meshes.Add(Mesh);

	FRandomStream Rng(9431);
	const GRTerrain::FWaterway& Big = GRTerrain::River();
	TArray<FVector2D> Placed;
	int32 Tries = 0;
	while (Placed.Num() < 420 && Tries++ < 12000)
	{
		const FVector2D P(Rng.FRandRange(-58000.f, 58000.f), Rng.FRandRange(-58000.f, 58000.f));
		const float Out = float(FMath::Max(FMath::Abs(P.X), FMath::Abs(P.Y)));
		const float RiverD = GRTerrain::DistanceTo(Big, P);
		const bool bBank = RiverD > Big.BankOut - 200.f && RiverD < Big.BankOut + 2500.f;
		if ((Out < 30000.f && !bBank) || RiverD < Big.BankOut - 200.f || GRTerrain::IsWater(P, 400.f))
		{
			continue;
		}
		// Clear of the roads, the field and the project.
		const float AX = float(FMath::Abs(P.X));
		const float AY = float(FMath::Abs(P.Y));
		if (AX < 3500.f || AY < 3500.f || FMath::Abs(AX - 27000.f) < 1500.f || FMath::Abs(AY - 27000.f) < 1500.f
			|| GRTerrain::Field().Contains(P, 800.f) || GRTerrain::Project().Contains(P, 800.f))
		{
			continue;
		}
		bool bCrowded = false;
		for (const FVector2D& Other : Placed)
		{
			if (FVector2D::DistSquared(Other, P) < FMath::Square(700.f))
			{
				bCrowded = true;
				break;
			}
		}
		// Only on open landscape (not a roof, a bridge or a foundation).
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GRTerrainTree), false, this);
		if (bCrowded || !GetWorld()->LineTraceSingleByChannel(Hit, FVector(P, 6000.f), FVector(P, -1500.f), ECC_WorldStatic, Params)
			|| !Hit.GetActor() || !Hit.GetActor()->IsA<ALandscapeProxy>())
		{
			continue;
		}
		// And not right against a building.
		FCollisionShape Probe = FCollisionShape::MakeSphere(250.f);
		if (GetWorld()->OverlapAnyTestByChannel(Hit.ImpactPoint + FVector(0.f, 0.f, 420.f), FQuat::Identity, ECC_WorldStatic, Probe, Params))
		{
			continue;
		}
		Placed.Add(P);
		const float Scale = Rng.FRandRange(0.8f, 1.35f);
		Mesh->AddInstance(FTransform(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f),
			Hit.ImpactPoint - FVector(0.f, 0.f, Tree->GetBoundingBox().Min.Z * Scale + 15.f), FVector(Scale)), true);
	}
	UE_LOG(LogGRTerrain, Display, TEXT("GRTANAH %d pohon di bukit dan tepi sungai"), Placed.Num());
}

void AGRTerrainDetails::FieldGoals()
{
	// (The weir at the end of the kali walls is built here too: it is where the channel drops to the natural creek.)
	{
		const GRTerrain::FWaterway& Kali = GRTerrain::Creek();
		for (int32 i = 1; i < Kali.Points.Num(); ++i)
		{
			if (GRTerrain::IsWalledCreek(Kali.Points[i - 1]) && !GRTerrain::IsWalledCreek(Kali.Points[i]))
			{
				const FVector Dir = FVector(Kali.Points[i] - Kali.Points[i - 1], 0.f).GetSafeNormal();
				const FVector Side(-Dir.Y, Dir.X, 0.f);
				const FRotator Turn = FRotationMatrix::MakeFromXY(Dir, Side).Rotator();
				const FVector At(Kali.Points[i - 1], 0.f);
				Box(Concrete, At + FVector(0.f, 0.f, (Kali.BedZ - 40.f + GRTerrain::CreekWaterZ) * 0.5f), Turn,
					FVector(60.f, Kali.HalfWidth * 2.f + 60.f, GRTerrain::CreekWaterZ - Kali.BedZ + 40.f));
				// A rusty sluice gate standing in it.
				Box(Rust, At + Dir * 35.f + FVector(0.f, 0.f, GRTerrain::CreekWaterZ + 60.f), Turn, FVector(8.f, Kali.HalfWidth * 2.f - 40.f, 150.f));
				Box(Rail, At + Dir * 35.f + FVector(0.f, 0.f, GRTerrain::CreekWaterZ + 160.f), Turn, FVector(14.f, Kali.HalfWidth * 2.f + 80.f, 14.f));
				break;
			}
		}
	}

	const GRTerrain::FArea& Pitch = GRTerrain::Field();
	for (const float End : { -1.f, 1.f })
	{
		const FVector2D Mouth = Pitch.Center + FVector2D(0.f, End * (Pitch.Half.Y - 60.f));
		const float Z = GroundZ(Mouth);
		for (const float Side : { -1.f, 1.f })
		{
			Beam(White, FVector(Mouth + FVector2D(Side * 250.f, 0.f), Z - 10.f), FVector(Mouth + FVector2D(Side * 250.f, 0.f), Z + 200.f), 10.f, true);
			// The frame leaning back behind the goal.
			Beam(White, FVector(Mouth + FVector2D(Side * 250.f, 0.f), Z + 200.f), FVector(Mouth + FVector2D(Side * 250.f, End * 120.f), Z), 5.f, true);
		}
		Beam(White, FVector(Mouth + FVector2D(-255.f, 0.f), Z + 200.f), FVector(Mouth + FVector2D(255.f, 0.f), Z + 200.f), 10.f, true);
		Beam(White, FVector(Mouth + FVector2D(-250.f, End * 120.f), Z), FVector(Mouth + FVector2D(250.f, End * 120.f), Z), 5.f, true);
	}
	UE_LOG(LogGRTerrain, Display, TEXT("GRTANAH lapangan di %.0f %.0f, tanah %.0f"), Pitch.Center.X, Pitch.Center.Y, GroundZ(Pitch.Center));
}

void AGRTerrainDetails::ProjectSite()
{
	// A zinc fence (some panels gone, some leaning, some rusted) around heaps of earth, and the sign that promised it all.
	const GRTerrain::FArea& Site = GRTerrain::Project();
	FRandomStream Rng(4807);
	const FVector2D Half = Site.Half + FVector2D(150.f);
	constexpr float PanelLength = 240.f;
	for (int32 Edge = 0; Edge < 4; ++Edge)
	{
		const bool bAlongY = Edge < 2;
		const float Sign = Edge % 2 == 0 ? -1.f : 1.f;
		const float Span = bAlongY ? Half.Y : Half.X;
		const int32 Count = FMath::CeilToInt32(Span * 2.f / PanelLength);
		for (int32 i = 0; i < Count; ++i)
		{
			if (Rng.FRand() < 0.15f)
			{
				continue;
			}
			const float U = -Span + (i + 0.5f) * (Span * 2.f / Count);
			const FVector2D P = Site.Center + (bAlongY ? FVector2D(Sign * Half.X, U) : FVector2D(U, Sign * Half.Y));
			const FVector Along = bAlongY ? FVector(0.f, 1.f, 0.f) : FVector(1.f, 0.f, 0.f);
			const FVector Out = bAlongY ? FVector(Sign, 0.f, 0.f) : FVector(0.f, Sign, 0.f);
			const float Lean = Rng.FRand() < 0.2f ? Rng.FRandRange(6.f, 14.f) : 0.f;
			const FRotator Turn = FRotationMatrix::MakeFromXY(Along, Out.RotateAngleAxis(-Lean, Along)).Rotator();
			Box(Rng.FRand() < 0.35f ? Rust : Zinc, FVector(P, GroundZ(P) + 105.f), Turn, FVector(PanelLength - 6.f, 4.f, 220.f));
		}
	}
	// The sign at the north side, facing out.
	const FVector2D SignAt = Site.Center + FVector2D(Half.X + 250.f, -600.f);
	const float Z = GroundZ(SignAt);
	const FVector Facing(1.f, 0.f, 0.f);
	const FRotator Turn = FRotationMatrix::MakeFromXY(Facing, FVector(0.f, 1.f, 0.f)).Rotator();
	for (const float Side : { -1.f, 1.f })
	{
		Beam(Rust, FVector(SignAt + FVector2D(0.f, Side * 260.f), Z - 20.f), FVector(SignAt + FVector2D(0.f, Side * 260.f), Z + 460.f), 12.f);
	}
	Box(Board, FVector(SignAt, Z + 340.f), Turn, FVector(6.f, 600.f, 250.f));
	const TCHAR* Lines[] = {
		TEXT("PROYEK PEMBANGUNAN"),
		TEXT("GEDUNG SERBAGUNA RAKYAT"),
		TEXT("Nilai Kontrak: Rp 48.700.000.000"),
		TEXT("Mulai: 2019    Selesai: SEGERA"),
		TEXT("Pelaksana: PT Karya Kerabat Sejahtera"),
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(Lines); ++i)
	{
		AddText(Lines[i], FVector(SignAt, Z + 430.f - i * 42.f) + Facing * 5.f, Facing, i < 2 ? 40.f : 28.f, FColor(25, 25, 30));
	}
}

void AGRTerrainDetails::AddText(const FString& Text, const FVector& Where, const FVector& Facing, float Size, const FColor& Color)
{
	UTextRenderComponent* Comp = NewObject<UTextRenderComponent>(this);
	Comp->SetupAttachment(RootComponent);
	Comp->SetWorldLocationAndRotation(Where, Facing.Rotation());
	Comp->SetText(FText::FromString(Text));
	Comp->SetWorldSize(Size);
	Comp->SetHorizontalAlignment(EHTA_Center);
	Comp->SetVerticalAlignment(EVRTA_TextCenter);
	Comp->SetTextRenderColor(Color);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->RegisterComponent();
}

void AGRTerrainDetails::Box(EPaint Paint, const FVector& Where, const FRotator& Rotation, const FVector& Size)
{
	Parts[Paint][0].Add(FTransform(Rotation, Where, Size / 100.f));
}

void AGRTerrainDetails::Beam(EPaint Paint, const FVector& A, const FVector& B, float Thickness, bool bRound)
{
	const FVector Dir = B - A;
	const float Length = Dir.Size();
	if (Length < 1.f)
	{
		return;
	}
	if (bRound)
	{
		Parts[Paint][1].Add(FTransform(FRotationMatrix::MakeFromZ(Dir).Rotator(), (A + B) * 0.5f, FVector(Thickness / 100.f, Thickness / 100.f, Length / 100.f)));
		return;
	}
	Parts[Paint][0].Add(FTransform(FRotationMatrix::MakeFromX(Dir).Rotator(), (A + B) * 0.5f, FVector(Length / 100.f, Thickness / 100.f, Thickness / 100.f)));
}

float AGRTerrainDetails::GroundZ(const FVector2D& P) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GRTerrainGround), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(P, 5000.f), FVector(P, -1500.f), ECC_WorldStatic, Params))
	{
		return Hit.ImpactPoint.Z;
	}
	return 0.f;
}

#undef LOCTEXT_NAMESPACE
