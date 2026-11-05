#include "World/GRFacades.h"
#include "World/GRPosters.h"
#include "Story/GRStoryUtil.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogGRFacades, Log, All);

namespace
{
	struct FPaintDef
	{
		FLinearColor Color;
		float Cull;     // 0 = always drawn
		bool bShadow;
	};

	const FPaintDef Paints[] = {
		{ FLinearColor(0.24f, 0.13f, 0.06f), 9000.f, false },   // Wood
		{ FLinearColor(0.86f, 0.84f, 0.78f), 9000.f, false },   // Trim
		{ FLinearColor(0.05f, 0.08f, 0.11f), 14000.f, false },  // Glass
		{ FLinearColor(0.60f, 0.58f, 0.54f), 9000.f, false },   // Sill
		{ FLinearColor(0.58f, 0.60f, 0.62f), 14000.f, false },  // Shutter
		{ FLinearColor(0.36f, 0.38f, 0.40f), 6000.f, false },   // Slat
		{ FLinearColor(0.03f, 0.03f, 0.035f), 14000.f, false }, // Dark
		{ FLinearColor(0.50f, 0.17f, 0.08f), 0.f, true },       // Roof
		{ FLinearColor(0.33f, 0.35f, 0.37f), 0.f, true },       // RoofTin
		{ FLinearColor(0.82f, 0.76f, 0.62f), 0.f, true },       // Gable
		{ FLinearColor(0.42f, 0.41f, 0.40f), 0.f, true },       // Cap
		{ FLinearColor(0.12f, 0.35f, 0.70f), 16000.f, true },   // Tank
		{ FLinearColor(0.88f, 0.88f, 0.86f), 8000.f, false },   // Ac
		{ FLinearColor(0.55f, 0.25f, 0.12f), 6000.f, false },   // Pot
		{ FLinearColor(0.18f, 0.42f, 0.16f), 6000.f, false },   // Leaf
		{ FLinearColor(0.70f, 0.12f, 0.10f), 0.f, true },       // Awning0
		{ FLinearColor(0.10f, 0.45f, 0.20f), 0.f, true },       // Awning1
		{ FLinearColor(0.12f, 0.25f, 0.60f), 0.f, true },       // Awning2
		{ FLinearColor(0.85f, 0.45f, 0.08f), 0.f, true },       // Awning3
		{ FLinearColor(0.95f, 0.80f, 0.15f), 16000.f, false },  // Sign0
		{ FLinearColor(0.75f, 0.10f, 0.10f), 16000.f, false },  // Sign1
		{ FLinearColor(0.10f, 0.25f, 0.65f), 16000.f, false },  // Sign2
		{ FLinearColor(0.10f, 0.50f, 0.25f), 16000.f, false },  // Sign3
		{ FLinearColor(0.92f, 0.92f, 0.90f), 16000.f, false },  // Sign4
		{ FLinearColor(0.85f, 0.60f, 0.20f), 5000.f, false },   // Goods0
		{ FLinearColor(0.30f, 0.60f, 0.85f), 5000.f, false },   // Goods1
		{ FLinearColor(0.90f, 0.30f, 0.40f), 5000.f, false },   // Goods2
		{ FLinearColor(0.50f, 0.80f, 0.30f), 5000.f, false },   // Goods3
	};

	// Made-up shop names; no real brands.
	const TCHAR* ShopNames[] = {
		TEXT("Toko Sumber Rejeki"), TEXT("Konter Pulsa Berkah"), TEXT("Fotokopi Kilat"), TEXT("Apotek Sehat Sentosa"),
		TEXT("Bengkel Jaya Motor"), TEXT("Warung Kopi Mampir"), TEXT("Toko Emas Gemilang"), TEXT("Salon Ayu"),
		TEXT("Laundry Wangi"), TEXT("Toko Bangunan Kokoh"), TEXT("Penjahit Rapi Jali"), TEXT("Rumah Makan Murah Meriah"),
		TEXT("Mie Ayam Pak Kumis"), TEXT("Optik Terang"), TEXT("Kelontong Barokah"), TEXT("Servis HP Kilat"),
		TEXT("Grosir Sembako Sentosa"), TEXT("Bakso Mas Gendut"), TEXT("Toko Buku Pena"), TEXT("Studio Foto Senyum"),
		TEXT("Toko Listrik Terang"), TEXT("Martabak Bang Udin"), TEXT("Warteg Rasa Bahari"), TEXT("Pinjol Cepat Cair"),
		TEXT("Cetak Baliho Kilat"), TEXT("Jasa Urus Surat Sehari Jadi"), TEXT("Koperasi Makmur (Tutup)"), TEXT("Toko Sepatu Melangkah"),
	};

	const float Roads[] = { 0.f, -27000.f, 27000.f };
}

AGRFacades::AGRFacades()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AGRFacades::BeginPlay()
{
	Super::BeginPlay();
	const UStaticMesh* Cube = GRStoryUtil::LoadShape(TEXT("cube"));
	int32 Count = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		const UStaticMeshComponent* Comp = It->GetStaticMeshComponent();
		if (!Comp || Comp->GetStaticMesh() != Cube || !Comp->GetMaterial(0))
		{
			continue;
		}
		// Buildings stand on the ground with a house-to-school sized footprint; walls,
		// roads, stalls and the Gedung Dewan fall outside this.
		const FBox Bounds = Comp->Bounds.GetBox();
		const FVector Half = Bounds.GetExtent();
		const float Small = FMath::Min(Half.X, Half.Y) * 2.f;
		const float Large = FMath::Max(Half.X, Half.Y) * 2.f;
		if (FMath::Abs(Bounds.Min.Z) > 20.f || Half.Z * 2.f < 300.f || Small < 700.f || Large > 5200.f)
		{
			continue;
		}
		Decorate(Bounds.GetCenter(), Half, Comp->GetMaterial(0)->GetName());
		++Count;
	}

	int32 Instances = 0;
	for (int32 Paint = 0; Paint < PaintCount; ++Paint)
	{
		for (int32 Shape = 0; Shape < 2; ++Shape)
		{
			TArray<FTransform>& Items = Parts[Paint][Shape];
			if (Items.IsEmpty())
			{
				continue;
			}
			const FPaintDef& Def = Paints[Paint];
			UHierarchicalInstancedStaticMeshComponent* Mesh = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
			Mesh->SetMobility(EComponentMobility::Movable);
			Mesh->SetupAttachment(RootComponent);
			Mesh->SetStaticMesh(GRStoryUtil::LoadShape(Shape ? TEXT("cylinder") : TEXT("cube")));
			Mesh->SetMaterial(0, GRStoryUtil::MakeColorMaterial(this, Def.Color, 0.9f));
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetCanEverAffectNavigation(false);
			Mesh->SetCastShadow(Def.bShadow);
			if (Def.Cull > 0.f)
			{
				Mesh->SetCullDistances(int32(Def.Cull * 0.8f), int32(Def.Cull));
			}
			Mesh->RegisterComponent();
			Mesh->AddInstances(Items, false, true);
			Meshes.Add(Mesh);
			Instances += Items.Num();
			Items.Empty();
		}
	}
	UE_LOG(LogGRFacades, Display, TEXT("GRFASAD %d bangunan, %d bagian, %d papan nama, %d poster, %d mesh"), Count, Instances, SignCount, PosterCount, Meshes.Num());
}

AGRFacades::FFace AGRFacades::MakeFace(const FVector& Center, const FVector& Half, const FVector& Out) const
{
	FFace Face;
	Face.Out = Out;
	Face.Along = FVector::CrossProduct(FVector::UpVector, -Out);
	Face.Origin = FVector(Center.X, Center.Y, 0.f) + Out * (FMath::Abs(Out.X) * Half.X + FMath::Abs(Out.Y) * Half.Y);
	Face.Width = 2.f * (FMath::Abs(Face.Along.X) * Half.X + FMath::Abs(Face.Along.Y) * Half.Y);
	return Face;
}

void AGRFacades::Decorate(const FVector& Center, const FVector& Half, const FString& Material)
{
	FRandomStream Rng(int32(Center.X * 0.37f) * 7919 + int32(Center.Y * 0.53f));
	const float H = Half.Z * 2.f;
	const float Large = FMath::Max(Half.X, Half.Y) * 2.f;
	const bool bShopRow = Material.StartsWith(TEXT("MI_Ruko"));
	const bool bRuko = bShopRow && FMath::IsNearlyEqual(Large, 2300.f, 20.f);
	const bool bToko = bShopRow && FMath::IsNearlyEqual(Large, 1200.f, 20.f);
	const bool bHouse = Material.StartsWith(TEXT("MI_Rumah")) && Large <= 1050.f;

	// The wall looking at the nearest road is the front; deep in the kampung any wall will do.
	float BestX = BIG_NUMBER, BestY = BIG_NUMBER, RoadX = 0.f, RoadY = 0.f;
	for (const float Road : Roads)
	{
		if (FMath::Abs(Center.X - Road) - Half.X < BestX)
		{
			BestX = FMath::Abs(Center.X - Road) - Half.X;
			RoadX = Road;
		}
		if (FMath::Abs(Center.Y - Road) - Half.Y < BestY)
		{
			BestY = FMath::Abs(Center.Y - Road) - Half.Y;
			RoadY = Road;
		}
	}
	FVector Out = BestX < BestY ? FVector(FMath::Sign(RoadX - Center.X), 0.f, 0.f) : FVector(0.f, FMath::Sign(RoadY - Center.Y), 0.f);
	if (bRuko)
	{
		Out = FVector(-FMath::Sign(Center.X), 0.f, 0.f);
	}
	else if (bHouse && FMath::Min(BestX, BestY) > 3000.f)
	{
		static const FVector Ways[] = { FVector(1.f, 0.f, 0.f), FVector(-1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), FVector(0.f, -1.f, 0.f) };
		Out = Ways[Rng.RandRange(0, 3)];
	}
	const FFace Front = MakeFace(Center, Half, Out);
	const FFace Back = MakeFace(Center, Half, -Out);
	const FFace Left = MakeFace(Center, Half, Front.Along);
	const FFace Right = MakeFace(Center, Half, -Front.Along);

	if (bRuko || bToko)
	{
		ShopFront(Front, bRuko ? 2 : 1, Rng);
		for (float Z = 500.f; Z + 140.f < H - 40.f; Z += 300.f)
		{
			const int32 Count = bRuko ? 4 : 2;
			const float Step = (Front.Width - 300.f) / Count;
			for (int32 i = 0; i < Count; ++i)
			{
				Window(Front, -Front.Width * 0.5f + 150.f + Step * (i + 0.5f), Z, 160.f, 140.f);
			}
		}
		Door(Back, -Back.Width * 0.3f, 100.f);
		WindowRows(Back, 130.f, H, -Back.Width * 0.3f, 200.f);
		if (bToko)
		{
			WindowRows(Left, 130.f, H);
			WindowRows(Right, 130.f, H);
		}
		Box(Ac, Back, Back.Width * 0.25f, 330.f, 25.f, FVector(50.f, 80.f, 55.f));
		FlatRoof(Center, Half, Rng);
	}
	else if (bHouse)
	{
		const float Side = Rng.FRand() < 0.5f ? 1.f : -1.f;
		const float DoorU = -Front.Width * 0.22f * Side;
		Door(Front, DoorU, 100.f);
		Window(Front, Front.Width * 0.2f * Side, 110.f, 200.f, 130.f);
		Window(Left, 0.f, 110.f, 110.f, 120.f);
		Window(Right, 0.f, 110.f, 110.f, 120.f);
		Window(Back, Back.Width * 0.2f, 110.f, 110.f, 120.f);
		if (H > 460.f)
		{
			Window(Front, 0.f, 380.f, 160.f, 120.f);
		}
		// Terrace in front of the door, sometimes a potted plant.
		Box(Sill, Front, DoorU * 0.5f, 3.f, 75.f, FVector(150.f, Front.Width * 0.55f, 6.f));
		SidePosters(Left, Right, Center, 3);
		if (Rng.FRand() < 0.6f)
		{
			const FVector Spot = Front.Origin + Front.Along * (DoorU + 90.f * Side) + Front.Out * 45.f;
			Add(Pot, FTransform(FRotator::ZeroRotator, Spot + FVector(0.f, 0.f, 22.f), FVector(0.35f, 0.35f, 0.45f)), true);
			Add(Leaf, FTransform(FRotator::ZeroRotator, Spot + FVector(0.f, 0.f, 70.f), FVector(0.6f)));
		}
		if (H <= 460.f && Rng.FRand() < 0.75f)
		{
			PitchedRoof(Center, Half, Front, Rng);
		}
		else
		{
			FlatRoof(Center, Half, Rng);
		}
	}
	else
	{
		Door(Front, 0.f, 180.f);
		WindowRows(Front, 110.f, H, 0.f, 200.f);
		WindowRows(Back, 110.f, H);
		WindowRows(Left, 110.f, H);
		WindowRows(Right, 110.f, H);
		for (const FFace* Face : { &Left, &Right })
		{
			for (float Z = 110.f; Z + 150.f < H - 40.f; Z += 320.f)
			{
				if (Rng.FRand() < 0.35f)
				{
					Box(Ac, *Face, Rng.FRandRange(-0.3f, 0.3f) * Face->Width, Z + 40.f, 25.f, FVector(50.f, 80.f, 55.f));
				}
			}
		}
		FlatRoof(Center, Half, Rng);
		SidePosters(Left, Right, Center, 1);
	}
}

void AGRFacades::SidePosters(const FFace& Left, const FFace& Right, const FVector& Center, int32 MaxCount)
{
	// Its own stream, so the posters leave every other random choice of the building as it was.
	FRandomStream Rng(int32(Center.X * 0.71f) * 4099 + int32(Center.Y * 0.29f));
	for (const FFace* Face : { &Left, &Right })
	{
		if (Face->Width < 400.f || Rng.FRand() > 0.4f)
		{
			continue;
		}
		// Near a corner: houses have one window in the middle of a side wall, other buildings a row of windows
		// from about 170 cm in, so only one poster fits there.
		const float Edge = Face->Width * 0.5f - 85.f;
		const float U = Rng.FRand() < 0.5f ? -Edge : Edge;
		// Pasted in a row, the same poster over and over or a mix.
		const int32 Count = Rng.RandRange(1, MaxCount);
		const int32 First = Rng.RandRange(0, 3);
		const bool bSame = Rng.FRand() < 0.5f;
		for (int32 i = 0; i < Count; ++i)
		{
			const float Along = U - FMath::Sign(U) * i * 66.f;
			if (FMath::Abs(Along) < 105.f)
			{
				break;
			}
			GRPosters::Add(this, GRPosters::WallPoster(bSame ? First : First + i),
				Face->Origin + Face->Along * Along + FVector(0.f, 0.f, 150.f + Rng.FRandRange(-6.f, 6.f)) + Face->Out * 1.5f,
				Face->Out, FVector::UpVector, 58.f, 86.f);
			if (PosterCount == 0)
			{
				const FVector Spot = Face->Origin + Face->Along * Along;
				UE_LOG(LogGRFacades, Display, TEXT("GRFASAD poster pertama di %.0f %.0f, menghadap %.0f"), Spot.X, Spot.Y, Face->Out.Rotation().Yaw);
			}
			++PosterCount;
		}
	}
}

void AGRFacades::ShopFront(const FFace& Face, int32 Bays, FRandomStream& Rng)
{
	const float Pillar = 60.f;
	const float Bay = (Face.Width - Pillar * (Bays + 1)) / Bays;
	for (int32 p = 0; p <= Bays; ++p)
	{
		Box(Trim, Face, -Face.Width * 0.5f + Pillar * 0.5f + p * (Bay + Pillar), 165.f, 6.f, FVector(12.f, Pillar, 330.f));
	}
	for (int32 b = 0; b < Bays; ++b)
	{
		const float U = -Face.Width * 0.5f + Pillar + Bay * 0.5f + b * (Bay + Pillar);
		const float Inner = Bay - 20.f;
		if (Rng.FRand() < 0.45f)
		{
			// Closed: rolling door.
			Box(Shutter, Face, U, 150.f, 3.f, FVector(6.f, Inner, 300.f));
			for (int32 k = 1; k <= 7; ++k)
			{
				Box(Slat, Face, U, k * 37.f, 6.5f, FVector(2.f, Inner, 4.f));
			}
			Box(Dark, Face, U, 8.f, 7.f, FVector(4.f, Inner, 16.f));
		}
		else
		{
			// Open: dark shop inside, a display table with goods out front.
			Box(Dark, Face, U, 150.f, 2.f, FVector(4.f, Inner, 300.f));
			const float Table = Inner * 0.55f;
			Box(Wood, Face, U, 40.f, 50.f, FVector(80.f, Table, 80.f));
			const int32 Goods = 2 + Rng.RandRange(0, 3);
			for (int32 g = 0; g < Goods; ++g)
			{
				const float S = Rng.FRandRange(25.f, 45.f);
				Box(EPaint(Goods0 + Rng.RandRange(0, 3)), Face, U - Table * 0.5f + Table * (g + 0.5f) / Goods, 80.f + S * 0.5f, 50.f, FVector(S, S * 1.3f, S));
			}
		}
	}

	// Awning over the shop front, tilted 15 degrees, with a hanging valance.
	const EPaint Awning = EPaint(Awning0 + Rng.RandRange(0, 3));
	const float Tilt = FMath::DegreesToRadians(15.f);
	const FVector Down = Face.Out * FMath::Cos(Tilt) - FVector::UpVector * FMath::Sin(Tilt);
	const FVector Normal = Face.Out * FMath::Sin(Tilt) + FVector::UpVector * FMath::Cos(Tilt);
	const FVector Mid = Face.Origin + FVector::UpVector * (330.f - 75.f * FMath::Tan(Tilt)) + Face.Out * 75.f;
	Add(Awning, FTransform(FRotationMatrix::MakeFromXZ(Down, Normal).Rotator(), Mid, FVector(155.f, Face.Width - 40.f, 8.f) / 100.f));
	Box(Awning, Face, 0.f, 330.f - 150.f * FMath::Tan(Tilt) - 12.f, 150.f, FVector(3.f, Face.Width - 40.f, 26.f));
	Sign(Face, 400.f, Face.Width - 240.f, Rng);
}

void AGRFacades::Sign(const FFace& Face, float Z, float Width, FRandomStream& Rng)
{
	const int32 Color = Rng.RandRange(0, 4);
	Box(Dark, Face, 0.f, Z, 4.f, FVector(4.f, Width + 14.f, 104.f));
	Box(EPaint(Sign0 + Color), Face, 0.f, Z, 6.f, FVector(8.f, Width, 90.f));

	UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this);
	Text->SetupAttachment(RootComponent);
	Text->SetRelativeLocationAndRotation(Face.Origin + FVector::UpVector * Z + Face.Out * 10.5f, Face.Out.Rotation());
	Text->SetText(FText::FromString(ShopNames[Rng.RandRange(0, UE_ARRAY_COUNT(ShopNames) - 1)]));
	Text->SetWorldSize(66.f);
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetTextRenderColor(Color == 0 || Color == 4 ? FColor(30, 30, 30) : FColor(250, 250, 245));
	Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Text->SetCastShadow(false);
	Text->SetCullDistance(9000.f);
	Text->RegisterComponent();
	++SignCount;
}

void AGRFacades::Door(const FFace& Face, float U, float Width)
{
	Box(Trim, Face, U, 113.f, 2.f, FVector(4.f, Width + 20.f, 226.f));
	Box(Wood, Face, U, 105.f, 4.f, FVector(4.f, Width, 210.f));
	if (Width > 150.f)
	{
		Box(Dark, Face, U, 105.f, 6.5f, FVector(2.f, 3.f, 210.f));
		Box(Dark, Face, U - 14.f, 100.f, 7.f, FVector(3.f, 6.f, 14.f));
		Box(Dark, Face, U + 14.f, 100.f, 7.f, FVector(3.f, 6.f, 14.f));
	}
	else
	{
		Box(Dark, Face, U + Width * 0.35f, 100.f, 7.f, FVector(3.f, 6.f, 14.f));
	}
	Box(Sill, Face, U, 4.f, 40.f, FVector(80.f, Width + 60.f, 8.f));
}

void AGRFacades::Window(const FFace& Face, float U, float Z, float W, float H)
{
	const float Mid = Z + H * 0.5f;
	Box(Trim, Face, U, Mid, 2.f, FVector(4.f, W + 16.f, H + 16.f));
	Box(Glass, Face, U, Mid, 4.f, FVector(4.f, W, H));
	Box(Trim, Face, U, Mid, 6.5f, FVector(3.f, 5.f, H));
	Box(Sill, Face, U, Z - 6.f, 8.f, FVector(16.f, W + 30.f, 8.f));
}

void AGRFacades::WindowRows(const FFace& Face, float FirstZ, float Top, float DoorU, float DoorHalf)
{
	const int32 Count = FMath::Max(1, int32((Face.Width - 200.f) / 320.f));
	const float Step = (Face.Width - 200.f) / Count;
	for (float Z = FirstZ; Z + 150.f < Top - 40.f; Z += 320.f)
	{
		for (int32 i = 0; i < Count; ++i)
		{
			const float U = -Face.Width * 0.5f + 100.f + Step * (i + 0.5f);
			// The ground row leaves room for the door.
			if (Z == FirstZ && FMath::Abs(U - DoorU) < DoorHalf)
			{
				continue;
			}
			Window(Face, U, Z, 140.f, 150.f);
		}
	}
}

void AGRFacades::PitchedRoof(const FVector& Center, const FVector& Half, const FFace& Front, FRandomStream& Rng)
{
	// Ridge runs along the front so the slope faces the street. Pitch 45 degrees: the gable
	// is a cube turned about the ridge with its lower half inside the house box.
	const FVector Up = FVector::UpVector;
	const FVector Fw = Front.Out;
	const FVector Ridge = Front.Along;
	const float Depth = 2.f * (FMath::Abs(Fw.X) * Half.X + FMath::Abs(Fw.Y) * Half.Y);
	const float W = Front.Width;
	const float Slope = Depth / UE_SQRT_2;
	const FVector Top(Center.X, Center.Y, Half.Z * 2.f);
	const EPaint Paint = Rng.FRand() < 0.7f ? Roof : RoofTin;

	Add(Gable, FTransform(FRotationMatrix::MakeFromXY((Fw + Up).GetSafeNormal(), Ridge).Rotator(), Top, FVector(Slope, W, Slope) / 100.f));
	const float Over = 60.f;
	for (const float SideSign : { 1.f, -1.f })
	{
		const FVector Out = Fw * SideSign;
		const FVector Down = (Out - Up).GetSafeNormal();
		const FVector Normal = (Out + Up).GetSafeNormal();
		const FVector Mid = Top + (Out + Up) * (Depth * 0.25f) + Normal * 7.f + Down * (Over * 0.5f);
		Add(Paint, FTransform(FRotationMatrix::MakeFromXZ(Down, Normal).Rotator(), Mid, FVector(Slope + Over + 10.f, W + 80.f, 12.f) / 100.f));
	}
	Add(Paint, FTransform(FRotationMatrix::MakeFromXY((Fw + Up).GetSafeNormal(), Ridge).Rotator(), Top + Up * (Depth * 0.5f + 8.f), FVector(28.f, W + 90.f, 28.f) / 100.f));
}

void AGRFacades::FlatRoof(const FVector& Center, const FVector& Half, FRandomStream& Rng)
{
	const float H = Half.Z * 2.f;
	Add(Cap, FTransform(FRotator::ZeroRotator, FVector(Center.X, Center.Y, H + 10.f), FVector(Half.X * 2.f + 24.f, Half.Y * 2.f + 24.f, 20.f) / 100.f));
	if (Rng.FRand() < 0.55f)
	{
		// Water tank (tandon) on a stand, in a roof corner.
		const FVector Spot(Center.X + (Rng.FRand() < 0.5f ? 1.f : -1.f) * (Half.X - 130.f), Center.Y + (Rng.FRand() < 0.5f ? 1.f : -1.f) * (Half.Y - 130.f), H + 20.f);
		Add(Sill, FTransform(FRotator::ZeroRotator, Spot + FVector(0.f, 0.f, 30.f), FVector(1.0f, 1.0f, 0.6f)));
		Add(Tank, FTransform(FRotator::ZeroRotator, Spot + FVector(0.f, 0.f, 125.f), FVector(1.1f, 1.1f, 1.3f)), true);
	}
}

void AGRFacades::Box(EPaint Paint, const FFace& Face, float U, float Z, float D, const FVector& Size)
{
	const FVector Center = Face.Origin + Face.Along * U + FVector::UpVector * Z + Face.Out * D;
	Add(Paint, FTransform(FRotationMatrix::MakeFromXZ(Face.Out, FVector::UpVector).Rotator(), Center, Size / 100.f));
}

void AGRFacades::Add(EPaint Paint, const FTransform& Transform, bool bCylinder)
{
	Parts[Paint][bCylinder ? 1 : 0].Add(Transform);
}
