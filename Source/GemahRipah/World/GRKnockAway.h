#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GRKnockAway.generated.h"

class USceneComponent;
class UPrimitiveComponent;

/**
 * Small things Bayu's scooter hits (chairs, pots, crates, buckets): flung off tumbling and landing where they fall,
 * or flattened when they are low enough to go under the wheel. No physics bodies needed: the props are built from
 * several basic shapes on a scene root, so the flight is moved by hand (gravity, a bounce, then rest).
 */
UCLASS()
class GEMAHRIPAH_API UGRKnockAway : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Sends Piece (a small actor's root, or one small part of a big one) off at Velocity, spinning. */
	void Launch(USceneComponent* Piece, const FVector& Velocity);
	/** Run over: flattened where it stands (once). */
	void Squash(USceneComponent* Piece);
	/** True while Piece (or the actor it belongs to) is in the air: the scooter rides through it. */
	bool IsFlying(const AActor* Actor, const UPrimitiveComponent* Part) const;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGRKnockAway, STATGROUP_Tickables); }

private:
	struct FFlight
	{
		TWeakObjectPtr<USceneComponent> Piece;
		FVector Velocity = FVector::ZeroVector;
		FRotator Spin = FRotator::ZeroRotator;
		float FloorZ = 0.f;
		int32 Bounces = 0;
		/** Lowest point of the thing when it was hit: where it lies once it lands. */
		float GroundZ = 0.f;
		/** Collision put back on landing. */
		TArray<TPair<TWeakObjectPtr<UPrimitiveComponent>, ECollisionEnabled::Type>> Collision;
	};
	TArray<FFlight> Flights;
};
