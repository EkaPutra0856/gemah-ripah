#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GRMapBounds.generated.h"

class UBoxComponent;

/** Marks the playable city area shown on the map (the larger box extent is used on both axes). */
UCLASS()
class GEMAHRIPAH_API AGRMapBounds : public AActor
{
	GENERATED_BODY()

public:
	AGRMapBounds();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Peta")
	TObjectPtr<UBoxComponent> Area;

	FBox2D GetArea2D() const;

protected:
	virtual void BeginPlay() override;
};
