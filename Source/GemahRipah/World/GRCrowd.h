#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Interaction/GRInteractable.h"
#include "Math/RandomStream.h"
#include "GRCrowd.generated.h"

class AGRCrowd;
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

protected:
	UPROPERTY(VisibleAnywhere, Category = "Warga")
	TObjectPtr<UStaticMeshComponent> Headwear;

	/** Cap brim, hijab drape or the crown of a straw hat. */
	UPROPERTY(VisibleAnywhere, Category = "Warga")
	TObjectPtr<UStaticMeshComponent> HeadExtra;

	UPROPERTY(VisibleAnywhere, Category = "Warga")
	TObjectPtr<UTextRenderComponent> BarkText;

private:
	friend class AGRCrowd;

	enum class EMode : uint8 { Walk, Pause, Chat, Sit };
	enum class ERole : uint8 { Stroller, Jogger, Vendor };

	/** Body, clothes, headwear and (for vendors) the pushcart, all from the seed. */
	void Dress(int32 Seed, ERole InRole, bool bNight);
	void Activate(const FVector& Feet, float Yaw);
	void Deactivate();
	void Say(const FString& Text, float Duration);
	void PlayPose(UAnimSequenceBase* Anim);
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
	/** Moves a head piece laid out around the actor onto the head bone, keeping where it sits. */
	void PinToHead(UStaticMeshComponent* Piece);
	void SetSeatedLook(bool bSeated);

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

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> CurrentPose;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> CartParts;

	TWeakObjectPtr<AGRCrowd> Crowd;
	FRandomStream Rng;
	ERole Role = ERole::Stroller;
	EMode Mode = EMode::Pause;
	FVector Target = FVector::ZeroVector;
	/** Where a chatting person looks. */
	FVector LookAt = FVector::ZeroVector;
	/** Height of the speech line above the actor, standing. */
	float BarkZ = 135.f;
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
	int32 Group = -1;
	int32 Bench = -1;
	int32 Seat = 0;
	bool bActive = false;
	bool bCrossing = false;
	/** Someone in the group just raised a topic: answer it. */
	bool bReply = false;
	FString Cry;
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

	/** Cheat GRWargaBaris: a row of townspeople standing still in front of Bayu, for checking their looks. */
	void Lineup();

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
	/** A free spot on the ground MinR..MaxR from Around; bOutOfSight keeps it out of the camera's view. */
	bool FindSpot(const FVector& Around, float MinR, float MaxR, bool bOutOfSight, bool bPavement, FVector& OutFeet) const;
	bool IsRoad(const FVector2D& P) const;
	bool IsWalkable(const FVector2D& P, float& OutZ) const;
	bool InView(const FVector& Feet) const;
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
	FVector LastPlayerPos = FVector(1e9f);
	float Timer = 0.f;
	float KeepOutTimer = 0.f;
	int32 NextGroup = 0;
	int32 NextSeed = 1;
};
