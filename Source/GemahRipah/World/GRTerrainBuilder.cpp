// Editor-only: builds the landscape of L_KotaSentosa from GRTerrain (Tools/Python/build_terrain.py calls it).
#include "Core/GRScriptLibrary.h"

#if WITH_EDITOR
#include "Async/ParallelFor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "World/GRTerrain.h"

namespace
{
	// 2017 x 2017 vertices, 60 cm apart: 1209.6 m, the size of the old ground plane. 16 x 16 components of 2 x 63 quads.
	constexpr int32 Verts = 2017;
	constexpr float Spacing = 60.f;
	constexpr float Origin = -0.5f * Spacing * (Verts - 1);
	constexpr int32 SectionQuads = 63;
	constexpr int32 SectionsPerComponent = 2;

	float Smooth(float Edge0, float Edge1, float X)
	{
		const float T = FMath::Clamp((X - Edge0) / (Edge1 - Edge0), 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}

	float BoxDistance(const FVector2D& P, const FBox2D& Box)
	{
		const FVector2D C = Box.GetCenter();
		const FVector2D H = Box.GetExtent();
		const FVector2D D(FMath::Max(FMath::Abs(P.X - C.X) - H.X, 0.0), FMath::Max(FMath::Abs(P.Y - C.Y) - H.Y, 0.0));
		return float(D.Size());
	}

	FVector2D VertexAt(int32 X, int32 Y)
	{
		return FVector2D(Origin + X * Spacing, Origin + Y * Spacing);
	}

	struct FGrid
	{
		TArray<float> Z;
		/** Vertices in a waterway or on its banks: pads never fill them in. */
		TBitArray<> Wet;

		/** Pulls the land towards Target: fully within Inner of the box, fading out over Blend. */
		void Pad(const FBox2D& Box, float Target, float Inner, float Blend, float Strength)
		{
			const float Reach = Inner + Blend;
			const int32 X0 = FMath::Clamp(FMath::FloorToInt32((Box.Min.X - Reach - Origin) / Spacing), 0, Verts - 1);
			const int32 X1 = FMath::Clamp(FMath::CeilToInt32((Box.Max.X + Reach - Origin) / Spacing), 0, Verts - 1);
			const int32 Y0 = FMath::Clamp(FMath::FloorToInt32((Box.Min.Y - Reach - Origin) / Spacing), 0, Verts - 1);
			const int32 Y1 = FMath::Clamp(FMath::CeilToInt32((Box.Max.Y + Reach - Origin) / Spacing), 0, Verts - 1);
			for (int32 Y = Y0; Y <= Y1; ++Y)
			{
				for (int32 X = X0; X <= X1; ++X)
				{
					const int32 I = Y * Verts + X;
					if (Wet[I])
					{
						continue;
					}
					const float Weight = Strength * (1.f - Smooth(Inner, Reach, BoxDistance(VertexAt(X, Y), Box)));
					Z[I] = FMath::Lerp(Z[I], Target, Weight);
				}
			}
		}

		float Sample(const FVector2D& P) const
		{
			const float FX = FMath::Clamp(float((P.X - Origin) / Spacing), 0.f, float(Verts - 1));
			const float FY = FMath::Clamp(float((P.Y - Origin) / Spacing), 0.f, float(Verts - 1));
			const int32 X = FMath::Min(int32(FX), Verts - 2);
			const int32 Y = FMath::Min(int32(FY), Verts - 2);
			const float U = FX - X;
			const float V = FY - Y;
			const float A = FMath::Lerp(Z[Y * Verts + X], Z[Y * Verts + X + 1], U);
			const float B = FMath::Lerp(Z[(Y + 1) * Verts + X], Z[(Y + 1) * Verts + X + 1], U);
			return FMath::Lerp(A, B, V);
		}
	};

	bool IsGenericBuilding(const FString& Label)
	{
		for (const TCHAR* Prefix : { TEXT("Rumah_"), TEXT("Toko_"), TEXT("Gedung_NE_"), TEXT("Gedung_NW_"), TEXT("Ruko_") })
		{
			if (Label.StartsWith(Prefix))
			{
				return true;
			}
		}
		return false;
	}

	ULandscapeLayerInfoObject* LayerInfo(const FString& Name)
	{
		const FString Path = TEXT("/Game/GemahRipah/Landscape/LI_") + Name;
		const FString ObjectPath = Path + TEXT(".LI_") + Name;
		if (ULandscapeLayerInfoObject* Found = LoadObject<ULandscapeLayerInfoObject>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			return Found;
		}
		UPackage* Package = CreatePackage(*Path);
		ULandscapeLayerInfoObject* Info = NewObject<ULandscapeLayerInfoObject>(Package, *(TEXT("LI_") + Name), RF_Public | RF_Standalone | RF_Transactional);
		Info->SetLayerName(FName(*Name), false);
		FAssetRegistryModule::AssetCreated(Info);
		Package->MarkPackageDirty();
		return Info;
	}

	struct FBuilding
	{
		AStaticMeshActor* Actor;
		FBox Bounds;
	};
}
#endif

FString UGRScriptLibrary::BuildTerrain(UObject* WorldContext, const TArray<FVector2D>& KeepFlat, const FString& LandscapeMaterial,
	const FString& FoundationMaterial)
{
#if WITH_EDITOR
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UMaterialInterface* Ground = LoadObject<UMaterialInterface>(nullptr, *LandscapeMaterial);
	UMaterialInterface* Stone = LoadObject<UMaterialInterface>(nullptr, *FoundationMaterial);
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!World || !Ground || !Stone || !Cube)
	{
		return TEXT("GAGAL: world, material, atau cube tidak ada");
	}

	// A rebuild starts over: the old landscape and foundations go, the buildings are placed again from their footprints.
	TArray<AActor*> Doomed;
	for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
	{
		Doomed.Add(*It);
	}
	TArray<FBuilding> Buildings;
	TArray<FBox2D> Grounded;
	TArray<FString> Removed;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		AStaticMeshActor* Actor = *It;
		UStaticMeshComponent* Comp = Actor->GetStaticMeshComponent();
		if (!Comp || !Comp->GetStaticMesh())
		{
			continue;
		}
		const FString Label = Actor->GetActorLabel();
		if (Actor->Tags.Contains(TEXT("GRPondasi")) || Label == TEXT("Tanah"))
		{
			Doomed.Add(Actor);
			continue;
		}
		const FBox Bounds = Comp->Bounds.GetBox();
		const FBox2D Print(FVector2D(Bounds.Min), FVector2D(Bounds.Max));
		const FVector2D Center = Print.GetCenter();
		if (IsGenericBuilding(Label))
		{
			if (GRTerrain::BlocksBuilding(Print))
			{
				Removed.Add(Label);
				Doomed.Add(Actor);
			}
			else
			{
				Buildings.Add({ Actor, Bounds });
			}
			continue;
		}
		// Roads, pavements and plazas are long; everything else on the ground keeps its level ground, except
		// lamps and trees that the river now runs through.
		if (Print.GetSize().GetMax() > 6000.0 || Bounds.Min.Z < -60.f || Bounds.Min.Z > 60.f)
		{
			continue;
		}
		if (GRTerrain::DistanceTo(GRTerrain::River(), Center) < GRTerrain::River().BankOut + 100.f
			|| GRTerrain::DistanceTo(GRTerrain::Creek(), Center) < GRTerrain::Creek().HalfWidth + 250.f)
		{
			Removed.Add(Label);
			Doomed.Add(Actor);
			continue;
		}
		Grounded.Add(Print);
	}
	// World Partition keeps each actor in its own package; a commandlet save does not delete those files, so the
	// script deletes them after saving.
	TArray<FString> DeadFiles;
	for (AActor* Actor : Doomed)
	{
		if (Actor->IsPackageExternal())
		{
			DeadFiles.Add(FPackageName::LongPackageNameToFilename(Actor->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()));
		}
		World->EditorDestroyActor(Actor, true);
	}

	// Heights from the terrain shape.
	FGrid Grid;
	Grid.Z.SetNumZeroed(Verts * Verts);
	Grid.Wet.Init(false, Verts * Verts);
	TArray<uint8> WetRows;
	WetRows.SetNumZeroed(Verts * Verts);
	ParallelFor(Verts, [&](int32 Y)
	{
		for (int32 X = 0; X < Verts; ++X)
		{
			const FVector2D P = VertexAt(X, Y);
			Grid.Z[Y * Verts + X] = GRTerrain::BaseHeight(P);
			WetRows[Y * Verts + X] = GRTerrain::DistanceTo(GRTerrain::River(), P) < GRTerrain::River().BankOut
				|| GRTerrain::DistanceTo(GRTerrain::Creek(), P) < (GRTerrain::IsWalledCreek(P) ? 330.f : GRTerrain::Creek().BankOut);
		}
	});
	for (int32 I = 0; I < WetRows.Num(); ++I)
	{
		Grid.Wet[I] = WetRows[I] != 0;
	}

	// Story spots and everything standing on the ground keep level ground at 0.
	for (const FVector2D& Spot : KeepFlat)
	{
		Grid.Pad(FBox2D(Spot, Spot), 0.f, 1200.f, 1000.f, 1.f);
	}
	for (const FBox2D& Print : Grounded)
	{
		Grid.Pad(Print, 0.f, 150.f, 600.f, 1.f);
	}

	// Buildings: on level ground they stay put; on a slope they rise onto a stone foundation.
	const FBox2D Field(GRTerrain::Field().Center - GRTerrain::Field().Half, GRTerrain::Field().Center + GRTerrain::Field().Half);
	int32 Lifted = 0;
	float Highest = 0.f;
	for (const FBuilding& Building : Buildings)
	{
		const FBox2D Print(FVector2D(Building.Bounds.Min), FVector2D(Building.Bounds.Max));
		float Sum = 0.f;
		float Low = FLT_MAX;
		float High = -FLT_MAX;
		for (int32 i = -1; i <= 1; ++i)
		{
			for (int32 j = -1; j <= 1; ++j)
			{
				const float Z = Grid.Sample(Print.GetCenter() + Print.GetExtent() * FVector2D(i, j));
				Sum += Z;
				Low = FMath::Min(Low, Z);
				High = FMath::Max(High, Z);
			}
		}
		const float Mean = Sum / 9.f;
		if (High < 60.f && Low > -40.f)
		{
			Grid.Pad(Print, 0.f, 200.f, 600.f, 1.f);
			if (FMath::Abs(Building.Bounds.Min.Z) > 1.f)
			{
				Building.Actor->Modify();
				Building.Actor->SetActorLocation(Building.Actor->GetActorLocation() - FVector(0.f, 0.f, Building.Bounds.Min.Z));
			}
			continue;
		}
		// Half-level the plot (cut and fill), then stand the house 40 cm above its highest corner.
		Grid.Pad(Print, Mean, 300.f, 700.f, 0.6f);
		Low = FLT_MAX;
		High = -FLT_MAX;
		for (int32 i = -1; i <= 1; ++i)
		{
			for (int32 j = -1; j <= 1; ++j)
			{
				const float Z = Grid.Sample(Print.GetCenter() + Print.GetExtent() * FVector2D(i, j));
				Low = FMath::Min(Low, Z);
				High = FMath::Max(High, Z);
			}
		}
		const float Floor = High + 40.f;
		AStaticMeshActor* Actor = Building.Actor;
		Actor->Modify();
		Actor->SetActorLocation(Actor->GetActorLocation() + FVector(0.f, 0.f, Floor - Building.Bounds.Min.Z));

		const float Bottom = Low - 80.f;
		const FVector Size(Print.GetSize().X + 60.f, Print.GetSize().Y + 60.f, Floor - Bottom);
		const FVector Where(Print.GetCenter().X, Print.GetCenter().Y, (Floor + Bottom) * 0.5f);
		FActorSpawnParameters Params;
		Params.Name = MakeUniqueObjectName(World->PersistentLevel, AStaticMeshActor::StaticClass(), *(TEXT("Pondasi_") + Actor->GetActorLabel()));
		AStaticMeshActor* Base = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FRotator::ZeroRotator, Where, Size / 100.f), Params);
		Base->GetStaticMeshComponent()->SetStaticMesh(Cube);
		Base->GetStaticMeshComponent()->SetMaterial(0, Stone);
		Base->SetActorLabel(TEXT("Pondasi_") + Actor->GetActorLabel());
		Base->SetFolderPath(TEXT("Bangunan/Pondasi"));
		Base->Tags.Add(TEXT("GRPondasi"));
		++Lifted;
		Highest = FMath::Max(Highest, Floor);
	}

	// Paint layers from the final shape.
	TArray<TArray<uint8>> Weights;
	Weights.SetNum(GRTerrain::LayerCount);
	for (TArray<uint8>& Layer : Weights)
	{
		Layer.SetNumZeroed(Verts * Verts);
	}
	ParallelFor(Verts, [&](int32 Y)
	{
		for (int32 X = 0; X < Verts; ++X)
		{
			const int32 I = Y * Verts + X;
			const float DX = Grid.Z[Y * Verts + FMath::Min(X + 1, Verts - 1)] - Grid.Z[Y * Verts + FMath::Max(X - 1, 0)];
			const float DY = Grid.Z[FMath::Min(Y + 1, Verts - 1) * Verts + X] - Grid.Z[FMath::Max(Y - 1, 0) * Verts + X];
			const float Slope = FMath::RadiansToDegrees(FMath::Atan(FMath::Sqrt(DX * DX + DY * DY) / (2.f * Spacing)));
			float W[GRTerrain::LayerCount];
			GRTerrain::LayerWeights(VertexAt(X, Y), Slope, W);
			int32 Total = 0;
			int32 Biggest = 0;
			for (int32 L = 0; L < GRTerrain::LayerCount; ++L)
			{
				const int32 V = FMath::RoundToInt32(FMath::Clamp(W[L], 0.f, 1.f) * 255.f);
				Weights[L][I] = uint8(V);
				Total += V;
				Biggest = W[L] > W[Biggest] ? L : Biggest;
			}
			Weights[Biggest][I] = uint8(FMath::Clamp(int32(Weights[Biggest][I]) + 255 - Total, 0, 255));
		}
	});

	// The landscape itself.
	TArray<uint16> Heights;
	Heights.SetNumUninitialized(Verts * Verts);
	float MinZ = FLT_MAX;
	float MaxZ = -FLT_MAX;
	for (int32 I = 0; I < Heights.Num(); ++I)
	{
		MinZ = FMath::Min(MinZ, Grid.Z[I]);
		MaxZ = FMath::Max(MaxZ, Grid.Z[I]);
		// Z scale 100: one height unit is 100/128 cm, 32768 is zero.
		Heights[I] = uint16(FMath::Clamp(FMath::RoundToInt32(32768.f + Grid.Z[I] * 1.28f), 0, 65535));
	}

	ALandscape* Landscape = World->SpawnActor<ALandscape>(ALandscape::StaticClass(),
		FTransform(FRotator::ZeroRotator, FVector(Origin, Origin, 0.f), FVector(Spacing, Spacing, 100.f)));
	// The landscape root has a default scale of its own (128, 128, 256), which the spawn transform multiplies.
	Landscape->SetActorRelativeScale3D(FVector(Spacing, Spacing, 100.f));
	Landscape->SetActorLocation(FVector(Origin, Origin, 0.f));
	Landscape->SetActorLabel(TEXT("Tanah_Landscape"));
	Landscape->LandscapeMaterial = Ground;
	TArray<FLandscapeImportLayerInfo> Layers;
	for (int32 L = 0; L < GRTerrain::LayerCount; ++L)
	{
		const FString Name = GRTerrain::LayerName(L);
		ULandscapeLayerInfoObject* Info = LayerInfo(Name);
		Landscape->AddTargetLayer(FName(*Name), FLandscapeTargetLayerSettings(Info), false);
		FLandscapeImportLayerInfo& Import = Layers.Emplace_GetRef(FName(*Name));
		Import.LayerInfo = Info;
		Import.LayerData = MoveTemp(Weights[L]);
	}
	TMap<FGuid, TArray<uint16>> HeightData;
	HeightData.Add(FGuid(), MoveTemp(Heights));
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> LayerData;
	LayerData.Add(FGuid(), MoveTemp(Layers));
	Landscape->Import(FGuid::NewGuid(), 0, 0, Verts - 1, Verts - 1, SectionsPerComponent, SectionQuads, HeightData, nullptr, LayerData,
		ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());
	if (ULandscapeInfo* Info = Landscape->GetLandscapeInfo())
	{
		Info->UpdateLayerInfoMap(Landscape);
	}

	return FString::Printf(TEXT("OK: landscape %d komponen (skala %s), tinggi %.0f..%.0f cm; %d bangunan di lereng diberi pondasi (tertinggi %.0f cm); %d dihapus: %s"),
		Landscape->LandscapeComponents.Num(), *Landscape->GetActorScale3D().ToString(), MinZ, MaxZ, Lifted, Highest, Removed.Num(), *FString::Join(Removed, TEXT(", "))) + TEXT("|FILES|") + FString::Join(DeadFiles, TEXT(";"));
#else
	return TEXT("GAGAL: hanya di editor");
#endif
}
