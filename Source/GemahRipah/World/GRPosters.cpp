#include "World/GRPosters.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const TCHAR* const Billboards[] = {
		TEXT("BL01"), TEXT("BL02"), TEXT("BL03"), TEXT("BL04"), TEXT("BL05"), TEXT("BL06"), TEXT("BL07"), TEXT("BL08"), TEXT("BL09"),
	};
	const TCHAR* const Banners[] = { TEXT("SP01"), TEXT("SP02"), TEXT("SP03"), TEXT("SP04"), TEXT("SP05"), TEXT("SP06") };
	const TCHAR* const Walls[] = { TEXT("PS01"), TEXT("PS02"), TEXT("PS03"), TEXT("PS04") };

	struct FFace
	{
		/** Mesh space, cm. */
		FVector Center;
		FVector Out;
		FVector Up;
		float Width;
		float Height;
	};

	// Measured by ray casts in Blender on the exported FBX (Art/_ekspor_ue/Props/PropertiLuar); FBX import mirrors
	// Blender's Y, hence the minus signs. Each picture floats 1.5 cm off the surface.
	// P07: the panel is 3.9 m wide, 2.45 to 4.68 m up, leaning 7 degrees forward (a 0.55 m deep box).
	const FFace BoardFaces[] = {
		{ FVector(0.f, 62.4f, 356.5f), FVector(0.f, 1.f, -0.125f), FVector(0.f, 0.125f, 1.f), 390.f, 225.f },
		{ FVector(0.f, 5.4f, 370.f), FVector(0.f, -1.f, 0.125f), FVector(0.f, 0.125f, 1.f), 390.f, 200.f },
	};
	// P08: the cloth hangs between two poles, 2.35 m wide, 0.38 to 1.52 m up, a few cm of folds.
	const FFace BannerFaces[] = {
		{ FVector(0.f, 11.5f, 95.f), FVector(0.f, 1.f, 0.f), FVector(0.f, 0.f, 1.f), 235.f, 112.f },
		{ FVector(0.f, -6.5f, 95.f), FVector(0.f, -1.f, 0.f), FVector(0.f, 0.f, 1.f), 235.f, 112.f },
	};

	UMaterialInterface* LoadPicture(const FString& Image)
	{
		static TMap<FString, TWeakObjectPtr<UMaterialInterface>> Cache;
		if (TWeakObjectPtr<UMaterialInterface>* Found = Cache.Find(Image))
		{
			if (Found->IsValid())
			{
				return Found->Get();
			}
		}
		const FString Path = FString::Printf(TEXT("/Game/GemahRipah/Props/Baliho/MI_Poster_%s.MI_Poster_%s"), *Image, *Image);
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		Cache.Add(Image, Material);
		return Material;
	}
}

UStaticMeshComponent* GRPosters::Add(AActor* Owner, const FString& Image, const FVector& Center, const FVector& Out,
	const FVector& Up, float Width, float Height)
{
	static TWeakObjectPtr<UStaticMesh> PlaneRef;
	if (!PlaneRef.IsValid())
	{
		PlaneRef = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	}
	UStaticMesh* Plane = PlaneRef.Get();
	UMaterialInterface* Material = LoadPicture(Image);
	USceneComponent* Root = Owner ? Owner->GetRootComponent() : nullptr;
	if (!Plane || !Material || !Root)
	{
		return nullptr;
	}
	// The plane lies in its XY with +Z out; its texture runs along +X (left to right) and +Y (top to bottom).
	const FVector Normal = Out.GetSafeNormal();
	const FVector Top = (Up - Normal * FVector::DotProduct(Up, Normal)).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(Top, -Normal);
	const FMatrix Axes(Right, -Top, Normal, FVector::ZeroVector);

	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
	Comp->SetMobility(Root->Mobility);
	Comp->SetupAttachment(Root);
	Comp->SetStaticMesh(Plane);
	Comp->SetMaterial(0, Material);
	Comp->SetWorldTransform(FTransform(Axes.Rotator(), Center, FVector(Width / 100.f, Height / 100.f, 1.f)));
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCanEverAffectNavigation(false);
	Comp->SetCastShadow(false);
	Comp->RegisterComponent();
	return Comp;
}

void GRPosters::DressProp(AActor* Owner, const FString& Code, const FTransform& Transform, int32 Index)
{
	const bool bBoard = Code == TEXT("P07");
	if (!bBoard && Code != TEXT("P08"))
	{
		return;
	}
	const FFace* Faces = bBoard ? BoardFaces : BannerFaces;
	const int32 Count = bBoard ? UE_ARRAY_COUNT(Billboards) : UE_ARRAY_COUNT(Banners);
	const float Scale = Transform.GetScale3D().X;
	UE_LOG(LogTemp, Display, TEXT("GRPoster %s #%d di %.0f %.0f yaw %.0f"), *Code, Index, Transform.GetLocation().X,
		Transform.GetLocation().Y, Transform.Rotator().Yaw);
	for (int32 Side = 0; Side < 2; ++Side)
	{
		// The back shows a different picture from the front, about half the list further on.
		const int32 Pick = (Index + Side * (Count / 2 + 1)) % Count;
		const FFace& Face = Faces[Side];
		Add(Owner, bBoard ? Billboards[Pick] : Banners[Pick], Transform.TransformPosition(Face.Center),
			Transform.TransformVectorNoScale(Face.Out), Transform.TransformVectorNoScale(Face.Up), Face.Width * Scale, Face.Height * Scale);
	}
}

FString GRPosters::WallPoster(int32 Index)
{
	const int32 Count = UE_ARRAY_COUNT(Walls);
	return Walls[((Index % Count) + Count) % Count];
}
