#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GRInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGROnInteractFocusChanged, AActor*, NewFocus);

/** Finds the best nearby interactable (closest, in front of the player) and triggers it. */
UCLASS(ClassGroup = (GemahRipah), meta = (BlueprintSpawnableComponent))
class GEMAHRIPAH_API UGRInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGRInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Interaksi")
	bool TryInteract();

	UFUNCTION(BlueprintPure, Category = "Interaksi")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	/** Prompt for the focused actor, empty if nothing is focused. */
	UFUNCTION(BlueprintPure, Category = "Interaksi")
	FText GetFocusedPrompt() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaksi")
	float Radius = 200.f;

	UPROPERTY(BlueprintAssignable, Category = "Interaksi")
	FGROnInteractFocusChanged OnFocusChanged;

private:
	AActor* FindBestInteractable() const;

	TWeakObjectPtr<AActor> FocusedActor;
	float FocusTimer = 0.f;
	/** Yaw Bayu turns toward after an interaction, while TurnTimeLeft runs. */
	float TurnYaw = 0.f;
	float TurnTimeLeft = 0.f;
};
