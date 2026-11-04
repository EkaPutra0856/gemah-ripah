#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GRInteractable.h"
#include "GRMotor.generated.h"

class ACharacter;
class UBoxComponent;
class UGRMapMarkerComponent;
class UStaticMeshComponent;

/**
 * Bayu's scooter, built from the "motor" prop model. Ridden, it hangs under his capsule and steers, spins its
 * wheels and leans with him; parked, it stands on its side stand where he got off, with a map marker. The parked
 * spot lives in story vars (motor_parkir 0 = home yard, 1 = motor_x/y/z/yaw, 2 = with Bayu), so it survives
 * saves; waking up in the morning sends it back to the yard.
 */
UCLASS()
class GEMAHRIPAH_API AGRMotor : public AActor, public IGRInteractable
{
	GENERATED_BODY()

public:
	AGRMotor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Hangs the scooter under the rider's capsule, wheels on the ground, and turns its collision off. */
	void AttachToRider(ACharacter* Rider);

	/** Stands the scooter at a ground spot and remembers the spot in the story vars. */
	void Park(const FVector& Ground, float Yaw);

	bool IsParked() const { return !bRidden && !bCrashed && !IsHidden(); }

	/** Thrown down in a crash: comes off the rider, falls on its side (the way it leaned) and slides to a stop. */
	void Crash(const FVector& Velocity, float Lean);

	/** Picked up after a crash: stands on its side stand where it came to rest. */
	void FinishCrash();

	bool IsCrashed() const { return bCrashed; }

	/** Lean (roll) and steering in degrees, and how far the wheels rolled since the last call in cm. */
	void SetRidePose(float Lean, float Steer, float Rolled);

	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	/** Yard in front of Bayu's house, where bab1's "Ambil motor" stands. */
	static const FVector2D HomeSpot;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<USceneComponent> Root;

	/** Upright blocker around the whole scooter while it is parked. */
	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<UBoxComponent> Blocker;

	/** On the ground between the wheels: leaning rolls the scooter about the tyre contact line. */
	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<USceneComponent> LeanPivot;

	/** Steering head, tilted back along the fork (rake). */
	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<USceneComponent> SteerPivot;

	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<USceneComponent> SteerTurn;

	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<USceneComponent> FrontHub;

	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<USceneComponent> FrontSpin;

	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<USceneComponent> RearSpin;

	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<UGRMapMarkerComponent> Marker;

private:
	void BuildModel();
	/** Follows the story vars while not ridden: owned or not, and where it stands. */
	void SyncFromStory();
	void SetOnMap(bool bShow);
	FVector GroundBelow(const FVector& Spot) const;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> StandParts;

	float WheelAngle = 0.f;
	bool bRidden = false;
	bool bOnMap = false;
	bool bCrashed = false;
	FVector CrashVelocity = FVector::ZeroVector;
	float CrashLean = 0.f;
	float CrashFallRate = 0.f;
};
