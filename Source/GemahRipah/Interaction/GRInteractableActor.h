#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interaction/GRInteractable.h"
#include "GRInteractableActor.generated.h"

class UStaticMeshComponent;

/**
 * Data-driven interactable for prototyping: set the prompt and effects in the Details panel.
 * Subclass in Blueprint and implement OnInteracted for custom logic.
 */
UCLASS()
class GEMAHRIPAH_API AGRInteractableActor : public AActor, public IGRInteractable
{
	GENERATED_BODY()

public:
	AGRInteractableActor();

	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponen")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaksi")
	FText Prompt = NSLOCTEXT("GemahRipah", "InteractablePrompt", "Periksa");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaksi")
	bool bSingleUse = false;

	/** Action reported to the regulation system (empty = never breaks a rule). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Efek", meta = (Categories = "Aksi"))
	FGameplayTag ActionTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Efek")
	int32 UangDelta = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Efek")
	float SuaraDelta = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Efek")
	float DiawasiDelta = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Efek", meta = (Categories = "Flag"))
	FGameplayTag StoryFlagToAdd;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaksi")
	void OnInteracted(AActor* Interactor, bool bBrokeRule);

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Interaksi")
	bool bUsed = false;
};
