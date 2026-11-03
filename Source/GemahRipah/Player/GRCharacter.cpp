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
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Story/GRStoryUtil.h"

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
	// Kerbs (12 cm) and stage steps (32 cm) yes, chair seats (46 cm) no: with the default 45 cm, the floor
	// check took the edges of two chairs side by side for a step and Bayu walked over the dining table.
	Move->MaxStepHeight = 34.f;
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

	// Grey-box scooter under Bayu, shown while riding. Feet are at Z -96.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	struct FPart { const TCHAR* Name; bool bWheel; FVector Location; FVector Scale; };
	const FPart Parts[] = {
		{ TEXT("MotorBody"), false, FVector(0.f, 0.f, -66.f), FVector(1.1f, 0.3f, 0.2f) },
		{ TEXT("MotorCowl"), false, FVector(50.f, 0.f, -45.f), FVector(0.18f, 0.36f, 0.6f) },
		{ TEXT("MotorSeat"), false, FVector(-28.f, 0.f, -46.f), FVector(0.6f, 0.3f, 0.2f) },
		{ TEXT("MotorBar"), false, FVector(44.f, 0.f, -12.f), FVector(0.06f, 0.75f, 0.06f) },
		{ TEXT("MotorWheelF"), true, FVector(58.f, 0.f, -76.f), FVector(0.4f, 0.4f, 0.12f) },
		{ TEXT("MotorWheelR"), true, FVector(-52.f, 0.f, -76.f), FVector(0.4f, 0.4f, 0.12f) },
	};
	for (const FPart& Info : Parts)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Info.Name);
		Part->SetupAttachment(RootComponent);
		Part->SetStaticMesh(Info.bWheel ? CylinderMesh.Object : CubeMesh.Object);
		Part->SetRelativeLocation(Info.Location);
		Part->SetRelativeRotation(Info.bWheel ? FRotator(0.f, 0.f, 90.f) : FRotator::ZeroRotator);
		Part->SetRelativeScale3D(Info.Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetHiddenInGame(true);
		MotorParts.Add(Part);
	}

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
	bSprinting = bNewSprinting;
	GetCharacterMovement()->MaxWalkSpeed = bRiding ? RideSpeed * (bSprinting ? 1.3f : 1.f) : (bSprinting ? SprintSpeed : WalkSpeed);
}

void AGRCharacter::SetRiding(bool bNewRiding)
{
	if (bRiding == bNewRiding)
	{
		return;
	}
	bRiding = bNewRiding;
	// Body and cowl in the ojol green; seat, handlebar and wheels dark.
	for (int32 i = 0; i < MotorParts.Num(); ++i)
	{
		MotorParts[i]->SetHiddenInGame(!bRiding);
		if (bRiding)
		{
			GRStoryUtil::ApplyColor(MotorParts[i], i < 2 ? FLinearColor(0.1f, 0.55f, 0.25f) : FLinearColor(0.06f, 0.06f, 0.06f));
		}
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxAcceleration = bRiding ? 1400.f : 2048.f;
	Move->RotationRate = FRotator(0.f, bRiding ? 260.f : 600.f, 0.f);
	SetSprinting(bSprinting);
}
