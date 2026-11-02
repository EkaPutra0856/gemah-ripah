#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/GRInteractable.h"
#include "GRNpc.generated.h"

class UTextRenderComponent;
class USpotLightComponent;
class UStaticMeshComponent;
struct FGRSpawnDef;

/**
 * Story NPC spawned from bab1.json: talks (dialog or actions), says short barks when the player is near,
 * wanders around its spawn point, or patrols. SPN patrols carry a flashlight and raise Diawasi while
 * they can see a suspicious player.
 */
UCLASS()
class GEMAHRIPAH_API AGRNpc : public ACharacter, public IGRInteractable
{
	GENERATED_BODY()

public:
	AGRNpc();

	void Setup(const FGRSpawnDef& Def);

	virtual void Tick(float DeltaSeconds) override;

	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	void SetActive(bool bActive);
	bool IsSpn() const { return bSpn; }
	void Say(const FString& Text, float Duration = 4.f);

	/** World time when any SPN last saw the player while suspicious (HUD warning). */
	static double LastSpottedTime;

	FString SpawnId;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	TObjectPtr<UTextRenderComponent> NameText;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	TObjectPtr<UTextRenderComponent> BarkText;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	TObjectPtr<UStaticMeshComponent> Hat;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	TObjectPtr<USpotLightComponent> Flashlight;

private:
	void TickMovement(float DeltaSeconds, const APawn* Player);
	void TickVision(float DeltaSeconds, APawn* Player);
	void TickBarks(float DeltaSeconds, const APawn* Player);
	void PickWanderTarget();

	FString Prompt;
	FString Actions;
	FString DialogId;
	TArray<FString> Barks;
	FVector Home = FVector::ZeroVector;
	float HomeYaw = 0.f;
	float WanderRadius = 0.f;
	TArray<FVector> Patrol;
	int32 PatrolIndex = 0;
	FVector MoveTarget = FVector::ZeroVector;
	bool bHasTarget = false;
	float WaitTime = 0.f;
	float StuckTime = 0.f;
	float BarkCooldown = 3.f;
	float BarkHideTime = 0.f;
	float TalkPause = 0.f;
	bool bSpn = false;
	bool bChasing = false;
};
