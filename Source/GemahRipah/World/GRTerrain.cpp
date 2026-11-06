#include "World/GRTerrain.h"

namespace
{
	using namespace GRTerrain;

	float Smooth(float Edge0, float Edge1, float X)
	{
		const float T = FMath::Clamp((X - Edge0) / (Edge1 - Edge0), 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}

	float Hash(int32 X, int32 Y, int32 Seed)
	{
		uint32 H = uint32(X) * 374761393u + uint32(Y) * 668265263u + uint32(Seed) * 2246822519u;
		H = (H ^ (H >> 13)) * 1274126177u;
		H ^= H >> 16;
		return float(H & 0xFFFFFFu) / float(0xFFFFFFu);
	}

	/** Smooth value noise in 0..1 with features about Cell cm across. */
	float Noise(const FVector2D& P, float Cell, int32 Seed)
	{
		const double X = P.X / Cell;
		const double Y = P.Y / Cell;
		const int32 X0 = FMath::FloorToInt32(X);
		const int32 Y0 = FMath::FloorToInt32(Y);
		const float FX = float(X - X0);
		const float FY = float(Y - Y0);
		const float U = FX * FX * (3.f - 2.f * FX);
		const float V = FY * FY * (3.f - 2.f * FY);
		const float A = FMath::Lerp(Hash(X0, Y0, Seed), Hash(X0 + 1, Y0, Seed), U);
		const float B = FMath::Lerp(Hash(X0, Y0 + 1, Seed), Hash(X0 + 1, Y0 + 1, Seed), U);
		return FMath::Lerp(A, B, V);
	}

	/** Distance from P to a box (0 inside). */
	float BoxDistance(const FVector2D& P, const FVector2D& Center, const FVector2D& Half)
	{
		const FVector2D D(FMath::Max(FMath::Abs(P.X - Center.X) - Half.X, 0.0), FMath::Max(FMath::Abs(P.Y - Center.Y) - Half.Y, 0.0));
		return float(D.Size());
	}

	TArray<FVector2D> CatmullRom(const TArray<FVector2D>& Keys, float Step)
	{
		TArray<FVector2D> Out;
		for (int32 i = 0; i + 1 < Keys.Num(); ++i)
		{
			const FVector2D& P0 = Keys[FMath::Max(i - 1, 0)];
			const FVector2D& P1 = Keys[i];
			const FVector2D& P2 = Keys[i + 1];
			const FVector2D& P3 = Keys[FMath::Min(i + 2, Keys.Num() - 1)];
			const int32 Steps = FMath::Max(1, FMath::CeilToInt32(FVector2D::Distance(P1, P2) / Step));
			for (int32 s = 0; s < Steps; ++s)
			{
				const double T = double(s) / Steps;
				const double T2 = T * T;
				const double T3 = T2 * T;
				Out.Add(0.5 * ((2.0 * P1) + (P2 - P0) * T + (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * T2 + (3.0 * P1 - P0 - 3.0 * P2 + P3) * T3));
			}
		}
		Out.Add(Keys.Last());
		return Out;
	}

	/** Bounding boxes over runs of the centerline, so a distance query only looks at the near runs. */
	struct FChunks
	{
		static constexpr int32 Run = 16;
		TArray<FBox2D> Boxes;

		void Build(const TArray<FVector2D>& Points)
		{
			for (int32 Start = 0; Start + 1 < Points.Num(); Start += Run)
			{
				FBox2D Box(ForceInit);
				for (int32 i = Start; i <= FMath::Min(Start + Run, Points.Num() - 1); ++i)
				{
					Box += Points[i];
				}
				Boxes.Add(Box);
			}
		}
	};

	struct FWays
	{
		FWaterway River;
		FWaterway Creek;
		FChunks RiverChunks;
		FChunks CreekChunks;

		FWays()
		{
			// An L around the south-west of town, through the outer hills: west along y = -40000, a bend in the
			// corner, then east along x = -40000. It crosses every road square on, and -40000 is one row of the
			// 4000 cm building grid, so it takes out a single row of the outer houses.
			River.Points = CatmullRom({
				{ 64000.f, -40000.f }, { 44000.f, -40600.f }, { 27000.f, -40000.f }, { 13500.f, -39400.f }, { 0.f, -40000.f },
				{ -13500.f, -40600.f }, { -27000.f, -40000.f }, { -34500.f, -39600.f }, { -38800.f, -38800.f },
				{ -39600.f, -34500.f }, { -40000.f, -27000.f }, { -40600.f, -13500.f }, { -40000.f, 0.f }, { -39400.f, 13500.f },
				{ -40000.f, 27000.f }, { -40600.f, 44000.f }, { -40000.f, 64000.f } }, 400.f);
			River.HalfWidth = 1250.f;
			River.BedZ = -450.f;
			River.BankOut = 2600.f;

			// The kali comes out of a culvert under Jl. Merdeka, runs straight between the kampung houses
			// (in place of the row at x = -15000) and winds down to the river.
			Creek.Points = CatmullRom({
				{ -15000.f, -1700.f }, { -15000.f, -14000.f }, { -15000.f, -25600.f }, { -15000.f, -27000.f }, { -14400.f, -31000.f },
				{ -15600.f, -35000.f }, { -15000.f, -38500.f }, { -15000.f, -40800.f } }, 300.f);
			Creek.HalfWidth = 250.f;
			Creek.BedZ = -330.f;
			Creek.BankOut = 900.f;

			RiverChunks.Build(River.Points);
			CreekChunks.Build(Creek.Points);
		}
	};

	const FWays& Ways()
	{
		static const FWays Instance;
		return Instance;
	}

	float SegmentDistance(const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D AB = B - A;
		const double Len2 = AB.SizeSquared();
		const double T = Len2 > 0.0 ? FMath::Clamp(FVector2D::DotProduct(P - A, AB) / Len2, 0.0, 1.0) : 0.0;
		return float(FVector2D::Distance(P, A + AB * T));
	}

	float ChunkedDistance(const FWaterway& Way, const FChunks& Chunks, const FVector2D& P)
	{
		float Best = FLT_MAX;
		for (int32 c = 0; c < Chunks.Boxes.Num(); ++c)
		{
			const FBox2D& Box = Chunks.Boxes[c];
			if (FMath::Sqrt(Box.ComputeSquaredDistanceToPoint(P)) >= Best)
			{
				continue;
			}
			const int32 Start = c * FChunks::Run;
			const int32 End = FMath::Min(Start + FChunks::Run, Way.Points.Num() - 1);
			for (int32 i = Start; i < End; ++i)
			{
				Best = FMath::Min(Best, SegmentDistance(P, Way.Points[i], Way.Points[i + 1]));
			}
		}
		return Best;
	}

	float CrossAtX(const FWaterway& Way, float X)
	{
		for (int32 i = 0; i + 1 < Way.Points.Num(); ++i)
		{
			const FVector2D& A = Way.Points[i];
			const FVector2D& B = Way.Points[i + 1];
			if ((A.X - X) * (B.X - X) <= 0.0 && A.X != B.X)
			{
				return float(A.Y + (B.Y - A.Y) * (X - A.X) / (B.X - A.X));
			}
		}
		return 0.f;
	}

	float CrossAtY(const FWaterway& Way, float Y)
	{
		for (int32 i = 0; i + 1 < Way.Points.Num(); ++i)
		{
			const FVector2D& A = Way.Points[i];
			const FVector2D& B = Way.Points[i + 1];
			if ((A.Y - Y) * (B.Y - Y) <= 0.0 && A.Y != B.Y)
			{
				return float(A.X + (B.X - A.X) * (Y - A.Y) / (B.Y - A.Y));
			}
		}
		return 0.f;
	}

	struct FMound
	{
		FVector2D Offset;
		float Radius;
		float Height;
	};
	// Heaps of dug-up earth on the abandoned project site.
	const FMound Mounds[] = {
		{ { -1200.f, -900.f }, 700.f, 330.f }, { { 600.f, -1300.f }, 500.f, 220.f }, { { 1300.f, 700.f }, 900.f, 380.f },
		{ { -400.f, 1100.f }, 600.f, 250.f }, { { -1600.f, 1500.f }, 400.f, 170.f }, { { 200.f, 100.f }, 350.f, 140.f },
	};

	float CreekBankOut(const FVector2D& P)
	{
		return IsWalledCreek(P) ? 330.f : Ways().Creek.BankOut;
	}

	/** How much of Jl. Lingkar's ring P is outside of: 0 in town, 1 in the outer hills. */
	float RingAmount(const FVector2D& P)
	{
		return Smooth(28000.f, 40000.f, float(FMath::Max(FMath::Abs(P.X), FMath::Abs(P.Y))));
	}

	/** The rolling ground of the town, about -1..1: rises and hollows a few hundred metres across. */
	float Swell(const FVector2D& P)
	{
		return ((0.6f * Noise(P, 7000.f, 14) + 0.4f * Noise(P, 2800.f, 16)) - 0.5f) * 2.6f;
	}

	/** 1 on the main roads, Jl. Lingkar and the story plots, fading to 0 away from them. */
	float FlatAmount(const FVector2D& P)
	{
		const float AX = float(FMath::Abs(P.X));
		const float AY = float(FMath::Abs(P.Y));
		auto Flat = [](float D, float Keep, float Blend) { return 1.f - Smooth(Keep, Keep + Blend, D); };
		float Flatten = FMath::Max(Flat(AX, 1000.f, 1800.f), Flat(AY, 1000.f, 1800.f));
		for (const float Ring2 : { -27000.f, 27000.f })
		{
			Flatten = FMath::Max(Flatten, Flat(float(FMath::Abs(P.X - Ring2)), 600.f, 1500.f));
			Flatten = FMath::Max(Flatten, Flat(float(FMath::Abs(P.Y - Ring2)), 600.f, 1500.f));
		}
		for (const FBox2D& Plot : StoryPlots())
		{
			Flatten = FMath::Max(Flatten, Flat(BoxDistance(P, Plot.GetCenter(), Plot.GetExtent()), 400.f, 1200.f));
		}
		return Flatten;
	}

	/** Land before the field pad and the water: hills, the rolling core, flat roads and story plots, project heaps. */
	float NaturalHeight(const FVector2D& P, float RiverDistance)
	{
		const float Out = float(FMath::Max(FMath::Abs(P.X), FMath::Abs(P.Y)));

		// Hills rise outside Jl. Lingkar to 7-30 m, with crests and gullies (ridged noise), and a little more at the
		// very edge so the map ends in high ground.
		const float Ring = RingAmount(P);
		const float Edge = Smooth(47000.f, 58000.f, Out);
		const float Ridge = 1.f - FMath::Abs(Noise(P, 7000.f, 15) * 2.f - 1.f);
		const float Hill = 0.45f * Noise(P, 15000.f, 11) + 0.25f * Noise(P, 6000.f, 12) + 0.15f * Noise(P, 2500.f, 13) + 0.15f * Ridge * Ridge;
		const float HillZ = Ring * (400.f + 3600.f * FMath::Pow(Hill, 1.5f)) + Edge * 900.f;
		// The town rolls up and down by up to 4 m between its roads.
		const float CoreZ = (1.f - Ring) * 400.f * FMath::Clamp(Swell(P), -1.f, 1.f);
		// The hills fall away into a flat valley along the river.
		float Z = HillZ * Smooth(2400.f, 9000.f, RiverDistance) + CoreZ * Smooth(2600.f, 4000.f, RiverDistance);

		const float Flatten = FlatAmount(P);
		// Small bumps everywhere off the flat ground.
		const float Bumps = 45.f * (Noise(P, 1100.f, 17) - 0.5f) * 2.f + 18.f * (Noise(P, 380.f, 18) - 0.5f) * 2.f;
		Z = (Z + Bumps) * (1.f - Flatten);

		const FArea& Site = Project();
		if (Site.Contains(P, 1000.f))
		{
			const float Inside = 1.f - Smooth(0.f, 400.f, BoxDistance(P, Site.Center, Site.Half));
			for (const FMound& Mound : Mounds)
			{
				const float D = float(FVector2D::Distance(P, Site.Center + Mound.Offset));
				Z += Inside * Mound.Height * FMath::Exp(-FMath::Square(D / Mound.Radius));
			}
		}
		return Z;
	}

	float FieldHeight()
	{
		static const float Z = NaturalHeight(Field().Center, DistanceTo(River(), Field().Center));
		return Z;
	}

	/** Cuts a waterway into the land: a flat bed, a shore at Shore, banks up to the land. */
	float Carve(float Z, float D, float HalfWidth, float BankOut, float BedZ, float Shore = WaterZ + 30.f)
	{
		if (D >= BankOut)
		{
			return Z;
		}
		const float Cut = D <= HalfWidth ? FMath::Lerp(BedZ, Shore, Smooth(0.5f * HalfWidth, HalfWidth, D))
			: FMath::Lerp(Shore, Z, Smooth(HalfWidth, BankOut, D));
		return FMath::Min(Z, Cut);
	}
}

const TCHAR* GRTerrain::LayerName(int32 Layer)
{
	static const TCHAR* const Names[] = { TEXT("Rumput"), TEXT("RumputJarang"), TEXT("TanahMerah"), TEXT("Lumpur"), TEXT("BatuSungai"), TEXT("TepiSungai") };
	return Names[FMath::Clamp(Layer, 0, LayerCount - 1)];
}

const GRTerrain::FWaterway& GRTerrain::River()
{
	return Ways().River;
}

const GRTerrain::FWaterway& GRTerrain::Creek()
{
	return Ways().Creek;
}

bool GRTerrain::IsWalledCreek(const FVector2D& P)
{
	return P.Y > -25600.f;
}

float GRTerrain::DistanceTo(const FWaterway& Way, const FVector2D& P)
{
	const FWays& All = Ways();
	return ChunkedDistance(Way, &Way == &All.River ? All.RiverChunks : All.CreekChunks, P);
}

const TArray<GRTerrain::FBridge>& GRTerrain::Bridges()
{
	static const TArray<FBridge> List = []
	{
		const FWaterway& Big = River();
		const FWaterway& Kali = Creek();
		TArray<FBridge> Out;
		// Roads over the river: Jl. Raya and Jl. Lingkar run along Y, Jl. Merdeka and the other Lingkar along X.
		Out.Add({ FVector2D(0.f, CrossAtX(Big, 0.f)), FVector2D(0.f, 1.f), 6400.f, 2400.f, EBridge::Beton, true });
		for (const float X : { 27000.f, -27000.f })
		{
			Out.Add({ FVector2D(X, CrossAtX(Big, X)), FVector2D(0.f, 1.f), 6000.f, 1000.f, EBridge::Beton, true });
		}
		Out.Add({ FVector2D(CrossAtY(Big, 0.f), 0.f), FVector2D(1.f, 0.f), 6400.f, 2400.f, EBridge::Rangka, true });
		for (const float Y : { -27000.f, 27000.f })
		{
			Out.Add({ FVector2D(CrossAtY(Big, Y), Y), FVector2D(1.f, 0.f), 6000.f, 1000.f, EBridge::Beton, true });
		}
		// The kali under Jl. Lingkar, and its culvert at Jl. Merdeka.
		Out.Add({ FVector2D(CrossAtY(Kali, -27000.f), -27000.f), FVector2D(1.f, 0.f), 2000.f, 1000.f, EBridge::Beton, true });
		Out.Add({ FVector2D(-15000.f, -1450.f), FVector2D(0.f, 1.f), 100.f, 900.f, EBridge::Gorong, false });
		// Gang footbridges over the kali, in the alleys between the house rows.
		for (const float Y : { -6250.f, -10750.f, -15250.f, -19750.f })
		{
			Out.Add({ FVector2D(-15000.f, Y), FVector2D(1.f, 0.f), 900.f, 160.f, EBridge::Kampung, false });
		}
		// A suspension footbridge from the outer kampung across the river.
		// x = -10000 is between two columns of the outer kampung grid, so no house stands on its landings.
		Out.Add({ FVector2D(-10000.f, CrossAtX(Big, -10000.f)), FVector2D(0.f, 1.f), 7600.f, 220.f, EBridge::Gantung, false });
		return Out;
	}();
	return List;
}

const GRTerrain::FArea& GRTerrain::Field()
{
	static const FArea Area{ FVector2D(-32750.f, -14000.f), FVector2D(1300.f, 2100.f) };
	return Area;
}

const GRTerrain::FArea& GRTerrain::Project()
{
	static const FArea Area{ FVector2D(18000.f, -18000.f), FVector2D(2500.f, 2500.f) };
	return Area;
}

const TArray<FBox2D>& GRTerrain::StoryPlots()
{
	// The reserved plots of Tools/Python/build_kota_sentosa.py (x0, x1, y0, y1).
	static const TArray<FBox2D> Plots = []
	{
		const float R[][4] = {
			{ -9900.f, -6800.f, -10000.f, -7600.f },    // rumah Bayu + halaman
			{ -3000.f, -2000.f, 5000.f, 7400.f },       // warnet
			{ -18000.f, -6000.f, 8000.f, 18000.f },     // pasar
			{ 6000.f, 24000.f, 6000.f, 26000.f },       // sekolah, dapur, puskesmas
			{ 5000.f, 12000.f, -14000.f, -3000.f },     // kelurahan, pos SPN
			{ 33000.f, 50000.f, -9000.f, 9000.f },      // gedung dewan and its plaza
			{ -3500.f, 3500.f, -3500.f, 3500.f },       // tugu
			{ -14500.f, -10500.f, -22500.f, -18500.f }, // musala
			{ -6600.f, -5400.f, -13100.f, -11400.f },   // pos ronda and its warung
		};
		TArray<FBox2D> Out;
		for (const auto& B : R)
		{
			Out.Add(FBox2D(FVector2D(B[0], B[2]), FVector2D(B[1], B[3])));
		}
		return Out;
	}();
	return Plots;
}

float GRTerrain::BaseHeight(const FVector2D& P)
{
	const float RiverD = DistanceTo(River(), P);
	float Z = NaturalHeight(P, RiverD);

	// The field: a level pitch on the valley terrace, dished a little in the middle.
	const FArea& Pitch = Field();
	const float FieldD = BoxDistance(P, Pitch.Center, Pitch.Half + FVector2D(200.f));
	if (FieldD < 1500.f)
	{
		const float UX = float(FMath::Clamp((P.X - Pitch.Center.X) / Pitch.Half.X, -1.0, 1.0));
		const float UY = float(FMath::Clamp((P.Y - Pitch.Center.Y) / Pitch.Half.Y, -1.0, 1.0));
		const float Dish = 20.f * (1.f - UX * UX) * (1.f - UY * UY);
		Z = FMath::Lerp(Z, FieldHeight() - Dish, 1.f - Smooth(0.f, 1500.f, FieldD));
	}

	Z = Carve(Z, RiverD, River().HalfWidth, River().BankOut, River().BedZ);
	const float CreekD = DistanceTo(Creek(), P);
	if (CreekD < Creek().BankOut)
	{
		// Between the walls the kali is a shallow concrete channel with its own water level (CreekWaterZ).
		Z = IsWalledCreek(P) ? Carve(Z, CreekD, Creek().HalfWidth, CreekBankOut(P), CreekWaterZ - 80.f, CreekWaterZ - 80.f)
			: Carve(Z, CreekD, Creek().HalfWidth, CreekBankOut(P), Creek().BedZ);
	}
	return Z;
}

void GRTerrain::LayerWeights(const FVector2D& P, float SlopeDeg, float Out[LayerCount])
{
	for (int32 i = 0; i < LayerCount; ++i)
	{
		Out[i] = 0.f;
	}
	Out[int32(ELayer::Rumput)] = 1.f;
	auto Over = [Out](ELayer Layer, float Alpha)
	{
		Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
		for (int32 i = 0; i < LayerCount; ++i)
		{
			Out[i] *= 1.f - Alpha;
		}
		Out[int32(Layer)] += Alpha;
	};

	const float AX = float(FMath::Abs(P.X));
	const float AY = float(FMath::Abs(P.Y));
	// Thin grass in patches, on steep ground, along the roads and all over the trodden kampung.
	Over(ELayer::RumputJarang, (Noise(P, 1700.f, 21) - 0.52f) * 3.5f * 0.85f);
	Over(ELayer::RumputJarang, Smooth(14.f, 26.f, SlopeDeg));
	Over(ELayer::RumputJarang, 0.7f * (1.f - Smooth(1300.f, 2600.f, FMath::Min(AX, AY))));
	if (P.X < 0.f && P.Y < 0.f && FMath::Max(AX, AY) < 26000.f)
	{
		Over(ELayer::RumputJarang, 0.45f + 0.3f * Noise(P, 900.f, 22));
	}

	// Bare red earth in patches, mud in the hollows of the town, stones on the steep hillsides.
	const float Open = 1.f - FlatAmount(P);
	Over(ELayer::TanahMerah, (Noise(P, 2300.f, 31) - 0.70f) * 3.2f * Open);
	Over(ELayer::Lumpur, 0.55f * Smooth(-0.35f, -0.65f, Swell(P)) * (1.f - RingAmount(P)) * Open);
	Over(ELayer::BatuSungai, 0.85f * Smooth(28.f, 40.f, SlopeDeg));

	const FArea& Pitch = Field();
	Over(ELayer::TanahMerah, 1.f - Smooth(0.f, 250.f, BoxDistance(P, Pitch.Center, Pitch.Half)));
	const FArea& Site = Project();
	Over(ELayer::Lumpur, 1.f - Smooth(0.f, 500.f + 400.f * Noise(P, 700.f, 23), BoxDistance(P, Site.Center, Site.Half)));

	const FWaterway& Big = River();
	const float RiverD = DistanceTo(Big, P);
	Over(ELayer::TepiSungai, 1.f - Smooth(Big.BankOut - 300.f, Big.BankOut + 400.f, RiverD));
	Over(ELayer::BatuSungai, 1.f - Smooth(Big.HalfWidth * 0.75f, Big.HalfWidth, RiverD));
	const FWaterway& Kali = Creek();
	const float CreekD = DistanceTo(Kali, P);
	const float KaliBank = CreekBankOut(P);
	Over(ELayer::TepiSungai, 1.f - Smooth(KaliBank - 100.f, KaliBank + 250.f, CreekD));
	Over(ELayer::BatuSungai, 1.f - Smooth(Kali.HalfWidth * 0.7f, Kali.HalfWidth, CreekD));
}

bool GRTerrain::IsWater(const FVector2D& P, float Margin)
{
	return DistanceTo(River(), P) < River().HalfWidth + Margin || DistanceTo(Creek(), P) < Creek().HalfWidth + Margin;
}

bool GRTerrain::BlocksBuilding(const FBox2D& Footprint)
{
	const FVector2D C = Footprint.GetCenter();
	const FVector2D H = Footprint.GetExtent();
	float Nearest[2] = { FLT_MAX, FLT_MAX };
	for (int32 i = -1; i <= 1; ++i)
	{
		for (int32 j = -1; j <= 1; ++j)
		{
			const FVector2D P = C + FVector2D(H.X * i, H.Y * j);
			Nearest[0] = FMath::Min(Nearest[0], DistanceTo(River(), P));
			Nearest[1] = FMath::Min(Nearest[1], DistanceTo(Creek(), P));
		}
	}
	if (Nearest[0] < River().BankOut + 300.f || Nearest[1] < CreekBankOut(C) + 300.f)
	{
		return true;
	}
	const FArea& Pitch = Field();
	const FArea& Site = Project();
	if (BoxDistance(C, Pitch.Center, Pitch.Half + H) < 500.f || BoxDistance(C, Site.Center, Site.Half + H) < 300.f)
	{
		return true;
	}
	// Nothing on the landings of the suspension bridge.
	for (const FBridge& Bridge : Bridges())
	{
		if (Bridge.Kind == EBridge::Gantung)
		{
			const FVector2D Across(-Bridge.Along.Y, Bridge.Along.X);
			const FVector2D D = C - Bridge.Center;
			if (FMath::Abs(FVector2D::DotProduct(D, Bridge.Along)) < Bridge.Length * 0.5f + 700.f + H.GetMax()
				&& FMath::Abs(FVector2D::DotProduct(D, Across)) < Bridge.Width * 0.5f + 400.f + H.GetMax())
			{
				return true;
			}
		}
	}
	return false;
}
