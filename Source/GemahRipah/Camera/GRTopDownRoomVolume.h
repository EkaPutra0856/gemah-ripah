#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GRTopDownRoomVolume.generated.h"

class UBoxComponent;
class UCameraComponent;

/**
 * Room that switches the camera to top-down while the player is inside (kelurahan, dapur PPR, warnet, ...).
 * Scale the Trigger box to the room and place the camera above it.
 */
UCLASS()
class GEMAHRIPAH_API AGRTopDownRoomVolume : public AActor
{
	GENERATED_BODY()

public:
	AGRTopDownRoomVolume();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponen")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponen")
	TObjectPtr<UCameraComponent> RoomCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kamera")
	float BlendTime = 0.6f;

	/** Big rooms: the camera follows the player, keeping its offset from the room center. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kamera")
	bool bFollowPlayer = false;

	/** Minigame that starts in this room (empty = plain room). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minigame", meta = (Categories = "Minigame"))
	FGameplayTag MinigameId;

	/** Camera offset (from the room center, or from the player when following), rotation and follow mode. */
	void SetView(const FVector& Offset, const FRotator& Rotation, bool bFollow);
	void GetView(FVector& OutOffset, FRotator& OutRotation, bool& bOutFollow) const;

	bool HasPlayerInside() const { return ActivePlayer.IsValid(); }

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Ruangan")
	void OnPlayerEntered(APawn* Player);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ruangan")
	void OnPlayerExited(APawn* Player);

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	TWeakObjectPtr<APawn> ActivePlayer;
	FVector CameraOffsetFromCenter = FVector::ZeroVector;
};
