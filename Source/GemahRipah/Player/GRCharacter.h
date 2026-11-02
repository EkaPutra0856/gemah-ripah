#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GRCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UGRInteractionComponent;
class UStaticMeshComponent;

/**
 * Bayu. Third-person open-world camera (Genshin-like) by default.
 * Input arrives from AGRPlayerController; movement is relative to whatever camera is active,
 * so it also works in top-down rooms.
 */
UCLASS()
class GEMAHRIPAH_API AGRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGRCharacter();

	void MoveFromInput(const FVector2D& Axis);
	void LookFromInput(const FVector2D& Axis);
	void ZoomFromInput(float Delta);

	UFUNCTION(BlueprintCallable, Category = "Gerak")
	void SetSprinting(bool bNewSprinting);

	/** Disables mouse look while a top-down room camera is active. */
	void SetTopDownMode(bool bEnabled) { bTopDownMode = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Kamera")
	bool IsInTopDownMode() const { return bTopDownMode; }

	UGRInteractionComponent* GetInteraction() const { return Interaction; }

	/** Bayu's ojol motor: much faster, no stamina cost. */
	UFUNCTION(BlueprintCallable, Category = "Gerak")
	void SetRiding(bool bNewRiding);

	UFUNCTION(BlueprintPure, Category = "Gerak")
	bool IsRiding() const { return bRiding; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kamera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kamera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaksi")
	TObjectPtr<UGRInteractionComponent> Interaction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gerak")
	float WalkSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gerak")
	float SprintSpeed = 750.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gerak")
	float RideSpeed = 1600.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gerak")
	TObjectPtr<UStaticMeshComponent> MotorBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gerak")
	TObjectPtr<UStaticMeshComponent> MotorFront;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamera")
	float MinZoom = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamera")
	float MaxZoom = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamera")
	float ZoomStep = 60.f;

private:
	bool bTopDownMode = false;
	bool bRiding = false;
	bool bSprinting = false;
};
