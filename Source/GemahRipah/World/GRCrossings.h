#pragma once

#include "CoreMinimal.h"

/**
 * Zebra crossings on the two main roads. Townspeople cross only here (AGRWarga::NextTarget); AGRTraffic paints the
 * stripes. Road axis as in AGRCrowd::PavementAt: 0 = Jl. Raya (runs along Y, across is X), 1 = Jl. Merdeka (runs
 * along X, across is Y). Each crossing exists on both sides of the Tugu, on both roads.
 */
namespace GRCrossings
{
	/** Distance from the Tugu along the road (both directions). Clear of the roundabout approach (2800). */
	constexpr float Along[] = { 5000.f, 11000.f, 17000.f };
	/** Half the width of the striped band, along the road. */
	constexpr float HalfBand = 200.f;
	/** Asphalt half width (build_kota_sentosa.py: Jl_Raya / Jl_Merdeka are 1600 wide). */
	constexpr float RoadHalf = 800.f;

	/** The crossing nearest to At (signed distance along the road), signed the same way. */
	inline float Nearest(float At)
	{
		float Best = Along[0] * (At >= 0.f ? 1.f : -1.f);
		for (const float A : Along)
		{
			for (const float Sign : { -1.f, 1.f })
			{
				if (FMath::Abs(Sign * A - At) < FMath::Abs(Best - At))
				{
					Best = Sign * A;
				}
			}
		}
		return Best;
	}

	/** World point (cm) at AlongRoad on road Axis, Across from the road's center line. */
	inline FVector2D Point(int32 Axis, float AlongRoad, float Across)
	{
		return Axis == 0 ? FVector2D(Across, AlongRoad) : FVector2D(AlongRoad, Across);
	}
}
