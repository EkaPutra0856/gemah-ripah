#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GRCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UGRInteractionComponent;
class AGRMotor;
class UAnimMontage;
class UAnimSequenceBase;

/**
 * Bayu. Third-person open-world camera (Genshin-like) by default.
 * Input arrives from AGRPlayerController; movement is relative to whatever camera is active,
 * so it also works in top-down rooms.
 */
UCLASS()
class GEMAHRIPAH_API AGRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGRCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanJumpInternal_Implementation() const override;

	void MoveFromInput(const FVector2D& Axis);
	void LookFromInput(const FVector2D& Axis);
	void ZoomFromInput(float Delta);

	UFUNCTION(BlueprintCallable, Category = "Gerak")
	void SetSprinting(bool bNewSprinting);

	/** Disables mouse look while a top-down room camera is active. */
	void SetTopDownMode(bool bEnabled) { bTopDownMode = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Kamera")
	bool IsInTopDownMode() const { return bTopDownMode; }

	UGRInteractionComponent* GetInteraction() const { return Interaction; }

	/**
	 * Bayu's ojol motor: much faster, no stamina cost. Getting on near the parked scooter seats him on it;
	 * anywhere else (story, cheat) the scooter comes to him. Getting off parks it where he stands.
	 */
	UFUNCTION(BlueprintCallable, Category = "Gerak")
	void SetRiding(bool bNewRiding);

	UFUNCTION(BlueprintPure, Category = "Gerak")
	bool IsRiding() const { return bRiding; }

	/** F while riding: brake to a stop first, then get off. */
	void RequestDismount();

	/** The parked scooter is close enough to get on. */
	bool IsNearMotor() const;

	/** Metres to the parked scooter, or -1 when it is not parked. */
	float GetMotorDistance() const;

	/** Signed speed along the scooter's heading, cm/s. */
	float GetRideSpeed() const { return RideSpeedNow; }

	/** Roll into the turn, degrees (positive leans right). */
	float GetRideLean() const { return LeanAngle; }

	/**
	 * A traffic car and Bayu met at Closing cm/s; Fling is the way his body goes, bRammed means he rode into it.
	 * A bump shakes the camera, a harder one costs energy, and from about 27 km/h he comes off (ragdoll) for a moment.
	 */
	void HandleCarImpact(float Closing, const FVector& Fling, bool bRammed);
	/**
	 * Rode into something big and solid (a wall, a tree, a lamp post, a stall) at Closing cm/s head-on: fast enough
	 * and Bayu goes over the bars, a bit slower and the scooter bounces back, slower still and it just stops.
	 * Returns true when the scooter's speed was set here.
	 */
	bool HandleWallImpact(float Closing, const FVector& Heading);
	/** The scooter's nose met something that is not a person or a car: small things fly off or go under the wheel. */
	bool RideIntoThing(const FHitResult& Hit, const FVector& Ahead, float SpeedAbs);
	/** Head-on speeds into something big: below Bounce it just stops, from Crash on Bayu comes off (cm/s, 11 and 25 km/h). */
	static constexpr float WallBounceSpeed = 300.f;
	static constexpr float WallCrashSpeed = 700.f;

	bool IsKnockedDown() const { return bKnockedDown; }

	/** Back on his feet now: after the ragdoll settles, or straight away before a teleport. */
	void EndKnockdown();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kamera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kamera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaksi")
	TObjectPtr<UGRInteractionComponent> Interaction;

	/** Bayu's Tripo model, copying the hidden mannequin's pose (World/GRBody.h). */
	UPROPERTY(VisibleAnywhere, Category = "Tubuh")
	TObjectPtr<USkeletalMeshComponent> TripoBody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gerak")
	float WalkSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gerak")
	float SprintSpeed = 750.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gerak")
	float RideSpeed = 1600.f;

	/** Pushing the scooter backwards with the feet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gerak")
	float RideReverseSpeed = 160.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gerak")
	TObjectPtr<AGRMotor> Motor;

	/** Seated on the scooter, feet on the floorboard, hands on the grips (MM_GR_Ride). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gerak")
	TObjectPtr<UAnimSequenceBase> RideAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamera")
	float MinZoom = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamera")
	float MaxZoom = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamera")
	float ZoomStep = 60.f;

private:
	/** Throttle, brakes, steering, lean and the camera behind the scooter. */
	void TickRide(float DeltaSeconds);

	void StartKnockdown(const FVector& Velocity);
	void TickKnockdown(float DeltaSeconds);
	void ShakeCamera(float Amplitude, float Duration);

	bool bTopDownMode = false;
	bool bRiding = false;
	bool bSprinting = false;
	bool bWantDismount = false;
	/** Latest move input while riding: Y throttle (S brakes), X steers. Used up every frame. */
	FVector2D RideInput = FVector2D::ZeroVector;
	float RideSpeedNow = 0.f;
	float SteerAngle = 0.f;
	float LeanAngle = 0.f;
	/** How long S has been held at a standstill; backing up starts after a moment. */
	float StoppedBrakeTime = 0.f;
	float LastLookTime = -100.f;

	bool bKnockedDown = false;
	/** Hits can arrive in the middle of a movement update; the ragdoll starts on the next tick. */
	bool bPendingKnock = false;
	FVector PendingKnock = FVector::ZeroVector;
	bool bKnockFade = false;
	float KnockTime = 0.f;
	FName MeshProfile;
	float ShakeTime = 0.f;
	float ShakeLength = 0.f;
	float ShakeAmp = 0.f;
	float ImpactCooldown = 0.f;
	/** Real time when the short slow-motion on a hard hit ends. */
	double HitStopEnd = 0.0;
};
