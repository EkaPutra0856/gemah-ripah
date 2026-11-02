#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GRDistrictVolume.generated.h"

class UBoxComponent;

/** A named area of the city. Its name pops up when the player walks in and it is drawn on the big map. */
UCLASS()
class GEMAHRIPAH_API AGRDistrictVolume : public AActor
{
	GENERATED_BODY()

public:
	AGRDistrictVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kawasan")
	TObjectPtr<UBoxComponent> Area;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kawasan")
	FName DistrictId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kawasan")
	FText DisplayName;

	/** Small line under the name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kawasan")
	FText Subtitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kawasan")
	FLinearColor MapColor = FLinearColor(0.9f, 0.6f, 0.2f);

	/** World-space XY rectangle (for the big map). */
	FBox2D GetArea2D() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	static bool IsLocalPlayerRoot(const AActor* Actor, const UPrimitiveComponent* Comp);
};
