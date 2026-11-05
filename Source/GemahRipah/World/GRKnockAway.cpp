#include "World/GRKnockAway.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	const FName SquashedTag(TEXT("GRGepeng"));

	/** Everything that moves with Piece: the whole actor for its root, else the part itself. */
	FBox BoxOf(USceneComponent* Piece)
	{
		const AActor* Owner = Piece->GetOwner();
		if (Owner && Owner->GetRootComponent() == Piece)
		{
			return Owner->GetComponentsBoundingBox(true);
		}
		Piece->UpdateBounds();
		return Piece->Bounds.GetBox();
	}
}

void UGRKnockAway::Launch(USceneComponent* Piece, const FVector& Velocity)
{
	if (!Piece || Piece->Mobility != EComponentMobility::Movable)
	{
		return;
	}
	for (const FFlight& Flight : Flights)
	{
		if (Flight.Piece.Get() == Piece)
		{
			return;
		}
	}
	FFlight Flight;
	Flight.Piece = Piece;
	Flight.Velocity = Velocity;
	Flight.FloorZ = Piece->GetComponentLocation().Z;
	Flight.GroundZ = BoxOf(Piece).Min.Z;
	// Tumbles end over end along the hit, with a little twist.
	const float Pace = FMath::Clamp(Velocity.Size2D() / 600.f, 0.4f, 1.6f);
	Flight.Spin = FRotator(FMath::FRandRange(-1.f, -0.6f) * 520.f * Pace, FMath::FRandRange(-160.f, 160.f), FMath::FRandRange(-180.f, 180.f) * Pace);
	// Through everything while airborne (walls included), so it never snags; solid again once it lands.
	TArray<USceneComponent*> Parts;
	Piece->GetChildrenComponents(true, Parts);
	Parts.Add(Piece);
	for (USceneComponent* Part : Parts)
	{
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Part))
		{
			Flight.Collision.Emplace(Prim, Prim->GetCollisionEnabled());
			Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	Flights.Add(MoveTemp(Flight));
}

void UGRKnockAway::Squash(USceneComponent* Piece)
{
	if (!Piece || Piece->Mobility != EComponentMobility::Movable || Piece->ComponentHasTag(SquashedTag))
	{
		return;
	}
	Piece->ComponentTags.Add(SquashedTag);
	// Flat and a little spread, still sitting on the ground.
	const FBox Before = BoxOf(Piece);
	Piece->SetWorldScale3D(Piece->GetComponentScale() * FVector(1.15f, 1.15f, 0.3f));
	Piece->SetWorldRotation(FRotator(0.f, Piece->GetComponentRotation().Yaw + FMath::FRandRange(-25.f, 25.f), 0.f));
	const float Drop = BoxOf(Piece).Min.Z - Before.Min.Z;
	Piece->AddWorldOffset(FVector(0.f, 0.f, -Drop));
}

bool UGRKnockAway::IsFlying(const AActor* Actor, const UPrimitiveComponent* Part) const
{
	for (const FFlight& Flight : Flights)
	{
		const USceneComponent* Piece = Flight.Piece.Get();
		if (Piece && (Piece == Part || (Actor && Piece == Actor->GetRootComponent()) || (Part && Part->IsAttachedTo(Piece))))
		{
			return true;
		}
	}
	return false;
}

void UGRKnockAway::Tick(float DeltaTime)
{
	const float Dt = FMath::Min(DeltaTime, 0.05f);
	for (int32 i = Flights.Num() - 1; i >= 0; --i)
	{
		FFlight& Flight = Flights[i];
		USceneComponent* Piece = Flight.Piece.Get();
		if (!Piece)
		{
			Flights.RemoveAtSwap(i);
			continue;
		}
		Flight.Velocity.Z -= 980.f * Dt;
		FVector Where = Piece->GetComponentLocation() + Flight.Velocity * Dt;
		FRotator Turn = Piece->GetComponentRotation() + Flight.Spin * Dt;
		bool bRest = false;
		if (Where.Z <= Flight.FloorZ && Flight.Velocity.Z < 0.f)
		{
			Where.Z = Flight.FloorZ;
			if (++Flight.Bounces >= 3 || FMath::Abs(Flight.Velocity.Z) < 120.f)
			{
				// Comes to rest on whichever side it fell: upright or tipped over.
				const auto Settle = [](float Angle)
				{
					const float A = FRotator::NormalizeAxis(Angle);
					return FMath::Abs(A) < 45.f ? 0.f : FMath::Abs(A) > 135.f ? 180.f : 90.f * FMath::Sign(A);
				};
				Turn = FRotator(Settle(Turn.Pitch), Turn.Yaw, Settle(Turn.Roll));
				bRest = true;
			}
			else
			{
				Flight.Velocity = FVector(Flight.Velocity.X * 0.45f, Flight.Velocity.Y * 0.45f, -Flight.Velocity.Z * 0.35f);
				Flight.Spin *= 0.4f;
			}
		}
		Piece->SetWorldLocationAndRotation(Where, Turn);
		if (bRest)
		{
			// Lying on its side its lowest point is no longer under the origin: sit it back on the ground.
			Piece->AddWorldOffset(FVector(0.f, 0.f, Flight.GroundZ - BoxOf(Piece).Min.Z));
			for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, ECollisionEnabled::Type>& Saved : Flight.Collision)
			{
				if (UPrimitiveComponent* Prim = Saved.Key.Get())
				{
					Prim->SetCollisionEnabled(Saved.Value);
				}
			}
			Flights.RemoveAtSwap(i);
		}
	}
}
