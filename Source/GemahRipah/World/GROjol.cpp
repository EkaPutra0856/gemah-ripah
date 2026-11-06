#include "World/GROjol.h"
#include "World/GRSfx.h"

#include "Player/GRCharacter.h"
#include "Story/GRStorySubsystem.h"
#include "Systems/GRClockSubsystem.h"
#include "Systems/GRNotificationSubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "UI/GRUISubsystem.h"
#include "World/GRCrowd.h"
#include "World/GRMapMarkerComponent.h"
#include "World/GRMapSubsystem.h"
#include "World/GRMotor.h"
#include "World/GRStealthLevel.h"
#include "Components/CapsuleComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// The main road grid (World/GRCrowd.cpp, Tools/Python/build_kota_sentosa.py): roads along Y at these X, and along X at these Y.
	constexpr float RoadLines[] = { -27000.f, 0.f, 27000.f };
	constexpr float RoadSpan = 48000.f;

	/** Arriving: close enough and slow enough. */
	constexpr float PickupReach = 480.f;
	constexpr float DropReach = 650.f;
	constexpr float StopSpeed = 160.f;
	/** Harder than any brake (1400 cm/s^2): a wall, a car, a kerb taken too fast. */
	constexpr float JoltDecel = 2600.f;
	constexpr float SpeedingSpeed = 1350.f;

	const TCHAR* const WomenNames[] = { TEXT("Sari"), TEXT("Rina"), TEXT("Dewi"), TEXT("Fitri"), TEXT("Wulan"), TEXT("Ayu"), TEXT("Nurul"), TEXT("Indah"), TEXT("Tini"), TEXT("Ratih"), TEXT("Lestari"), TEXT("Yuni") };
	const TCHAR* const MenNames[] = { TEXT("Budi"), TEXT("Agus"), TEXT("Joko"), TEXT("Rudi"), TEXT("Hendra"), TEXT("Fajar"), TEXT("Dimas"), TEXT("Yusuf"), TEXT("Anton"), TEXT("Wahyu"), TEXT("Slamet"), TEXT("Rizal") };
	const TCHAR* const SmallPlaces[] = { TEXT("Gang Kenanga"), TEXT("Kos Putri Melati"), TEXT("Warung Bu Tini"), TEXT("Bengkel Jaya Motor"), TEXT("Konter Pulsa Berkah"),
		TEXT("Apotek Sehat Sentosa"), TEXT("Laundry Kilat"), TEXT("Toko Bangunan Sinar"), TEXT("Gang Mawar"), TEXT("Rumah Makan Padang Raya") };

	struct FQuestion
	{
		const TCHAR* Ask;
		FGROjolChoice Answers[3];
	};

	const FQuestion Questions[] = {
		{ TEXT("Mas, potongan aplikasinya sekarang gede ya?"), {
			{ TEXT("Iya, 20 persen. Belum bensin."), TEXT("Kasihan... nanti saya kasih bintang lima deh."), 0.3f },
			{ TEXT("Biasa aja, yang penting jalan."), TEXT("Wah, sabar banget, Mas."), 0.f },
			{ TEXT("Jangan dibahas, pusing."), TEXT("Oh... maaf, Mas."), -0.2f } } },
		{ TEXT("Jalan Merdeka masih macet nggak? Katanya ada proyek."), {
			{ TEXT("Ada galian, saya cari jalan lain."), TEXT("Mantap, hafal jalan ya, Mas."), 0.3f },
			{ TEXT("Proyek abadi. Tiap tahun digali."), TEXT("Hahaha, bener banget."), 0.2f },
			{ TEXT("Nggak tahu, saya ikut peta aja."), TEXT("Oh, oke..."), -0.1f } } },
		{ TEXT("Harga beras naik lagi ya, Mas? Di pasar sudah 18 ribu."), {
			{ TEXT("Iya. Gaji nggak ikut naik."), TEXT("Sama. Yang naik cuma harga."), 0.2f },
			{ TEXT("Katanya program PPR bikin murah."), TEXT("Murahnya di baliho doang, Mas."), 0.1f },
			{ TEXT("Saya mah makan warteg."), TEXT("Hahaha, sama."), 0.1f } } },
		{ TEXT("Mas, jangan ngebut ya. Saya takut."), {
			{ TEXT("Siap, pelan-pelan saja."), TEXT("Makasih, Mas."), 0.3f },
			{ TEXT("Tenang, saya sudah biasa."), TEXT("Hmm... ya sudah."), 0.f },
			{ TEXT("Kalau pelan nanti telat."), TEXT("Aduh..."), -0.3f } } },
		{ TEXT("Udah lama jadi driver, Mas?"), {
			{ TEXT("Baru. Habis kena PHK."), TEXT("Semangat ya, Mas."), 0.2f },
			{ TEXT("Lumayan, sambil cari kerja lain."), TEXT("Semoga cepat dapat."), 0.1f },
			{ TEXT("Rahasia."), TEXT("Oh. Oke deh."), -0.1f } } },
		{ TEXT("Mas, katanya bakal ada pemilihan lagi ya?"), {
			{ TEXT("Iya, baliho sudah di mana-mana."), TEXT("Janji lagi, janji lagi."), 0.2f },
			{ TEXT("Yang penting jangan ambil amplopnya."), TEXT("Wah, berani juga Mas ngomong gitu."), 0.1f },
			{ TEXT("Saya nggak ngurus politik."), TEXT("Tapi politik ngurusin kita, Mas."), 0.f } } },
	};

	const TCHAR* const JoltLines[] = { TEXT("Aduh! Hati-hati, Mas!"), TEXT("Astaga, jantungku..."), TEXT("Mas! Lihat jalan dong!") };
	const TCHAR* const SpeedLines[] = { TEXT("Mas, pelan dikit dong!"), TEXT("Ini ojek apa balapan, Mas?"), TEXT("Pelan, Mas, saya mau sampai hidup.") };

	FString Distance(float Meters)
	{
		return Meters >= 1000.f ? FString::Printf(TEXT("%.1f km"), Meters / 1000.f) : FString::Printf(TEXT("%d m"), FMath::RoundToInt(Meters));
	}

	void Notify(const UObject* Context, const FString& Text, EGRNotifyType Type)
	{
		const UWorld* World = Context->GetWorld();
		if (UGRNotificationSubsystem* Notes = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UGRNotificationSubsystem>() : nullptr)
		{
			Notes->Notify(FText::FromString(Text), Type);
		}
	}

	UGRStorySubsystem* StoryOf(const UObject* Context)
	{
		const UWorld* World = Context->GetWorld();
		return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UGRStorySubsystem>() : nullptr;
	}
}

UGROjolSubsystem* UGROjolSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UGROjolSubsystem>() : nullptr;
}

AGRCharacter* UGROjolSubsystem::GetBayu() const
{
	return Cast<AGRCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
}

int32 UGROjolSubsystem::FareFor(float Meters)
{
	return FMath::Max(8000, FMath::RoundToInt((5000.f + 25.f * Meters) / 500.f) * 500);
}

void UGROjolSubsystem::BuildRoute(const FVector2D& A, const FVector2D& B, TArray<FVector2D>& Out)
{
	Out.Reset();
	Out.Add(A);
	if (FVector2D::Distance(A, B) < 2500.f)
	{
		Out.Add(B);
		return;
	}
	struct FSnap
	{
		int32 Axis = 0;
		float Line = 0.f;
		FVector2D Point = FVector2D::ZeroVector;
	};
	// Axis 0: the road along Y at X = Line; axis 1: the road along X at Y = Line.
	auto Snap = [](const FVector2D& P)
	{
		FSnap Best;
		float BestDist = TNumericLimits<float>::Max();
		for (const float Line : RoadLines)
		{
			if (const float D = FMath::Abs(P.X - Line); D < BestDist)
			{
				BestDist = D;
				Best = { 0, Line, FVector2D(Line, FMath::Clamp(P.Y, -RoadSpan, RoadSpan)) };
			}
			if (const float D = FMath::Abs(P.Y - Line); D < BestDist)
			{
				BestDist = D;
				Best = { 1, Line, FVector2D(FMath::Clamp(P.X, -RoadSpan, RoadSpan), Line) };
			}
		}
		return Best;
	};
	const FSnap From = Snap(A);
	const FSnap To = Snap(B);
	Out.Add(From.Point);
	if (From.Axis != To.Axis)
	{
		Out.Add(From.Axis == 0 ? FVector2D(From.Line, To.Line) : FVector2D(To.Line, From.Line));
	}
	else if (From.Line != To.Line)
	{
		// Two parallel roads: cross over on whichever cross road makes the shortest trip.
		float BestCost = TNumericLimits<float>::Max();
		float Cross = 0.f;
		for (const float Line : RoadLines)
		{
			const float Cost = From.Axis == 0 ? FMath::Abs(From.Point.Y - Line) + FMath::Abs(To.Point.Y - Line)
				: FMath::Abs(From.Point.X - Line) + FMath::Abs(To.Point.X - Line);
			if (Cost < BestCost)
			{
				BestCost = Cost;
				Cross = Line;
			}
		}
		Out.Add(From.Axis == 0 ? FVector2D(From.Line, Cross) : FVector2D(Cross, From.Line));
		Out.Add(From.Axis == 0 ? FVector2D(To.Line, Cross) : FVector2D(Cross, To.Line));
	}
	Out.Add(To.Point);
	Out.Add(B);
	// Drop corners that fall on top of each other.
	for (int32 i = Out.Num() - 2; i >= 1; --i)
	{
		if (FVector2D::DistSquared(Out[i], Out[i + 1]) < FMath::Square(60.f) || FVector2D::DistSquared(Out[i], Out[i - 1]) < FMath::Square(60.f))
		{
			Out.RemoveAt(i);
		}
	}
}

float UGROjolSubsystem::RouteLength(const TArray<FVector2D>& Points)
{
	float Length = 0.f;
	for (int32 i = 1; i < Points.Num(); ++i)
	{
		Length += FVector2D::Distance(Points[i - 1], Points[i]);
	}
	return Length;
}

FString UGROjolSubsystem::WhyOffline() const
{
	const UGRStorySubsystem* Story = StoryOf(this);
	if (!Story)
	{
		return TEXT("Aplikasi belum siap.");
	}
	if (Story->GetVar(TEXT("ojek_blokir")) > 0)
	{
		return TEXT("Akun driver dibekukan sementara.");
	}
	if (Story->GetVar(TEXT("punya_motor")) == 0)
	{
		return TEXT("Belum punya motor.");
	}
	return FString();
}

bool UGROjolSubsystem::CanTakeOffers(FString* OutWhy) const
{
	FString Why = WhyOffline();
	const UGRStorySubsystem* Story = StoryOf(this);
	const AGRCharacter* Bayu = GetBayu();
	const UGRClockSubsystem* Clock = GetWorld()->GetGameInstance()->GetSubsystem<UGRClockSubsystem>();
	const int32 Hour = Clock ? Clock->GetHour() % 24 : 9;
	if (Why.IsEmpty() && Story && Story->GetActiveOrder())
	{
		Why = TEXT("Selesaikan order yang sedang jalan dulu.");
	}
	if (Why.IsEmpty() && AGRStealthLevel::GetActive(GetWorld()))
	{
		Why = TEXT("Sinyal hilang di sini.");
	}
	if (Why.IsEmpty() && Bayu && !Bayu->IsRiding())
	{
		Why = TEXT("Naik motor (F) supaya dapat order.");
	}
	if (Why.IsEmpty() && (Hour >= 23 || Hour < 5))
	{
		Why = TEXT("Tengah malam, order sepi.");
	}
	if (OutWhy)
	{
		*OutWhy = Why;
	}
	return Why.IsEmpty();
}

void UGROjolSubsystem::SetStage(EGROjolStage NewStage)
{
	Stage = NewStage;
	StageTime = 0.f;
	++Revision;
}

void UGROjolSubsystem::SetOnline(bool bOnline)
{
	if (bOnline == IsOnline())
	{
		return;
	}
	if (bOnline)
	{
		const FString Why = WhyOffline();
		if (!Why.IsEmpty())
		{
			Notify(this, TEXT("OjekKu: ") + Why, EGRNotifyType::Bad);
			return;
		}
		SetStage(EGROjolStage::Waiting);
		OfferTimer = 6.f;
		Notify(this, TEXT("OjekKu: kamu ONLINE. Tunggu order masuk."), EGRNotifyType::Good);
		return;
	}
	if (Stage == EGROjolStage::ToPickup || Stage == EGROjolStage::Boarding || Stage == EGROjolStage::Riding)
	{
		CancelTrip();
	}
	ChatChoices.Reset();
	SetStage(EGROjolStage::Offline);
	Route.Reset();
	Notify(this, TEXT("OjekKu: kamu OFFLINE."), EGRNotifyType::Info);
}

bool UGROjolSubsystem::MakeOffer()
{
	AGRCrowd* Crowd = AGRCrowd::Get(this);
	const AGRCharacter* Bayu = GetBayu();
	if (!Crowd || !Bayu)
	{
		return false;
	}
	const FVector Here = Bayu->GetActorLocation();
	FRandomStream Rng(Seed * 7919 + 17);
	++Seed;
	FVector Pickup;
	if (!Crowd->FindPickupSpot(Here, 2500.f, 8000.f, Pickup))
	{
		return false;
	}

	// Drop-off: a named place on the map, or a small spot somewhere in town.
	struct FDest
	{
		FVector Where;
		FString Label;
		bool bMarker;
	};
	TArray<FDest> Options;
	if (const UGRMapSubsystem* Map = GetWorld()->GetSubsystem<UGRMapSubsystem>())
	{
		for (const TWeakObjectPtr<UGRMapMarkerComponent>& Weak : Map->GetMarkers())
		{
			const UGRMapMarkerComponent* Marker = Weak.Get();
			if (!Marker || !Marker->bInLegend || Marker->Label.IsEmpty() || Cast<AGRMotor>(Marker->GetOwner()))
			{
				continue;
			}
			const FVector L = Marker->GetComponentLocation();
			const float D = FVector::Dist2D(L, Pickup);
			if (D > 3500.f && D < 45000.f)
			{
				Options.Add({ L, Marker->Label.ToString(), true });
			}
		}
	}
	for (int32 i = 0; i < 3; ++i)
	{
		const float Angle = Rng.FRandRange(0.f, 2.f * PI);
		const FVector Around(FMath::Clamp(Pickup.X + FMath::Cos(Angle) * Rng.FRandRange(5000.f, 16000.f), -45000.f, 45000.f),
			FMath::Clamp(Pickup.Y + FMath::Sin(Angle) * Rng.FRandRange(5000.f, 16000.f), -45000.f, 45000.f), Pickup.Z);
		FVector Spot;
		if (Crowd->FindPickupSpot(Around, 0.f, 2500.f, Spot) && FVector::Dist2D(Spot, Pickup) > 3500.f)
		{
			Options.Add({ Spot, SmallPlaces[Rng.RandRange(0, UE_ARRAY_COUNT(SmallPlaces) - 1)], false });
		}
	}
	if (Options.IsEmpty())
	{
		return false;
	}
	const FDest& Pick = Options[Rng.RandRange(0, Options.Num() - 1)];
	FVector Dest = Pick.Where;
	if (Pick.bMarker && !Crowd->FindGroundSpot(Pick.Where, 300.f, 1500.f, Dest))
	{
		Dest = Pick.Where;
	}

	TArray<FVector2D> Path;
	BuildRoute(FVector2D(Pickup), FVector2D(Dest), Path);
	const float Length = RouteLength(Path);

	Trip = FGROjolTrip();
	// AGRCrowd::SpawnPassenger dresses them to match.
	const int32 PaxSeed = 50000 + Seed * 13;
	Trip.bFemale = Rng.FRand() < 0.5f;
	Trip.Name = Trip.bFemale ? WomenNames[Rng.RandRange(0, UE_ARRAY_COUNT(WomenNames) - 1)] : MenNames[Rng.RandRange(0, UE_ARRAY_COUNT(MenNames) - 1)];
	Trip.Honorific = Trip.bFemale ? (Rng.FRand() < 0.5f ? TEXT("Bu") : TEXT("Mbak")) : (Rng.FRand() < 0.5f ? TEXT("Pak") : TEXT("Mas"));
	Trip.PaxRating = FMath::RoundToFloat(Rng.FRandRange(4.3f, 5.f) * 10.f) / 10.f;
	Trip.Pickup = Pickup;
	Trip.PickupYaw = (Here - Pickup).Rotation().Yaw;
	Trip.Dest = Dest;
	Trip.DestLabel = Pick.Label;
	Trip.Meters = Length / 100.f;
	Trip.Fare = FareFor(Trip.Meters);
	// About 32 km/h on the scooter, plus time to get going.
	Trip.ExpectedSeconds = Length / 900.f + 20.f;
	Trip.Question = Rng.RandRange(0, UE_ARRAY_COUNT(Questions) - 1);
	Trip.PickupLabel = TEXT("Pinggir jalan");
	float Nearest = 4000.f;
	if (const UGRMapSubsystem* Map = GetWorld()->GetSubsystem<UGRMapSubsystem>())
	{
		for (const TWeakObjectPtr<UGRMapMarkerComponent>& Weak : Map->GetMarkers())
		{
			const UGRMapMarkerComponent* Marker = Weak.Get();
			if (Marker && Marker->bInLegend && !Marker->Label.IsEmpty() && !Cast<AGRMotor>(Marker->GetOwner()))
			{
				if (const float D = FVector::Dist2D(Marker->GetComponentLocation(), Pickup); D < Nearest)
				{
					Nearest = D;
					Trip.PickupLabel = TEXT("Dekat ") + Marker->Label.ToString();
				}
			}
		}
	}
	PaxSeedNow = PaxSeed;
	OfferLeft = OfferSeconds;
	SetStage(EGROjolStage::Offer);
	GRSfx::Play(this, EGRSfx::OrderMasuk);
	return true;
}

void UGROjolSubsystem::ForceOffer()
{
	if (!IsOnline())
	{
		SetStage(EGROjolStage::Waiting);
	}
	if (Stage == EGROjolStage::Waiting || Stage == EGROjolStage::Summary)
	{
		MakeOffer();
	}
}

void UGROjolSubsystem::Accept()
{
	if (Stage != EGROjolStage::Offer)
	{
		return;
	}
	AGRCrowd* Crowd = AGRCrowd::Get(this);
	AGRWarga* Pax = Crowd ? Crowd->SpawnPassenger(Trip.Pickup, Trip.PickupYaw, PaxSeedNow, Trip.bFemale) : nullptr;
	if (!Pax)
	{
		Notify(this, TEXT("OjekKu: penumpang membatalkan pesanan."), EGRNotifyType::Warning);
		SetStage(EGROjolStage::Waiting);
		OfferTimer = 8.f;
		return;
	}
	Passenger = Pax;
	bWaved = false;
	bWalkingToSeat = false;
	RouteTimer = 0.f;
	SetStage(EGROjolStage::ToPickup);
	if (const AGRCharacter* Bayu = GetBayu())
	{
		UpdateRoute(Bayu);
	}
	Notify(this, FString::Printf(TEXT("Order diterima. Jemput %s %s, %s (%s)."), *Trip.Honorific, *Trip.Name, *Trip.PickupLabel, *Distance(RouteMeters)), EGRNotifyType::Good);
}

void UGROjolSubsystem::Decline()
{
	if (Stage != EGROjolStage::Offer)
	{
		return;
	}
	if (UGRStorySubsystem* Story = StoryOf(this))
	{
		Story->SetVar(TEXT("ojol_tolak"), Story->GetVar(TEXT("ojol_tolak")) + 1);
	}
	SetStage(EGROjolStage::Waiting);
	OfferTimer = FMath::FRandRange(8.f, 16.f);
}

void UGROjolSubsystem::StartChat(const FString& Prompt, const TArray<FGROjolChoice>& Choices, float Seconds)
{
	ChatPrompt = Prompt;
	ChatDefs = Choices;
	ChatChoices.Reset();
	for (const FGROjolChoice& Choice : Choices)
	{
		ChatChoices.Add(Choice.Text);
	}
	ChatLeft = Seconds;
	++Revision;
}

void UGROjolSubsystem::Choose(int32 Index)
{
	if (!IsChatOpen())
	{
		return;
	}
	if (ChatDefs.IsValidIndex(Index))
	{
		Trip.Comfort += ChatDefs[Index].Comfort;
		PassengerSay(ChatDefs[Index].Reply);
	}
	else
	{
		// No answer: the passenger feels ignored.
		Trip.Comfort -= 0.2f;
		PassengerSay(TEXT("...Mas? Halo?"), 2.5f);
	}
	ChatChoices.Reset();
	ChatDefs.Reset();
	ChatLeft = 0.f;
	++Revision;
}

void UGROjolSubsystem::PassengerSay(const FString& Text, float Seconds) const
{
	if (AGRWarga* Pax = Passenger.Get())
	{
		Pax->OjolSay(Text, Seconds);
	}
}

void UGROjolSubsystem::UpdateRoute(const AGRCharacter* Bayu)
{
	FVector Target;
	FText Label;
	if (!GetTarget(Target, Label))
	{
		Route.Reset();
		RouteMeters = 0.f;
		return;
	}
	BuildRoute(FVector2D(Bayu->GetActorLocation()), FVector2D(Target), Route);
	RouteMeters = RouteLength(Route) / 100.f;
}

bool UGROjolSubsystem::GetTarget(FVector& OutLocation, FText& OutLabel) const
{
	switch (Stage)
	{
	case EGROjolStage::ToPickup:
	case EGROjolStage::Boarding:
		if (const AGRWarga* Pax = Passenger.Get())
		{
			OutLocation = Pax->GetActorLocation();
		}
		else
		{
			OutLocation = Trip.Pickup;
		}
		OutLabel = FText::FromString(FString::Printf(TEXT("Jemput %s %s"), *Trip.Honorific, *Trip.Name));
		return true;
	case EGROjolStage::Riding:
		OutLocation = Trip.Dest;
		OutLabel = FText::FromString(Trip.DestLabel);
		return true;
	default:
		return false;
	}
}

FString UGROjolSubsystem::GetTurnText() const
{
	if (Route.Num() < 2)
	{
		return FString();
	}
	// The next corner more than a few metres ahead.
	int32 Corner = 1;
	while (Corner < Route.Num() - 1 && FVector2D::Distance(Route[0], Route[Corner]) < 300.f)
	{
		++Corner;
	}
	const float Ahead = FVector2D::Distance(Route[0], Route[Corner]) / 100.f;
	if (Corner >= Route.Num() - 1)
	{
		return FString::Printf(TEXT("Tujuan %s lagi"), *Distance(Ahead));
	}
	const FVector2D In = (Route[Corner] - Route[Corner - 1]).GetSafeNormal();
	const FVector2D Out = (Route[Corner + 1] - Route[Corner]).GetSafeNormal();
	const float Cross = In.X * Out.Y - In.Y * Out.X;
	if (FMath::Abs(Cross) < 0.35f)
	{
		return FString::Printf(TEXT("Lurus %s"), *Distance(Ahead));
	}
	return FString::Printf(TEXT("%s dalam %s"), Cross > 0.f ? TEXT("Belok kanan") : TEXT("Belok kiri"), *Distance(Ahead));
}

FString UGROjolSubsystem::GetStatusLine() const
{
	const AGRCharacter* Bayu = GetBayu();
	switch (Stage)
	{
	case EGROjolStage::Waiting:
	{
		FString Why;
		return CanTakeOffers(&Why) ? FString(TEXT("Menunggu order...")) : Why;
	}
	case EGROjolStage::Offer:
		return TEXT("Order masuk!");
	case EGROjolStage::ToPickup:
	{
		FString Line = FString::Printf(TEXT("JEMPUT  %s %s  |  %s"), *Trip.Honorific, *Trip.Name, *Trip.PickupLabel);
		if (Bayu && !Bayu->IsRiding())
		{
			Line += TEXT("  |  naik motor dulu (F)");
		}
		return Line;
	}
	case EGROjolStage::Boarding:
		return FString::Printf(TEXT("%s %s naik ke motor..."), *Trip.Honorific, *Trip.Name);
	case EGROjolStage::Riding:
		return FString::Printf(TEXT("ANTAR  %s %s  ke  %s"), *Trip.Honorific, *Trip.Name, *Trip.DestLabel);
	case EGROjolStage::Summary:
		return TEXT("Perjalanan selesai");
	default:
		return TEXT("Offline");
	}
}

float UGROjolSubsystem::GetRatingAverage() const
{
	const UGRStorySubsystem* Story = StoryOf(this);
	const int32 Count = Story ? Story->GetVar(TEXT("ojol_rating_n")) : 0;
	return Count > 0 ? static_cast<float>(Story->GetVar(TEXT("ojol_rating_sum"))) / Count : 0.f;
}

int32 UGROjolSubsystem::GetTripsDone() const
{
	const UGRStorySubsystem* Story = StoryOf(this);
	return Story ? Story->GetVar(TEXT("ojol_trips")) : 0;
}

int32 UGROjolSubsystem::GetEarnings() const
{
	const UGRStorySubsystem* Story = StoryOf(this);
	return Story ? Story->GetVar(TEXT("ojol_pendapatan")) : 0;
}

void UGROjolSubsystem::Board()
{
	AGRCharacter* Bayu = GetBayu();
	AGRWarga* Pax = Passenger.Get();
	AGRMotor* Motor = Bayu ? Bayu->GetMotor() : nullptr;
	if (!Pax || !Motor || !Bayu->IsRiding())
	{
		return;
	}
	USceneComponent* Seat = Motor->GetSeat();
	// Behind Bayu on the long seat: astride like him, or side-saddle facing right as many women ride.
	const float Half = Pax->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FTransform Wanted = Trip.bFemale
		? FTransform(FRotator(0.f, 90.f, 0.f), FVector(-46.f, -14.f, Half + 26.f)) * Motor->GetActorTransform()
		: FTransform(FRotator::ZeroRotator, FVector(-44.f, 0.f, Half + 4.f)) * Motor->GetActorTransform();
	Pax->OjolBoard(Seat, Wanted.GetRelativeTransform(Seat->GetComponentTransform()), Trip.bFemale, Bayu->GetRideAnim());
	PrevSpeed = FMath::Abs(Bayu->GetRideSpeed());
	SpeedingTime = 0.f;
	OffMotorTime = 0.f;
	ComplainCooldown = 3.f;
	Trip.Elapsed = 0.f;
	RouteTimer = 0.f;
	SetStage(EGROjolStage::Riding);
	UpdateRoute(Bayu);
}

void UGROjolSubsystem::Tick(float DeltaTime)
{
	if (Stage == EGROjolStage::Offline)
	{
		return;
	}
	AGRCharacter* Bayu = GetBayu();
	if (!Bayu)
	{
		return;
	}
	StageTime += DeltaTime;
	ComplainCooldown -= DeltaTime;
	const ULocalPlayer* LP = GetWorld()->GetFirstLocalPlayerFromController();
	const UGRUISubsystem* UI = LP ? LP->GetSubsystem<UGRUISubsystem>() : nullptr;
	const bool bScreens = UI && UI->HasScreens();

	// A jump (story scene, waking up at home): whatever trip there was cannot go on.
	const FVector Here = Bayu->GetActorLocation();
	const bool bJumped = !LastBayu.IsZero() && FVector::DistSquared2D(Here, LastBayu) > FMath::Square(3000.f);
	LastBayu = Here;
	if (bJumped && (Stage == EGROjolStage::ToPickup || Stage == EGROjolStage::Boarding || Stage == EGROjolStage::Riding))
	{
		Abandon(TEXT("OjekKu: perjalanan terputus."), false);
		return;
	}

	if (IsChatOpen() && !bScreens)
	{
		ChatLeft -= DeltaTime;
		if (ChatLeft <= 0.f)
		{
			Choose(-1);
		}
	}
	if ((RouteTimer -= DeltaTime) <= 0.f && (Stage == EGROjolStage::ToPickup || Stage == EGROjolStage::Boarding || Stage == EGROjolStage::Riding))
	{
		RouteTimer = 0.5f;
		UpdateRoute(Bayu);
	}

	switch (Stage)
	{
	case EGROjolStage::Waiting:
		if (bScreens || !CanTakeOffers(nullptr))
		{
			break;
		}
		if ((OfferTimer -= DeltaTime) <= 0.f && !MakeOffer())
		{
			OfferTimer = 4.f;
		}
		break;
	case EGROjolStage::Offer:
		if (!bScreens && (OfferLeft -= DeltaTime) <= 0.f)
		{
			Notify(this, TEXT("OjekKu: order terlewat, penumpang mencari driver lain."), EGRNotifyType::Warning);
			SetStage(EGROjolStage::Waiting);
			OfferTimer = FMath::FRandRange(10.f, 20.f);
		}
		break;
	case EGROjolStage::ToPickup:
	{
		AGRWarga* Pax = Passenger.Get();
		if (!Pax || !Pax->IsOjol())
		{
			Abandon(TEXT("OjekKu: penumpang membatalkan pesanan."), false);
			break;
		}
		const float Dist = FVector::Dist2D(Here, Pax->GetActorLocation());
		if (Dist < 1500.f)
		{
			if (!bWaved)
			{
				bWaved = true;
				Pax->OjolPose(2);
				Pax->OjolSay(TEXT("Mas! Di sini!"), 2.5f);
			}
			Pax->OjolFace(Here);
		}
		if (StageTime > 300.f)
		{
			Abandon(TEXT("OjekKu: penumpang kelamaan menunggu dan membatalkan. Rating akunmu turun."), true);
			break;
		}
		if (Dist < PickupReach && Bayu->IsRiding() && FMath::Abs(Bayu->GetRideSpeed()) < StopSpeed)
		{
			SetStage(EGROjolStage::Boarding);
			bWalkingToSeat = false;
			Pax->OjolPose(0);
			Pax->OjolFace(Here);
			Pax->OjolSay(FString::Printf(TEXT("Mas Bayu ya? Saya %s, ke %s."), *Trip.Name, *Trip.DestLabel), 4.f);
			TArray<FGROjolChoice> Greet = {
				{ FString::Printf(TEXT("Dengan %s %s? Ini helmnya, silakan."), *Trip.Honorific, *Trip.Name), TEXT("Iya, Mas. Makasih."), 0.3f },
				{ TEXT("Ayo naik, buruan."), TEXT("Iya, iya... sabar dong, Mas."), -0.3f },
				{ FString::Printf(TEXT("Sesuai titik ya, %s, ke %s."), *Trip.Honorific, *Trip.DestLabel), TEXT("Betul, Mas."), 0.1f },
			};
			StartChat(TEXT("Sapa penumpang"), Greet, 10.f);
		}
		break;
	}
	case EGROjolStage::Boarding:
	{
		AGRWarga* Pax = Passenger.Get();
		AGRMotor* Motor = Bayu->GetMotor();
		if (!Pax || !Motor)
		{
			Abandon(TEXT("OjekKu: penumpang membatalkan pesanan."), false);
			break;
		}
		if (FVector::Dist2D(Here, Pax->GetActorLocation()) > 1200.f || !Bayu->IsRiding())
		{
			// Rode off (or got off) before they climbed on.
			ChatChoices.Reset();
			Trip.Comfort -= 0.5f;
			Pax->OjolSay(TEXT("Lho, Mas, mau ke mana?!"), 2.5f);
			SetStage(EGROjolStage::ToPickup);
			break;
		}
		if (IsChatOpen())
		{
			break;
		}
		if (!bWalkingToSeat)
		{
			bWalkingToSeat = true;
			FVector Side = Motor->GetActorTransform().TransformPosition(FVector(-40.f, Trip.bFemale ? 75.f : -75.f, 0.f));
			Side.Z = Pax->GetActorLocation().Z;
			Pax->OjolWalkTo(Side);
		}
		else if (!Pax->IsOjolWalking() && StageTime > 1.f)
		{
			Board();
		}
		break;
	}
	case EGROjolStage::Riding:
		TickRide(DeltaTime, Bayu);
		break;
	case EGROjolStage::Summary:
		SummaryTime += DeltaTime;
		if (SummaryTime > 7.f)
		{
			SetStage(EGROjolStage::Waiting);
			OfferTimer = FMath::FRandRange(12.f, 25.f);
		}
		break;
	default:
		break;
	}
}

void UGROjolSubsystem::TickRide(float DeltaTime, AGRCharacter* Bayu)
{
	AGRWarga* Pax = Passenger.Get();
	if (!Pax)
	{
		Abandon(TEXT("OjekKu: perjalanan terputus."), false);
		return;
	}
	Trip.Elapsed += DeltaTime;
	if (Bayu->IsKnockedDown())
	{
		// Both come off: no fare, one star, and a passenger on the asphalt.
		Trip.bCrashed = true;
		Pax->OjolFall(Bayu->GetVelocity() * 0.8f + FVector(0.f, 0.f, 250.f));
		Passenger = nullptr;
		Finish(false);
		return;
	}
	if (!Bayu->IsRiding())
	{
		if ((OffMotorTime += DeltaTime) > 3.f)
		{
			Pax->OjolSay(TEXT("Lho, kok diturunin di sini, Mas?!"), 3.f);
			Finish(FVector::Dist2D(Bayu->GetActorLocation(), Trip.Dest) < DropReach * 1.5f);
		}
		return;
	}
	OffMotorTime = 0.f;

	const float Speed = FMath::Abs(Bayu->GetRideSpeed());
	const float Decel = DeltaTime > 0.f ? (PrevSpeed - Speed) / DeltaTime : 0.f;
	if (PrevSpeed > 380.f && Decel > JoltDecel)
	{
		++Trip.Jolts;
		Trip.Comfort -= 0.7f;
		if (ComplainCooldown <= 0.f)
		{
			ComplainCooldown = 4.f;
			Pax->OjolSay(JoltLines[FMath::RandRange(0, UE_ARRAY_COUNT(JoltLines) - 1)], 2.5f);
		}
	}
	PrevSpeed = Speed;
	if (Speed > SpeedingSpeed)
	{
		SpeedingTime += DeltaTime;
		if (SpeedingTime > 3.f)
		{
			SpeedingTime = 0.f;
			Trip.Comfort -= 0.4f;
			if (ComplainCooldown <= 0.f)
			{
				ComplainCooldown = 5.f;
				Pax->OjolSay(SpeedLines[FMath::RandRange(0, UE_ARRAY_COUNT(SpeedLines) - 1)], 2.5f);
			}
		}
	}
	else
	{
		SpeedingTime = FMath::Max(0.f, SpeedingTime - DeltaTime * 0.5f);
	}

	if (!Trip.bAsked && Trip.Elapsed > 7.f && !IsChatOpen())
	{
		Trip.bAsked = true;
		const FQuestion& Q = Questions[Trip.Question % UE_ARRAY_COUNT(Questions)];
		Pax->OjolSay(Q.Ask, 5.f);
		StartChat(Q.Ask, { Q.Answers[0], Q.Answers[1], Q.Answers[2] }, 14.f);
	}

	if (FVector::Dist2D(Bayu->GetActorLocation(), Trip.Dest) < DropReach && Speed < StopSpeed)
	{
		Finish(true);
	}
}

void UGROjolSubsystem::Finish(bool bCompleted)
{
	ChatChoices.Reset();
	ChatDefs.Reset();
	AGRCharacter* Bayu = GetBayu();
	AGRWarga* Pax = Passenger.Get();
	UGRStorySubsystem* Story = StoryOf(this);
	UGRStatusSubsystem* Status = GetWorld()->GetGameInstance()->GetSubsystem<UGRStatusSubsystem>();
	if (bCompleted)
	{
		const float Late = Trip.Elapsed / FMath::Max(Trip.ExpectedSeconds, 1.f);
		if (Late > 1.8f)
		{
			Trip.Comfort -= 2.f;
		}
		else if (Late > 1.3f)
		{
			Trip.Comfort -= 1.f;
		}
		Trip.Stars = FMath::Clamp(FMath::RoundToInt(Trip.Comfort), 1, 5);
		Trip.Paid = FMath::RoundToInt(Trip.Fare * 0.8f);
		Trip.Tip = Trip.Stars == 5 && FMath::FRand() < 0.6f ? FMath::RandRange(4, 12) * 500 : 0;
		static const TCHAR* const Thanks[] = { TEXT("Nggak lagi-lagi deh."), TEXT("Lain kali hati-hati, Mas."), TEXT("Ya. Makasih."), TEXT("Makasih, Mas."), TEXT("Makasih, Mas! Bintang lima ya.") };
		static const TCHAR* const Reviews[] = { TEXT("\"Bikin jantungan. Jangan pesan driver ini.\""), TEXT("\"Ugal-ugalan, saya hampir jatuh.\""), TEXT("\"Lumayan, tapi kurang hati-hati.\""),
			TEXT("\"Aman, sampai tujuan.\""), TEXT("\"Driver ramah, nyetirnya halus. Mantap!\"") };
		Trip.Review = Reviews[Trip.Stars - 1];
		if (Late > 1.3f && Trip.Stars < 5)
		{
			Trip.Review += TEXT(" (telat)");
		}
		if (Pax && Bayu)
		{
			const float Half = Bayu->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			const FVector Feet = Bayu->GetActorTransform().TransformPosition(FVector(-30.f, Trip.bFemale ? 85.f : -85.f, 0.f)) - FVector(0.f, 0.f, Half);
			Pax->OjolAlight(Feet, Bayu->GetActorRotation().Yaw + (Trip.bFemale ? 90.f : -90.f));
			Pax->OjolSay(Thanks[Trip.Stars - 1], 3.5f);
			Pax->OjolRelease();
		}
		if (Status)
		{
			Status->AddUang(Trip.Paid + Trip.Tip);
		}
		if (Story)
		{
			Story->AddLedger(Trip.Paid, FString::Printf(TEXT("OjekKu: %s %s (potongan aplikasi 20%%)"), *Trip.Honorific, *Trip.Name));
			if (Trip.Tip > 0)
			{
				Story->AddLedger(Trip.Tip, TEXT("Tip penumpang OjekKu"));
			}
			Story->SetVar(TEXT("ojol_trips"), Story->GetVar(TEXT("ojol_trips")) + 1);
			Story->SetVar(TEXT("ojol_pendapatan"), Story->GetVar(TEXT("ojol_pendapatan")) + Trip.Paid + Trip.Tip);
			Story->AddEnergi(-3.f);
		}
	}
	else
	{
		Trip.Stars = 1;
		Trip.Paid = 0;
		Trip.Tip = 0;
		Trip.Review = Trip.bCrashed ? TEXT("\"Saya jatuh dari motor! Lapor ke aplikasi.\"") : TEXT("\"Diturunkan di tengah jalan.\"");
		if (Pax)
		{
			Pax->OjolRelease();
		}
	}
	if (Story)
	{
		Story->SetVar(TEXT("ojol_rating_sum"), Story->GetVar(TEXT("ojol_rating_sum")) + Trip.Stars);
		Story->SetVar(TEXT("ojol_rating_n"), Story->GetVar(TEXT("ojol_rating_n")) + 1);
	}
	Passenger = nullptr;
	Route.Reset();
	RouteMeters = 0.f;
	SummaryTime = 0.f;
	SetStage(EGROjolStage::Summary);
}

void UGROjolSubsystem::Abandon(const FString& Why, bool bPenalty)
{
	ChatChoices.Reset();
	ChatDefs.Reset();
	if (AGRWarga* Pax = Passenger.Get())
	{
		Pax->OjolRelease();
	}
	Passenger = nullptr;
	Route.Reset();
	RouteMeters = 0.f;
	if (bPenalty)
	{
		if (UGRStorySubsystem* Story = StoryOf(this))
		{
			Story->SetVar(TEXT("ojol_rating_sum"), Story->GetVar(TEXT("ojol_rating_sum")) + 1);
			Story->SetVar(TEXT("ojol_rating_n"), Story->GetVar(TEXT("ojol_rating_n")) + 1);
		}
	}
	Notify(this, Why, bPenalty ? EGRNotifyType::Bad : EGRNotifyType::Warning);
	SetStage(EGROjolStage::Waiting);
	OfferTimer = 20.f;
}

void UGROjolSubsystem::CancelTrip()
{
	if (Stage == EGROjolStage::Offer)
	{
		Decline();
		return;
	}
	if (Stage == EGROjolStage::ToPickup || Stage == EGROjolStage::Boarding || Stage == EGROjolStage::Riding)
	{
		Abandon(TEXT("OjekKu: kamu membatalkan perjalanan. Rating akunmu turun."), true);
	}
}

void UGROjolSubsystem::CheatJumpToTarget()
{
	AGRCharacter* Bayu = GetBayu();
	FVector Target;
	FText Label;
	if (!Bayu || !GetTarget(Target, Label))
	{
		return;
	}
	FVector Away = (Bayu->GetActorLocation() - Target).GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = FVector(1.f, 0.f, 0.f);
	}
	const FVector Where = Target + Away * 320.f + FVector(0.f, 0.f, 120.f);
	Bayu->SetActorLocationAndRotation(Where, FRotator(0.f, (-Away).Rotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* Controller = Bayu->GetController())
	{
		Controller->SetControlRotation(FRotator(-15.f, (-Away).Rotation().Yaw, 0.f));
	}
	LastBayu = Bayu->GetActorLocation();
}

void UGROjolSubsystem::PassengerHit(AGRWarga* Who)
{
	if (Who && Who == Passenger.Get())
	{
		Passenger = nullptr;
		Abandon(TEXT("Kamu menabrak calon penumpangmu! Order batal, akunmu kena peringatan."), true);
	}
}
