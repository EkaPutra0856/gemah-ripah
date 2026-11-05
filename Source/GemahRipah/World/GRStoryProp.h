#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GRInteractable.h"
#include "GRStoryProp.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
struct FGRSpawnDef;
struct FGRPropPart;

/** Interactable object ("prop") or invisible trigger area ("zone") spawned from bab1.json. */
UCLASS()
class GEMAHRIPAH_API AGRStoryProp : public AActor, public IGRInteractable
{
	GENERATED_BODY()

public:
	AGRStoryProp();

	void Setup(const FGRSpawnDef& Def);
	void SetActive(bool bActive);
	virtual void Tick(float DeltaSeconds) override;
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
	/** Builds a furniture model from parts; returns the height of its top. */
	float BuildParts(const TArray<FGRPropPart>& Parts);
	/**
	 * Shows TripoMesh in place of the basic shapes, fitted to their footprint and height. The shapes stay as
	 * invisible collision (seats, table tops and roofs keep working the same). Returns the new top.
	 */
	float ShowTripo(UStaticMesh* TripoMesh, const TArray<FGRPropPart>& Parts, float YawOffset);

	/** Tent roofs and awnings, hidden while the player is underneath. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> RoofParts;
	FVector2D RoofExtent = FVector2D::ZeroVector;
	/** Basic shapes under a roof shown only while the Tripo roofed model is hidden (player underneath). */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> UnderParts;

	FString Prompt;
	FString Actions;
	FString DialogId;
	float Radius = 300.f;
	bool bZone = false;
};
