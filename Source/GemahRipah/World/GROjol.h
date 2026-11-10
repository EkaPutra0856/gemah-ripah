#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GROjol.generated.h"

class AGRCharacter;
class AGRWarga;

/** Where the OjekKu driver app is: off, waiting for an order, an offer on screen, or a trip under way. */
enum class EGROjolStage : uint8
{
	Offline,
	Waiting,
	Offer,
	ToPickup,
	Boarding,
	Riding,
	Summary
};

struct FGROjolChoice
{
	FString Text;
	FString Reply;
	float Comfort = 0.f;
};

/** One OjekKu order: who, from where to where, how far, the fare, and how the ride is going. */
struct FGROjolTrip
{
	FString Name;
	FString Honorific;
	bool bFemale = false;
	float PaxRating = 5.f;
	FVector Pickup = FVector::ZeroVector;
	float PickupYaw = 0.f;
	FString PickupLabel;
	FVector Dest = FVector::ZeroVector;
	FString DestLabel;
	/** Route length pickup to drop-off, metres (HUD units: cm / 100). */
	float Meters = 0.f;
	int32 Fare = 0;
	float ExpectedSeconds = 60.f;
	int32 Question = 0;

	float Comfort = 5.f;
	float Elapsed = 0.f;
	int32 Jolts = 0;
	bool bAsked = false;
	bool bCrashed = false;

	int32 Stars = 5;
	int32 Paid = 0;
	int32 Tip = 0;
	FString Review;
};

/**
 * OjekKu, Bayu's ride-hailing driver app. Online (phone, OjekKu app) he gets order offers every so often while on
 * the scooter: Y takes one, N turns it down. A townsperson waits at the pickup point (AGRWarga::OjolStart), greets
 * him when he pulls up, climbs on behind him and chats on the way (answers on 1/2/3). Jolts, crashes, speeding and
 * a late arrival cost stars; the fare goes by route length, less the app's 20%, plus a tip for five stars. The route
 * follows the main roads and shows on the phone map, the minimap and the HUD waypoint. Story orders
 * (UGRStorySubsystem::StartOrder) come first: no offers while one runs.
 */
UCLASS()
class GEMAHRIPAH_API UGROjolSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UGROjolSubsystem* Get(const UObject* WorldContext);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGROjolSubsystem, STATGROUP_Tickables); }
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override { return WorldType == EWorldType::Game || WorldType == EWorldType::PIE; }

	bool IsOnline() const { return Stage != EGROjolStage::Offline; }
	/** Why Bayu cannot go online right now, or empty when he can. */
	FString WhyOffline() const;
	void SetOnline(bool bOnline);
	/** Y / N on an offer. */
	void Accept();
	void Decline();
	/** 1 / 2 / 3 in a chat with the passenger (Index 0..2). */
	void Choose(int32 Index);
	/** Driver cancels a trip in progress (phone): the passenger leaves, no pay, the account rating drops. */
	void CancelTrip();
	/** Cheat GROjol: online, and an offer at once. */
	void ForceOffer();
	/** Dev bot: an offer at once whose pickup and drop-off lie on the main roads, between MinMeters and MaxMeters long, away from the roundabout. */
	void ForceRoadOffer(float MinMeters, float MaxMeters);
	/** Dev bot: seats a passenger on the parked scooter at once, to check how they sit. */
	void CheatSeatPassenger(bool bFemale);
	/** Cheat GROjolLompat: Bayu (and whoever rides with him) jumps to a few metres before the pickup or drop-off. */
	void CheatJumpToTarget();
	/** The customer was knocked over at the kerb before boarding. */
	void PassengerHit(AGRWarga* Who);

	EGROjolStage GetStage() const { return Stage; }
	const FGROjolTrip& GetTrip() const { return Trip; }
	float GetOfferTimeLeft() const { return OfferLeft; }
	static constexpr float OfferSeconds = 15.f;
	float GetSummaryTime() const { return SummaryTime; }
	/** Bumped whenever the HUD should rebuild (new stage, new chat). */
	int32 GetRevision() const { return Revision; }

	/** The place to head for now: pickup or drop-off. */
	bool GetTarget(FVector& OutLocation, FText& OutLabel) const;
	/** Route from Bayu to the target along the roads, world XY. */
	const TArray<FVector2D>& GetRoute() const { return Route; }
	float GetRouteMeters() const { return RouteMeters; }
	/** "Belok kanan 120 m" style next-step text along the route. */
	FString GetTurnText() const;
	/** One status line for the HUD trip bar. */
	FString GetStatusLine() const;

	bool IsChatOpen() const { return ChatChoices.Num() > 0; }
	const FString& GetChatPrompt() const { return ChatPrompt; }
	const TArray<FString>& GetChatChoices() const { return ChatChoices; }
	float GetChatTimeLeft() const { return ChatLeft; }

	float GetRatingAverage() const;
	int32 GetTripsDone() const;
	int32 GetEarnings() const;

	/** Fare for a route of Meters: base, per metre, rounded to Rp 500. */
	static int32 FareFor(float Meters);
	/** Road-following path from A to B on the main road grid. */
	static void BuildRoute(const FVector2D& A, const FVector2D& B, TArray<FVector2D>& Out);
	static float RouteLength(const TArray<FVector2D>& Points);

private:
	AGRCharacter* GetBayu() const;
	bool CanTakeOffers(FString* OutWhy) const;
	bool MakeOffer();
	void ShowOffer();
	void SetStage(EGROjolStage NewStage);
	void StartChat(const FString& Prompt, const TArray<FGROjolChoice>& Choices, float Seconds);
	void PassengerSay(const FString& Text, float Seconds = 3.5f) const;
	void Board();
	void Finish(bool bCompleted);
	void Abandon(const FString& Why, bool bPenalty);
	void TickRide(float DeltaTime, AGRCharacter* Bayu);
	void UpdateRoute(const AGRCharacter* Bayu);

	EGROjolStage Stage = EGROjolStage::Offline;
	FGROjolTrip Trip;
	TWeakObjectPtr<AGRWarga> Passenger;
	TArray<FVector2D> Route;
	float RouteMeters = 0.f;
	float RouteTimer = 0.f;
	float OfferTimer = 0.f;
	float OfferLeft = 0.f;
	float SummaryTime = 0.f;
	float StageTime = 0.f;
	int32 Revision = 0;
	int32 Seed = 1;
	int32 PaxSeedNow = 0;

	FString ChatPrompt;
	TArray<FString> ChatChoices;
	TArray<FGROjolChoice> ChatDefs;
	float ChatLeft = 0.f;
	/** 1 greeting at the pickup, 2 the question on the way. */
	int32 ChatKind = 0;
	bool bWaved = false;
	bool bWalkingToSeat = false;

	float PrevSpeed = 0.f;
	float SpeedingTime = 0.f;
	float ComplainCooldown = 0.f;
	float OffMotorTime = 0.f;
	FVector LastBayu = FVector::ZeroVector;
};
