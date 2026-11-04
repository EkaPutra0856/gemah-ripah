#include "World/GRTraffic.h"
#include "Player/GRCharacter.h"
#include "Story/GRStoryUtil.h"
#include "World/GRMotor.h"
#include "World/GRPropModels.h"
#include "Algo/BinarySearch.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogGRTraffic, Log, All);

namespace
{
	// Road layout from build_kota_sentosa.py: Jl. Merdeka along X, Jl. Raya along Y, 16 m wide, Tugu island
	// (12 m radius) at the crossing. Arm k points along angle k * 90 degrees.
	constexpr float Lane = 650.f;
	constexpr float RingRadius = 1650.f;
	/** Where the straight lane turns into the curve onto the ring: the give-way line. */
	constexpr float ApproachStart = 2800.f;
	/** How far round the ring the entry curve joins it, past where the lane line would meet it. */
	constexpr float JoinDegrees = 20.f;
	/** Edge of town: cars U-turn here, out of sight of the centre. */
	constexpr float FarEnd = 25000.f;
	constexpr float SampleStep = 100.f;
	constexpr int32 CarCount = 20;

	// Intelligent driver model, cm and seconds.
	constexpr float MaxAccel = 280.f;
	constexpr float ComfortBrake = 300.f;
	constexpr float HardBrake = 950.f;
	constexpr float MinGap = 250.f;
	constexpr float Headway = 1.2f;
	/** Sideways acceleration drivers accept in curves. */
	constexpr float LatAccel = 260.f;
	constexpr float NoGap = 1e9f;

	/** Bayu and the scooter together, kg. */
	constexpr float RiderMass = 200.f;

	FVector2D Polar(float Angle, float Radius)
	{
		return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
	}

	void AddBezier(TArray<FVector2D>& Out, const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D)
	{
		const int32 Steps = FMath::Max(2, FMath::CeilToInt((FVector2D::Distance(A, B) + FVector2D::Distance(B, C) + FVector2D::Distance(C, D)) / SampleStep));
		for (int32 i = 0; i <= Steps; ++i)
		{
			const float T = static_cast<float>(i) / Steps;
			const float U = 1.f - T;
			Out.Add(A * (U * U * U) + B * (3.f * U * U * T) + C * (3.f * U * T * T) + D * (T * T * T));
		}
	}

	const TCHAR* const CarColors[] = { TEXT("#B8BEC6"), TEXT("#F0F0EC"), TEXT("#26282C"), TEXT("#9B2A22"), TEXT("#2D4E86"),
		TEXT("#6E7378"), TEXT("#D9D2C0"), TEXT("#3E6B4A") };
	const TCHAR* const AngkotColors[] = { TEXT("#2A8C82"), TEXT("#2F6DB5"), TEXT("#C9A227"), TEXT("#B23A3A") };
}

// ---------------------------------------------------------------------------------------------------------------------

AGRTrafficCar::AGRTrafficCar()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
	RootComponent = Body;
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetCollisionObjectType(ECC_Vehicle);
	Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Body->SetCollisionResponseToAllChannels(ECR_Block);
	// Driving, the lanes are clear and the kerbs would only snag; sliding after a knock, the road holds it up.
	Body->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Body->SetNotifyRigidBodyCollision(true);
	Body->CanCharacterStepUpOn = ECB_No;
	Body->SetCanEverAffectNavigation(false);
	Body->SetLinearDamping(0.2f);
	Body->SetAngularDamping(1.5f);
	// Engine and axles sit low: a shove makes it rock and slide, not tip over.
	Body->BodyInstance.COMNudge = FVector(0.f, 0.f, -35.f);

	ShoutText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ShoutText"));
	ShoutText->SetupAttachment(Body);
	ShoutText->SetUsingAbsoluteLocation(true);
	ShoutText->SetUsingAbsoluteRotation(true);
	ShoutText->SetHorizontalAlignment(EHTA_Center);
	ShoutText->SetVerticalAlignment(EVRTA_TextCenter);
	ShoutText->SetWorldSize(42.f);
	ShoutText->SetHiddenInGame(true);
	ShoutText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGRTrafficCar::Build(bool bInAngkot, const TCHAR* Color, float InCruise)
{
	bAngkot = bInAngkot;
	Cruise = InCruise;
	HalfLength = bAngkot ? 237.f : 212.f;
	HalfWidth = 88.f;
	HalfHeight = bAngkot ? 96.f : 70.f;
	// The box reaches the ground so a sliding car rests on its wheels.
	Body->SetBoxExtent(FVector(HalfLength, HalfWidth, HalfHeight));
	Body->SetMassOverrideInKg(NAME_None, bAngkot ? 1500.f : 1100.f, true);

	TArray<FGRPropPart> Parts;
	GRPropModels::Build(bAngkot ? TEXT("angkot") : TEXT("mobil"), {}, Color, 0, Parts);
	TMap<FColor, UMaterialInstanceDynamic*> Materials;
	for (const FGRPropPart& Part : Parts)
	{
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetupAttachment(Body);
		Comp->SetStaticMesh(GRStoryUtil::LoadShape(Part.Shape));
		Comp->SetRelativeTransform(FTransform(Part.Rot, Part.Loc - FVector(0.f, 0.f, HalfHeight), Part.Size / 100.f));
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCanEverAffectNavigation(false);
		UMaterialInstanceDynamic*& Material = Materials.FindOrAdd(Part.Color.ToFColor(true));
		if (!Material)
		{
			Material = GRStoryUtil::MakeColorMaterial(this, Part.Color, 0.9f);
		}
		Comp->SetMaterial(0, Material);
		Comp->RegisterComponent();
	}
	Body->OnComponentHit.AddDynamic(this, &AGRTrafficCar::OnBodyHit);
}

FVector AGRTrafficCar::GetMoveVelocity() const
{
	return bKnocked ? Body->GetPhysicsLinearVelocity() : GetActorForwardVector() * Speed;
}

void AGRTrafficCar::Knock(const FVector& Impulse, const FVector& Point)
{
	if (!bKnocked)
	{
		const FVector Velocity = GetMoveVelocity();
		bKnocked = true;
		KnockTime = 0.f;
		Speed = 0.f;
		HoldTime = 0.f;
		Body->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Body->SetSimulatePhysics(true);
		Body->SetPhysicsLinearVelocity(Velocity);
	}
	Body->AddImpulseAtLocation(Impulse, Point);
}

void AGRTrafficCar::HitByRider(float Closing, const FVector& Direction, const FVector& Point)
{
	if (HitCooldown > 0.f)
	{
		return;
	}
	HitCooldown = 0.4f;
	UE_LOG(LogGRTraffic, Display, TEXT("GRTABRAK motor menabrak %s, %.0f cm/s"), *GetName(), Closing);
	// Momentum of scooter and rider, with a little extra so the shove reads on screen, and a lift that rocks the body.
	const float Momentum = Closing * RiderMass * 1.6f;
	Knock(Direction.GetSafeNormal2D() * Momentum + FVector(0.f, 0.f, Momentum * 0.15f), Point);
	Shout(Closing > 700.f ? TEXT("BRAKK!!") : TEXT("BRAK!"), FColor(255, 90, 60), 1.f);
}

void AGRTrafficCar::EndKnock()
{
	bKnocked = false;
	Body->SetSimulatePhysics(false);
	Body->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	// Back on its wheels; AGRTraffic finds the lane and the driver steers back into it.
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	bRejoin = true;
	Shout(FMath::RandBool() ? TEXT("Woi! Hati-hati!") : TEXT("Aduh, mobilku!"), FColor(255, 230, 120), 2.f);
}

void AGRTrafficCar::OnBodyHit(UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!Other || Other == this || HitCooldown > 0.f)
	{
		return;
	}
	const FVector Toward = (Other->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const float Mass = bAngkot ? 1500.f : 1100.f;
	if (AGRCharacter* Bayu = Cast<AGRCharacter>(Other))
	{
		// The scooter's own speed, not the capsule's: after the block (or a long frame that skipped the bumper
		// check) the capsule already reads as stopped. On foot, walking into a car is just being in the way.
		const float CarPart = FVector::DotProduct(GetMoveVelocity(), Toward);
		const float BayuPart = Bayu->IsRiding() ? FVector::DotProduct(Bayu->GetActorForwardVector() * Bayu->GetRideSpeed(), -Toward) : 0.f;
		const float Closing = CarPart + BayuPart;
		if (Closing < 150.f)
		{
			return;
		}
		UE_LOG(LogGRTraffic, Display, TEXT("GRTABRAK %s dan Bayu, %.0f cm/s"), *GetName(), Closing);
		if (BayuPart >= CarPart)
		{
			const FVector Heading = Bayu->GetActorForwardVector() * FMath::Sign(Bayu->GetRideSpeed());
			HitByRider(Closing, Heading, Hit.ImpactPoint);
			Bayu->HandleCarImpact(Closing, Heading, true);
		}
		else
		{
			HitCooldown = 0.5f;
			Bayu->HandleCarImpact(Closing, Toward, false);
			Shout(TEXT("BRAK!"), FColor(255, 90, 60), 1.f);
		}
		return;
	}
	AGRTrafficCar* OtherCar = Cast<AGRTrafficCar>(Other);
	const FVector OtherVelocity = OtherCar ? OtherCar->GetMoveVelocity() : Other->GetVelocity();
	const float Closing = FVector::DotProduct(GetMoveVelocity() - OtherVelocity, Toward);
	if (Closing < 150.f)
	{
		return;
	}
	HitCooldown = 0.5f;
	if (OtherCar)
	{
		// Car into car: the one hit slides off too, so a shove can run down a queue.
		OtherCar->HitCooldown = 0.5f;
		OtherCar->Knock(Toward * Closing * Mass * 0.5f, Hit.ImpactPoint);
		if (!bKnocked)
		{
			Knock(-Toward * Closing * Mass * 0.3f, Hit.ImpactPoint);
		}
		Shout(TEXT("BRAK!"), FColor(255, 90, 60), 1.f);
	}
}

void AGRTrafficCar::Shout(const FString& Text, const FColor& Color, float Duration)
{
	ShoutText->SetText(FText::FromString(Text));
	ShoutText->SetTextRenderColor(Color);
	ShoutText->SetHiddenInGame(false);
	ShoutTime = Duration;
}

void AGRTrafficCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	HitCooldown -= DeltaSeconds;
	HonkCooldown -= DeltaSeconds;
	if (bKnocked)
	{
		KnockTime += DeltaSeconds;
		const bool bSettled = Body->GetPhysicsLinearVelocity().Size() < 40.f && Body->GetPhysicsAngularVelocityInDegrees().Size() < 25.f;
		if ((KnockTime > 1.2f && bSettled) || KnockTime > 6.f)
		{
			EndKnock();
		}
	}
	if (ShoutTime > 0.f)
	{
		ShoutTime -= DeltaSeconds;
		if (ShoutTime <= 0.f)
		{
			ShoutText->SetHiddenInGame(true);
		}
		else if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			const FVector Above = GetActorLocation() + FVector(0.f, 0.f, HalfHeight + 70.f);
			ShoutText->SetWorldLocationAndRotation(Above, FRotator(0.f, (Camera->GetCameraLocation() - Above).Rotation().Yaw, 0.f));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------

AGRTraffic::AGRTraffic()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Rng.Initialize(20261104);
}

AGRTraffic* AGRTraffic::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<AGRTraffic> It(World);
	return It ? *It : nullptr;
}

void AGRTraffic::BeginPlay()
{
	Super::BeginPlay();
	// The map has two straight roads crossing with pavement strips over the crossing; a thin asphalt disc just
	// above the pavement (hidden inside the Tugu island) turns the crossing into a roundabout.
	RingRoad = NewObject<UStaticMeshComponent>(this);
	RingRoad->SetStaticMesh(GRStoryUtil::LoadShape(TEXT("cylinder")));
	RingRoad->SetupAttachment(RootComponent);
	RingRoad->SetWorldLocation(FVector(0.f, 0.f, 12.f));
	RingRoad->SetWorldScale3D(FVector(50.f, 50.f, 0.01f));
	RingRoad->SetCollisionProfileName(TEXT("BlockAll"));
	RingRoad->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	RingRoad->SetCanEverAffectNavigation(false);
	RingRoad->RegisterComponent();
	GRStoryUtil::ApplyColor(RingRoad, FLinearColor(0.07f, 0.07f, 0.08f));
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < CarCount; ++i)
	{
		const int32 Arm = i % 4;
		const int32 Slot = i / 4;
		const bool bAngkot = i % 5 == 2;
		AGRTrafficCar* Car = GetWorld()->SpawnActor<AGRTrafficCar>(AGRTrafficCar::StaticClass(), FVector(0.f, 0.f, -5000.f), FRotator::ZeroRotator, Params);
		Car->Build(bAngkot, bAngkot ? AngkotColors[i % UE_ARRAY_COUNT(AngkotColors)] : CarColors[i % UE_ARRAY_COUNT(CarColors)],
			bAngkot ? Rng.FRandRange(800.f, 1000.f) : Rng.FRandRange(950.f, 1300.f));
		Car->NextArm = Arm;
		AppendLeg(*Car);
		AppendLeg(*Car);

		// Spread out along the first leg, never starting on the roundabout or on top of another car.
		const float RingFrom = Car->Yields[0].S - 1500.f;
		float RingTo = Car->LegEnds[0];
		for (int32 j = Algo::LowerBound(Car->PathDist, Car->Yields[0].S); j < Car->Path.Num(); ++j)
		{
			if (Car->Path[j].Size() > 3500.f)
			{
				RingTo = Car->PathDist[j];
				break;
			}
		}
		float At = (Slot + 0.5f) / 5.f * Car->LegEnds[0] + Arm * 1300.f;
		for (int32 Try = 0; Try < 20; ++Try)
		{
			if (At > RingFrom && At < RingTo)
			{
				At = RingTo;
			}
			const FVector2D Spot = PathAt(*Car, At);
			const bool bCrowded = Cars.ContainsByPredicate([&Spot](const AGRTrafficCar* Other)
			{
				return FVector2D::Distance(FVector2D(Other->GetActorLocation()), Spot) < 1400.f;
			});
			if (!bCrowded)
			{
				break;
			}
			At += 1500.f;
		}
		Car->S = At;
		Car->Speed = Car->Cruise * 0.7f;
		FVector2D Dir;
		const FVector2D Spot = PathAt(*Car, At, &Dir);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByObjectType(Hit, FVector(Spot, 500.f), FVector(Spot, -500.f), FCollisionObjectQueryParams(ECC_WorldStatic)))
		{
			Car->GroundZ = Hit.ImpactPoint.Z;
		}
		Car->SetActorLocationAndRotation(FVector(Spot, Car->GroundZ + Car->HalfHeight), FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)), 0.f));
		Cars.Add(Car);
	}
	UE_LOG(LogGRTraffic, Display, TEXT("Lalu lintas: %d kendaraan"), Cars.Num());
}

void AGRTraffic::AppendLeg(AGRTrafficCar& Car)
{
	// Keeping left, the first exit is the left turn: left 25%, straight on 50%, right 25%.
	const float Roll = Rng.FRand();
	const int32 Turns = Roll < 0.25f ? 1 : (Roll < 0.75f ? 2 : 3);
	const float Phi = Car.NextArm * HALF_PI;
	const float Psi = Phi + Turns * HALF_PI;
	const FVector2D U(FMath::Cos(Phi), FMath::Sin(Phi));
	const FVector2D V(-U.Y, U.X);
	const FVector2D U2(FMath::Cos(Psi), FMath::Sin(Psi));
	const FVector2D V2(-U2.Y, U2.X);
	const float Join = FMath::Asin(Lane / RingRadius) + FMath::DegreesToRadians(JoinDegrees);

	// Inbound on the left (+V seen from the centre), toward the Tugu.
	TArray<FVector2D> Points;
	for (float Along = FarEnd; Along > ApproachStart + 1.f; Along -= SampleStep)
	{
		Points.Add(U * Along + V * Lane);
	}
	const int32 GiveWay = Points.Num();
	// Curve onto the ring, which runs clockwise on the map (angle increasing), island on the driver's right.
	const FVector2D P0 = U * ApproachStart + V * Lane;
	const float InAngle = Phi + Join;
	const FVector2D P3 = Polar(InAngle, RingRadius);
	const FVector2D InTangent(-FMath::Sin(InAngle), FMath::Cos(InAngle));
	const float K = FVector2D::Distance(P0, P3) * 0.35f;
	AddBezier(Points, P0, P0 - U * K, P3 - InTangent * K, P3);
	const float OutAngle = Psi - Join;
	const int32 ArcSteps = FMath::Max(2, FMath::CeilToInt((OutAngle - InAngle) * RingRadius / SampleStep));
	for (int32 i = 1; i < ArcSteps; ++i)
	{
		Points.Add(Polar(InAngle + (OutAngle - InAngle) * i / ArcSteps, RingRadius));
	}
	// Off the ring onto the outbound lane of the exit arm.
	const FVector2D Q0 = Polar(OutAngle, RingRadius);
	const FVector2D OutTangent(-FMath::Sin(OutAngle), FMath::Cos(OutAngle));
	const FVector2D Q3 = U2 * ApproachStart - V2 * Lane;
	const float K2 = FVector2D::Distance(Q0, Q3) * 0.35f;
	AddBezier(Points, Q0, Q0 + OutTangent * K2, Q3 - U2 * K2, Q3);
	for (float Along = ApproachStart + SampleStep; Along < FarEnd - 1.f; Along += SampleStep)
	{
		Points.Add(U2 * Along - V2 * Lane);
	}
	// U-turn to the right across the road at the edge of town, ending on that arm's inbound lane.
	const FVector2D Centre = U2 * FarEnd;
	const int32 TurnSteps = FMath::CeilToInt(PI * Lane / SampleStep);
	for (int32 i = 0; i <= TurnSteps; ++i)
	{
		const float Beta = PI * i / TurnSteps;
		Points.Add(Centre + (V2 * -FMath::Cos(Beta) + U2 * FMath::Sin(Beta)) * Lane);
	}
	Car.NextArm = (Car.NextArm + Turns) % 4;

	// The last leg ended where this one starts.
	const int32 First = Car.Path.Num() > 0 ? 1 : 0;
	const int32 Base = Car.Path.Num();
	for (int32 i = First; i < Points.Num(); ++i)
	{
		const float D = Car.Path.Num() > 0 ? Car.PathDist.Last() + FVector2D::Distance(Car.Path.Last(), Points[i]) : 0.f;
		Car.Path.Add(Points[i]);
		Car.PathDist.Add(D);
		Car.PathLimit.Add(Car.Cruise);
	}
	Car.Yields.Add({ Car.PathDist[Base + GiveWay - First], InAngle });
	Car.LegEnds.Add(Car.PathDist.Last());

	// Curve speeds: v = sqrt(lateral accel / curvature), from the turn at each point.
	for (int32 i = FMath::Max(Base - 1, 1); i < Car.Path.Num() - 1; ++i)
	{
		const FVector2D In = Car.Path[i] - Car.Path[i - 1];
		const FVector2D Out = Car.Path[i + 1] - Car.Path[i];
		const float Turn = FMath::Abs(FMath::FindDeltaAngleRadians(FMath::Atan2(In.Y, In.X), FMath::Atan2(Out.Y, Out.X)));
		const float Curvature = Turn / FMath::Max((In.Size() + Out.Size()) * 0.5f, 1.f);
		Car.PathLimit[i] = Curvature > 1e-5f ? FMath::Clamp(FMath::Sqrt(LatAccel / Curvature), 200.f, Car.Cruise) : Car.Cruise;
	}
}

void AGRTraffic::DropFirstLeg(AGRTrafficCar& Car)
{
	const float Cut = Car.LegEnds[0];
	const int32 Keep = FMath::Clamp(Algo::LowerBound(Car.PathDist, Cut - 0.5f), 0, Car.Path.Num() - 1);
	Car.Path.RemoveAt(0, Keep);
	Car.PathDist.RemoveAt(0, Keep);
	Car.PathLimit.RemoveAt(0, Keep);
	for (float& D : Car.PathDist)
	{
		D -= Cut;
	}
	Car.S -= Cut;
	Car.LegEnds.RemoveAt(0);
	for (float& End : Car.LegEnds)
	{
		End -= Cut;
	}
	for (AGRTrafficCar::FYield& Yield : Car.Yields)
	{
		Yield.S -= Cut;
	}
	Car.Yields.RemoveAll([](const AGRTrafficCar::FYield& Yield) { return Yield.S < -1000.f; });
	AppendLeg(Car);
}

FVector2D AGRTraffic::PathAt(const AGRTrafficCar& Car, float At, FVector2D* OutDir) const
{
	auto Sample = [&Car](float D)
	{
		const int32 Num = Car.Path.Num();
		const float Clamped = FMath::Clamp(D, 0.f, Car.PathDist.Last());
		const int32 i = FMath::Clamp(Algo::UpperBound(Car.PathDist, Clamped) - 1, 0, Num - 2);
		const float Len = FMath::Max(Car.PathDist[i + 1] - Car.PathDist[i], 0.01f);
		return FMath::Lerp(Car.Path[i], Car.Path[i + 1], FMath::Clamp((Clamped - Car.PathDist[i]) / Len, 0.f, 1.f));
	};
	if (OutDir)
	{
		*OutDir = (Sample(At + 120.f) - Sample(At - 120.f)).GetSafeNormal();
	}
	return Sample(At);
}

void AGRTraffic::Rejoin(AGRTrafficCar& Car)
{
	Car.bRejoin = false;
	Car.Speed = 0.f;
	Car.Shift = 0.f;
	Car.ShiftTarget = 0.f;
	Car.ShiftYaw = 0.f;
	Car.bAvoiding = false;
	const FVector2D Here(Car.GetActorLocation());
	int32 Best = 0;
	float BestDist = MAX_flt;
	for (int32 i = 0; i < Car.Path.Num(); ++i)
	{
		const float D = FVector2D::DistSquared(Car.Path[i], Here);
		if (D < BestDist)
		{
			BestDist = D;
			Best = i;
		}
	}
	if (BestDist > FMath::Square(1500.f))
	{
		// Shoved (or placed) far from its own route: start a fresh route on the inbound lane of the nearest arm.
		const int32 Arm = (FMath::RoundToInt(FMath::Atan2(Here.Y, Here.X) / HALF_PI) % 4 + 4) % 4;
		const FVector2D U(FMath::Cos(Arm * HALF_PI), FMath::Sin(Arm * HALF_PI));
		Car.Path.Reset();
		Car.PathDist.Reset();
		Car.PathLimit.Reset();
		Car.LegEnds.Reset();
		Car.Yields.Reset();
		Car.NextArm = Arm;
		AppendLeg(Car);
		AppendLeg(Car);
		Best = FMath::Clamp(FMath::RoundToInt((FarEnd - FVector2D::DotProduct(Here, U)) / SampleStep), 0, Car.Path.Num() - 1);
	}
	Car.S = Car.PathDist[Best];
	FVector2D Dir;
	const FVector2D OnLane = PathAt(Car, Car.S, &Dir);
	Car.Offset = Here - OnLane;
	Car.YawOffset = FMath::FindDeltaAngleDegrees(FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)), Car.GetActorRotation().Yaw);
	Car.GroundZ = Car.GetActorLocation().Z - Car.HalfHeight;
	if (Car.Offset.Size() > 1500.f)
	{
		// Too far to drive back believably (it was on the roundabout): it just carries on from its lane.
		Car.Offset = FVector2D::ZeroVector;
		Car.YawOffset = 0.f;
	}
}

bool AGRTraffic::RingBusy(const AGRTrafficCar& Car, float MergeAngle) const
{
	auto Upstream = [MergeAngle](const FVector2D& P)
	{
		const float R = P.Size();
		if (R < 1300.f || R > 2300.f)
		{
			return false;
		}
		// Coming round toward the merge point (the ring turns with increasing angle).
		const float Ahead = FMath::RadiansToDegrees(FMath::FindDeltaAngleRadians(FMath::Atan2(P.Y, P.X), MergeAngle));
		return Ahead > 0.f && Ahead < 55.f;
	};
	for (const AGRTrafficCar* Other : Cars)
	{
		if (Other != &Car && Upstream(FVector2D(Other->GetActorLocation())))
		{
			return true;
		}
	}
	return PlayerVel.SizeSquared() > FMath::Square(50.f) && Upstream(PlayerPos);
}

void AGRTraffic::GatherObstacles()
{
	Obstacles.Reset();
	for (const AGRTrafficCar* Car : Cars)
	{
		// Three circles along the body.
		const FVector Loc = Car->GetActorLocation();
		const FVector Forward = Car->GetActorForwardVector().GetSafeNormal2D();
		const FVector2D Velocity(Car->GetMoveVelocity());
		const float Reach = Car->HalfLength - Car->HalfWidth;
		for (int32 k = -1; k <= 1; ++k)
		{
			Obstacles.Add({ FVector2D(Loc + Forward * Reach * k), Velocity, Car->HalfWidth, Car, false });
		}
	}
	// Only people and the scooter at street level near the two roads matter (hidden actors wait far below).
	auto OnRoad = [](const FVector& L)
	{
		return L.Z > -500.f && L.Z < 1500.f && FMath::Max(FMath::Abs(L.X), FMath::Abs(L.Y)) < FarEnd + 1500.f
			&& (FMath::Abs(L.X) < 1300.f || FMath::Abs(L.Y) < 1300.f || FVector2D(L).Size() < 3000.f);
	};
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	PlayerPos = FVector2D(1e7f, 1e7f);
	PlayerVel = FVector2D::ZeroVector;
	if (Player && OnRoad(Player->GetActorLocation()))
	{
		const FVector Loc = Player->GetActorLocation();
		PlayerPos = FVector2D(Loc);
		PlayerVel = FVector2D(Player->GetVelocity());
		const AGRCharacter* Bayu = Cast<AGRCharacter>(Player);
		if (Bayu && Bayu->IsRiding())
		{
			const FVector Forward = Player->GetActorForwardVector();
			Obstacles.Add({ FVector2D(Loc + Forward * 55.f), PlayerVel, 45.f, nullptr, true });
			Obstacles.Add({ FVector2D(Loc - Forward * 55.f), PlayerVel, 45.f, nullptr, true });
		}
		else
		{
			Obstacles.Add({ PlayerPos, PlayerVel, 40.f, nullptr, true });
		}
	}
	for (TActorIterator<AGRMotor> It(GetWorld()); It; ++It)
	{
		// Parked or lying where it fell; while ridden it hangs under Bayu.
		if (!It->IsHidden() && !It->GetAttachParentActor() && OnRoad(It->GetActorLocation()))
		{
			const FVector Forward = It->GetActorForwardVector();
			Obstacles.Add({ FVector2D(It->GetActorLocation() + Forward * 50.f), FVector2D::ZeroVector, 40.f, nullptr, true });
			Obstacles.Add({ FVector2D(It->GetActorLocation() - Forward * 50.f), FVector2D::ZeroVector, 40.f, nullptr, true });
		}
	}
	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
	{
		if (*It != Player && OnRoad(It->GetActorLocation()))
		{
			Obstacles.Add({ FVector2D(It->GetActorLocation()), FVector2D(It->GetVelocity()), 40.f, nullptr, false });
		}
	}
}

void AGRTraffic::Drive(AGRTrafficCar& Car, float DeltaSeconds)
{
	if (Car.bKnocked)
	{
		return;
	}
	if (Car.bRejoin)
	{
		Rejoin(Car);
	}
	if (Car.HoldTime > 0.f)
	{
		Car.HoldTime -= DeltaSeconds;
		Car.Speed = 0.f;
		return;
	}
	if (Car.S > Car.LegEnds[0])
	{
		DropFirstLeg(Car);
	}

	// Slow down in time for the curves ahead.
	const float Look = FMath::Max(3500.f, Car.Speed * Car.Speed / (2.f * ComfortBrake) + 1500.f);
	const int32 Start = FMath::Clamp(Algo::UpperBound(Car.PathDist, Car.S) - 1, 0, Car.Path.Num() - 2);
	float Target = Car.Cruise;
	for (int32 j = Start; j < Car.Path.Num() && Car.PathDist[j] < Car.S + Look; ++j)
	{
		const float Before = FMath::Max(Car.PathDist[j] - Car.S - Car.HalfLength * 0.5f, 0.f);
		Target = FMath::Min(Target, FMath::Sqrt(FMath::Square(Car.PathLimit[j]) + 2.f * ComfortBrake * Before));
	}

	// Whatever stands on the lane ahead: the closest one sets the gap.
	const FVector2D Here(Car.GetActorLocation());
	TArray<const FObstacle*, TInlineAllocator<24>> Near;
	for (const FObstacle& Obstacle : Obstacles)
	{
		if (Obstacle.Car != &Car && FVector2D::DistSquared(Obstacle.Pos, Here) < FMath::Square(Look + 500.f))
		{
			Near.Add(&Obstacle);
		}
	}
	float Gap = NoGap;
	float LeadSpeed = 0.f;
	bool bForPlayer = false;
	const FObstacle* Blocker = nullptr;
	float BlockerSide = 0.f;
	for (int32 j = Start; j < Car.Path.Num() - 1 && Car.PathDist[j] < Car.S + Look && Near.Num() > 0; ++j)
	{
		const FVector2D AB = Car.Path[j + 1] - Car.Path[j];
		const float Len = FMath::Max(Car.PathDist[j + 1] - Car.PathDist[j], 0.01f);
		const FVector2D Right(-AB.Y / Len, AB.X / Len);
		const FVector2D A = Car.Path[j] + Right * (Car.bAvoiding ? Car.ShiftTarget : Car.Shift);
		for (const FObstacle* Obstacle : Near)
		{
			const float T = FMath::Clamp(FVector2D::DotProduct(Obstacle->Pos - A, AB) / (Len * Len), 0.f, 1.f);
			const FVector2D Across = Obstacle->Pos - (A + AB * T);
			if (Across.Size() > Car.HalfWidth + Obstacle->Radius + 25.f)
			{
				continue;
			}
			const float Along = Car.PathDist[j] + T * Len - Car.S;
			const float ThisGap = Along - Car.HalfLength - Obstacle->Radius;
			if (Along > 0.f && ThisGap < Gap)
			{
				Gap = ThisGap;
				LeadSpeed = FMath::Max(FVector2D::DotProduct(Obstacle->Vel, AB / Len), 0.f);
				bForPlayer = Obstacle->bPlayer;
				Blocker = Obstacle;
				BlockerSide = FVector2D::DotProduct(Across, Right) + (Car.bAvoiding ? Car.ShiftTarget : Car.Shift);
			}
		}
	}

	// Held up by something that is not moving and not a car (a parked scooter, someone standing on the road):
	// after a honk, ease past it on the right, staying on this side of the road (or off the Tugu island).
	const bool bStatic = Blocker && !Blocker->Car && Blocker->Vel.SizeSquared() < FMath::Square(30.f);
	Car.StuckBehind = bStatic && Gap < 700.f && Car.Speed < 30.f ? Car.StuckBehind + DeltaSeconds : 0.f;
	if (Car.StuckBehind > 2.5f && !Car.bAvoiding)
	{
		const float Need = BlockerSide + Car.HalfWidth + Blocker->Radius + 70.f;
		if (Need > 0.f && Need <= (Here.Size() < 2600.f ? 250.f : 560.f))
		{
			Car.bAvoiding = true;
			Car.ShiftTarget = Need;
			Car.AvoidPos = Blocker->Pos;
			Car.AvoidRadius = Blocker->Radius;
			Car.StuckBehind = 0.f;
		}
	}
	if (Car.bAvoiding && FVector2D::DotProduct(Car.AvoidPos - Here, FVector2D(Car.GetActorForwardVector())) < -(Car.HalfLength + Car.AvoidRadius + 100.f))
	{
		Car.bAvoiding = false;
		Car.ShiftTarget = 0.f;
	}

	// Give way to traffic already on the roundabout, unless too close to the line to stop.
	for (const AGRTrafficCar::FYield& Yield : Car.Yields)
	{
		const float ToLine = Yield.S - Car.S - Car.HalfLength;
		if (ToLine > -10.f && ToLine < 3000.f && ToLine > Car.Speed * Car.Speed / (2.f * HardBrake) && ToLine < Gap
			&& RingBusy(Car, Yield.MergeAngle))
		{
			Gap = FMath::Max(ToLine, 0.f);
			LeadSpeed = 0.f;
			bForPlayer = false;
		}
	}

	float Accel = MaxAccel * (1.f - FMath::Pow(Car.Speed / FMath::Max(Target, 10.f), 4.f));
	if (Gap < NoGap)
	{
		const float Wanted = MinGap + Car.Speed * Headway + Car.Speed * (Car.Speed - LeadSpeed) / (2.f * FMath::Sqrt(MaxAccel * ComfortBrake));
		Accel -= MaxAccel * FMath::Square(FMath::Max(Wanted, 0.f) / FMath::Max(Gap, 1.f));
	}
	Car.Speed = FMath::Max(Car.Speed + FMath::Clamp(Accel, -HardBrake, MaxAccel) * DeltaSeconds, 0.f);
	if (Gap <= 0.f)
	{
		Car.Speed = 0.f;
	}
	if (Car.ShiftTarget - Car.Shift > 20.f)
	{
		// Swinging out: walking pace until the car is clear of what it is passing.
		Car.Speed = FMath::Min(Car.Speed, 150.f);
	}

	// Bayu standing in the way: honk.
	Car.BlockedByPlayer = bForPlayer && Gap < 700.f && Car.Speed < 30.f ? Car.BlockedByPlayer + DeltaSeconds : 0.f;
	if (Car.BlockedByPlayer > 1.5f && Car.HonkCooldown <= 0.f)
	{
		Car.Shout(TEXT("TIIN! TIIN!"), FColor(255, 220, 90), 1.2f);
		Car.HonkCooldown = 4.f;
	}

	const float OldS = Car.S;
	Car.S += Car.Speed * DeltaSeconds;
	if (!Car.Offset.IsZero() || Car.YawOffset != 0.f)
	{
		// Steering back into the lane after a knock, over the next few metres driven.
		const float Keep = FMath::Exp(-(Car.S - OldS) / 500.f);
		Car.Offset *= Keep;
		Car.YawOffset *= Keep;
		if (Car.Offset.SizeSquared() < 1.f && FMath::Abs(Car.YawOffset) < 0.2f)
		{
			Car.Offset = FVector2D::ZeroVector;
			Car.YawOffset = 0.f;
		}
	}
	const float OldShift = Car.Shift;
	Car.Shift = FMath::FInterpConstantTo(Car.Shift, Car.ShiftTarget, DeltaSeconds, 60.f + 0.3f * Car.Speed);
	// Nose turned toward where it is heading sideways.
	const float SteerYaw = FMath::RadiansToDegrees(FMath::Atan2(Car.Shift - OldShift, FMath::Max(Car.S - OldS, 1.f)));
	Car.ShiftYaw = FMath::FInterpTo(Car.ShiftYaw, FMath::Clamp(SteerYaw, -20.f, 20.f), DeltaSeconds, 6.f);
	FVector2D Dir;
	const FVector2D Spot = PathAt(Car, Car.S, &Dir) + FVector2D(-Dir.Y, Dir.X) * Car.Shift + Car.Offset;
	FHitResult Ground;
	if (GetWorld()->LineTraceSingleByObjectType(Ground, FVector(Spot, Car.GroundZ + 300.f), FVector(Spot, Car.GroundZ - 300.f), FCollisionObjectQueryParams(ECC_WorldStatic)))
	{
		// Kerb strips across the crossing are 8 cm up: ease over them.
		Car.GroundZ = FMath::FInterpTo(Car.GroundZ, Ground.ImpactPoint.Z, DeltaSeconds, 8.f);
	}
	const FRotator Facing(0.f, FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)) + Car.YawOffset + Car.ShiftYaw, 0.f);
	FHitResult Hit;
	Car.SetActorLocationAndRotation(FVector(Spot, Car.GroundZ + Car.HalfHeight), Facing, true, &Hit);
	if (Hit.bBlockingHit)
	{
		// Something the lane check missed, or that stepped right in front: stop at it instead of passing through.
		Car.S = OldS + (Car.S - OldS) * Hit.Time;
		Car.Speed = 0.f;
	}
}

void AGRTraffic::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GatherObstacles();
	for (AGRTrafficCar* Car : Cars)
	{
		if (Car)
		{
			Drive(*Car, FMath::Min(DeltaSeconds, 0.1f));
		}
	}
}

void AGRTraffic::PlaceCar(const FVector& Spot, float Yaw, float HoldSeconds)
{
	AGRTrafficCar* Best = nullptr;
	for (AGRTrafficCar* Car : Cars)
	{
		if (!Car->bKnocked && (!Best || FVector::DistSquared(Car->GetActorLocation(), Spot) < FVector::DistSquared(Best->GetActorLocation(), Spot)))
		{
			Best = Car;
		}
	}
	if (!Best)
	{
		return;
	}
	FHitResult Hit;
	const float Z = GetWorld()->LineTraceSingleByObjectType(Hit, Spot + FVector(0.f, 0.f, 500.f), Spot - FVector(0.f, 0.f, 500.f), FCollisionObjectQueryParams(ECC_WorldStatic))
		? Hit.ImpactPoint.Z : Spot.Z;
	Best->SetActorLocationAndRotation(FVector(Spot.X, Spot.Y, Z + Best->HalfHeight), FRotator(0.f, Yaw, 0.f));
	Best->Speed = 0.f;
	Best->HoldTime = HoldSeconds;
	Best->bRejoin = true;
}

void AGRTraffic::LogCars() const
{
	for (int32 i = 0; i < Cars.Num(); ++i)
	{
		const AGRTrafficCar* Car = Cars[i];
		const FVector L = Car->GetActorLocation();
		UE_LOG(LogGRTraffic, Warning, TEXT("GRLALIN %2d %s (%.0f, %.0f, %.0f) yaw %.0f %.0f km/j %s"), i, Car->bAngkot ? TEXT("angkot") : TEXT("mobil "),
			L.X, L.Y, L.Z, Car->GetActorRotation().Yaw, Car->GetMoveVelocity().Size() * 0.036f,
			Car->bKnocked ? TEXT("TERPENTAL") : (Car->HoldTime > 0.f ? TEXT("DITAHAN") : TEXT("")));
	}
	// Any two cars overlapping means something drove through something.
	for (int32 i = 0; i < Cars.Num(); ++i)
	{
		for (int32 j = i + 1; j < Cars.Num(); ++j)
		{
			if (FVector::Dist2D(Cars[i]->GetActorLocation(), Cars[j]->GetActorLocation()) < 300.f)
			{
				UE_LOG(LogGRTraffic, Warning, TEXT("GRLALIN tumpang tindih: %d dan %d"), i, j);
			}
		}
	}
}
