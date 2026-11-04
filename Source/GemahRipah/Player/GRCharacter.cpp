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
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/World.h"
#include "Story/GRStorySubsystem.h"
#include "World/GRMotor.h"

namespace
{
	/** Wheel to wheel on the "motor" model. */
	constexpr float WheelBase = 116.f;
	/** About 1 g sideways: faster means wider turns, as on a real scooter. */
	constexpr float GripAccel = 1000.f;
	constexpr float MountRange = 260.f;
}

AGRCharacter::AGRCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
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

	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> RideSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Ride.MM_GR_Ride"));
	RideAnim = RideSeq.Object;

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

void AGRCharacter::BeginPlay()
{
	Super::BeginPlay();
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Motor = GetWorld()->SpawnActor<AGRMotor>(AGRMotor::StaticClass(), FVector(AGRMotor::HomeSpot, 0.f), FRotator::ZeroRotator, Params);
}

void AGRCharacter::MoveFromInput(const FVector2D& Axis)
{
	if (bRiding)
	{
		// Scooter controls, not camera-relative: W throttle, S brake, A/D steer.
		if (!IsMoveInputIgnored())
		{
			RideInput = Axis;
		}
		return;
	}
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
	if (!Axis.IsNearlyZero())
	{
		LastLookTime = GetWorld()->GetTimeSeconds();
	}
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

bool AGRCharacter::CanJumpInternal_Implementation() const
{
	return !bRiding && Super::CanJumpInternal_Implementation();
}

float AGRCharacter::GetMotorDistance() const
{
	return Motor && Motor->IsParked() ? FVector::Dist(Motor->GetActorLocation(), GetActorLocation()) / 100.f : -1.f;
}

bool AGRCharacter::IsNearMotor() const
{
	return Motor && Motor->IsParked() && FVector::Dist2D(Motor->GetActorLocation(), GetActorLocation()) < MountRange
		&& FMath::Abs(Motor->GetActorLocation().Z - GetActorLocation().Z) < 200.f;
}

void AGRCharacter::RequestDismount()
{
	if (FMath::Abs(RideSpeedNow) < 40.f)
	{
		SetRiding(false);
	}
	else
	{
		bWantDismount = true;
	}
}

void AGRCharacter::SetRiding(bool bNewRiding)
{
	if (bRiding == bNewRiding || !Motor)
	{
		return;
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const bool bAtMotor = bNewRiding && IsNearMotor();
	bRiding = bNewRiding;
	bWantDismount = false;
	RideInput = FVector2D::ZeroVector;
	RideSpeedNow = 0.f;
	SteerAngle = 0.f;
	LeanAngle = 0.f;
	StoppedBrakeTime = 0.f;
	Move->StopMovementImmediately();
	if (bRiding)
	{
		// Sit down on the scooter where it stands, facing its way. Story and cheat hand-overs bring it to Bayu instead.
		if (bAtMotor)
		{
			Motor->SetActorEnableCollision(false);
			TeleportTo(Motor->GetActorLocation() + FVector(0.f, 0.f, HalfHeight + 2.f), FRotator(0.f, Motor->GetActorRotation().Yaw, 0.f));
		}
		Motor->AttachToRider(this);
		Move->bOrientRotationToMovement = false;
		Move->MaxAcceleration = 20000.f;
		if (Anim && RideAnim)
		{
			Anim->PlaySlotAnimationAsDynamicMontage(RideAnim, TEXT("DefaultSlot"), 0.3f, 0.3f, 1.f, 10000);
		}
	}
	else
	{
		Motor->Park(GetActorLocation() - FVector(0.f, 0.f, HalfHeight), GetActorRotation().Yaw);
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		Move->bOrientRotationToMovement = true;
		Move->MaxAcceleration = 2048.f;
		if (Anim)
		{
			Anim->StopSlotAnimation(0.3f, TEXT("DefaultSlot"));
		}
		// Step off on the side-stand side; the right side or behind when something is in the way.
		const FVector Here = GetActorLocation();
		const FVector Right = GetActorRightVector();
		const FVector Back = -GetActorForwardVector();
		for (const FVector& Offset : { -Right * 85.f + Back * 20.f, Right * 85.f + Back * 20.f, Back * 140.f })
		{
			if (TeleportTo(Here + Offset, GetActorRotation()))
			{
				break;
			}
		}
	}
	SetSprinting(bSprinting);
}

void AGRCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bRiding)
	{
		TickRide(DeltaSeconds);
	}
	// A touch wider view at speed.
	const float SpeedRatio = bRiding ? FMath::Clamp(FMath::Abs(RideSpeedNow) / RideSpeed, 0.f, 1.3f) : 0.f;
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, 90.f + 10.f * SpeedRatio, DeltaSeconds, 2.f));
}

void AGRCharacter::TickRide(float DeltaSeconds)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (const UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>(); Story && Story->GetVar(TEXT("punya_motor")) == 0)
	{
		// A save from before Bayu had the scooter was loaded.
		SetRiding(false);
		return;
	}
	const FVector Forward = GetActorForwardVector();
	// Walls and kerbs take speed off: the capsule covered less ground than the scooter wanted.
	const float Actual = FVector::DotProduct(Move->Velocity, Forward);
	if (Move->IsMovingOnGround())
	{
		if (RideSpeedNow > 0.f && Actual < RideSpeedNow * 0.8f - 30.f)
		{
			RideSpeedNow = FMath::Max(Actual, 0.f);
		}
		else if (RideSpeedNow < 0.f && Actual > RideSpeedNow * 0.8f + 30.f)
		{
			RideSpeedNow = FMath::Min(Actual, 0.f);
		}
	}

	const float Throttle = FMath::Clamp(RideInput.Y, -1.f, 1.f);
	const float SteerInput = FMath::Clamp(RideInput.X, -1.f, 1.f);
	RideInput = FVector2D::ZeroVector;
	const float TopSpeed = RideSpeed * (bSprinting ? 1.3f : 1.f);
	if (bWantDismount)
	{
		RideSpeedNow = FMath::FInterpConstantTo(RideSpeedNow, 0.f, DeltaSeconds, 1400.f);
		if (FMath::Abs(RideSpeedNow) < 40.f)
		{
			SetRiding(false);
			return;
		}
	}
	else if (Throttle > 0.1f)
	{
		StoppedBrakeTime = 0.f;
		if (RideSpeedNow < 0.f)
		{
			RideSpeedNow = FMath::Min(RideSpeedNow + 900.f * DeltaSeconds, 0.f);
		}
		else if (RideSpeedNow > TopSpeed)
		{
			// Shift let go: ease back down to cruising speed.
			RideSpeedNow = FMath::Max(RideSpeedNow - 300.f * DeltaSeconds, TopSpeed);
		}
		else
		{
			// Strong pull away, fading out toward top speed (0 to 50 km/h in about 3 s).
			const float Ratio = RideSpeedNow / TopSpeed;
			RideSpeedNow = FMath::Min(RideSpeedNow + 760.f * Throttle * (1.06f - Ratio * Ratio) * DeltaSeconds, TopSpeed);
		}
	}
	else if (Throttle < -0.1f)
	{
		if (RideSpeedNow > 20.f)
		{
			// Both brakes.
			RideSpeedNow = FMath::Max(RideSpeedNow - 1300.f * -Throttle * DeltaSeconds, 0.f);
			StoppedBrakeTime = 0.f;
		}
		else if ((StoppedBrakeTime += DeltaSeconds) > 0.35f)
		{
			// Stopped and still holding S: walk the scooter backwards.
			RideSpeedNow = FMath::Max(RideSpeedNow - 220.f * DeltaSeconds, -RideReverseSpeed);
		}
		else
		{
			RideSpeedNow = FMath::FInterpConstantTo(RideSpeedNow, 0.f, DeltaSeconds, 1300.f);
		}
	}
	else
	{
		// Rolling resistance, air drag and the engine braking of a CVT scooter.
		StoppedBrakeTime = 0.f;
		RideSpeedNow = FMath::FInterpConstantTo(RideSpeedNow, 0.f, DeltaSeconds, 140.f + 0.00012f * RideSpeedNow * RideSpeedNow);
	}

	// Steering: lock-to-lock at walking pace, a few degrees at speed, and never more grip than the tyres have.
	const float SpeedAbs = FMath::Abs(RideSpeedNow);
	const float MaxSteer = FMath::Lerp(32.f, 5.f, FMath::Sqrt(FMath::Clamp(SpeedAbs / RideSpeed, 0.f, 1.f)));
	SteerAngle = FMath::FInterpTo(SteerAngle, SteerInput * MaxSteer, DeltaSeconds, 7.f);
	float YawRate = RideSpeedNow / WheelBase * FMath::Tan(FMath::DegreesToRadians(SteerAngle));
	const float GripLimit = GripAccel / FMath::Max(SpeedAbs, 1.f);
	YawRate = Move->IsMovingOnGround() ? FMath::Clamp(YawRate, -GripLimit, GripLimit) : 0.f;
	AddActorWorldRotation(FRotator(0.f, FMath::RadiansToDegrees(YawRate) * DeltaSeconds, 0.f));

	// Lean into the turn as far as the cornering force asks (tan lean = v * yaw rate / g).
	const float LeanTarget = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan(RideSpeedNow * YawRate / 980.f)), -40.f, 40.f);
	LeanAngle = FMath::FInterpTo(LeanAngle, LeanTarget, DeltaSeconds, 5.f);

	// The capsule only covers the seat; sweep the nose (or the tail when backing up) so it stops at walls and people.
	const FVector Heading = GetActorForwardVector();
	if (SpeedAbs > 1.f)
	{
		const float Dir = FMath::Sign(RideSpeedNow);
		const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f - GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) + Heading * Dir * 20.f;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GRRideBumper), false, this);
		FCollisionResponseParams Response;
		GetCapsuleComponent()->InitSweepCollisionParams(Params, Response);
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(Motor);
		FHitResult Hit;
		// The bumper sphere starts 42 cm behind where it sits at the nose (62 cm out, reaching the tyre at 87).
		if (GetWorld()->SweepSingleByChannel(Hit, Start, Start + Heading * Dir * (42.f + SpeedAbs * DeltaSeconds), FQuat::Identity,
				GetCapsuleComponent()->GetCollisionObjectType(), FCollisionShape::MakeSphere(25.f), Params, Response)
			&& Hit.ImpactNormal.Z < 0.7f)
		{
			RideSpeedNow = Dir * FMath::Min(SpeedAbs, FMath::Max(Hit.Distance - 42.f, 0.f) / DeltaSeconds);
		}
	}

	Move->MaxWalkSpeed = FMath::Max(FMath::Abs(RideSpeedNow), 10.f);
	Move->Velocity = Heading * RideSpeedNow + FVector(0.f, 0.f, Move->Velocity.Z);
	if (FMath::Abs(RideSpeedNow) > 5.f)
	{
		AddMovementInput(Heading * FMath::Sign(RideSpeedNow));
	}

	// Rider and scooter lean together about the tyres; the bars turn only a little so the hands stay on the grips.
	GetMesh()->SetRelativeRotation(FQuat(FRotator(0.f, 0.f, LeanAngle)) * FQuat(FRotator(0.f, -90.f, 0.f)));
	Motor->SetRidePose(LeanAngle, FMath::Clamp(SteerAngle, -12.f, 12.f), RideSpeedNow * DeltaSeconds);

	// The camera swings in behind the scooter when the mouse has been left alone for a moment.
	AController* C = GetController();
	if (C && !bTopDownMode && SpeedAbs > 150.f && GetWorld()->GetTimeSeconds() - LastLookTime > 1.2f)
	{
		const FRotator Current = C->GetControlRotation();
		const FRotator Behind(-12.f, GetActorRotation().Yaw, 0.f);
		C->SetControlRotation(FMath::RInterpTo(FRotator(FRotator::NormalizeAxis(Current.Pitch), Current.Yaw, 0.f), Behind, DeltaSeconds, 2.5f));
	}
}
