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
	/** Back to the room's camera, or the player's when not in a room. For the end of a cutscene. */
	void ReturnToPlayView();

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
	void HandlePause();
	void HandleMap();
	void HandlePhone();
	void HandleJournal();
	void HandleMotor();
	void HandleRadio();
	friend class UGRCheatManager;
	friend class AGRMotor;

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

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MapAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PhoneAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JournalAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MotorAction;

	UPROPERTY()
	TObjectPtr<UInputAction> RadioAction;

	/** OjekKu: Y accept, N decline, 1-3 chat answers (World/GROjol.h). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> OjolActions;

	TWeakObjectPtr<AGRTopDownRoomVolume> CurrentRoom;
};
