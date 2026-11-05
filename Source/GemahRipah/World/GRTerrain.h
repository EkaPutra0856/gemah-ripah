#pragma once

#include "CoreMinimal.h"

/**
 * The shape of the land in L_KotaSentosa: a flat city core and flat roads, hills outside Jl. Lingkar,
 * a river around the south-west of town and a walled kali through Kampung Gang.
 * One source for the editor builder (heightmap, weightmaps, which buildings move) and for the runtime
 * (water surface, bridges, the phone map). +X is north, +Y is east; all values in cm.
 * Design: landscaping design notes (not included)
 */
namespace GRTerrain
{
	/** One water surface for the whole map; the land is above it everywhere except in the river and the kali. */
	constexpr float WaterZ = -250.f;
	/** Water level of the kali where it runs between its concrete walls, higher than the river. */
	constexpr float CreekWaterZ = -110.f;

	/** Landscape paint layers, in the order of the landscape material. */
	enum class ELayer : uint8
	{
		Rumput,
		RumputJarang,
		TanahMerah,
		Lumpur,
		BatuSungai,
		TepiSungai,
		Count
	};
	constexpr int32 LayerCount = int32(ELayer::Count);
	GEMAHRIPAH_API const TCHAR* LayerName(int32 Layer);

	struct FWaterway
	{
		/** Centerline, a point every few metres. */
		TArray<FVector2D> Points;
		/** Half the width of the water. */
		float HalfWidth = 0.f;
		float BedZ = 0.f;
		/** Distance from the centerline where the banks meet the land. */
		float BankOut = 0.f;
	};
	GEMAHRIPAH_API const FWaterway& River();
	GEMAHRIPAH_API const FWaterway& Creek();
	/** The kali has concrete walls (talud) where it runs between the kampung houses. */
	GEMAHRIPAH_API bool IsWalledCreek(const FVector2D& P);
	/** 2D distance from P to the waterway's centerline. */
	GEMAHRIPAH_API float DistanceTo(const FWaterway& Way, const FVector2D& P);

	enum class EBridge : uint8
	{
		/** Concrete road bridge with railings. */
		Beton,
		/** Green steel truss, the landmark on Jl. Merdeka. */
		Rangka,
		/** Steel-cable suspension footbridge for the kampung. */
		Gantung,
		/** Small concrete footbridge over the kali. */
		Kampung,
		/** Culvert headwall where the kali comes out under Jl. Merdeka. */
		Gorong,
	};
	struct FBridge
	{
		FVector2D Center;
		/** Direction of travel over the bridge. */
		FVector2D Along;
		/** Span along Along. */
		float Length = 0.f;
		/** Width across. */
		float Width = 0.f;
		EBridge Kind = EBridge::Beton;
		/** Road bridges keep the road's own slab as their deck; footbridges get their own. */
		bool bRoadDeck = false;
	};
	GEMAHRIPAH_API const TArray<FBridge>& Bridges();

	struct FArea
	{
		FVector2D Center;
		FVector2D Half;
		bool Contains(const FVector2D& P, float Margin = 0.f) const
		{
			return FMath::Abs(P.X - Center.X) < Half.X + Margin && FMath::Abs(P.Y - Center.Y) < Half.Y + Margin;
		}
	};
	/** Lapangan bola kampung, on the flat land by the river. */
	GEMAHRIPAH_API const FArea& Field();
	/** Lahan proyek mangkrak in the government blocks. */
	GEMAHRIPAH_API const FArea& Project();

	/** Height of the land without the building pads (the builder flattens those from the level). */
	GEMAHRIPAH_API float BaseHeight(const FVector2D& P);
	/** Weight of each paint layer at P (they sum to 1). SlopeDeg is the ground's tilt. */
	GEMAHRIPAH_API void LayerWeights(const FVector2D& P, float SlopeDeg, float Out[LayerCount]);
	/** True where P is in the river or the kali (in the water or on the low bank). */
	GEMAHRIPAH_API bool IsWater(const FVector2D& P, float Margin = 0.f);
	/** Whether a building on this footprint has to go: it stands in a waterway, the field or the project site. */
	GEMAHRIPAH_API bool BlocksBuilding(const FBox2D& Footprint);
	/** The flat plots kept for the story (rumah Bayu, pasar, sekolah, kelurahan, dewan, tugu, musala). */
	GEMAHRIPAH_API const TArray<FBox2D>& StoryPlots();
}
