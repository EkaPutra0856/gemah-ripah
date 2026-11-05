#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Interaction/GRInteractable.h"
#include "Math/RandomStream.h"
#include "GRCrowd.generated.h"

class AGRCrowd;
class AGRTrafficCar;
class UAnimMontage;
class UAnimSequenceBase;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USkeletalMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * A townsperson with no story role: strolls the pavements and lanes, crosses the road now and then, chats in a
 * small group, sits on a bench, jogs or pushes a food cart. AGRCrowd places them around Bayu and recycles the ones
 * he leaves behind, so the streets near him stay alive wherever he goes. E greets them.
 */
UCLASS()
class GEMAHRIPAH_API AGRWarga : public ACharacter, public IGRInteractable
{
	GENERATED_BODY()

public:
	AGRWarga();

	virtual void Tick(float DeltaSeconds) override;

	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	/**
	 * Hit at Closing cm/s along Direction by Culprit (Bayu, or a car). A hard hit throws them down as a ragdoll, a
	 * light one shoves them; either way they get up angry at Culprit. Diawasi > 0 puts it on Bayu's record (What
	 * says what happened, or a fitting line is picked). False while they are already down or were just hit.
	 */
	bool KnockedBy(AActor* Culprit, float Closing, const FVector& Direction, float Diawasi, const TCHAR* What = nullptr);

	/** Talk menu (UGRWargaChatScreen): who they are, whether they still listen, and their answers. */
	FText GetChatTitle() const;
	bool IsVendor() const { return Role == ERole::Vendor; }
	/** For GRGerobak: anim playing, speed, and where the hands hold the handlebar (actor space). */
	FString CartDebug() const;

	/**
	 * A sale from the cart (UGRJajanScreen): the vendor makes the order, holds it out and tosses it over the cart; Bayu
	 * catches it, then eats it there (bEatHere) or puts it away. bDrink shows a cup instead of a paper parcel.
	 */
	void StartServe(bool bEatHere, bool bDrink);
	/** Stops a sale halfway (Bayu walked off): hands back to the cart, nothing shown. */
	void CancelServe();
	/** 0 idle, 1 making it, 2 holding it out, 3 in the air, 4 Bayu has it, 5 done. */
	int32 GetServeStage() const;
	/** A speech bubble over them, for screens that talk through them (UGRJajanScreen). */
	void Speak(const FString& Text, float Duration) { Say(Text, Duration); }
	bool CanChat() const;
	/** Keeps them standing and facing Bayu for Seconds more. */
	void HoldChat(float Seconds);
	/** Answers a menu topic: 0 greet (with a wave), 1 how things are, 2 the way to Bayu's goal, 3 buy a snack (vendors). */
	void Chat(int32 Topic);

	/**
	 * Ojol passenger (World/GROjol.h): while set, the crowd leaves them alone (no recycling, no wandering) and the
	 * ojol subsystem moves them: waiting at the kerb, walking to the scooter, riding pillion, getting off.
	 */
	bool IsOjol() const { return bOjol; }
	bool IsFemale() const { return bIsFemale; }
	void OjolStart();
	/** 0 stand, 1 phone in hand, 2 wave at the scooter. */
	void OjolPose(int32 Kind);
	void OjolWalkTo(const FVector& Where);
	bool IsOjolWalking() const { return bOjolWalk; }
	void OjolFace(const FVector& Where);
	/** Sits on the scooter: Relative is in Seat's frame; side-saddle uses the bench pose turned sideways. */
	void OjolBoard(USceneComponent* SeatPart, const FTransform& Relative, bool bSideSaddle, UAnimSequenceBase* RidePose);
	void OjolAlight(const FVector& Feet, float Yaw);
	/** Thrown off the scooter in a crash: down like anyone run over, then back to the crowd. */
	void OjolFall(const FVector& Velocity);
	/** Back to an ordinary townsperson (walks off, recycled later). */
	void OjolRelease();
	void OjolSay(const FString& Text, float Duration) { Say(Text, Duration); }

protected:
	/** Tripo model copying the (then hidden) mannequin's pose, see World/GRBody.h. */
	UPROPERTY(VisibleAnywhere, Category = "Warga")
	TObjectPtr<USkeletalMeshComponent> TripoBody;

	UPROPERTY(VisibleAnywhere, Category = "Warga")
	TObjectPtr<UStaticMeshComponent> Headwear;

	/** Cap brim, hijab drape or the crown of a straw hat. */
	UPROPERTY(VisibleAnywhere, Category = "Warga")
	TObjectPtr<UStaticMeshComponent> HeadExtra;

private:
	friend class AGRCrowd;

	enum class EMode : uint8 { Walk, Pause, Chat, Sit, Down, Angry, Glance, Follow, Chase, Arrest };
	enum class ERole : uint8 { Stroller, Jogger, Vendor, Police };
	/** Who is shouting: the one who was hit, a driver whose car Bayu rammed, or someone watching. */
	enum class EAnger : uint8 { Victim, Shoved, Driver, Bystander };

	/** Body, clothes, headwear and (for vendors) the pushcart, all from the seed. */
	void Dress(int32 Seed, ERole InRole, bool bNight);
	void Activate(const FVector& Feet, float Yaw);
	void Deactivate();
	void Say(const FString& Text, float Duration, const FColor& Color = FColor::White);
	/** Faces At and tells them off for Duration seconds (MM_GR_Marah), then goes back to what they were doing. */
	void GetAngry(AActor* At, float Duration, EAnger Kind);
	void TickAngry(float DeltaSeconds);
	void StartFall(const FVector& Velocity);
	/** Lying in a heap, then blending from the ragdoll back into the standing pose. */
	void TickDown(float DeltaSeconds);
	void StopRagdoll();
	/** Loops Anim over the whole body (nullptr: back to the walk/idle blend); Rate also updates a pose already playing. */
	void PlayPose(UAnimSequenceBase* Anim, float Rate = 1.f);
	/** Picks where to walk next: on along the pavement, across the road, or somewhere in sight in the open. */
	void NextTarget(bool bBlocked);
	void SitDown(int32 InBench, int32 InSeat);
	void StandUp();
	void LeaveActivity();
	void TickWalk(float DeltaSeconds);
	void TickChat(float DeltaSeconds);
	void TickSit(float DeltaSeconds);
	/** Steps out of the way of Bayu's scooter coming straight at them. */
	void TickDodge(const APawn* Player);
	void UpdatePose();
	/** Puts the cart's handlebar in the Tripo body's hands (their arms are not the mannequin's length). */
	void FitCartToHands(float DeltaSeconds);
	/** Moves the sale along (StartServe): the vendor's and Bayu's hands, the parcel, the lines. */
	void TickServe(float DeltaSeconds);
	/** Moves a head piece laid out around the actor onto the head bone, keeping where it sits. */
	void PinToHead(UStaticMeshComponent* Piece);
	void SetSeatedLook(bool bSeated);
	/** High Diawasi: stops for a wary side-glance at Bayu (MM_GR_Lirik), then carries on. */
	void StartGlance();
	void TickGlance(float DeltaSeconds, const APawn* Player);
	/** Very high Diawasi: tails Bayu; outrun them to shake them off, or after a while they report him. */
	void StartFollow();
	void TickFollow(float DeltaSeconds, const APawn* Player);
	/** Bayu on foot walked (or ran, bHard) into them: a startled flinch, or anger when it keeps happening. */
	void Bumped(APawn* Player, const FVector& Direction, bool bHard);
	/** SPN on patrol: the partner keeps beside the leader; a stop is spent on watch or chatting with the partner. */
	void TickPartner(float DeltaSeconds);
	void TickPolicePause(float DeltaSeconds, const APawn* Player);
	/** Running Bayu down (Diawasi very high); close enough and he is caught. */
	void StartChase(bool bShout);
	void TickChase(float DeltaSeconds, APawn* Player);
	void EndChase(bool bLost);
	/** Holding Bayu (MM_GR_Tangkap), or the partner calling it in on the radio. */
	void TickArrest(float DeltaSeconds, const APawn* Player);
	bool IsPolice() const { return Role == ERole::Police; }
	bool IsLeader() const { return !Partner.IsValid() || bLeader || !Partner->bActive; }

	/** M_GR_Warga (Tools/Python/make_warga_material.py): shoes, trousers, shirt and skin by height. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> ClothesBase;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Clothes;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> QuinnMesh;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> MannyMesh;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> TalkAnim;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> SitAnim;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> SitTalkAnim;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> AngryAnim;

	/** Vendors: both hands on the cart's handlebar, standing and walking. */
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> CartAnim;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> CartWalkAnim;

	/** Waving at Bayu, and looking at a phone while standing about. */
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> WaveAnim;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> PhoneAnim;

	UPROPERTY(VisibleAnywhere, Category = "Warga")
	TObjectPtr<UStaticMeshComponent> PhoneProp;

	/** The order being handed over (StartServe), placed in world space every tick. */
	UPROPERTY(VisibleAnywhere, Category = "Warga")
	TObjectPtr<UStaticMeshComponent> FoodProp;
	/** Seconds into the sale, below zero when none; bServed once it is over. */
	float ServeTime = -1.f;
	bool bServeEat = false;
	bool bServed = false;
	int32 ServeLine = 0;
	FVector TossFrom = FVector::ZeroVector;
	FVector FoodScale = FVector::OneVector;

	/** Wary side-glance, and flinching when bumped into. */
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> GlanceAnim;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> StartleAnim;

	/** SPN: on watch (hands behind the back), on the radio, and holding someone. */
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> GuardAnim;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> RadioAnim;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> GrabAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> CurrentPose;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> CurrentMontage;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> CartParts;
	/** Where the hands hold the handlebar, actor space, smoothed (zero until first measured); bar length. */
	FVector CartGrip = FVector::ZeroVector;
	float CartGripWidth = 62.f;

	TWeakObjectPtr<AGRCrowd> Crowd;
	FRandomStream Rng;
	ERole Role = ERole::Stroller;
	EMode Mode = EMode::Pause;
	FVector Target = FVector::ZeroVector;
	/** Where a chatting person looks. */
	FVector LookAt = FVector::ZeroVector;
	/** Height of the speech line above the actor, standing. */
	float BarkZ = 135.f;
	/** Current speech bubble height (lower when seated). */
	float BubbleZ = 135.f;
	/** Waist and neck heights for the clothes material, standing. */
	float HipZ = 95.f;
	float NeckZ = 150.f;
	float Wait = 0.f;
	float Stuck = 0.f;
	float WalkTime = 0.f;
	float Speed = 130.f;
	/** Time left chatting or sitting. */
	float ActivityTime = 0.f;
	float TalkTimer = 0.f;
	/** Turned toward Bayu after he greets them. */
	float TalkPause = 0.f;
	float Dodge = 0.f;
	float DodgeCooldown = 0.f;
	float BarkHide = 0.f;
	float CryTimer = 0.f;
	/** Just stood up from a bench: walk clear of it before bumping into furniture again. */
	float FurnitureIgnore = 0.f;
	/** +1 walks away from the Tugu along the pavement, -1 toward it. */
	float StripDir = 1.f;
	/** Angry mode: time left, and until the next shout. */
	float AngryTime = 0.f;
	float ShoutTimer = 0.f;
	/** Knocked down this long (lying, then getting up). */
	float DownTime = 0.f;
	float HitCooldown = 0.f;
	float ShoveCooldown = 0.f;
	/** Waving (time left), and until they may notice Bayu walking past again. */
	float WaveTime = 0.f;
	float NoticeCooldown = 0.f;
	/** This stop is spent looking at the phone. */
	bool bPhone = false;
	/** Glancing: time left, what they were doing, and until they may glance again. */
	float GlanceTime = 0.f;
	float GlanceCooldown = 0.f;
	EMode GlanceResume = EMode::Walk;
	/** Flinching after a bump (time left). */
	float StartleTime = 0.f;
	/** Tailing Bayu: how long he has been out of reach, and how long they have kept up with him. */
	float FollowLost = 0.f;
	float FollowClose = 0.f;
	/** SPN: the other officer of a pair, and which of them leads. */
	TWeakObjectPtr<AGRWarga> Partner;
	bool bLeader = true;
	/** SPN chase: time Bayu has been out of sight or reach, until the next shout, and until this officer may chase again. */
	float ChaseLost = 0.f;
	float ChaseShout = 0.f;
	float ChaseCooldown = 0.f;
	/** Radio on (time left) while watching or tailing. */
	float RadioTime = 0.f;
	int32 Group = -1;
	int32 Bench = -1;
	int32 Seat = 0;
	bool bActive = false;
	bool bCrossing = false;
	/** On the way to a zebra crossing (GRCrossings) to get to the other pavement: walk there, wait at the kerb for
	 *  a gap in the traffic, then cross. ZebraAlong = where it is along the road. */
	bool bToZebra = false;
	float ZebraAlong = 0.f;
	/** Waiting at the kerb this long; past 12 s they give up and walk on (a car parked by the crossing). */
	float ZebraWait = 0.f;
	/** Someone in the group just raised a topic: answer it. */
	bool bReply = false;
	bool bRagdoll = false;
	bool bGettingUp = false;
	/** Walks up to whoever they are angry with (onlookers mostly shout from where they stand). */
	bool bApproach = false;
	EAnger Anger = EAnger::Victim;
	TWeakObjectPtr<AActor> AngryAt;
	/** Got out of this car to have a word with Bayu; gets back in afterwards. */
	TWeakObjectPtr<AGRTrafficCar> DriverCar;
	/** The driver's door, in the car's frame. */
	FVector DriverDoor = FVector::ZeroVector;
	FName MeshProfile;
	FString Cry;
	bool bIsFemale = false;
	bool bOjol = false;
	bool bOjolWalk = false;
	bool bOjolSeated = false;
	float OjolWalkTime = 0.f;
	FVector OjolTarget = FVector::ZeroVector;
	void TickOjol(float DeltaSeconds);
};

/**
 * Everyday town life around Bayu: a pool of AGRWarga walking, chatting, sitting on the pavement benches, jogging
 * and selling from carts. Busy by day, nearly empty late at night. The story director spawns one.
 */
UCLASS()
class GEMAHRIPAH_API AGRCrowd : public AActor
{
	GENERATED_BODY()

public:
	AGRCrowd();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static AGRCrowd* Get(const UObject* WorldContext);

	/** Cheat GRWarga: how many people are out and what they are doing. */
	void LogCrowd() const;

	/** Cheat GRWargaBaris: a row of townspeople standing still in front of Bayu (no dodging, the vendor pushes the cart a few steps), for checking their looks. */
	void Lineup();

	/**
	 * Bayu caused trouble at Where: Diawasi rises, more for each incident in a short time, people nearby turn and
	 * shout, and the third in a row is a riot (the street crowds round him).
	 */
	void ReportIncident(float Diawasi, const FVector& Where, const FString& What);

	/** The driver of a car Bayu rammed gets out to tell him off; false if there is no room at the door. */
	bool DriverOut(AGRTrafficCar* Car);

	/** Bayu on foot bumped into someone: returns the bump score of the last 15 s (a run-in counts double). */
	float NoteBump(bool bHard);

	/** An SPN officer caught up with Bayu: he is held (hands up), then the chapter's "caught" actions run. */
	void StartArrest(AGRWarga* Officer);
	bool IsArresting() const { return Arrester.IsValid(); }

	/** Cheat GRPolisi: an SPN pair and a lone officer close to Bayu. */
	void SpawnPoliceNear();

private:
	friend class AGRWarga;

	struct FBenchInstance
	{
		TWeakObjectPtr<UInstancedStaticMeshComponent> Mesh;
		int32 Index;
		FTransform Transform;
	};

	struct FBench
	{
		FVector Feet;
		float Yaw;
		TWeakObjectPtr<AGRWarga> Sitters[2];
		TArray<FBenchInstance> Instances;
		TWeakObjectPtr<UPrimitiveComponent> Block;
		/** A story prop or person stands here this chapter: the bench is put away. */
		bool bHidden = false;
	};

	void PlaceBenches();
	void AddBench(const FVector& Feet, float Yaw);
	void RefreshKeepOut();
	FVector SeatFeet(const FBench& B, int32 InSeat) const;
	/** Tops the crowd up near Bayu: a walker, a chatting group, bench sitters, a jogger or a vendor. */
	bool SpawnActivity(const FVector& Around, bool bAnywhere);
	AGRWarga* TakeFromPool();
public:
	/** A free spot for an ojol pickup MinR..MaxR from Around: a main-road pavement if there is one, else any open ground. */
	bool FindPickupSpot(const FVector& Around, float MinR, float MaxR, FVector& OutFeet) const;
	/** Open ground near Around (a drop-off by a building). */
	bool FindGroundSpot(const FVector& Around, float MinR, float MaxR, FVector& OutFeet) const { return FindSpot(Around, MinR, MaxR, false, false, OutFeet); }
	/** A townsperson waiting at Feet for Bayu's scooter: from the pool, or someone out of sight sent home for it. */
	AGRWarga* SpawnPassenger(const FVector& Feet, float Yaw, int32 Seed, bool bFemale);
private:
	/** A free spot on the ground MinR..MaxR from Around; bOutOfSight keeps it out of the camera's view. */
	bool FindSpot(const FVector& Around, float MinR, float MaxR, bool bOutOfSight, bool bPavement, FVector& OutFeet) const;
	bool IsRoad(const FVector2D& P) const;
	bool IsWalkable(const FVector2D& P, float& OutZ) const;
	bool InView(const FVector& Feet) const;
	/** The higher Diawasi, the more people nearby glance at Bayu as he passes; from 80 someone tails him. */
	void UpdateWatchers(const APawn* Player, float Step);
	/** SPN units near Bayu: one patrolling alone and one pair (pairs only at night); they tail or chase him by Diawasi. */
	void UpdatePolice(const APawn* Player, float Step);
	bool SpawnPolice(const FVector& Around, bool bAnywhere, bool bPair);
	void TickArrest(float DeltaSeconds);
	bool PersonNear(const FVector& P, float Radius, const AGRWarga* Except) const;
	/** On one of the four pavements along Jl. Raya / Jl. Merdeka: which one, which side and how far along. */
	bool PavementAt(const FVector2D& P, int32& OutAxis, float& OutSide, float& OutAlong) const;
	FVector2D PavementPoint(int32 Axis, float Side, float Along, float Lateral) const;
	/** Nothing solid in the way and no road to cross (crossings are chosen on purpose, not by accident). */
	bool ClearPath(const AGRWarga* Who, const FVector& From, const FVector& To, bool bMayCrossRoad) const;
	bool CarNear(const FVector2D& P, float Radius) const;
	FString PickChatLine(bool bOpener) const;
	int32 TargetPopulation() const;
	int32 CountActive(int32& OutVendors) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGRWarga>> People;

	/** Bayu with his hands up while an officer holds him. */
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> SurrenderAnim;

	UPROPERTY(Transient)
	/** Bench parts by packed color. */
	TMap<uint32, TObjectPtr<UInstancedStaticMeshComponent>> BenchMeshes;

	TArray<FBench> Benches;
	/** Walled rooms (offices, stealth levels): townspeople keep out. */
	TArray<FBox> Rooms;
	/** Story people and props (x, y, radius): the crowd keeps clear so scenes and E prompts stay theirs. */
	TArray<FVector> KeepOut;
	/** Story people on the streets, for walkers to step around. */
	TArray<TWeakObjectPtr<AActor>> StoryPeople;
	mutable FRandomStream Rng;
	/** World time of Bayu's recent incidents, for escalating Diawasi. */
	TArray<double> Incidents;
	double LastRiot = -1e9;
	/** Bayu walking into people: world time and weight of each bump. */
	TArray<TPair<double, float>> Bumps;
	/** Until the next person may start glancing, or tailing. */
	float GlanceGap = 0.f;
	float FollowGap = 0.f;
	/** Arrest under way: the officer holding Bayu and time since; all SPN stay calm for a while after. */
	TWeakObjectPtr<AGRWarga> Arrester;
	float ArrestTime = -1.f;
	float PoliceCalm = 0.f;
	float PoliceSpawnGap = 0.f;
	/** Some officer was chasing Bayu at the last check (to tell him when he got away). */
	bool bWasChased = false;
	FVector LastPlayerPos = FVector(1e9f);
	float Timer = 0.f;
	float KeepOutTimer = 0.f;
	int32 NextGroup = 0;
	int32 NextSeed = 1;
};
