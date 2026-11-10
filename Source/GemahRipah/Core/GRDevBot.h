#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GRDevBot.generated.h"

class AGRCharacter;
class AGRStealthLevel;

/**
 * Plays parts of the game by itself for recordings and regression runs, through the same controls a player has:
 * "susup" walks the stealth minigame (takes all the evidence, then leaves by the exit), "ojol N" goes online and
 * drives N OjekKu trips along the app's route. Only dev scripts and the GRBot cheat start it.
 */
UCLASS()
class GEMAHRIPAH_API UGRDevBot : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UGRDevBot* Get(const UObject* WorldContext);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGRDevBot, STATGROUP_Tickables); }
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override { return WorldType == EWorldType::Game || WorldType == EWorldType::PIE; }

	void StartStealth();
	void StartOjol(int32 Trips);
	/**
	 * Goes to Target the way a player would: on the scooter along the roads in the left lane (round the Tugu like the
	 * traffic), on foot along the pavements, changing sides only at a zebra crossing once no car is close. Speed is how
	 * far the stick is pushed when walking (0..1). The camera turns to show Bayu from CameraYaw off his heading.
	 */
	void StartPath(const FVector2D& Target, float Speed, float CameraYaw = 150.f);
	void Stop();
	bool IsBusy() const { return Mode != EMode::Idle; }

private:
	enum class EMode : uint8
	{
		Idle,
		Stealth,
		Ojol,
		Path
	};

	AGRCharacter* GetBayu() const;
	bool HasScreens() const;

	void TickStealth(float DeltaTime, AGRCharacter* Bayu, AGRStealthLevel& Level);
	/** Next cell towards Goal that keeps clear of the guards' flashlights over time, or Here to wait. */
	FIntPoint PlanStep(const AGRStealthLevel& Level, const FIntPoint& Here, const FIntPoint& Goal, float WalkSpeed) const;

	void TickOjol(float DeltaTime, AGRCharacter* Bayu);
	/** Follows Route, or the OjekKu route when there is none. */
	void Drive(float DeltaTime, AGRCharacter* Bayu, float StopShort, const TArray<FVector2D>* Route = nullptr);
	void TickPath(float DeltaTime, AGRCharacter* Bayu);
	/** A car within Radius of P. */
	bool CarNear(const FVector2D& P, float Radius) const;

	EMode Mode = EMode::Idle;
	float StartWait = 0.f;

	FIntPoint StepCell = FIntPoint(-1, -1);
	float TakeCooldown = 0.f;
	bool bLevelSeen = false;

	int32 TripsLeft = 0;
	int32 TripsDone = 0;
	int32 LastStage = -1;
	float StageTime = 0.f;
	float StuckTime = 0.f;
	float ReverseTime = 0.f;
	float ReverseSteer = 0.f;
	/** Waypoints still ahead; a kerb point is where he waits for the road to clear before crossing. */
	TArray<FVector2D> PathPoints;
	TArray<bool> PathKerbs;
	float PathSpeed = 1.f;
	float PathCameraYaw = 150.f;
	float PathWaitLogged = 0.f;
};
