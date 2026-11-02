#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GRPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class AGRCharacter;
class AGRTopDownRoomVolume;
struct FInputActionValue;

/**
 * Owns input and the camera mode.
 * Input actions and the mapping context are built in C++ (no binary .uasset) so they are easy to review.
 */
UCLASS()
class GEMAHRIPAH_API AGRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGRPlayerController();

	void EnterTopDownRoom(AGRTopDownRoomVolume* Room);
	void ExitTopDownRoom(AGRTopDownRoomVolume* Room);

	UFUNCTION(BlueprintPure, Category = "Kamera")
	AGRTopDownRoomVolume* GetCurrentRoom() const { return CurrentRoom.Get(); }

protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	void BuildInputAssets();
	void AddMappingContext();
	AGRCharacter* GetGRCharacter() const;

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleZoom(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpCompleted();
	void HandleSprintStarted();
	void HandleSprintCompleted();
	void HandleInteract();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> DefaultContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractAction;

	TWeakObjectPtr<AGRTopDownRoomVolume> CurrentRoom;
};
