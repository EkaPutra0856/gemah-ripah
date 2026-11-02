#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GRInteractable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UGRInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Anything the player can press "Interaksi" on: NPC, door, PC warnet, baliho, etc. */
class GEMAHRIPAH_API IGRInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaksi")
	FText GetInteractionPrompt(AActor* Interactor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaksi")
	bool CanInteract(AActor* Interactor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaksi")
	void Interact(AActor* Interactor);

	// Declared explicitly so these defaults are used instead of UHT's empty ones.
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const;
	virtual bool CanInteract_Implementation(AActor* Interactor) const;
	virtual void Interact_Implementation(AActor* Interactor);
};
