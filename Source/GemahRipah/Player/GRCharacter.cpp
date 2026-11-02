#include "Player/GRCharacter.h"
#include "Core/GRGameUserSettings.h"
#include "Player/GRInteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

AGRCharacter::AGRCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// The camera turns freely; the body turns toward where it walks.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 600.f, 0.f);
	Move->JumpZVelocity = 500.f;
	Move->AirControl = 0.35f;
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MinAnalogWalkSpeed = 20.f;
	Move->BrakingDecelerationWalking = 2000.f;
	Move->BrakingDecelerationFalling = 1500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 450.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Interaction = CreateDefaultSubobject<UGRInteractionComponent>(TEXT("Interaction"));

	// Placeholder body until Bayu's model arrives from Tripo.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> MannyAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (MannyMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(MannyMesh.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));
	}
	if (MannyAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(MannyAnim.Class);
	}
}

void AGRCharacter::MoveFromInput(const FVector2D& Axis)
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// Use the active camera's yaw, so "up" is screen-up in both third-person and top-down views.
	const float Yaw = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation().Yaw : PC->GetControlRotation().Yaw;
	const FRotationMatrix YawMatrix(FRotator(0.f, Yaw, 0.f));

	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Axis.X);
}

void AGRCharacter::LookFromInput(const FVector2D& Axis)
{
	if (bTopDownMode)
	{
		return;
	}
	const UGRGameUserSettings* Settings = UGRGameUserSettings::Get();
	const float Sensitivity = Settings ? Settings->MouseSensitivity : 1.f;
	const float InvertY = Settings && Settings->bInvertLookY ? -1.f : 1.f;
	AddControllerYawInput(Axis.X * Sensitivity);
	AddControllerPitchInput(Axis.Y * Sensitivity * InvertY);
}

void AGRCharacter::ZoomFromInput(float Delta)
{
	if (bTopDownMode)
	{
		return;
	}
	CameraBoom->TargetArmLength = FMath::Clamp(CameraBoom->TargetArmLength - Delta * ZoomStep, MinZoom, MaxZoom);
}

void AGRCharacter::SetSprinting(bool bNewSprinting)
{
	GetCharacterMovement()->MaxWalkSpeed = bNewSprinting ? SprintSpeed : WalkSpeed;
}
