#include "Core/GRDevBot.h"

#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/GRCharacter.h"
#include "Player/GRInteractionComponent.h"
#include "UI/GRUISubsystem.h"
#include "World/GRMotor.h"
#include "World/GRNpc.h"
#include "World/GROjol.h"
#include "World/GRStealthLevel.h"
#include "World/GRStoryProp.h"
#include "World/GRCrossings.h"
#include "World/GRTraffic.h"
#include "EngineUtils.h"

namespace
{
	constexpr float PlanStepSeconds = 0.2f;
	constexpr int32 PlanHorizon = 200;
	constexpr float DangerCost = 60.f;

	/** The roundabout in the middle of town, with room to pass it. */
	constexpr float RoundaboutClear = 2100.f;
	/** Below the speed the app counts as speeding. */
	constexpr float CruiseSpeed = 1150.f;
	constexpr float CornerSpeed = 380.f;
	constexpr float BrakeAccel = 650.f;
	constexpr float Wheelbase = 130.f;
	/** Trips worth taking on camera, metres. */
	constexpr float ShortestTrip = 120.f;
	constexpr float LongestTrip = 800.f;

	struct FGuardPath
	{
		FVector2D A = FVector2D::ZeroVector;
		FVector2D B = FVector2D::ZeroVector;
		FVector2D Pos = FVector2D::ZeroVector;
		/** Direction of travel along A to B: +1, -1. */
		float Dir = 1.f;
		float Speed = 160.f;
		float Range = 0.f;
	};

	/** Where a guard will be and which way it will face, Seconds from now. Guards pause 2 s at each end. */
	void PredictGuard(const FGuardPath& Guard, float Seconds, FVector2D& OutPos, FVector2D& OutFacing)
	{
		const float Length = FMath::Max(FVector2D::Distance(Guard.A, Guard.B), 1.f);
		const FVector2D Axis = (Guard.B - Guard.A) / Length;
		float S = FMath::Clamp(FVector2D::DotProduct(Guard.Pos - Guard.A, Axis), 0.f, Length);
		float Dir = Guard.Dir;
		float Left = Seconds;
		for (int32 i = 0; i < 12 && Left > 0.f; ++i)
		{
			const float ToEnd = (Dir > 0.f ? Length - S : S) / Guard.Speed;
			if (Left < ToEnd)
			{
				S += Dir * Guard.Speed * Left;
				break;
			}
			Left -= ToEnd;
			S = Dir > 0.f ? Length : 0.f;
			if (Left < 2.f)
			{
				break;
			}
			Left -= 2.f;
			Dir = -Dir;
		}
		OutPos = Guard.A + Axis * S;
		OutFacing = Axis * Dir;
	}
}

UGRDevBot* UGRDevBot::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UGRDevBot>() : nullptr;
}

AGRCharacter* UGRDevBot::GetBayu() const
{
	return Cast<AGRCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
}

bool UGRDevBot::HasScreens() const
{
	const ULocalPlayer* LP = GetWorld()->GetFirstLocalPlayerFromController();
	const UGRUISubsystem* UI = LP ? LP->GetSubsystem<UGRUISubsystem>() : nullptr;
	return UI && UI->HasScreens();
}

void UGRDevBot::StartStealth()
{
	Mode = EMode::Stealth;
	StartWait = 0.f;
	StepCell = FIntPoint(-1, -1);
	TakeCooldown = 0.f;
	bLevelSeen = false;
	UE_LOG(LogTemp, Display, TEXT("GRBot: mulai menyusup"));
}

void UGRDevBot::StartOjol(int32 Trips)
{
	Mode = EMode::Ojol;
	TripsLeft = FMath::Max(Trips, 1);
	TripsDone = 0;
	LastStage = -1;
	StageTime = 0.f;
	StuckTime = 0.f;
	ReverseTime = 0.f;
	UE_LOG(LogTemp, Display, TEXT("GRBot: mulai ngojol, %d perjalanan"), TripsLeft);
}

void UGRDevBot::StartPath(const FVector2D& Target, float Speed, float CameraYaw)
{
	AGRCharacter* Bayu = GetBayu();
	if (!Bayu)
	{
		return;
	}
	Mode = EMode::Path;
	PathSpeed = FMath::Clamp(Speed, 0.05f, 1.f);
	PathCameraYaw = CameraYaw;
	PathPoints.Reset();
	PathKerbs.Reset();
	StuckTime = 0.f;
	ReverseTime = 0.f;
	const FVector2D Here(Bayu->GetActorLocation());
	if (Bayu->IsRiding())
	{
		// The OjekKu route runs on the centre lines; keep left of it like the traffic does (lane 650 off the centre).
		TArray<FVector2D> Centre;
		UGROjolSubsystem::BuildRoute(Here, Target, Centre);
		for (int32 i = 1; i < Centre.Num(); ++i)
		{
			const FVector2D Dir = (Centre[i] - Centre[i - 1]).GetSafeNormal();
			if (Dir.IsNearlyZero())
			{
				continue;
			}
			const FVector2D Left(Dir.Y, -Dir.X);
			if (i > 1)
			{
				PathPoints.Add(Centre[i - 1] + Left * 650.f);
			}
			PathPoints.Add(Centre[i] + Left * 650.f);
		}
	}
	else
	{
		// Pavements run 1000 off the centre line of Jl. Raya (along Y) and Jl. Merdeka (along X). Changing sides
		// means walking to the zebra crossing that makes the shortest trip and crossing straight over there.
		auto Snap = [](const FVector2D& P, int32& Axis, float& Side, float& Along)
		{
			Axis = FMath::Abs(FMath::Abs(P.X) - 1000.f) < FMath::Abs(FMath::Abs(P.Y) - 1000.f) ? 0 : 1;
			const float Lateral = Axis == 0 ? P.X : P.Y;
			Side = Lateral >= 0.f ? 1.f : -1.f;
			Along = Axis == 0 ? P.Y : P.X;
		};
		int32 AxisA, AxisB;
		float SideA, SideB, AlongA, AlongB;
		Snap(Here, AxisA, SideA, AlongA);
		Snap(Target, AxisB, SideB, AlongB);
		if (AxisA == AxisB && SideA != SideB)
		{
			float Best = 0.f;
			float BestCost = TNumericLimits<float>::Max();
			for (const float A : GRCrossings::Along)
			{
				for (const float Sign : { -1.f, 1.f })
				{
					const float Cost = FMath::Abs(AlongA - Sign * A) + FMath::Abs(AlongB - Sign * A);
					if (Cost < BestCost)
					{
						BestCost = Cost;
						Best = Sign * A;
					}
				}
			}
			PathPoints.Add(GRCrossings::Point(AxisA, Best, SideA * 1000.f));
			PathKerbs.Add(false);
			PathPoints.Add(GRCrossings::Point(AxisA, Best, SideA * (GRCrossings::RoadHalf + 60.f)));
			PathKerbs.Add(true);
			PathPoints.Add(GRCrossings::Point(AxisA, Best, SideB * 1000.f));
			PathKerbs.Add(false);
		}
		PathPoints.Add(Target);
	}
	PathKerbs.SetNum(PathPoints.Num());
	UE_LOG(LogTemp, Display, TEXT("GRBot: %s ke %.0f %.0f lewat %d titik"), Bayu->IsRiding() ? TEXT("naik motor") : TEXT("jalan kaki"), Target.X, Target.Y, PathPoints.Num());
}

bool UGRDevBot::CarNear(const FVector2D& P, float Radius) const
{
	for (TActorIterator<AGRTrafficCar> It(GetWorld()); It; ++It)
	{
		if (FVector2D::DistSquared(FVector2D(It->GetActorLocation()), P) < FMath::Square(Radius))
		{
			return true;
		}
	}
	return false;
}

void UGRDevBot::TickPath(float DeltaTime, AGRCharacter* Bayu)
{
	const FVector2D Here(Bayu->GetActorLocation());
	const bool bRide = Bayu->IsRiding();
	// Waypoints count as passed when close, or (riding) once he is nearer the next one than they are to each other.
	while (PathPoints.Num() > 1)
	{
		const bool bClose = FVector2D::Distance(Here, PathPoints[0]) < (bRide ? 500.f : 60.f);
		const bool bPast = bRide && FVector2D::Distance(Here, PathPoints[1]) < FVector2D::Distance(PathPoints[0], PathPoints[1]) - 200.f;
		if (!bClose && !bPast)
		{
			break;
		}
		PathPoints.RemoveAt(0);
		PathKerbs.RemoveAt(0);
	}
	if (PathPoints.Num() == 0 || (PathPoints.Num() == 1 && FVector2D::Distance(Here, PathPoints[0]) < (bRide ? 300.f : 60.f)))
	{
		Bayu->MoveFromInput(FVector2D::ZeroVector);
		Stop();
		return;
	}
	if (bRide)
	{
		TArray<FVector2D> Route;
		Route.Add(Here);
		Route.Append(PathPoints);
		Drive(DeltaTime, Bayu, 0.f, &Route);
		return;
	}

	// On foot. At the kerb, wait until no car is close to the crossing.
	FVector2D Goal = PathPoints[0];
	if (PathKerbs[0] && PathPoints.Num() > 1 && FVector2D::Distance(Here, Goal) < 120.f)
	{
		if (CarNear(FMath::Lerp(Goal, PathPoints[1], 0.5f), 1600.f))
		{
			if ((PathWaitLogged -= DeltaTime) <= 0.f)
			{
				PathWaitLogged = 2.f;
				UE_LOG(LogTemp, Display, TEXT("GRBot: tunggu mobil lewat di zebra"));
			}
			return;
		}
		PathPoints.RemoveAt(0);
		PathKerbs.RemoveAt(0);
		Goal = PathPoints[0];
	}
	FVector Dir(FVector2D(Goal - Here).GetSafeNormal(), 0.f);
	// Feelers at waist height: a bench, pole or person ahead turns him toward the clearer side.
	const FVector Start = Bayu->GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GRBotWalk), false, Bayu);
	Params.AddIgnoredActor(Bayu->GetMotor());
	auto Clear = [&](float Degrees)
	{
		FHitResult Hit;
		const FVector D = Dir.RotateAngleAxis(Degrees, FVector::UpVector);
		const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, Start + D * 170.f, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(30.f), Params);
		return bHit && Hit.ImpactNormal.Z < 0.7f ? Hit.Distance : 170.f;
	};
	if (Clear(0.f) < 170.f)
	{
		Dir = Dir.RotateAngleAxis(Clear(40.f) >= Clear(-40.f) ? 45.f : -45.f, FVector::UpVector);
	}
	Bayu->AddMovementInput(Dir, PathSpeed);
	if (APlayerController* PC = Cast<APlayerController>(Bayu->GetController()))
	{
		const FRotator Want(-10.f, Bayu->GetActorRotation().Yaw + PathCameraYaw, 0.f);
		PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(), Want, DeltaTime, 1.2f));
	}
}

void UGRDevBot::Stop()
{
	if (Mode != EMode::Idle)
	{
		UE_LOG(LogTemp, Display, TEXT("GRBot: selesai"));
	}
	Mode = EMode::Idle;
}

void UGRDevBot::Tick(float DeltaTime)
{
	if (Mode == EMode::Idle)
	{
		return;
	}
	AGRCharacter* Bayu = GetBayu();
	if (!Bayu)
	{
		return;
	}
	if (Mode == EMode::Ojol)
	{
		TickOjol(DeltaTime, Bayu);
		return;
	}
	if (Mode == EMode::Path)
	{
		TickPath(DeltaTime, Bayu);
		return;
	}
	if (AGRStealthLevel* Level = AGRStealthLevel::GetActive(GetWorld()))
	{
		bLevelSeen = true;
		TickStealth(DeltaTime, Bayu, *Level);
	}
	else if (bLevelSeen || (StartWait += DeltaTime) > 20.f)
	{
		UE_LOG(LogTemp, Display, TEXT("GRBot: menyusup berakhir, level %s"), bLevelSeen ? TEXT("sudah ditutup") : TEXT("tidak pernah mulai"));
		Stop();
	}
}

FIntPoint UGRDevBot::PlanStep(const AGRStealthLevel& Level, const FIntPoint& Here, const FIntPoint& Goal, float WalkSpeed) const
{
	const TArray<FString>& Grid = Level.GetGrid();
	const float Cell = Level.GetCell();
	const int32 Rows = Grid.Num();
	const int32 Cols = Rows > 0 ? Grid[0].Len() : 0;
	if (!Grid.IsValidIndex(Here.X) || Here.Y < 0 || Here.Y >= Cols)
	{
		return Here;
	}
	auto Solid = [&](int32 R, int32 C)
	{
		return Grid.IsValidIndex(R) && C >= 0 && C < Grid[R].Len() && (Grid[R][C] == '#' || Grid[R][C] == 'C');
	};
	auto Blocked = [&](int32 R, int32 C)
	{
		if (!Grid.IsValidIndex(R) || C < 0 || C >= Grid[R].Len())
		{
			return true;
		}
		// The exit ends the run, so it only counts when it is where we are going.
		return Solid(R, C) || (Grid[R][C] == 'X' && FIntPoint(R, C) != Goal);
	};
	auto SightClear = [&](const FVector2D& From, const FVector2D& To)
	{
		const int32 Steps = FMath::CeilToInt(FVector2D::Distance(From, To) / (Cell * 0.25f));
		for (int32 i = 1; i < Steps; ++i)
		{
			const FVector2D P = FMath::Lerp(From, To, static_cast<float>(i) / Steps);
			const FIntPoint At = Level.CellOf(FVector(P, 0.f));
			if (Solid(At.X, At.Y))
			{
				return false;
			}
		}
		return true;
	};

	TArray<FGuardPath> Paths;
	for (const AGRNpc* Guard : Level.GetGuards())
	{
		if (!Guard || Guard->GetPatrol().Num() < 2)
		{
			continue;
		}
		FGuardPath Path;
		Path.A = FVector2D(Guard->GetPatrol()[1]);
		Path.B = FVector2D(Guard->GetPatrol()[0]);
		Path.Pos = FVector2D(Guard->GetActorLocation());
		const FVector2D Axis = (Path.B - Path.A).GetSafeNormal();
		Path.Dir = FVector2D::DotProduct(FVector2D(Guard->GetActorForwardVector()), Axis) >= 0.f ? 1.f : -1.f;
		Path.Speed = FMath::Max(Guard->GetCharacterMovement()->MaxWalkSpeed, 100.f);
		Path.Range = Cell * 3.5f;
		Paths.Add(Path);
	}

	auto Index = [&](int32 T, int32 R, int32 C) { return (T * Rows + R) * Cols + C; };

	// Seen from this cell at this step? With a margin all round, because guards turn and the walk is not exact.
	const int32 States = Rows * Cols * PlanHorizon;
	TArray<uint8> Danger;
	Danger.Init(0, States);
	for (int32 T = 0; T < PlanHorizon; ++T)
	{
		for (const FGuardPath& Path : Paths)
		{
			FVector2D Pos, Facing;
			PredictGuard(Path, T * PlanStepSeconds, Pos, Facing);
			for (int32 R = 0; R < Rows; ++R)
			{
				for (int32 C = 0; C < Cols; ++C)
				{
					const FVector2D At(Level.GetCellCenter(R, C));
					const FVector2D To = At - Pos;
					const float Dist = To.Size();
					const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector2D::DotProduct(Facing, To.GetSafeNormal()), -1.f, 1.f)));
					if (Dist < 240.f || (Dist < Path.Range + Cell * 0.6f && Angle < 42.f && SightClear(Pos, At)))
					{
						Danger[Index(T, R, C)] = 1;
					}
				}
			}
		}
	}

	// Cheapest walk through (cell, time): waiting and moving both take time, standing in a beam costs far more.
	const int32 MoveSteps = FMath::Max(1, FMath::CeilToInt(Cell / WalkSpeed / PlanStepSeconds));
	constexpr float Unreached = TNumericLimits<float>::Max();
	TArray<float> Best;
	Best.Init(Unreached, States);
	TArray<int32> Prev;
	Prev.Init(-1, States);
	Best[Index(0, Here.X, Here.Y)] = 0.f;
	int32 Arrived = -1;
	float ArrivedCost = Unreached;
	static const FIntPoint Moves[] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
	for (int32 T = 0; T < PlanHorizon; ++T)
	{
		for (int32 R = 0; R < Rows; ++R)
		{
			for (int32 C = 0; C < Cols; ++C)
			{
				const int32 From = Index(T, R, C);
				if (Best[From] == Unreached)
				{
					continue;
				}
				if (FIntPoint(R, C) == Goal && Best[From] < ArrivedCost)
				{
					ArrivedCost = Best[From];
					Arrived = From;
				}
				auto Relax = [&](int32 NR, int32 NC, int32 Steps)
				{
					const int32 NT = T + Steps;
					if (NT >= PlanHorizon)
					{
						return;
					}
					float Cost = Best[From] + Steps;
					for (int32 S = 1; S <= Steps; ++S)
					{
						Cost += Danger[Index(T + S, NR, NC)] ? DangerCost : 0.f;
					}
					const int32 To = Index(NT, NR, NC);
					if (Cost < Best[To])
					{
						Best[To] = Cost;
						Prev[To] = From;
					}
				};
				Relax(R, C, 1);
				for (const FIntPoint& Move : Moves)
				{
					if (!Blocked(R + Move.X, C + Move.Y))
					{
						Relax(R + Move.X, C + Move.Y, MoveSteps);
					}
				}
			}
		}
	}
	if (Arrived < 0)
	{
		return Here;
	}
	int32 At = Arrived;
	while (Prev[At] >= 0 && Prev[Prev[At]] >= 0)
	{
		At = Prev[At];
	}
	const int32 InLayer = At % (Rows * Cols);
	return FIntPoint(InLayer / Cols, InLayer % Cols);
}

void UGRDevBot::TickStealth(float DeltaTime, AGRCharacter* Bayu, AGRStealthLevel& Level)
{
	if (HasScreens())
	{
		StepCell = FIntPoint(-1, -1);
		return;
	}
	TakeCooldown -= DeltaTime;

	// Nearest evidence still lying there; with none left, the way out.
	const FVector2D Here2D(Bayu->GetActorLocation());
	const AGRStoryProp* Loot = nullptr;
	float Nearest = TNumericLimits<float>::Max();
	for (const AGRStoryProp* Prop : Level.GetLoot())
	{
		if (Prop && !Prop->IsHidden())
		{
			if (const float D = FVector2D::Distance(Here2D, FVector2D(Prop->GetActorLocation())); D < Nearest)
			{
				Nearest = D;
				Loot = Prop;
			}
		}
	}
	if (Loot && Nearest < 150.f)
	{
		// Face it and reach for it, the way E works for a player.
		Bayu->AddMovementInput((Loot->GetActorLocation() - Bayu->GetActorLocation()).GetSafeNormal2D(), 0.25f);
		if (TakeCooldown <= 0.f && Bayu->GetInteraction()->GetFocusedActor() == Loot)
		{
			TakeCooldown = 0.6f;
			if (APlayerController* PC = Cast<APlayerController>(Bayu->GetController()))
			{
				PC->ConsoleCommand(TEXT("GRTekan E 0.1"));
			}
		}
		return;
	}

	const FIntPoint Here = Level.CellOf(Bayu->GetActorLocation());
	const FIntPoint Goal = Level.CellOf(Loot ? Loot->GetActorLocation() : Level.GetExit());
	if (StepCell.X < 0 || FVector2D::Distance(Here2D, FVector2D(Level.GetCellCenter(StepCell.X, StepCell.Y))) < 40.f)
	{
		StepCell = PlanStep(Level, Here, Goal, Bayu->GetCharacterMovement()->MaxWalkSpeed);
	}
	const FVector2D To = FVector2D(Level.GetCellCenter(StepCell.X, StepCell.Y)) - Here2D;
	if (To.Size() > 25.f)
	{
		Bayu->AddMovementInput(FVector(To.GetSafeNormal(), 0.f), 1.f);
	}
}

void UGRDevBot::TickOjol(float DeltaTime, AGRCharacter* Bayu)
{
	UGROjolSubsystem* Ojol = UGROjolSubsystem::Get(this);
	if (!Ojol || HasScreens() || Bayu->IsKnockedDown())
	{
		return;
	}
	if (!Bayu->IsRiding())
	{
		Bayu->SetRiding(true);
		return;
	}
	const int32 Stage = static_cast<int32>(Ojol->GetStage());
	if (Stage != LastStage)
	{
		if (Ojol->GetStage() == EGROjolStage::Summary)
		{
			const FGROjolTrip& Trip = Ojol->GetTrip();
			++TripsDone;
			--TripsLeft;
			UE_LOG(LogTemp, Display, TEXT("GRBot: perjalanan %d selesai, %d bintang, Rp %d, tip Rp %d, %d guncangan, %.0f dari %.0f detik"),
				TripsDone, Trip.Stars, Trip.Paid, Trip.Tip, Trip.Jolts, Trip.Elapsed, Trip.ExpectedSeconds);
		}
		LastStage = Stage;
		StageTime = 0.f;
	}
	StageTime += DeltaTime;

	switch (Ojol->GetStage())
	{
	case EGROjolStage::Offline:
		Ojol->SetOnline(true);
		break;
	case EGROjolStage::Waiting:
		if (TripsLeft <= 0)
		{
			Stop();
		}
		else if (StageTime > 5.f)
		{
			StageTime = 0.f;
			Ojol->ForceRoadOffer(ShortestTrip, LongestTrip);
		}
		break;
	case EGROjolStage::Offer:
		if (StageTime > 3.f)
		{
			const FGROjolTrip& Trip = Ojol->GetTrip();
			UE_LOG(LogTemp, Display, TEXT("GRBot: order %s %s, jemput %.0f %.0f, antar %.0f %.0f (%s), %.0f m"), *Trip.Honorific, *Trip.Name,
				Trip.Pickup.X, Trip.Pickup.Y, Trip.Dest.X, Trip.Dest.Y, *Trip.DestLabel, Trip.Meters);
			Ojol->Accept();
		}
		break;
	case EGROjolStage::ToPickup:
		Drive(DeltaTime, Bayu, 330.f);
		break;
	case EGROjolStage::Riding:
		if (StageTime > 150.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("GRBot: tujuan tidak tercapai, order dibatalkan"));
			Ojol->CancelTrip();
			break;
		}
		if (Ojol->IsChatOpen() && Ojol->GetChatTimeLeft() < 10.f)
		{
			Ojol->Choose(0);
		}
		Drive(DeltaTime, Bayu, 420.f);
		break;
	case EGROjolStage::Boarding:
		if (Ojol->IsChatOpen() && StageTime > 3.f)
		{
			Ojol->Choose(0);
		}
		Bayu->MoveFromInput(FVector2D(0.f, -1.f));
		break;
	case EGROjolStage::Summary:
		if (TripsLeft <= 0 && StageTime > 7.f)
		{
			Stop();
			break;
		}
		Bayu->MoveFromInput(FVector2D(0.f, -1.f));
		break;
	default:
		break;
	}
}

void UGRDevBot::Drive(float DeltaTime, AGRCharacter* Bayu, float StopShort, const TArray<FVector2D>* RouteOverride)
{
	const UGROjolSubsystem* Ojol = UGROjolSubsystem::Get(this);
	const TArray<FVector2D>& Route = RouteOverride ? *RouteOverride : Ojol->GetRoute();
	if (Route.Num() < 2)
	{
		return;
	}
	const FVector2D Here(Bayu->GetActorLocation());
	const float Speed = Bayu->GetRideSpeed();

	// The point to aim at, some way along the route.
	const float Lookahead = 450.f + FMath::Abs(Speed) * 0.55f;
	FVector2D Aim = Route.Last();
	float Walked = 0.f;
	for (int32 i = 1; i < Route.Num(); ++i)
	{
		const float Length = FVector2D::Distance(Route[i - 1], Route[i]);
		if (Walked + Length >= Lookahead)
		{
			Aim = FMath::Lerp(Route[i - 1], Route[i], (Lookahead - Walked) / FMath::Max(Length, 1.f));
			break;
		}
		Walked += Length;
	}
	// Round the roundabout instead of through the monument, the way the traffic goes.
	if (Aim.Size() < RoundaboutClear)
	{
		const FVector2D Out = Aim.IsNearlyZero() ? Here.GetSafeNormal() : Aim.GetSafeNormal();
		const FVector2D Around = FVector2D(Here.Y, -Here.X).GetSafeNormal();
		Aim = (Out + Around * 0.8f).GetSafeNormal() * RoundaboutClear;
	}

	// How fast: the next sharp corner and the end of the route both set a limit.
	constexpr float NoCorner = TNumericLimits<float>::Max();
	float ToEnd = 0.f;
	float ToCorner = NoCorner;
	for (int32 i = 1; i < Route.Num(); ++i)
	{
		ToEnd += FVector2D::Distance(Route[i - 1], Route[i]);
		if (i + 1 < Route.Num() && ToCorner == NoCorner && ToEnd > 150.f)
		{
			const FVector2D In = (Route[i] - Route[i - 1]).GetSafeNormal();
			const FVector2D Next = (Route[i + 1] - Route[i]).GetSafeNormal();
			if (FVector2D::DotProduct(In, Next) < 0.8f)
			{
				ToCorner = ToEnd;
			}
		}
	}
	float Wanted = FMath::Min(CruiseSpeed, FMath::Sqrt(2.f * BrakeAccel * FMath::Max(ToEnd - StopShort, 0.f)));
	if (ToCorner != NoCorner)
	{
		Wanted = FMath::Min(Wanted, FMath::Sqrt(CornerSpeed * CornerSpeed + 2.f * BrakeAccel * ToCorner));
	}

	// Feelers: what stands in the way ahead, and which side is clearer.
	const FVector Forward = Bayu->GetActorForwardVector();
	const FVector Start = Bayu->GetActorLocation() + FVector(0.f, 0.f, 50.f - Bayu->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GRBotFeeler), false, Bayu);
	FCollisionResponseParams Response;
	Bayu->GetCapsuleComponent()->InitSweepCollisionParams(Params, Response);
	Params.AddIgnoredActor(Bayu);
	Params.AddIgnoredActor(Bayu->GetMotor());
	const float Reach = 500.f + FMath::Abs(Speed) * 1.1f;
	auto Clearance = [&](float Degrees)
	{
		FHitResult Hit;
		const FVector Dir = Forward.RotateAngleAxis(Degrees, FVector::UpVector);
		const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, Start + Dir * Reach, FQuat::Identity, Bayu->GetCapsuleComponent()->GetCollisionObjectType(),
			FCollisionShape::MakeSphere(45.f), Params, Response);
		return bHit && Hit.ImpactNormal.Z < 0.7f ? Hit.Distance : Reach;
	};
	const float Ahead = Clearance(0.f);
	float Dodge = 0.f;
	if (Ahead < Reach)
	{
		const float Left = FMath::Min(Clearance(-16.f), Clearance(-32.f));
		const float Right = FMath::Min(Clearance(16.f), Clearance(32.f));
		Dodge = Right >= Left ? 28.f : -28.f;
		Wanted = FMath::Min(Wanted, FMath::Max(FMath::Sqrt(2.f * BrakeAccel * FMath::Max(Ahead - 160.f, 0.f)), 120.f));
	}

	// Steering: the yaw rate that turns toward the aim point, as a share of what the handlebars can do at this speed.
	const FVector2D ToAim = Aim - Here;
	const float AimYaw = FMath::RadiansToDegrees(FMath::Atan2(ToAim.Y, ToAim.X));
	const float Error = FMath::FindDeltaAngleDegrees(Bayu->GetActorRotation().Yaw, AimYaw) + Dodge;
	const float SpeedAbs = FMath::Max(FMath::Abs(Speed), 60.f);
	const float WantedRate = FMath::Clamp(FMath::DegreesToRadians(Error) * 1.6f, -2.2f, 2.2f);
	const float SteerDegrees = FMath::RadiansToDegrees(FMath::Atan(WantedRate * Wheelbase / SpeedAbs));
	const float MaxSteer = FMath::Lerp(32.f, 5.f, FMath::Sqrt(FMath::Clamp(SpeedAbs / 1600.f, 0.f, 1.f)));
	const float Steer = FMath::Clamp(SteerDegrees / MaxSteer, -1.f, 1.f);
	// A sharp turn from standing or at the start of a corner: slow enough to make it.
	if (FMath::Abs(Error) > 35.f)
	{
		Wanted = FMath::Min(Wanted, CornerSpeed);
	}

	float Throttle = 0.f;
	if (Wanted > Speed + 25.f)
	{
		Throttle = FMath::Clamp((Wanted - Speed) / 300.f, 0.25f, 1.f);
	}
	else if (Speed > Wanted + 60.f)
	{
		Throttle = -FMath::Clamp((Speed - Wanted) / 500.f, 0.2f, 0.6f);
	}

	// The camera follows behind, as a player would keep it.
	if (APlayerController* PC = Cast<APlayerController>(Bayu->GetController()))
	{
		const FRotator Want(-14.f, Bayu->GetActorRotation().Yaw, 0.f);
		PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(), Want, DeltaTime, 2.5f));
	}

	// Wedged against something: back out, then try again from a different angle.
	if (ReverseTime > 0.f)
	{
		ReverseTime -= DeltaTime;
		Bayu->MoveFromInput(FVector2D(ReverseSteer, -1.f));
		return;
	}
	StuckTime = Wanted > 100.f && FMath::Abs(Speed) < 50.f ? StuckTime + DeltaTime : 0.f;
	if (StuckTime > 2.f)
	{
		StuckTime = 0.f;
		ReverseTime = 1.8f;
		ReverseSteer = Steer >= 0.f ? -1.f : 1.f;
		UE_LOG(LogTemp, Display, TEXT("GRBot: tersangkut di %.0f %.0f, mundur"), Here.X, Here.Y);
	}
	Bayu->MoveFromInput(FVector2D(Steer, Throttle));
}
