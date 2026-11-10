#include "World/GRRoomShells.h"
#include "Camera/GRTopDownRoomVolume.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Story/GRStoryUtil.h"
#include "UI/GRStoryScreens.h"
#include "UI/GRUISubsystem.h"
#include "World/GRPlusDressing.h"

DEFINE_LOG_CATEGORY_STATIC(LogGRRoomShells, Log, All);

namespace
{
	/** The trigger stops this far short of the wall's middle, the wall is twice WallHalf thick (build_kota_sentosa.py). */
	constexpr float TriggerInset = 40.f;
	constexpr float WallHalf = 12.5f;
	constexpr float RoofPitch = 35.f;
	constexpr float Eave = 40.f;
	constexpr float DoorTop = 230.f;
	constexpr float DoorWidth = 260.f;
	constexpr float PartCull = 16000.f;
	/** What is left of the roof seen from above, and how long it takes to get there. */
	constexpr float SeeThroughOpacity = 0.22f;
	constexpr float SeeThroughTime = 0.35f;
	const FLinearColor RoofColor(0.50f, 0.17f, 0.08f);

	const FVector Sides[] = { FVector(1.f, 0.f, 0.f), FVector(-1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), FVector(0.f, -1.f, 0.f) };

	float Reach(const FVector& Half, const FVector& Way)
	{
		return FMath::Abs(Way.X) * Half.X + FMath::Abs(Way.Y) * Half.Y;
	}

	FTransform OnWall(const FVector& Spot, const FVector& Out, const FVector& Size)
	{
		return FTransform(FRotationMatrix::MakeFromXZ(Out, FVector::UpVector).Rotator(), Spot, Size / 100.f);
	}

	/** One flat face, its corners going either way round. */
	void AddFace(TArray<FVector>& Verts, TArray<int32>& Tris, TArray<FVector>& Normals, TArray<FVector2D>& Uvs, const TArray<FVector>& Corners, const FVector& Out)
	{
		const int32 First = Verts.Num();
		FVector Normal = FVector::CrossProduct(Corners[2] - Corners[0], Corners[1] - Corners[0]).GetSafeNormal();
		const bool bFlip = FVector::DotProduct(Normal, Out) < 0.f;
		if (bFlip)
		{
			Normal = -Normal;
		}
		const FVector U = FVector::CrossProduct(FVector::UpVector, Normal).GetSafeNormal();
		const FVector AxisU = U.IsNearlyZero() ? FVector::ForwardVector : U;
		const FVector AxisV = FVector::CrossProduct(Normal, AxisU);
		for (const FVector& Corner : Corners)
		{
			Verts.Add(Corner);
			Normals.Add(Normal);
			Uvs.Emplace(FVector::DotProduct(Corner, AxisU) / 400.f, FVector::DotProduct(Corner, AxisV) / 400.f);
		}
		for (int32 i = 1; i + 1 < Corners.Num(); ++i)
		{
			Tris.Add(First);
			Tris.Add(First + (bFlip ? i + 1 : i));
			Tris.Add(First + (bFlip ? i : i + 1));
		}
	}
}

AGRRoomShells::AGRRoomShells()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.03f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

UProceduralMeshComponent* AGRRoomShells::MakePrism(AActor* Owner, USceneComponent* Parent, const FVector& BaseCenter, const FVector& Along, const FVector& Across,
	float Length, float Width, float Height, UMaterialInterface* Material)
{
	const FVector Up = FVector::UpVector;
	const FVector End = Along * (Length * 0.5f);
	const FVector Side = Across * (Width * 0.5f);
	const FVector Ridge = BaseCenter + Up * Height;

	TArray<FVector> Verts, Normals;
	TArray<int32> Tris;
	TArray<FVector2D> Uvs;
	AddFace(Verts, Tris, Normals, Uvs, { BaseCenter + End - Side, BaseCenter + End + Side, Ridge + End }, Along);
	AddFace(Verts, Tris, Normals, Uvs, { BaseCenter - End - Side, BaseCenter - End + Side, Ridge - End }, -Along);
	AddFace(Verts, Tris, Normals, Uvs, { BaseCenter - End + Side, BaseCenter + End + Side, Ridge + End, Ridge - End }, Across * Height + Up * (Width * 0.5f));
	AddFace(Verts, Tris, Normals, Uvs, { BaseCenter - End - Side, BaseCenter + End - Side, Ridge + End, Ridge - End }, -Across * Height + Up * (Width * 0.5f));
	AddFace(Verts, Tris, Normals, Uvs, { BaseCenter - End - Side, BaseCenter + End - Side, BaseCenter + End + Side, BaseCenter - End + Side }, -Up);

	UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(Owner);
	Mesh->SetupAttachment(Parent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->RegisterComponent();
	Mesh->CreateMeshSection_LinearColor(0, Verts, Tris, Normals, Uvs, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
	Mesh->SetMaterial(0, Material);
	return Mesh;
}

void AGRRoomShells::BeginPlay()
{
	Super::BeginPlay();
	RoofPaint = GRStoryUtil::MakeColorMaterial(this, RoofColor, 0.9f);
	SeeThroughParent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/GemahRipah/Materials/M_GR_Tembus.M_GR_Tembus"));
	TrimPaint = GRStoryUtil::MakeColorMaterial(this, FLinearColor(0.86f, 0.84f, 0.78f), 1.f);
	GlassPaint = GRStoryUtil::MakeColorMaterial(this, FLinearColor(0.05f, 0.08f, 0.11f), 1.f);
	BoardPaint = GRStoryUtil::MakeColorMaterial(this, FLinearColor(0.10f, 0.11f, 0.13f), 1.f);
	for (TActorIterator<AGRTopDownRoomVolume> It(GetWorld()); It; ++It)
	{
		Build(**It);
	}
	UE_LOG(LogGRRoomShells, Display, TEXT("GRRUANG %d ruangan beratap"), Rooms.Num());
}

AGRRoomShells::FShell AGRRoomShells::Inspect(const AGRTopDownRoomVolume& Room, const FVector& Half) const
{
	FShell Shell;
	const UStaticMesh* Cube = GRStoryUtil::LoadShape(TEXT("cube"));
	const FVector Center = Room.GetActorLocation();
	bool bClosed[UE_ARRAY_COUNT(Sides)] = {};
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		const UStaticMeshComponent* Comp = It->GetStaticMeshComponent();
		if (!Comp || Comp->GetStaticMesh() != Cube || It->Tags.Contains(AGRPlusDressing::ReplacedTag))
		{
			continue;
		}
		const FBox Bounds = Comp->Bounds.GetBox();
		const FVector Size = Bounds.GetSize();
		const FVector From = Bounds.GetCenter() - Center;
		if (FMath::Min(Size.X, Size.Y) > 40.f || Size.Z < 250.f || FMath::Abs(From.X) > Half.X + 30.f || FMath::Abs(From.Y) > Half.Y + 30.f)
		{
			continue;
		}
		++Shell.Walls;
		Shell.Height = FMath::Max(Shell.Height, Bounds.Max.Z);
		Shell.Material = Comp->GetMaterial(0);
		for (int32 i = 0; i < UE_ARRAY_COUNT(Sides); ++i)
		{
			// A side is closed when a wall runs along it through its middle.
			const FVector& Way = Sides[i];
			const FVector Along(Way.Y, Way.X, 0.f);
			const bool bOnSide = FMath::Abs(FVector::DotProduct(From, Way) - Reach(Half, Way) + WallHalf) < 30.f;
			const bool bCovers = Bounds.Min.Dot(Along.GetAbs()) - Center.Dot(Along.GetAbs()) < 0.f && Bounds.Max.Dot(Along.GetAbs()) - Center.Dot(Along.GetAbs()) > 0.f;
			bClosed[i] = bClosed[i] || (bOnSide && bCovers);
		}
	}
	for (int32 i = 0; i < UE_ARRAY_COUNT(Sides); ++i)
	{
		if (!bClosed[i])
		{
			Shell.Door = Sides[i];
		}
	}
	return Shell;
}

void AGRRoomShells::Build(AGRTopDownRoomVolume& Room)
{
	const FVector Extent = Room.Trigger->GetScaledBoxExtent();
	// Outer faces of the walls.
	const FVector Half(Extent.X + TriggerInset + WallHalf, Extent.Y + TriggerInset + WallHalf, 0.f);
	const FShell Shell = Inspect(Room, Half);
	if (Shell.Walls < 4 || Shell.Door.IsZero())
	{
		UE_LOG(LogGRRoomShells, Display, TEXT("GRRUANG %s dilewati: %d dinding"), *Room.GetName(), Shell.Walls);
		return;
	}

	const FVector Center(Room.GetActorLocation().X, Room.GetActorLocation().Y, 0.f);
	USceneComponent* Group = NewObject<USceneComponent>(this);
	Group->SetupAttachment(RootComponent);
	Group->RegisterComponent();
	Roof(Group, Center, Half, Shell);
	for (const FVector& Out : Sides)
	{
		Front(Center, Half, Shell, Out);
	}
	NameBoard(Group, Center, Half, Shell);

	// The gap in the door wall stops at the door's height.
	const FVector Wall = Center + Shell.Door * (Reach(Half, Shell.Door) - WallHalf);
	const float Lintel = Shell.Height - DoorTop;
	Part(Group, Shell.Material, OnWall(Wall + FVector::UpVector * (DoorTop + Lintel * 0.5f), Shell.Door, FVector(WallHalf * 2.f, DoorWidth, Lintel)), true);

	Rooms.Add(&Room);
	FGRRoofParts& Parts = Roofs.AddDefaulted_GetRef();
	Parts.SeeThrough = UMaterialInstanceDynamic::Create(SeeThroughParent, this);
	Parts.SeeThrough->SetVectorParameterValue(TEXT("Color"), RoofColor);
	TArray<USceneComponent*> Pieces;
	Group->GetChildrenComponents(false, Pieces);
	for (USceneComponent* Child : Pieces)
	{
		UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Child);
		if (Mesh && Mesh->GetMaterial(0) == RoofPaint)
		{
			Parts.Slopes.Add(Mesh);
		}
		else
		{
			Parts.Rest.Add(Child);
		}
	}
}

void AGRRoomShells::Roof(USceneComponent* Group, const FVector& Center, const FVector& Half, const FShell& Shell)
{
	// The ridge runs along the long side, which keeps the big rooms' roofs low.
	const FVector Ridge = Half.Y >= Half.X ? FVector(0.f, 1.f, 0.f) : FVector(1.f, 0.f, 0.f);
	const FVector Across = Half.Y >= Half.X ? FVector(1.f, 0.f, 0.f) : FVector(0.f, 1.f, 0.f);
	const float Length = 2.f * Reach(Half, Ridge);
	const float Width = 2.f * Reach(Half, Across);
	const float Pitch = FMath::DegreesToRadians(RoofPitch);
	const float Rise = Width * 0.5f * FMath::Tan(Pitch);
	const float Slope = Width * 0.5f / FMath::Cos(Pitch);
	const FVector Up = FVector::UpVector;
	const FVector Top = Center + Up * Shell.Height;

	UProceduralMeshComponent* Gable = MakePrism(this, Group, Top, Ridge, Across, Length, Width, Rise, Shell.Material);
	Gable->SetCastShadow(true);

	for (const float Way : { 1.f, -1.f })
	{
		const FVector Out = Across * Way;
		const FVector Down = Out * FMath::Cos(Pitch) - Up * FMath::Sin(Pitch);
		const FVector Normal = Out * FMath::Sin(Pitch) + Up * FMath::Cos(Pitch);
		const FVector Mid = Top + Out * (Width * 0.25f) + Up * (Rise * 0.5f) + Normal * 7.f + Down * (Eave * 0.5f);
		Part(Group, RoofPaint, FTransform(FRotationMatrix::MakeFromXZ(Down, Normal).Rotator(), Mid, FVector(Slope + Eave + 10.f, Length + 80.f, 12.f) / 100.f), true);
	}
	Part(Group, RoofPaint, FTransform(FRotationMatrix::MakeFromXZ(Across, Up).Rotator(), Top + Up * (Rise + 14.f), FVector(44.f, Length + 90.f, 16.f) / 100.f), true);
}

void AGRRoomShells::Front(const FVector& Center, const FVector& Half, const FShell& Shell, const FVector& Out)
{
	const FVector Along(Out.Y, Out.X, 0.f);
	const float Width = 2.f * Reach(Half, Along);
	const FVector Face = Center + Out * Reach(Half, Out);
	const bool bDoorWall = Out == Shell.Door;

	// The door wall keeps a window on each side of the door.
	const int32 Count = bDoorWall ? 2 : FMath::Max(1, int32(Width / 500.f));
	const float W = 130.f;
	const float H = 120.f;
	const float Bottom = 110.f;
	for (int32 i = 0; i < Count; ++i)
	{
		const float U = bDoorWall ? (i == 0 ? -1.f : 1.f) * (DoorWidth * 0.5f + (Width * 0.5f - DoorWidth * 0.5f) * 0.5f) : -Width * 0.5f + Width * (i + 0.5f) / Count;
		const FVector Spot = Face + Along * U + FVector::UpVector * (Bottom + H * 0.5f);
		Part(RootComponent, TrimPaint, OnWall(Spot + Out * 2.f, Out, FVector(4.f, W + 16.f, H + 16.f)), false);
		Part(RootComponent, GlassPaint, OnWall(Spot + Out * 4.f, Out, FVector(4.f, W, H)), false);
		Part(RootComponent, TrimPaint, OnWall(Spot + Out * 6.5f, Out, FVector(3.f, 5.f, H)), false);
		Part(RootComponent, TrimPaint, OnWall(Spot + Out * 8.f - FVector::UpVector * (H * 0.5f + 6.f), Out, FVector(16.f, W + 30.f, 8.f)), false);
	}

	if (bDoorWall)
	{
		const float Post = 14.f;
		for (const float Way : { 1.f, -1.f })
		{
			const FVector Spot = Face + Along * (Way * (DoorWidth * 0.5f + Post * 0.5f)) + FVector::UpVector * (DoorTop * 0.5f);
			Part(RootComponent, TrimPaint, OnWall(Spot + Out * 3.f, Out, FVector(6.f, Post, DoorTop)), false);
		}
	}
}

void AGRRoomShells::NameBoard(USceneComponent* Group, const FVector& Center, const FVector& Half, const FShell& Shell)
{
	// The level's labels float above the walls, and those on north doors face into the room.
	for (TActorIterator<ATextRenderActor> It(GetWorld()); It; ++It)
	{
		const FVector From = It->GetActorLocation() - Center;
		if (FMath::Abs(From.X) > Half.X + 350.f || FMath::Abs(From.Y) > Half.Y + 350.f)
		{
			continue;
		}
		const FText Name = It->GetTextRender()->Text;
		It->SetActorHiddenInGame(true);

		const FVector Out = Shell.Door;
		const float Size = 46.f;
		const float BoardHigh = Size + 18.f;
		const float BoardWide = Name.ToString().Len() * Size * 0.62f + 60.f;
		const FVector Spot = Center + Out * Reach(Half, Out) + FVector::UpVector * (DoorTop + BoardHigh * 0.5f);
		Part(Group, BoardPaint, OnWall(Spot + Out * 5.f, Out, FVector(10.f, BoardWide, BoardHigh)), false);

		UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this);
		Text->SetupAttachment(Group);
		Text->SetRelativeLocationAndRotation(Spot + Out * 11.f, Out.Rotation());
		Text->SetText(Name);
		Text->SetWorldSize(Size);
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetTextRenderColor(FColor(255, 230, 170));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Text->SetCastShadow(false);
		Text->RegisterComponent();
		return;
	}
}

UStaticMeshComponent* AGRRoomShells::Part(USceneComponent* Parent, UMaterialInterface* Material, const FTransform& Transform, bool bShadow)
{
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetupAttachment(Parent);
	Mesh->SetStaticMesh(GRStoryUtil::LoadShape(TEXT("cube")));
	Mesh->SetMaterial(0, Material);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCastShadow(bShadow);
	Mesh->SetCullDistance(bShadow ? 0.f : PartCull);
	Mesh->SetRelativeTransform(Transform);
	Mesh->RegisterComponent();
	return Mesh;
}

void AGRRoomShells::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	const UGRUISubsystem* UI = LP ? LP->GetSubsystem<UGRUISubsystem>() : nullptr;
	const bool bCutscene = UI && Cast<UGRCutsceneScreen>(UI->GetTopScreen());
	const APlayerCameraManager* Camera = PC ? PC->PlayerCameraManager.Get() : nullptr;
	for (int32 i = 0; i < Rooms.Num(); ++i)
	{
		const AActor* Room = Rooms[i];
		// The pending target counts: the camera must not fly down into a solid roof.
		const bool bFromAbove = !bCutscene && Camera && Rooms[i]->HasPlayerInside() && (Camera->GetViewTarget() == Room || Camera->PendingViewTarget.Target == Room);
		FGRRoofParts& Parts = Roofs[i];
		// A cutscene cuts to its camera, where a roof still fading in would show.
		const float Fade = bCutscene ? 0.f : FMath::FInterpConstantTo(Parts.Fade, bFromAbove ? 1.f : 0.f, DeltaSeconds, 1.f / SeeThroughTime);
		if (Fade == Parts.Fade)
		{
			continue;
		}
		if ((Fade == 0.f) != (Parts.Fade == 0.f))
		{
			const bool bSolid = Fade == 0.f;
			for (UMeshComponent* Slope : Parts.Slopes)
			{
				Slope->SetMaterial(0, bSolid ? RoofPaint.Get() : static_cast<UMaterialInterface*>(Parts.SeeThrough));
			}
			for (USceneComponent* Other : Parts.Rest)
			{
				Other->SetVisibility(bSolid, true);
			}
		}
		Parts.SeeThrough->SetScalarParameterValue(TEXT("Opacity"), FMath::Lerp(1.f, SeeThroughOpacity, Fade));
		Parts.Fade = Fade;
	}
}
