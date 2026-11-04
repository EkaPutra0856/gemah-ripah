#include "World/GRPropModels.h"
#include "Story/GRStoryUtil.h"
#include "Math/RandomStream.h"

namespace
{
	FLinearColor Hex(const TCHAR* Code)
	{
		return GRStoryUtil::ParseColor(Code);
	}

	/** Appends parts; bCollide / bRoof apply to every part added after they are set. */
	struct FParts
	{
		TArray<FGRPropPart>& Out;
		bool bCollide = true;
		bool bRoof = false;
		uint8 Group = 0;

		FGRPropPart& Add(const TCHAR* Shape, const FVector& Loc, const FVector& Size, const FLinearColor& Color, const FRotator& Rot)
		{
			FGRPropPart& Part = Out.AddDefaulted_GetRef();
			Part.Shape = Shape;
			Part.Loc = Loc;
			Part.Size = Size;
			Part.Color = Color;
			Part.Rot = Rot;
			Part.bCollide = bCollide;
			Part.bRoof = bRoof;
			Part.Group = Group;
			return Part;
		}
		FGRPropPart& Box(const FVector& Loc, const FVector& Size, const FLinearColor& Color, const FRotator& Rot = FRotator::ZeroRotator)
		{
			return Add(TEXT("cube"), Loc, Size, Color, Rot);
		}
		FGRPropPart& Cyl(const FVector& Loc, const FVector& Size, const FLinearColor& Color, const FRotator& Rot = FRotator::ZeroRotator)
		{
			return Add(TEXT("cylinder"), Loc, Size, Color, Rot);
		}
		FGRPropPart& Ball(const FVector& Loc, const FVector& Size, const FLinearColor& Color)
		{
			return Add(TEXT("sphere"), Loc, Size, Color, FRotator::ZeroRotator);
		}
		/** Four legs under a top with footprint Top (x, y), each Height tall, Inset from the edges. */
		void Legs(const FVector& Top, float Height, float Inset, float Thick, const FLinearColor& Color)
		{
			for (int32 SX = -1; SX <= 1; SX += 2)
			{
				for (int32 SY = -1; SY <= 1; SY += 2)
				{
					Box(FVector(SX * (Top.X * 0.5f - Inset), SY * (Top.Y * 0.5f - Inset), Height * 0.5f), FVector(Thick, Thick, Height), Color);
				}
			}
		}
		/** Monitor and keyboard on a desk top at height Z; the screen faces the user on the -X side. */
		void Computer(float DeskDepth, float Z, const FLinearColor& Dark, const FLinearColor& Screen)
		{
			const bool bWasColliding = bCollide;
			bCollide = false;
			const float Back = DeskDepth * 0.5f - 16.f;
			Box(FVector(Back, 0.f, Z + 1.f), FVector(16.f, 22.f, 2.f), Dark);
			Box(FVector(Back, 0.f, Z + 8.f), FVector(5.f, 5.f, 12.f), Dark);
			Box(FVector(Back - 2.f, 0.f, Z + 30.f), FVector(4.f, 54.f, 32.f), Dark);
			Box(FVector(Back - 4.3f, 0.f, Z + 30.f), FVector(0.6f, 50.f, 28.f), Screen);
			Box(FVector(-DeskDepth * 0.5f + 18.f, 0.f, Z + 1.f), FVector(14.f, 42.f, 2.f), Dark);
			Box(FVector(-DeskDepth * 0.5f + 20.f, 30.f, Z + 1.2f), FVector(8.f, 5.f, 2.5f), Dark);
			bCollide = bWasColliding;
		}
	};
}

bool GRPropModels::Build(const FString& Model, const TArray<float>& Size, const FString& Color, uint32 Seed, TArray<FGRPropPart>& Out)
{
	const FLinearColor Wood = Hex(TEXT("#8A5A3B"));
	const FLinearColor WoodLight = Hex(TEXT("#B4835A"));
	const FLinearColor WoodDark = Hex(TEXT("#5A3A26"));
	const FLinearColor Metal = Hex(TEXT("#9AA1A8"));
	const FLinearColor MetalDark = Hex(TEXT("#3E444B"));
	const FLinearColor Black = Hex(TEXT("#24272C"));
	const FLinearColor Paper = Hex(TEXT("#F3F0E6"));
	const FLinearColor Screen = Hex(TEXT("#4F86BD"));
	const FLinearColor White = Hex(TEXT("#F2F2F2"));

	// The spawn default color means "not set": keep the model's own colors.
	const bool bCustom = !Color.IsEmpty() && Color != TEXT("#B0A090");
	auto Main = [&](const FLinearColor& Default) { return bCustom ? GRStoryUtil::ParseColor(Color) : Default; };
	auto SizeOr = [&](float X, float Y, float Z)
	{
		return FVector(Size.IsValidIndex(0) ? Size[0] : X, Size.IsValidIndex(1) ? Size[1] : Y, Size.IsValidIndex(2) ? Size[2] : Z);
	};
	FRandomStream Rng(static_cast<int32>(Seed));
	FParts P{ Out };

	// Seats: origin under the sitter, who faces +X; the seat sits a little behind the feet.
	constexpr float SeatX = -8.f;
	constexpr float SeatZ = 44.f;

	if (Model == TEXT("meja") || Model == TEXT("meja_makan"))
	{
		const FVector S = SizeOr(70.f, 130.f, 75.f);
		P.Box(FVector(0.f, 0.f, S.Z - 2.5f), FVector(S.X, S.Y, 5.f), Main(WoodLight));
		P.Legs(S, S.Z - 5.f, 5.f, 6.f, WoodDark);
		if (Model == TEXT("meja_makan"))
		{
			// Tablecloth, a rice pot and plates that are (fittingly) empty.
			P.bCollide = false;
			P.Box(FVector(0.f, 0.f, S.Z + 0.5f), FVector(S.X + 8.f, S.Y + 8.f, 1.f), Hex(TEXT("#C8553D")));
			P.Cyl(FVector(0.f, 0.f, S.Z + 10.f), FVector(26.f, 26.f, 18.f), Metal);
			P.Cyl(FVector(0.f, 0.f, S.Z + 19.5f), FVector(28.f, 28.f, 2.f), MetalDark);
			for (int32 SX = -1; SX <= 1; SX += 2)
			{
				for (int32 SY = -1; SY <= 1; SY += 2)
				{
					P.Cyl(FVector(SX * (S.X * 0.5f - 16.f), SY * S.Y * 0.28f, S.Z + 1.8f), FVector(22.f, 22.f, 1.5f), White);
				}
			}
			P.Cyl(FVector(4.f, S.Y * 0.36f, S.Z + 6.f), FVector(9.f, 9.f, 10.f), Hex(TEXT("#7FB3D5")));
		}
		return true;
	}
	if (Model == TEXT("meja_kantor"))
	{
		const FVector S = SizeOr(70.f, 140.f, 75.f);
		const float T = S.Z;
		P.Box(FVector(0.f, 0.f, T - 2.5f), FVector(S.X, S.Y, 5.f), Main(WoodLight));
		P.Box(FVector(0.f, -(S.Y * 0.5f - 2.f), (T - 5.f) * 0.5f), FVector(S.X - 4.f, 4.f, T - 5.f), Wood);
		P.Box(FVector(0.f, S.Y * 0.5f - 21.f, (T - 5.f) * 0.5f), FVector(S.X - 4.f, 42.f, T - 5.f), Wood);
		// Modesty panel on the visitor's side.
		P.Box(FVector(S.X * 0.5f - 4.f, 0.f, T - 26.f), FVector(3.f, S.Y - 10.f, 40.f), Wood);
		P.bCollide = false;
		for (int32 i = 0; i < 3; ++i)
		{
			const float Z = 14.f + i * 20.f;
			P.Box(FVector(-(S.X * 0.5f - 2.4f), S.Y * 0.5f - 21.f, Z), FVector(1.f, 36.f, 1.5f), WoodDark);
			P.Box(FVector(-(S.X * 0.5f - 1.5f), S.Y * 0.5f - 21.f, Z + 9.f), FVector(2.f, 10.f, 2.f), Metal);
		}
		P.Computer(S.X, T, Black, Screen);
		// Paperwork: the stack that never shrinks, a pen cup and the all-important stamp.
		P.Box(FVector(-4.f, -(S.Y * 0.5f - 22.f), T + 2.f), FVector(24.f, 32.f, 4.f), Paper, FRotator(0.f, 6.f, 0.f));
		P.Box(FVector(6.f, -(S.Y * 0.5f - 24.f), T + 7.f), FVector(24.f, 32.f, 14.f), Hex(TEXT("#E6DFC9")), FRotator(0.f, -4.f, 0.f));
		P.Cyl(FVector(S.X * 0.5f - 12.f, S.Y * 0.5f - 14.f, T + 5.f), FVector(7.f, 7.f, 10.f), MetalDark);
		P.Cyl(FVector(S.X * 0.5f - 14.f, S.Y * 0.5f - 30.f, T + 3.f), FVector(6.f, 6.f, 6.f), Hex(TEXT("#B83A2E")));
		return true;
	}
	if (Model == TEXT("meja_pc"))
	{
		const FVector S = SizeOr(60.f, 110.f, 75.f);
		const float T = S.Z;
		P.Box(FVector(0.f, 0.f, T - 2.5f), FVector(S.X, S.Y, 5.f), Main(Hex(TEXT("#CFC8B8"))));
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(0.f, SY * (S.Y * 0.5f - 2.f), (T - 5.f) * 0.5f), FVector(S.X - 4.f, 4.f, T - 5.f), MetalDark);
		}
		P.Box(FVector(S.X * 0.5f - 4.f, 0.f, T - 26.f), FVector(3.f, S.Y - 10.f, 40.f), MetalDark);
		P.Box(FVector(2.f, S.Y * 0.5f - 18.f, 23.f), FVector(42.f, 20.f, 44.f), Black);
		P.bCollide = false;
		P.Box(FVector(-19.3f, S.Y * 0.5f - 18.f, 38.f), FVector(0.6f, 3.f, 3.f), Hex(TEXT("#5BE37A")));
		P.Computer(S.X, T, Black, Screen);
		P.Cyl(FVector(-6.f, -(S.Y * 0.5f - 14.f), T + 5.5f), FVector(6.5f, 6.5f, 11.f), Hex(TEXT("#C0392B")));
		return true;
	}
	if (Model == TEXT("kursi") || Model == TEXT("kursi_plastik"))
	{
		const bool bPlastic = Model == TEXT("kursi_plastik");
		const FLinearColor C = Main(bPlastic ? Hex(TEXT("#2E9E6A")) : Wood);
		const FLinearColor Leg = bPlastic ? C : WoodDark;
		// Only the seat is solid: it stops Bayu walking through the chair (seated NPCs ignore furniture).
		P.Box(FVector(SeatX, 0.f, SeatZ), FVector(42.f, 42.f, 5.f), C);
		P.bCollide = false;
		for (int32 SX = -1; SX <= 1; SX += 2)
		{
			for (int32 SY = -1; SY <= 1; SY += 2)
			{
				P.Box(FVector(SeatX + SX * 18.f, SY * 18.f, (SeatZ - 2.5f) * 0.5f), FVector(4.f, 4.f, SeatZ - 2.5f), Leg);
			}
		}
		if (bPlastic)
		{
			P.Box(FVector(SeatX - 21.f, 0.f, SeatZ + 22.f), FVector(4.f, 42.f, 40.f), C, FRotator(8.f, 0.f, 0.f));
		}
		else
		{
			for (int32 SY = -1; SY <= 1; SY += 2)
			{
				P.Box(FVector(SeatX - 19.f, SY * 18.f, SeatZ + 22.f), FVector(4.f, 4.f, 44.f), Leg);
			}
			P.Box(FVector(SeatX - 19.f, 0.f, SeatZ + 34.f), FVector(3.f, 40.f, 18.f), C);
			P.Box(FVector(SeatX - 19.f, 0.f, SeatZ + 14.f), FVector(3.f, 40.f, 6.f), C);
		}
		return true;
	}
	if (Model == TEXT("kursi_kantor"))
	{
		const FLinearColor C = Main(Hex(TEXT("#3B4352")));
		P.Box(FVector(SeatX, 0.f, SeatZ + 1.f), FVector(46.f, 46.f, 8.f), C);
		P.bCollide = false;
		P.Box(FVector(SeatX - 27.f, 0.f, SeatZ + 38.f), FVector(7.f, 44.f, 50.f), C, FRotator(8.f, 0.f, 0.f));
		P.Box(FVector(SeatX - 24.f, 0.f, SeatZ + 9.f), FVector(4.f, 8.f, 20.f), MetalDark);
		P.Cyl(FVector(SeatX, 0.f, (SeatZ + 6.f) * 0.5f), FVector(6.f, 6.f, SeatZ - 6.f), Metal);
		P.Box(FVector(SeatX, 0.f, 4.f), FVector(56.f, 6.f, 4.f), MetalDark);
		P.Box(FVector(SeatX, 0.f, 4.f), FVector(6.f, 56.f, 4.f), MetalDark);
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(SeatX - 4.f, SY * 25.f, SeatZ + 20.f), FVector(30.f, 5.f, 4.f), MetalDark);
			P.Box(FVector(SeatX - 8.f, SY * 25.f, SeatZ + 11.f), FVector(4.f, 4.f, 16.f), MetalDark);
		}
		return true;
	}
	if (Model == TEXT("bangku"))
	{
		const FVector S = SizeOr(40.f, 180.f, SeatZ);
		P.Box(FVector(SeatX, 0.f, S.Z), FVector(S.X, S.Y, 5.f), Main(Wood));
		P.bCollide = false;
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(SeatX, SY * (S.Y * 0.5f - 14.f), (S.Z - 2.5f) * 0.5f), FVector(S.X - 6.f, 5.f, S.Z - 2.5f), WoodDark);
		}
		P.Box(FVector(SeatX, 0.f, 12.f), FVector(4.f, S.Y - 28.f, 5.f), WoodDark);
		return true;
	}
	if (Model == TEXT("meja_kecil"))
	{
		const FVector S = SizeOr(50.f, 80.f, 45.f);
		P.Box(FVector(0.f, 0.f, S.Z - 2.f), FVector(S.X, S.Y, 4.f), Main(WoodLight));
		P.Legs(S, S.Z - 4.f, 4.f, 4.f, WoodDark);
		P.bCollide = false;
		// Two glasses of kopi and a plate of gorengan.
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Cyl(FVector(SY * 8.f, SY * S.Y * 0.28f, S.Z + 4.5f), FVector(7.f, 7.f, 9.f), Hex(TEXT("#E8E2D2")));
			P.Cyl(FVector(SY * 8.f, SY * S.Y * 0.28f, S.Z + 8.7f), FVector(6.2f, 6.2f, 0.6f), Hex(TEXT("#3A2416")));
		}
		P.Cyl(FVector(-4.f, 0.f, S.Z + 0.8f), FVector(20.f, 20.f, 1.5f), White);
		for (int32 i = 0; i < 4; ++i)
		{
			P.Ball(FVector(-4.f + Rng.FRandRange(-5.f, 5.f), Rng.FRandRange(-5.f, 5.f), S.Z + 3.f), FVector(7.f, 5.f, 3.5f), Hex(TEXT("#C98A3A")));
		}
		return true;
	}
	if (Model == TEXT("rak"))
	{
		// Open shelf (front +X) full of binders.
		const FVector S = SizeOr(40.f, 120.f, 180.f);
		const FLinearColor C = Main(Wood);
		P.Box(FVector(-S.X * 0.5f + 1.f, 0.f, S.Z * 0.5f), FVector(2.f, S.Y, S.Z), WoodDark);
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(0.f, SY * (S.Y * 0.5f - 1.5f), S.Z * 0.5f), FVector(S.X, 3.f, S.Z), C);
		}
		const float Gap = (S.Z - 3.f) / 3.f;
		for (int32 i = 0; i < 4; ++i)
		{
			P.Box(FVector(0.f, 0.f, 1.5f + i * Gap), FVector(S.X, S.Y - 6.f, 3.f), C);
		}
		P.bCollide = false;
		const FLinearColor Binders[] = { Hex(TEXT("#C0392B")), Hex(TEXT("#2E86C1")), Hex(TEXT("#E3B23C")), Hex(TEXT("#27AE60")), Hex(TEXT("#7D5BA6")), Paper };
		for (int32 i = 0; i < 3; ++i)
		{
			float Y = -S.Y * 0.5f + 4.f;
			while (Y < S.Y * 0.5f - 14.f)
			{
				const float W = FMath::Min(Rng.FRandRange(10.f, 24.f), S.Y * 0.5f - 4.f - Y);
				if (Rng.FRand() > 0.15f)
				{
					const float H = FMath::Min(Rng.FRandRange(24.f, 36.f), Gap - 8.f);
					P.Box(FVector(2.f, Y + W * 0.5f, 3.f + i * Gap + H * 0.5f), FVector(S.X - 10.f, W - 1.f, H), Binders[Rng.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Binders)) - 1)]);
				}
				Y += W;
			}
		}
		return true;
	}
	if (Model == TEXT("lemari"))
	{
		const FVector S = SizeOr(60.f, 120.f, 200.f);
		P.Box(FVector(0.f, 0.f, S.Z * 0.5f), FVector(S.X, S.Y, S.Z), Main(Wood));
		P.Box(FVector(0.f, 0.f, S.Z + 2.f), FVector(S.X + 4.f, S.Y + 4.f, 4.f), WoodDark);
		P.Box(FVector(0.f, 0.f, 4.f), FVector(S.X + 2.f, S.Y + 2.f, 8.f), WoodDark);
		P.bCollide = false;
		P.Box(FVector(S.X * 0.5f + 0.5f, 0.f, S.Z * 0.5f + 4.f), FVector(1.f, 1.5f, S.Z - 16.f), WoodDark);
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(S.X * 0.5f + 1.5f, SY * 7.f, S.Z * 0.55f), FVector(2.f, 3.f, 16.f), Metal);
		}
		P.Box(FVector(S.X * 0.5f + 0.6f, -S.Y * 0.25f, S.Z * 0.6f), FVector(0.6f, S.Y * 0.5f - 20.f, S.Z * 0.45f), Hex(TEXT("#BFD3DC")));
		return true;
	}
	if (Model == TEXT("lemari_arsip"))
	{
		const FVector S = SizeOr(50.f, 50.f, 130.f);
		P.Box(FVector(0.f, 0.f, S.Z * 0.5f), FVector(S.X, S.Y, S.Z), Main(Hex(TEXT("#8C939B"))));
		P.bCollide = false;
		const float H = S.Z / 4.f;
		for (int32 i = 0; i < 4; ++i)
		{
			const float Mid = H * (i + 0.5f);
			P.Box(FVector(S.X * 0.5f + 1.f, 0.f, Mid + H * 0.2f), FVector(2.f, 14.f, 2.5f), MetalDark);
			P.Box(FVector(S.X * 0.5f + 0.4f, 0.f, Mid - H * 0.12f), FVector(0.6f, 10.f, 5.f), Paper);
			if (i < 3)
			{
				P.Box(FVector(S.X * 0.5f + 0.3f, 0.f, H * (i + 1)), FVector(0.6f, S.Y - 4.f, 1.f), MetalDark);
			}
		}
		return true;
	}
	if (Model == TEXT("dispenser"))
	{
		P.Box(FVector(0.f, 0.f, 50.f), FVector(32.f, 32.f, 100.f), Main(White));
		P.bCollide = false;
		P.Cyl(FVector(0.f, 0.f, 122.f), FVector(27.f, 27.f, 44.f), Hex(TEXT("#9FD3EE")));
		P.Box(FVector(16.5f, -6.f, 70.f), FVector(3.f, 4.f, 5.f), Hex(TEXT("#2E86C1")));
		P.Box(FVector(16.5f, 6.f, 70.f), FVector(3.f, 4.f, 5.f), Hex(TEXT("#C0392B")));
		P.Box(FVector(14.f, 0.f, 52.f), FVector(6.f, 22.f, 2.f), MetalDark);
		return true;
	}
	if (Model == TEXT("pot"))
	{
		P.Cyl(FVector(0.f, 0.f, 20.f), FVector(34.f, 34.f, 40.f), Main(Hex(TEXT("#A0522D"))));
		P.bCollide = false;
		P.Ball(FVector(0.f, 0.f, 64.f), FVector(62.f, 62.f, 58.f), Hex(TEXT("#3C8D4F")));
		P.Ball(FVector(10.f, -8.f, 92.f), FVector(38.f, 38.f, 36.f), Hex(TEXT("#4CA35E")));
		return true;
	}
	if (Model == TEXT("dapur"))
	{
		// Kitchen counter (front +X) with a two-burner stove and a pot.
		const FVector S = SizeOr(60.f, 160.f, 85.f);
		P.Box(FVector(0.f, 0.f, (S.Z - 4.f) * 0.5f), FVector(S.X, S.Y, S.Z - 4.f), Main(Hex(TEXT("#D9D4C7"))));
		P.Box(FVector(0.f, 0.f, S.Z - 2.f), FVector(S.X + 4.f, S.Y + 4.f, 4.f), Hex(TEXT("#7A7F86")));
		P.bCollide = false;
		P.Box(FVector(S.X * 0.5f + 0.4f, 0.f, S.Z * 0.45f), FVector(0.6f, 1.f, S.Z * 0.7f), MetalDark);
		P.Box(FVector(2.f, -S.Y * 0.2f, S.Z + 4.f), FVector(40.f, 62.f, 8.f), Black);
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Cyl(FVector(2.f, -S.Y * 0.2f + SY * 15.f, S.Z + 8.5f), FVector(16.f, 16.f, 1.5f), MetalDark);
		}
		P.Cyl(FVector(2.f, -S.Y * 0.2f - 15.f, S.Z + 19.f), FVector(24.f, 24.f, 20.f), Metal);
		P.Cyl(FVector(0.f, S.Y * 0.3f, S.Z + 14.f), FVector(16.f, 16.f, 28.f), Hex(TEXT("#B5651D")));
		return true;
	}
	if (Model == TEXT("tenda"))
	{
		const FVector S = SizeOr(400.f, 600.f, 250.f);
		const FLinearColor C = Main(Hex(TEXT("#2F6DB5")));
		for (int32 SX = -1; SX <= 1; SX += 2)
		{
			for (int32 SY = -1; SY <= 1; SY += 2)
			{
				P.Cyl(FVector(SX * (S.X * 0.5f - 6.f), SY * (S.Y * 0.5f - 6.f), S.Z * 0.5f), FVector(8.f, 8.f, S.Z), Metal);
			}
		}
		P.bCollide = false;
		P.bRoof = true;
		P.Box(FVector(0.f, 0.f, S.Z + 3.f), FVector(S.X + 20.f, S.Y + 20.f, 6.f), C);
		P.Box(FVector(0.f, 0.f, S.Z + 18.f), FVector(S.X * 0.7f, S.Y * 0.7f, 24.f), C);
		P.Box(FVector(0.f, 0.f, S.Z + 34.f), FVector(S.X * 0.35f, S.Y * 0.35f, 10.f), C);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			P.Box(FVector(0.f, Side * (S.Y * 0.5f + 10.f), S.Z - 12.f), FVector(S.X + 20.f, 2.f, 24.f), White);
			P.Box(FVector(Side * (S.X * 0.5f + 10.f), 0.f, S.Z - 12.f), FVector(2.f, S.Y + 20.f, 24.f), White);
		}
		return true;
	}
	if (Model == TEXT("kotak_suara"))
	{
		const FVector S = SizeOr(55.f, 55.f, 55.f);
		const FVector Top(S.X + 16.f, S.Y + 16.f, 0.f);
		P.Box(FVector(0.f, 0.f, 72.5f), FVector(Top.X, Top.Y, 5.f), WoodLight);
		P.Legs(Top, 70.f, 4.f, 4.f, WoodDark);
		P.Box(FVector(0.f, 0.f, 75.f + S.Z * 0.5f), FVector(S.X, S.Y, S.Z), Main(Hex(TEXT("#CFE6F2"))));
		P.bCollide = false;
		P.Box(FVector(0.f, 0.f, 77.f + S.Z), FVector(S.X + 4.f, S.Y + 4.f, 4.f), White);
		P.Box(FVector(0.f, 0.f, 79.2f + S.Z), FVector(3.f, 20.f, 0.6f), Black);
		P.Box(FVector(S.X * 0.5f + 2.5f, 0.f, 75.f + S.Z - 10.f), FVector(3.f, 6.f, 8.f), Hex(TEXT("#C9A227")));
		P.Box(FVector(S.X * 0.5f + 0.4f, 0.f, 75.f + 10.f), FVector(0.6f, S.Y - 14.f, 14.f), Paper);
		return true;
	}
	if (Model == TEXT("bilik"))
	{
		// Voting booth on a small table; the voter stands on the -X side.
		const FVector S = SizeOr(60.f, 80.f, 75.f);
		const FLinearColor Card = Main(Hex(TEXT("#D8CFB8")));
		P.Box(FVector(0.f, 0.f, S.Z - 2.5f), FVector(S.X, S.Y, 5.f), WoodLight);
		P.Legs(S, S.Z - 5.f, 4.f, 4.f, WoodDark);
		P.Box(FVector(S.X * 0.5f - 1.f, 0.f, S.Z + 28.f), FVector(2.f, S.Y, 56.f), Card);
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(0.f, SY * (S.Y * 0.5f - 1.f), S.Z + 28.f), FVector(S.X, 2.f, 56.f), Card);
		}
		P.bCollide = false;
		P.Box(FVector(S.X * 0.5f - 2.3f, 0.f, S.Z + 40.f), FVector(0.6f, 30.f, 16.f), Hex(TEXT("#C0392B")));
		P.Box(FVector(-6.f, 10.f, S.Z + 0.8f), FVector(14.f, 1.2f, 1.2f), Black, FRotator(0.f, 30.f, 0.f));
		return true;
	}
	if (Model == TEXT("ekskavator"))
	{
		// Arm reaches towards +X.
		const FLinearColor Body = Main(Hex(TEXT("#E2A81C")));
		const FLinearColor BodyDark = Hex(TEXT("#B98712"));
		const FLinearColor Glass = Hex(TEXT("#2B3A4A"));
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(0.f, SY * 100.f, 45.f), FVector(480.f, 80.f, 90.f), Black);
		}
		P.Box(FVector(0.f, 0.f, 70.f), FVector(300.f, 140.f, 40.f), MetalDark);
		P.Box(FVector(-50.f, 0.f, 145.f), FVector(300.f, 240.f, 110.f), Body);
		P.Box(FVector(-215.f, 0.f, 150.f), FVector(70.f, 240.f, 90.f), BodyDark);
		P.Box(FVector(40.f, -60.f, 275.f), FVector(130.f, 110.f, 150.f), Body);
		P.bCollide = false;
		P.Box(FVector(106.f, -60.f, 285.f), FVector(2.f, 90.f, 90.f), Glass);
		P.Box(FVector(40.f, -116.f, 290.f), FVector(110.f, 2.f, 80.f), Glass);
		P.Cyl(FVector(-120.f, 70.f, 230.f), FVector(10.f, 10.f, 60.f), Black);
		P.Box(FVector(191.f, 40.f, 312.f), FVector(320.f, 45.f, 45.f), Body, FRotator(35.f, 0.f, 0.f));
		P.Box(FVector(180.f, 40.f, 262.f), FVector(220.f, 12.f, 12.f), Metal, FRotator(28.f, 0.f, 0.f));
		P.Box(FVector(382.f, 40.f, 299.f), FVector(240.f, 38.f, 38.f), Body, FRotator(-60.f, 0.f, 0.f));
		P.Box(FVector(455.f, 40.f, 170.f), FVector(70.f, 90.f, 70.f), MetalDark, FRotator(-25.f, 0.f, 0.f));
		return true;
	}
	if (Model == TEXT("motor"))
	{
		// Scooter facing +X. AGRMotor steers group 1-2, spins 2-3 and folds the side stand (4) while ridden.
		const FLinearColor Body = Main(Hex(TEXT("#C0392B")));
		// Seat top 79 cm and a flat floorboard: MM_GR_Ride puts Bayu's hips, feet and hands on these spots.
		P.Box(FVector(-11.f, 0.f, 32.f), FVector(78.f, 20.f, 14.f), MetalDark);
		P.Box(FVector(-35.f, 0.f, 59.f), FVector(60.f, 30.f, 22.f), Body);
		P.Box(FVector(-28.f, 0.f, 74.5f), FVector(64.f, 28.f, 9.f), Black);
		P.Box(FVector(44.f, 0.f, 70.f), FVector(26.f, 40.f, 46.f), Body, FRotator(-18.f, 0.f, 0.f));
		P.Box(FVector(8.f, 0.f, 40.f), FVector(50.f, 36.f, 4.f), Black);
		P.bCollide = false;
		P.Box(FVector(62.f, 0.f, 82.f), FVector(4.f, 16.f, 10.f), Hex(TEXT("#FFF2B0")));
		P.Box(FVector(-66.f, 0.f, 66.f), FVector(4.f, 14.f, 6.f), Hex(TEXT("#E04A3A")));
		P.Cyl(FVector(-40.f, 16.f, 38.f), FVector(8.f, 8.f, 46.f), Metal, FRotator(90.f, 0.f, 0.f));
		for (int32 SX = -1; SX <= 1; SX += 2)
		{
			// Tyre, rim and three dark spokes across the rim, so a turning wheel reads as turning.
			P.Group = SX > 0 ? 2 : 3;
			P.Cyl(FVector(SX * 58.f, 0.f, 30.f), FVector(60.f, 60.f, 11.f), Black, FRotator(0.f, 0.f, 90.f));
			P.Cyl(FVector(SX * 58.f, 0.f, 30.f), FVector(26.f, 26.f, 12.5f), Metal, FRotator(0.f, 0.f, 90.f));
			for (int32 Spoke = 0; Spoke < 3; ++Spoke)
			{
				P.Box(FVector(SX * 58.f, 0.f, 30.f), FVector(24.f, 13.5f, 3.f), MetalDark, FRotator(Spoke * 60.f, 0.f, 0.f));
			}
		}
		// Fork, fender, swept-back handlebar with its cover and speedometer, grips and mirrors all turn together.
		P.Group = 1;
		P.Box(FVector(52.f, 0.f, 62.f), FVector(6.f, 6.f, 64.f), Metal, FRotator(20.f, 0.f, 0.f));
		P.Box(FVector(58.f, 0.f, 63.f), FVector(30.f, 14.f, 4.f), Body);
		P.Box(FVector(46.f, 0.f, 100.f), FVector(16.f, 30.f, 10.f), Body);
		P.Box(FVector(44.f, 0.f, 105.2f), FVector(8.f, 12.f, 0.8f), Hex(TEXT("#1E2A30")));
		P.Box(FVector(46.f, 0.f, 102.f), FVector(5.f, 32.f, 5.f), Metal);
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(42.f, SY * 21.f, 102.f), FVector(12.8f, 5.f, 5.f), Metal, FRotator(0.f, SY * 128.66f, 0.f));
			P.Box(FVector(37.5f, SY * 31.f, 102.f), FVector(7.f, 10.f, 7.f), Black);
			P.Box(FVector(43.f, SY * 19.f, 110.f), FVector(1.5f, 1.5f, 14.f), Black);
			P.Box(FVector(43.f, SY * 21.f, 118.f), FVector(2.f, 9.f, 6.f), Black);
		}
		// Side stand on the left: the parked scooter leans on it.
		P.Group = 4;
		P.Box(FVector(-8.f, -19.f, 21.f), FVector(3.f, 3.f, 40.5f), MetalDark, FRotator(0.f, 0.f, 20.f));
		P.Group = 0;
		return true;
	}
	if (Model == TEXT("warung") || Model == TEXT("lapak"))
	{
		// Long counter: goods are lined up along its longer side.
		const bool bWarung = Model == TEXT("warung");
		const FVector S = bWarung ? SizeOr(220.f, 120.f, 100.f) : SizeOr(150.f, 260.f, 90.f);
		const bool bLongX = S.X >= S.Y;
		const float L = bLongX ? S.X : S.Y;
		const float W = bLongX ? S.Y : S.X;
		// (along the counter, across it, up) to local space; also used for sizes.
		auto At = [bLongX](float Along, float Across, float Z) { return bLongX ? FVector(Along, Across, Z) : FVector(Across, Along, Z); };
		if (bWarung)
		{
			P.Box(FVector(0.f, 0.f, (S.Z - 6.f) * 0.5f), FVector(S.X, S.Y, S.Z - 6.f), Main(Hex(TEXT("#9C6B3F"))));
			P.Box(FVector(0.f, 0.f, S.Z * 0.35f), FVector(S.X + 1.f, S.Y + 1.f, 8.f), Hex(TEXT("#2E7D5B")));
		}
		else
		{
			P.Legs(S, S.Z - 6.f, 6.f, 7.f, WoodDark);
			P.Box(FVector(0.f, 0.f, 20.f), FVector(S.X - 12.f, S.Y - 12.f, 3.f), WoodDark);
		}
		P.Box(FVector(0.f, 0.f, S.Z - 3.f), FVector(S.X + 12.f, S.Y + 12.f, 6.f), bWarung ? WoodLight : Main(Wood));
		P.bCollide = false;
		const int32 Count = FMath::Clamp(FMath::FloorToInt(L / (bWarung ? 45.f : 50.f)), 2, 6);
		for (int32 i = 0; i < Count; ++i)
		{
			const float Along = -L * 0.5f + L * (i + 0.5f) / Count;
			if (bWarung)
			{
				// Jars of krupuk and kacang.
				P.Cyl(At(Along, -W * 0.15f, S.Z + 12.f), FVector(16.f, 16.f, 24.f), Hex(TEXT("#E9DDB5")));
				P.Cyl(At(Along, -W * 0.15f, S.Z + 25.5f), FVector(17.f, 17.f, 3.f), Hex(TEXT("#C0392B")));
			}
			else
			{
				// Rice sacks, two rows.
				for (int32 Row = -1; Row <= 1; Row += 2)
				{
					P.Box(At(Along, Row * W * 0.22f, S.Z + 22.f), At(38.f, 30.f, 44.f), Hex(TEXT("#EFE8D5")), FRotator(0.f, Rng.FRandRange(-8.f, 8.f), 0.f));
					P.Box(At(Along, Row * W * 0.22f, S.Z + 44.5f), At(30.f, 22.f, 2.f), White);
				}
			}
		}
		if (bWarung)
		{
			P.Cyl(At(L * 0.32f, W * 0.22f, S.Z + 15.f), FVector(13.f, 13.f, 30.f), Hex(TEXT("#D35400")));
			P.Cyl(At(L * 0.18f, W * 0.24f, S.Z + 4.5f), FVector(7.f, 7.f, 9.f), Hex(TEXT("#E8E2D2")));
			// Striped awning on two back posts.
			P.bCollide = true;
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				P.Box(At(Side * (L * 0.5f + 4.f), -W * 0.5f - 4.f, 120.f), FVector(6.f, 6.f, 240.f), WoodDark);
			}
			P.bCollide = false;
			P.bRoof = true;
			const int32 Stripes = 6;
			const float StripeLen = (L + 40.f) / Stripes;
			for (int32 i = 0; i < Stripes; ++i)
			{
				P.Box(At(-(L + 40.f) * 0.5f + StripeLen * (i + 0.5f), 10.f, 238.f), At(StripeLen, W + 60.f, 4.f),
					i % 2 == 0 ? Hex(TEXT("#C0392B")) : White, bLongX ? FRotator(0.f, 0.f, -8.f) : FRotator(8.f, 0.f, 0.f));
			}
		}
		else
		{
			P.Box(At(L * 0.5f - 22.f, 0.f, S.Z + 7.f), At(26.f, 22.f, 14.f), Hex(TEXT("#C0392B")));
			P.Box(At(L * 0.5f - 22.f, 0.f, S.Z + 15.f), At(24.f, 24.f, 2.f), Metal);
		}
		return true;
	}
	if (Model == TEXT("stensil"))
	{
		const FVector S = SizeOr(120.f, 80.f, 100.f);
		P.Box(FVector(0.f, 0.f, 68.f), FVector(S.X, S.Y, 4.f), MetalDark);
		P.Legs(S, 66.f, 4.f, 4.f, Metal);
		P.Box(FVector(0.f, 0.f, 85.f), FVector(70.f, 50.f, 30.f), Main(Hex(TEXT("#56705F"))));
		P.bCollide = false;
		P.Cyl(FVector(0.f, 0.f, 103.f), FVector(28.f, 28.f, 52.f), Black, FRotator(0.f, 0.f, 90.f));
		P.Box(FVector(0.f, 30.f, 103.f), FVector(4.f, 4.f, 18.f), Metal);
		P.Box(FVector(6.f, 32.f, 112.f), FVector(12.f, 3.f, 3.f), Metal);
		P.Box(FVector(-48.f, 0.f, 71.f), FVector(28.f, 38.f, 2.f), Paper);
		P.Box(FVector(48.f, 0.f, 75.f), FVector(28.f, 38.f, 10.f), Paper);
		P.Cyl(FVector(-45.f, -28.f, 76.f), FVector(10.f, 10.f, 12.f), Black);
		return true;
	}
	if (Model == TEXT("papan"))
	{
		// Notice board on two posts, papers on the +X face.
		const FVector S = SizeOr(30.f, 220.f, 160.f);
		constexpr float Lift = 50.f;
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(0.f, SY * (S.Y * 0.5f - 5.f), (S.Z + Lift) * 0.5f), FVector(8.f, 8.f, S.Z + Lift), WoodDark);
		}
		P.Box(FVector(0.f, 0.f, Lift + S.Z * 0.5f), FVector(6.f, S.Y - 18.f, S.Z), Main(Hex(TEXT("#B98B5A"))));
		P.Box(FVector(0.f, 0.f, Lift + S.Z + 4.f), FVector(10.f, S.Y, 6.f), WoodDark);
		P.Box(FVector(0.f, 0.f, Lift + S.Z + 10.f), FVector(30.f, S.Y + 10.f, 4.f), Hex(TEXT("#7A3B2A")));
		P.bCollide = false;
		const int32 Count = FMath::Max(2, FMath::FloorToInt(S.Y / 40.f));
		for (int32 i = 0; i < Count; ++i)
		{
			const float Y = -S.Y * 0.5f + 30.f + i * (S.Y - 60.f) / (Count - 1);
			P.Box(FVector(3.6f, Y, Lift + S.Z * Rng.FRandRange(0.3f, 0.7f)), FVector(0.6f, Rng.FRandRange(22.f, 30.f), Rng.FRandRange(28.f, 38.f)),
				Rng.FRand() < 0.7f ? Paper : Hex(TEXT("#F6E27A")), FRotator(0.f, 0.f, Rng.FRandRange(-6.f, 6.f)));
		}
		return true;
	}
	if (Model == TEXT("baliho"))
	{
		const FVector S = SizeOr(40.f, 500.f, 300.f);
		const float Lift = S.Z * 0.5f;
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(-4.f, SY * S.Y * 0.3f, (Lift + S.Z) * 0.5f), FVector(14.f, 14.f, Lift + S.Z), MetalDark);
		}
		P.Box(FVector(4.f, 0.f, Lift + S.Z * 0.5f), FVector(10.f, S.Y, S.Z), Main(Hex(TEXT("#D4AF37"))));
		P.bCollide = false;
		P.Box(FVector(4.f, 0.f, Lift + S.Z + 4.f), FVector(14.f, S.Y + 6.f, 8.f), MetalDark);
		P.Box(FVector(4.f, 0.f, Lift - 4.f), FVector(14.f, S.Y + 6.f, 8.f), MetalDark);
		P.Box(FVector(9.4f, 0.f, Lift + S.Z * 0.5f), FVector(0.6f, S.Y * 0.8f, S.Z * 0.18f), White);
		return true;
	}
	if (Model == TEXT("spanduk"))
	{
		const FVector S = SizeOr(40.f, 600.f, 220.f);
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Cyl(FVector(0.f, SY * S.Y * 0.5f, (S.Z + 40.f) * 0.5f), FVector(8.f, 8.f, S.Z + 40.f), Hex(TEXT("#C9B37E")));
		}
		P.bCollide = false;
		P.Box(FVector(0.f, 0.f, S.Z + 30.f - S.Z * 0.225f), FVector(2.f, S.Y - 16.f, S.Z * 0.45f), Main(Hex(TEXT("#F4F1E8"))));
		P.Box(FVector(0.f, 0.f, S.Z + 32.f), FVector(1.f, S.Y, 1.f), Black);
		return true;
	}
	if (Model == TEXT("jeruji"))
	{
		const FVector S = SizeOr(30.f, 340.f, 240.f);
		const FLinearColor C = Main(MetalDark);
		P.Box(FVector(0.f, 0.f, 4.f), FVector(S.X, S.Y, 8.f), C);
		P.Box(FVector(0.f, 0.f, S.Z - 4.f), FVector(S.X, S.Y, 8.f), C);
		P.Box(FVector(0.f, 0.f, S.Z * 0.55f), FVector(S.X * 0.6f, S.Y, 5.f), C);
		const int32 Bars = FMath::Max(2, FMath::FloorToInt(S.Y / 20.f));
		for (int32 i = 0; i <= Bars; ++i)
		{
			P.Cyl(FVector(0.f, -S.Y * 0.5f + 5.f + i * (S.Y - 10.f) / Bars, S.Z * 0.5f), FVector(5.f, 5.f, S.Z - 8.f), C);
		}
		return true;
	}
	if (Model == TEXT("pintu"))
	{
		const FVector S = SizeOr(40.f, 160.f, 230.f);
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(0.f, SY * (S.Y * 0.5f - 5.f), S.Z * 0.5f), FVector(S.X, 10.f, S.Z), WoodDark);
		}
		P.Box(FVector(0.f, 0.f, S.Z - 6.f), FVector(S.X, S.Y, 12.f), WoodDark);
		P.Box(FVector(0.f, 0.f, (S.Z - 12.f) * 0.5f), FVector(6.f, S.Y - 20.f, S.Z - 12.f), Main(Wood));
		P.bCollide = false;
		for (int32 SX = -1; SX <= 1; SX += 2)
		{
			P.Box(FVector(SX * 3.4f, 0.f, S.Z * 0.68f), FVector(0.8f, S.Y - 50.f, S.Z * 0.3f), Hex(TEXT("#74492F")));
			P.Box(FVector(SX * 3.4f, 0.f, S.Z * 0.26f), FVector(0.8f, S.Y - 50.f, S.Z * 0.3f), Hex(TEXT("#74492F")));
			P.Box(FVector(SX * 5.f, S.Y * 0.5f - 28.f, 100.f), FVector(4.f, 12.f, 3.f), Metal);
		}
		return true;
	}
	if (Model == TEXT("tiang"))
	{
		const FVector S = SizeOr(30.f, 30.f, 480.f);
		P.Cyl(FVector(0.f, 0.f, S.Z * 0.5f), FVector(8.f, 8.f, S.Z), Main(Metal));
		P.bCollide = false;
		P.Box(FVector(0.f, 0.f, S.Z - 30.f), FVector(4.f, 140.f, 4.f), Metal);
		P.Box(FVector(0.f, 0.f, S.Z - 60.f), FVector(4.f, 100.f, 4.f), Metal);
		P.Box(FVector(0.f, 0.f, 160.f), FVector(1.f, 180.f, 1.f), Black);
		P.Box(FVector(0.f, -45.f, 138.f), FVector(2.f, 34.f, 44.f), Hex(TEXT("#4A90C0")));
		P.Box(FVector(0.f, 20.f, 134.f), FVector(2.f, 28.f, 52.f), Hex(TEXT("#E07A5F")));
		P.Box(FVector(0.f, 62.f, 142.f), FVector(2.f, 26.f, 36.f), White);
		return true;
	}
	if (Model == TEXT("patok"))
	{
		const FVector S = SizeOr(12.f, 12.f, 140.f);
		P.Box(FVector(0.f, 0.f, S.Z * 0.5f), FVector(S.X, S.Y, S.Z), Main(Hex(TEXT("#E67E22"))));
		P.bCollide = false;
		P.Box(FVector(0.f, 0.f, S.Z - 20.f), FVector(S.X + 0.6f, S.Y + 0.6f, 10.f), White);
		P.Box(FVector(0.f, 0.f, S.Z - 45.f), FVector(S.X + 0.6f, S.Y + 0.6f, 10.f), White);
		P.Box(FVector(0.f, S.Y * 0.5f + 10.f, S.Z - 8.f), FVector(1.f, 20.f, 8.f), Hex(TEXT("#E74C3C")), FRotator(0.f, 0.f, -20.f));
		return true;
	}
	if (Model == TEXT("sembako"))
	{
		// Pile of aid boxes, every one with a gold "No. 1" sticker facing +X.
		const FVector S = SizeOr(160.f, 160.f, 90.f);
		const FLinearColor Carton = Hex(TEXT("#C9A46C"));
		const FLinearColor Sticker = Main(Hex(TEXT("#D4AF37")));
		const float H = S.Z * 0.5f;
		for (int32 SX = -1; SX <= 1; SX += 2)
		{
			for (int32 SY = -1; SY <= 1; SY += 2)
			{
				P.Box(FVector(SX * S.X * 0.25f, SY * S.Y * 0.25f, H * 0.5f), FVector(S.X * 0.5f - 6.f, S.Y * 0.5f - 6.f, H - 2.f), Carton, FRotator(0.f, Rng.FRandRange(-5.f, 5.f), 0.f));
			}
		}
		P.Box(FVector(0.f, 0.f, H * 1.5f), FVector(S.X * 0.5f - 6.f, S.Y * 0.5f - 6.f, H - 2.f), Carton, FRotator(0.f, 12.f, 0.f));
		P.bCollide = false;
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(S.X * 0.5f - 2.6f, SY * S.Y * 0.25f, H * 0.5f), FVector(0.6f, 24.f, 24.f), Sticker);
		}
		P.Box(FVector(S.X * 0.25f - 1.5f, 0.f, H * 1.5f), FVector(0.6f, 24.f, 24.f), Sticker, FRotator(0.f, 12.f, 0.f));
		return true;
	}
	if (Model == TEXT("panggung"))
	{
		// Campaign stage, audience on the +X side.
		const FVector S = SizeOr(500.f, 900.f, 160.f);
		const float H = S.Z * 0.6f;
		const FLinearColor Gold = Main(Hex(TEXT("#D4AF37")));
		P.Box(FVector(0.f, 0.f, H * 0.5f), FVector(S.X, S.Y, H), Hex(TEXT("#3A3A46")));
		P.Box(FVector(S.X * 0.5f + 45.f, 0.f, H / 6.f), FVector(30.f, 140.f, H / 3.f), MetalDark);
		P.Box(FVector(S.X * 0.5f + 15.f, 0.f, H / 3.f), FVector(30.f, 140.f, H * 2.f / 3.f), MetalDark);
		P.Box(FVector(-S.X * 0.5f + 10.f, 0.f, H + 170.f), FVector(12.f, S.Y, 340.f), Gold);
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Box(FVector(-S.X * 0.5f + 10.f, SY * S.Y * 0.5f, H + 175.f), FVector(16.f, 16.f, 350.f), MetalDark);
			P.Box(FVector(S.X * 0.5f - 40.f, SY * (S.Y * 0.5f - 40.f), H + 70.f), FVector(60.f, 60.f, 140.f), Black);
		}
		P.Box(FVector(S.X * 0.5f - 90.f, 0.f, H + 55.f), FVector(50.f, 70.f, 110.f), WoodDark);
		P.bCollide = false;
		P.Box(FVector(S.X * 0.5f + 0.5f, 0.f, H * 0.5f), FVector(1.f, S.Y, H * 0.7f), Gold);
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			P.Cyl(FVector(S.X * 0.5f - 9.5f, SY * (S.Y * 0.5f - 40.f), H + 95.f), FVector(36.f, 36.f, 1.f), MetalDark, FRotator(90.f, 0.f, 0.f));
		}
		P.Box(FVector(-S.X * 0.5f + 16.6f, 0.f, H + 230.f), FVector(0.6f, S.Y * 0.6f, 90.f), White);
		return true;
	}
	return false;
}
