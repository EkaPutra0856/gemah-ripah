#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GRInteractable.h"
#include "GRStoryProp.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
struct FGRSpawnDef;

/** Interactable object ("prop") or invisible trigger area ("zone") spawned from bab1.json. */
UCLASS()
class GEMAHRIPAH_API AGRStoryProp : public AActor, public IGRInteractable
{
	GENERATED_BODY()

public:
	AGRStoryProp();

	void Setup(const FGRSpawnDef& Def);
	void SetActive(bool bActive);
	bool IsZone() const { return bZone; }
	float GetRadius() const { return Radius; }
	const FString& GetActions() const { return Actions; }

	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	FString SpawnId;
	/** Zone bookkeeping: the player was inside on the last check. */
	bool bPlayerInside = false;

	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<UTextRenderComponent> Label;

private:
	FString Prompt;
	FString Actions;
	FString DialogId;
	float Radius = 300.f;
	bool bZone = false;
};
