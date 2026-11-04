#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "GRTraffic.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * One car or angkot of the road traffic. AGRTraffic drives it kinematically along its lane (swept, so it stops at
 * whatever it touches instead of passing through). A hard knock turns it into a physics body for a moment: it
 * slides, spins and can shove other cars, then the driver steers back into the lane.
 */
UCLASS()
class GEMAHRIPAH_API AGRTrafficCar : public AActor
{
	GENERATED_BODY()

public:
	AGRTrafficCar();

	virtual void Tick(float DeltaSeconds) override;

	void Build(bool bInAngkot, const TCHAR* Color, float InCruise);

	/** Pushes the car as a physics body (impulse in kg cm/s at a world point). */
	void Knock(const FVector& Impulse, const FVector& Point);

	bool IsKnocked() const { return bKnocked; }

	/** World velocity, driving or sliding. */
	FVector GetMoveVelocity() const;

	/** Short shout above the roof ("TIIN!", "BRAK!"). */
	void Shout(const FString& Text, const FColor& Color, float Duration);

	float GetHalfLength() const { return HalfLength; }
	float GetHalfWidth() const { return HalfWidth; }

	/** Bayu on the scooter rams the car at Closing cm/s. */
	void HitByRider(float Closing, const FVector& Direction, const FVector& Point);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<UBoxComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<UTextRenderComponent> ShoutText;

private:
	friend class AGRTraffic;

	UFUNCTION()
	void OnBodyHit(UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	/** Physics back off: AGRTraffic finds the lane again and blends the car into it. */
	void EndKnock();

	bool bAngkot = false;
	float HalfLength = 210.f;
	float HalfWidth = 88.f;
	float HalfHeight = 70.f;
	/** Desired speed on a clear straight road, cm/s. */
	float Cruise = 1100.f;

	// Lane following, owned by AGRTraffic. The path is two legs (in, round the Tugu, out, U-turn) buffered ahead.
	TArray<FVector2D> Path;
	TArray<float> PathDist;
	/** Speed the curve allows at each path point. */
	TArray<float> PathLimit;
	TArray<float> LegEnds;
	struct FYield
	{
		float S;
		float MergeAngle;
	};
	TArray<FYield> Yields;
	int32 NextArm = 0;
	float S = 0.f;
	float Speed = 0.f;
	float GroundZ = 0.f;
	/** Left over from a knock: where the car stands relative to its lane, shrinking as it drives on. */
	FVector2D Offset = FVector2D::ZeroVector;
	float YawOffset = 0.f;
	bool bRejoin = false;
	/** Stopped for Bayu (or his scooter) this long: honk. */
	float BlockedByPlayer = 0.f;
	float HonkCooldown = 0.f;
	/** Easing past something standing on the lane (a parked scooter, a person): sideways to the right, cm. */
	float Shift = 0.f;
	float ShiftTarget = 0.f;
	float ShiftYaw = 0.f;
	float StuckBehind = 0.f;
	bool bAvoiding = false;
	FVector2D AvoidPos = FVector2D::ZeroVector;
	float AvoidRadius = 0.f;
	/** Cheat GRMobil: stand still here until this runs out. */
	float HoldTime = 0.f;

	bool bKnocked = false;
	float KnockTime = 0.f;
	float ShoutTime = 0.f;
	float HitCooldown = 0.f;
};

/**
 * Road traffic on Jl. Raya and Jl. Merdeka. Cars keep left, go round the Tugu clockwise (giving way to cars
 * already on the roundabout), take a random exit and U-turn at the edge of town. Each car follows the one ahead
 * (intelligent driver model) and stops for Bayu, his scooter and people on the road. The story director spawns one.
 */
UCLASS()
class GEMAHRIPAH_API AGRTraffic : public AActor
{
	GENERATED_BODY()

public:
	AGRTraffic();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static AGRTraffic* Get(const UObject* WorldContext);

	/** Cheat/test: the nearest car stands still at Spot facing Yaw for HoldSeconds. */
	void PlaceCar(const FVector& Spot, float Yaw, float HoldSeconds);

	void LogCars() const;

private:
	struct FObstacle
	{
		FVector2D Pos;
		FVector2D Vel;
		float Radius;
		const AGRTrafficCar* Car;
		bool bPlayer;
	};

	/** Appends a leg from Car.NextArm: inbound lane, round the Tugu, out on a random arm, U-turn at the far end. */
	void AppendLeg(AGRTrafficCar& Car);
	void DropFirstLeg(AGRTrafficCar& Car);
	FVector2D PathAt(const AGRTrafficCar& Car, float At, FVector2D* OutDir = nullptr) const;
	void Rejoin(AGRTrafficCar& Car);
	bool RingBusy(const AGRTrafficCar& Car, float MergeAngle) const;
	void Drive(AGRTrafficCar& Car, float DeltaSeconds);
	void GatherObstacles();

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGRTrafficCar>> Cars;

	TArray<FObstacle> Obstacles;
	/** Asphalt over the pavement strips that cross at the Tugu, so the roundabout reads as road. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> RingRoad;
	/** Fixed seed: the same cars take the same turns every run, which keeps scripted tests repeatable. */
	FRandomStream Rng;
	FVector2D PlayerPos = FVector2D::ZeroVector;
	FVector2D PlayerVel = FVector2D::ZeroVector;
	bool bPlayerOnRing = false;
};
