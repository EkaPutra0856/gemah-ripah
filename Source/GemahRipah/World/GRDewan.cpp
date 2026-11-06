#include "World/GRDewan.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"
#include "Story/GRStoryUtil.h"
#include "World/GRRoomShells.h"

DEFINE_LOG_CATEGORY_STATIC(LogGRDewan, Log, All);

namespace
{
	struct FPaintDef
	{
		FLinearColor Color;
		bool bShadow;
		bool bCollide;
	};

	const FPaintDef Paints[] = {
		{ FLinearColor(0.70f, 0.65f, 0.55f), true, false },   // Wall
		{ FLinearColor(0.86f, 0.82f, 0.72f), true, true },    // Stone
		{ FLinearColor(0.55f, 0.51f, 0.44f), true, false },   // Shade
		{ FLinearColor(0.42f, 0.40f, 0.37f), true, true },    // Base
		{ FLinearColor(0.05f, 0.08f, 0.11f), false, false },  // Glass
		{ FLinearColor(0.15f, 0.10f, 0.06f), false, false },  // Door
		{ FLinearColor(0.85f, 0.60f, 0.16f), true, false },   // Gold
		{ FLinearColor(0.50f, 0.32f, 0.08f), false, false },  // GoldDark
		{ FLinearColor(0.07f, 0.08f, 0.09f), true, false },   // Iron
		{ FLinearColor(0.10f, 0.42f, 0.28f), true, false },   // Flag
		{ FLinearColor(0.10f, 0.28f, 0.55f), true, false },   // Board
	};

	// The blockout from build_kota_sentosa.py: 6000 x 12000 x 3000, its front looking west down Jl. Raya.
	constexpr float CenterX = 44000.f;
	constexpr float FrontX = 41000.f;
	constexpr float BackX = 47000.f;
	constexpr float SideY = 6000.f;
	constexpr float WallTop = 3000.f;
	constexpr float FenceX = 37000.f;

	// Podium and the porch that stands out from it.
	constexpr float Floor = 540.f;
	constexpr float PorchX = 39400.f;
	constexpr float PorchY = 3000.f;
	constexpr float StepRise = 27.f;
	constexpr float StepTread = 60.f;

	constexpr float ColumnTop = 2400.f;
	constexpr float BeamTop = 2870.f;
	constexpr float BayStep = 600.f;

	constexpr float DrumRadius = 2200.f;
	constexpr float DrumBottom = 3300.f;
	constexpr float DrumTop = 3800.f;
	constexpr float DomeRise = 1500.f;
}

AGRDewan::AGRDewan()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AGRDewan::BeginPlay()
{
	Super::BeginPlay();
	if (!ClaimBlockout())
	{
		UE_LOG(LogGRDewan, Display, TEXT("GRDEWAN kotak gedung tidak ada di map ini"));
		return;
	}
	Podium();
	Portico();
	Windows();
	Dome();
	Gate();
	Baliho();
	Flush();
}

bool AGRDewan::ClaimBlockout()
{
	const UStaticMesh* Cube = GRStoryUtil::LoadShape(TEXT("cube"));
	const UStaticMesh* Ball = GRStoryUtil::LoadShape(TEXT("sphere"));
	bool bFound = false;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		UStaticMeshComponent* Comp = It->GetStaticMeshComponent();
		if (!Comp || FVector::Dist2D(It->GetActorLocation(), FVector(CenterX, 0.f, 0.f)) > 100.f)
		{
			continue;
		}
		if (Comp->GetStaticMesh() == Cube && Comp->Bounds.BoxExtent.Z > 1000.f)
		{
			Comp->SetMaterial(0, GRStoryUtil::MakeColorMaterial(this, Paints[Wall].Color, 1.f));
			bFound = true;
		}
		else if (Comp->GetStaticMesh() == Ball)
		{
			It->SetActorHiddenInGame(true);
			It->SetActorEnableCollision(false);
		}
	}
	// The level's floating label: the gate carries the name now.
	for (TActorIterator<ATextRenderActor> It(GetWorld()); It; ++It)
	{
		if (FVector::Dist2D(It->GetActorLocation(), FVector(FenceX, 0.f, 0.f)) < 1000.f)
		{
			It->SetActorHiddenInGame(true);
		}
	}
	return bFound;
}

void AGRDewan::Podium()
{
	const float Wide = 2.f * SideY + 800.f;
	const float Deep = BackX - FrontX + 800.f;
	Add(Base, Cube, FVector(CenterX, 0.f, (Floor - 40.f) * 0.5f), FVector(Deep, Wide, Floor - 40.f));
	Add(Shade, Cube, FVector(CenterX, 0.f, Floor - 20.f), FVector(Deep + 100.f, Wide + 100.f, 40.f));

	// The porch reaches out to the stairs.
	const float PorchDeep = FrontX - PorchX;
	Add(Base, Cube, FVector(PorchX + PorchDeep * 0.5f, 0.f, (Floor - 40.f) * 0.5f), FVector(PorchDeep, PorchY * 2.f, Floor - 40.f));
	Add(Shade, Cube, FVector(PorchX + PorchDeep * 0.5f, 0.f, Floor - 20.f), FVector(PorchDeep + 60.f, PorchY * 2.f + 60.f, 40.f));

	const int32 Steps = int32(Floor / StepRise) - 1;
	for (int32 i = 0; i < Steps; ++i)
	{
		const float Top = Floor - StepRise * (i + 1);
		Add(Stone, Cube, FVector(PorchX - StepTread * (i + 0.5f), 0.f, Top * 0.5f), FVector(StepTread, (PorchY - 300.f) * 2.f, Top));
	}

	// Cheek walls beside the stairs, a gold lamp on each.
	const float StairsEnd = PorchX - StepTread * Steps;
	for (const float Side : { 1.f, -1.f })
	{
		const FVector Foot(StairsEnd + 250.f, Side * (PorchY - 150.f), 0.f);
		Add(Base, Cube, FVector((StairsEnd + PorchX) * 0.5f, Foot.Y, Floor * 0.5f - 80.f), FVector(PorchX - StairsEnd, 300.f, Floor - 160.f));
		Add(Stone, Cube, Foot + FVector(0.f, 0.f, 380.f), FVector(420.f, 360.f, 760.f));
		Add(Gold, Sphere, Foot + FVector(0.f, 0.f, 860.f), FVector(200.f, 200.f, 200.f));
	}
}

void AGRDewan::Portico()
{
	// Two rows of columns under the beam.
	const int32 Columns = 10;
	const float Span = (PorchY - 300.f) * 2.f;
	for (const float X : { PorchX + 300.f, PorchX + 900.f })
	{
		for (int32 i = 0; i < Columns; ++i)
		{
			const float Y = -Span * 0.5f + Span * i / (Columns - 1);
			Add(Stone, Cube, FVector(X, Y, Floor + 40.f), FVector(330.f, 330.f, 80.f));
			Add(Stone, Cylinder, FVector(X, Y, (Floor + ColumnTop) * 0.5f), FVector(240.f, 240.f, ColumnTop - Floor));
			Add(Stone, Cube, FVector(X, Y, ColumnTop - 40.f), FVector(330.f, 330.f, 80.f));
		}
	}

	// Beam, cornice and the pediment on top.
	const float Deep = FrontX - PorchX;
	Add(Stone, Cube, FVector(PorchX + Deep * 0.5f, 0.f, ColumnTop + 200.f), FVector(Deep, PorchY * 2.f + 100.f, 400.f));
	Add(GoldDark, Cube, FVector(PorchX - 3.f, 0.f, ColumnTop + 30.f), FVector(6.f, PorchY * 2.f + 100.f, 12.f));
	Add(Shade, Cube, FVector(PorchX + Deep * 0.5f - 30.f, 0.f, BeamTop - 35.f), FVector(Deep + 60.f, PorchY * 2.f + 220.f, 70.f));

	const float Rise = 760.f;
	const float PedimentWide = PorchY * 2.f + 200.f;
	const FVector Foot(PorchX + Deep * 0.5f, 0.f, BeamTop);
	AGRRoomShells::MakePrism(this, RootComponent, Foot, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), Deep, PedimentWide, Rise,
		GRStoryUtil::MakeColorMaterial(this, Paints[Stone].Color, 1.f));

	// The raking cornices along its two slopes.
	const float Pitch = FMath::Atan2(Rise, PedimentWide * 0.5f);
	const float Slope = FMath::Sqrt(FMath::Square(Rise) + FMath::Square(PedimentWide * 0.5f));
	for (const float Side : { 1.f, -1.f })
	{
		const FVector Mid = Foot + FVector(-30.f, Side * PedimentWide * 0.25f, Rise * 0.5f + 34.f);
		Add(Shade, Cube, Mid, FVector(Deep + 60.f, Slope + 120.f, 70.f), FRotator(0.f, 0.f, Side * FMath::RadiansToDegrees(Pitch)));
	}

	// A gold seal in the pediment and the name on the beam.
	Add(Gold, Cylinder, FVector(PorchX - 6.f, 0.f, BeamTop + 280.f), FVector(380.f, 380.f, 12.f), FRotator(90.f, 0.f, 0.f));
	Add(GoldDark, Cylinder, FVector(PorchX - 9.f, 0.f, BeamTop + 280.f), FVector(240.f, 240.f, 12.f), FRotator(90.f, 0.f, 0.f));
	Text(TEXT("DEWAN RAKYAT"), FVector(PorchX - 6.f, 0.f, ColumnTop + 200.f), FVector(-1.f, 0.f, 0.f), 280.f, FColor(130, 85, 15));

	// The great door and its side windows, back in the shade of the columns.
	const FVector Out(-1.f, 0.f, 0.f);
	Add(Stone, Cube, FVector(FrontX - 10.f, 0.f, Floor + 650.f), FVector(20.f, 1100.f, 1300.f));
	Add(Door, Cube, FVector(FrontX - 24.f, 0.f, Floor + 600.f), FVector(10.f, 900.f, 1200.f));
	Add(GoldDark, Cube, FVector(FrontX - 30.f, 0.f, Floor + 600.f), FVector(4.f, 16.f, 1200.f));
	for (const float Y : { -1800.f, -1100.f, 1100.f, 1800.f })
	{
		Add(Stone, Cube, FVector(FrontX - 10.f, Y, Floor + 600.f), FVector(20.f, 400.f, 1100.f));
		Add(Glass, Cube, FVector(FrontX - 22.f, Y, Floor + 600.f), FVector(6.f, 320.f, 1020.f));
	}
}

void AGRDewan::Bays(const FVector& Origin, const FVector& Out, const FVector& Along, float First, int32 Count)
{
	const FRotator Facing = FRotationMatrix::MakeFromXZ(Out, FVector::UpVector).Rotator();
	for (int32 i = 0; i <= Count; ++i)
	{
		const FVector Pilaster = Origin + Along * (First + BayStep * (i - 0.5f)) + Out * 16.f;
		Add(Stone, Cube, Pilaster + FVector(0.f, 0.f, (Floor + WallTop) * 0.5f), FVector(32.f, 110.f, WallTop - Floor), Facing);
	}
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Bay = Origin + Along * (First + BayStep * i);
		for (const float Z : { 800.f, 1560.f, 2320.f })
		{
			Add(Stone, Cube, Bay + Out * 6.f + FVector(0.f, 0.f, Z + 270.f - 260.f), FVector(12.f, 340.f, 560.f), Facing);
			Add(Glass, Cube, Bay + Out * 14.f + FVector(0.f, 0.f, Z + 10.f), FVector(4.f, 280.f, 500.f), Facing);
			Add(Stone, Cube, Bay + Out * 15.f + FVector(0.f, 0.f, Z + 10.f), FVector(4.f, 12.f, 500.f), Facing);
			Add(Shade, Cube, Bay + Out * 22.f + FVector(0.f, 0.f, Z - 262.f), FVector(44.f, 380.f, 24.f), Facing);
		}
	}
}

void AGRDewan::Windows()
{
	const int32 Wing = int32((SideY - PorchY) / BayStep);
	const FVector West(-1.f, 0.f, 0.f);
	const FVector Base0(FrontX, 0.f, 0.f);
	Bays(Base0, West, FVector(0.f, 1.f, 0.f), PorchY + BayStep * 0.5f, Wing);
	Bays(Base0, West, FVector(0.f, -1.f, 0.f), PorchY + BayStep * 0.5f, Wing);

	const int32 Along = int32((BackX - FrontX) / BayStep);
	for (const float Side : { 1.f, -1.f })
	{
		Bays(FVector(FrontX, Side * SideY, 0.f), FVector(0.f, Side, 0.f), FVector(1.f, 0.f, 0.f), BayStep * 0.5f, Along);
	}
	Bays(FVector(BackX, -SideY, 0.f), FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), BayStep * 0.5f, int32(SideY * 2.f / BayStep));

	// Cornice and attic round the top, string course over the podium.
	const float Deep = BackX - FrontX;
	Add(Shade, Cube, FVector(CenterX, 0.f, WallTop + 30.f), FVector(Deep + 180.f, SideY * 2.f + 180.f, 120.f));
	Add(Stone, Cube, FVector(CenterX, 0.f, WallTop + 190.f), FVector(Deep - 100.f, SideY * 2.f - 100.f, 220.f));
	Add(Shade, Cube, FVector(CenterX, 0.f, WallTop + 310.f), FVector(Deep - 60.f, SideY * 2.f - 60.f, 30.f));
}

void AGRDewan::Dome()
{
	const FVector Middle(CenterX, 0.f, 0.f);
	const float Tall = DrumTop - DrumBottom;
	Add(Wall, Cylinder, Middle + FVector(0.f, 0.f, DrumBottom + Tall * 0.5f), FVector(DrumRadius * 2.f, DrumRadius * 2.f, Tall));
	Add(Shade, Cylinder, Middle + FVector(0.f, 0.f, DrumBottom + 40.f), FVector(DrumRadius * 2.f + 120.f, DrumRadius * 2.f + 120.f, 80.f));
	Add(Shade, Cylinder, Middle + FVector(0.f, 0.f, DrumTop), FVector(DrumRadius * 2.f + 160.f, DrumRadius * 2.f + 160.f, 80.f));

	// Columns and tall windows round the drum.
	const int32 Count = 28;
	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = 2.f * PI * i / Count;
		const FVector Way(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		Add(Stone, Cylinder, Middle + Way * (DrumRadius + 30.f) + FVector(0.f, 0.f, DrumBottom + Tall * 0.5f), FVector(90.f, 90.f, Tall));
		const float Between = Angle + PI / Count;
		const FVector Pane(FMath::Cos(Between), FMath::Sin(Between), 0.f);
		Add(Glass, Cube, Middle + Pane * (DrumRadius - 4.f) + FVector(0.f, 0.f, DrumBottom + Tall * 0.5f), FVector(20.f, 220.f, Tall - 180.f), Pane.Rotation());
	}

	// The dome: gold, with darker ribs.
	const float Wide = DrumRadius * 2.f - 60.f;
	const FVector Foot = Middle + FVector(0.f, 0.f, DrumTop + 20.f);
	Add(Gold, Sphere, Foot, FVector(Wide, Wide, DomeRise * 2.f));
	for (int32 i = 0; i < 12; ++i)
	{
		Add(GoldDark, Sphere, Foot, FVector(Wide + 24.f, 50.f, DomeRise * 2.f + 24.f), FRotator(0.f, i * 15.f, 0.f));
	}

	// Lantern and flag on the top.
	const float Top = DrumTop + DomeRise;
	Add(Stone, Cylinder, Middle + FVector(0.f, 0.f, Top + 120.f), FVector(420.f, 420.f, 300.f));
	Add(Shade, Cylinder, Middle + FVector(0.f, 0.f, Top + 280.f), FVector(500.f, 500.f, 40.f));
	Add(Gold, Sphere, Middle + FVector(0.f, 0.f, Top + 300.f), FVector(400.f, 400.f, 260.f));
	Add(Iron, Cylinder, Middle + FVector(0.f, 0.f, Top + 700.f), FVector(24.f, 24.f, 700.f));
	Add(Flag, Cube, Middle + FVector(0.f, 180.f, Top + 930.f), FVector(6.f, 340.f, 200.f));
	Add(Gold, Cube, Middle + FVector(-5.f, 180.f, Top + 930.f), FVector(6.f, 340.f, 34.f));
}

void AGRDewan::Gate()
{
	// The fence itself stays: columns along it, and a closed iron gate where Jl. Raya meets it.
	const float Reach = 8000.f;
	const float GateHalf = 1500.f;
	for (float Y = -Reach; Y <= Reach; Y += 1000.f)
	{
		if (FMath::Abs(Y) > GateHalf)
		{
			Add(Stone, Cube, FVector(FenceX, Y, 180.f), FVector(150.f, 150.f, 360.f));
			Add(Shade, Cube, FVector(FenceX, Y, 370.f), FVector(190.f, 190.f, 24.f));
		}
	}
	for (const float Side : { 1.f, -1.f })
	{
		Add(Stone, Cube, FVector(FenceX, Side * GateHalf, 410.f), FVector(240.f, 240.f, 820.f));
		Add(Shade, Cube, FVector(FenceX, Side * GateHalf, 830.f), FVector(290.f, 290.f, 30.f));
		Add(Gold, Sphere, FVector(FenceX, Side * GateHalf, 900.f), FVector(120.f, 120.f, 120.f));
	}
	Add(Stone, Cube, FVector(FenceX, 0.f, 640.f), FVector(120.f, GateHalf * 2.f - 240.f, 240.f));
	Text(TEXT("GEDUNG DEWAN RAKYAT"), FVector(FenceX - 64.f, 0.f, 640.f), FVector(-1.f, 0.f, 0.f), 170.f, FColor(120, 80, 20));

	Add(Iron, Cube, FVector(FenceX - 34.f, 0.f, 150.f), FVector(6.f, GateHalf * 2.f - 240.f, 300.f));
	for (float Y = -GateHalf + 160.f; Y < GateHalf - 120.f; Y += 110.f)
	{
		Add(Shade, Cube, FVector(FenceX - 39.f, Y, 150.f), FVector(4.f, 14.f, 300.f));
	}
}

void AGRDewan::Baliho()
{
	// Beside the road, turned to the camera of the opening cutscene.
	const FVector Spot(36500.f, 2500.f, 0.f);
	const FVector Out = FRotator(0.f, -148.f, 0.f).Vector();
	const FVector Along(Out.Y, -Out.X, 0.f);
	const FRotator Facing = Out.Rotation();
	for (const float Side : { 1.f, -1.f })
	{
		Add(Iron, Cylinder, Spot + Along * (Side * 500.f) + FVector(0.f, 0.f, 320.f), FVector(36.f, 36.f, 640.f));
	}
	Add(Iron, Cube, Spot + FVector(0.f, 0.f, 960.f), FVector(16.f, 1560.f, 760.f), Facing);
	Add(Board, Cube, Spot + Out * 10.f + FVector(0.f, 0.f, 960.f), FVector(6.f, 1500.f, 700.f), Facing);
	Add(Gold, Cube, Spot + Out * 14.f + FVector(0.f, 0.f, 720.f), FVector(4.f, 1500.f, 60.f), Facing);
	Text(TEXT("UDARA BAIK"), Spot + Out * 16.f + FVector(0.f, 0.f, 1160.f), Out, 210.f, FColor(255, 255, 250));
	Text(TEXT("RAKYAT SEHAT"), Spot + Out * 16.f + FVector(0.f, 0.f, 980.f), Out, 150.f, FColor(255, 215, 90));
	Text(TEXT("Program Piring Rakyat menyehatkan anak bangsa"), Spot + Out * 16.f + FVector(0.f, 0.f, 830.f), Out, 62.f, FColor(255, 255, 250));
}

void AGRDewan::Text(const FString& Words, const FVector& Spot, const FVector& Out, float Size, const FColor& Color)
{
	UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this);
	Label->SetupAttachment(RootComponent);
	Label->SetRelativeLocationAndRotation(Spot, Out.Rotation());
	Label->SetText(FText::FromString(Words));
	Label->SetWorldSize(Size);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetTextRenderColor(Color);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->SetCastShadow(false);
	Label->RegisterComponent();
}

void AGRDewan::Add(EPaint Paint, EShape Shape, const FVector& Center, const FVector& Size, const FRotator& Rotation)
{
	Parts[Paint][Shape].Add(FTransform(Rotation, Center, Size / 100.f));
}

void AGRDewan::Flush()
{
	static const TCHAR* Shapes[] = { TEXT("cube"), TEXT("cylinder"), TEXT("sphere") };
	int32 Instances = 0;
	for (int32 Paint = 0; Paint < PaintCount; ++Paint)
	{
		for (int32 Shape = 0; Shape < ShapeCount; ++Shape)
		{
			TArray<FTransform>& Items = Parts[Paint][Shape];
			if (Items.IsEmpty())
			{
				continue;
			}
			const FPaintDef& Def = Paints[Paint];
			UInstancedStaticMeshComponent* Mesh = NewObject<UInstancedStaticMeshComponent>(this);
			Mesh->SetMobility(EComponentMobility::Movable);
			Mesh->SetupAttachment(RootComponent);
			Mesh->SetStaticMesh(GRStoryUtil::LoadShape(Shapes[Shape]));
			Mesh->SetMaterial(0, GRStoryUtil::MakeColorMaterial(this, Def.Color, 1.f));
			Mesh->SetCollisionProfileName(Def.bCollide ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
			Mesh->SetCanEverAffectNavigation(false);
			Mesh->SetCastShadow(Def.bShadow);
			Mesh->RegisterComponent();
			Mesh->AddInstances(Items, false, true);
			Meshes.Add(Mesh);
			Instances += Items.Num();
			Items.Empty();
		}
	}
	UE_LOG(LogGRDewan, Display, TEXT("GRDEWAN %d bagian, %d mesh"), Instances, Meshes.Num());
}
