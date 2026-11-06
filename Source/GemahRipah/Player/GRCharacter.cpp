#include "Player/GRCharacter.h"
#include "Core/GRGameUserSettings.h"
#include "Player/GRInteractionComponent.h"
#include "World/GRBody.h"
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
#include "World/GRCrowd.h"
#include "World/GRKnockAway.h"
#include "World/GRSfx.h"
#include "World/GRVfx.h"
#include "Landscape.h"
#include "World/GRStoryProp.h"
#include "World/GRMotor.h"
#include "World/GRTraffic.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"

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
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> PhoneSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Ponsel.MM_GR_Ponsel"));
	PhoneAnim = PhoneSeq.Object;

	// The mannequin runs the animation; BeginPlay puts Bayu's Tripo model over it (World/GRBody.h).
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
	GRBody::Apply(this, TripoBody, GRBody::Find(TEXT("C01_Bayu")));
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Motor = GetWorld()->SpawnActor<AGRMotor>(AGRMotor::StaticClass(), FVector(AGRMotor::HomeSpot, 0.f), FRotator::ZeroRotator, Params);
}

void AGRCharacter::MoveFromInput(const FVector2D& Axis)
{
	if (bKnockedDown)
	{
		return;
	}
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

void AGRCharacter::SetPhoneView(bool bOn)
{
	if (bPhoneView == bOn)
	{
		return;
	}
	bPhoneView = bOn;
	UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (bOn)
	{
		PhoneSavedArm = CameraBoom->TargetArmLength;
		PhoneSavedSocket = CameraBoom->SocketOffset;
		PhoneSavedRotation = GetControlRotation();
		// Close behind the right shoulder, looking down at the hands where the phone is.
		CameraBoom->TargetArmLength = 150.f;
		CameraBoom->SocketOffset = FVector(0.f, 42.f, 58.f);
		CameraBoom->bEnableCameraLag = false;
		if (Controller)
		{
			Controller->SetControlRotation(FRotator(-34.f, GetActorRotation().Yaw - 12.f, 0.f));
		}
		bPhonePose = Anim && PhoneAnim && !bRiding && !bKnockedDown;
		if (bPhonePose)
		{
			Anim->PlaySlotAnimationAsDynamicMontage(PhoneAnim, TEXT("DefaultSlot"), 0.25f, 0.25f, 1.f, 10000);
		}
		return;
	}
	CameraBoom->TargetArmLength = PhoneSavedArm;
	CameraBoom->SocketOffset = PhoneSavedSocket;
	CameraBoom->bEnableCameraLag = true;
	if (Controller)
	{
		Controller->SetControlRotation(PhoneSavedRotation);
	}
	if (bPhonePose && Anim && !bRiding)
	{
		Anim->StopSlotAnimation(0.25f, TEXT("DefaultSlot"));
	}
	bPhonePose = false;
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
	return !bRiding && !bKnockedDown && Super::CanJumpInternal_Implementation();
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
	ImpactCooldown -= DeltaSeconds;
	if (HitStopEnd > 0.0 && FPlatformTime::Seconds() > HitStopEnd)
	{
		HitStopEnd = 0.0;
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	}
	if (bPendingKnock)
	{
		bPendingKnock = false;
		StartKnockdown(PendingKnock);
	}
	if (bKnockedDown)
	{
		TickKnockdown(DeltaSeconds);
	}
	else if (bRiding)
	{
		TickRide(DeltaSeconds);
		if (bRiding)
		{
			Motor->TickEngine(RideSpeedNow, RideSpeed, DeltaSeconds);
		}
	}
	TickFootsteps(DeltaSeconds);
	if (ShakeTime > 0.f)
	{
		ShakeTime = FMath::Max(ShakeTime - DeltaSeconds, 0.f);
		const float Amp = ShakeAmp * ShakeTime / ShakeLength;
		FollowCamera->SetRelativeLocation(FVector(0.f, FMath::FRandRange(-Amp, Amp), FMath::FRandRange(-Amp, Amp)));
	}
	// A touch wider view at speed.
	const float SpeedRatio = bRiding ? FMath::Clamp(FMath::Abs(RideSpeedNow) / RideSpeed, 0.f, 1.3f) : 0.f;
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, 90.f + 10.f * SpeedRatio, DeltaSeconds, 2.f));
}

void AGRCharacter::TickFootsteps(float DeltaSeconds)
{
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const float Speed = GetVelocity().Size2D();
	if (bRiding || bKnockedDown || !Move->IsMovingOnGround() || Speed < 60.f)
	{
		StepDistance = 0.f;
		return;
	}
	StepDistance += Speed * DeltaSeconds;
	const float Stride = bSprinting ? 210.f : 170.f;
	if (StepDistance < Stride)
	{
		return;
	}
	StepDistance -= Stride;
	const bool bGrass = Cast<ALandscapeProxy>(Move->CurrentFloor.HitResult.GetActor()) != nullptr;
	// Bayu's own feet: not placed in the world, or the camera boom thins them out under the town sound.
	GRSfx::Play(this, bGrass ? EGRSfx::LangkahRumput : EGRSfx::LangkahBeton, bSprinting ? 0.7f : 0.5f, FMath::FRandRange(0.94f, 1.06f));
}

void AGRCharacter::PlayCrunch(float Closing, const FVector& Direction)
{
	const FVector At = GetActorLocation() + Direction.GetSafeNormal2D() * 70.f;
	GRSfx::PlayAt(this, EGRSfx::TabrakLogam, At, FMath::Clamp(Closing / 750.f, 0.45f, 1.f), FMath::FRandRange(0.88f, 1.05f));
	if (UGRVfxSubsystem* Vfx = UGRVfxSubsystem::Get(this))
	{
		Vfx->Sparks(At, -Direction, Closing >= 450.f ? 16 : 7);
	}
}

void AGRCharacter::HandleCarImpact(float Closing, const FVector& Fling, bool bRammed)
{
	if (bKnockedDown || bPendingKnock || ImpactCooldown > 0.f || Closing < 150.f)
	{
		return;
	}
	ImpactCooldown = 0.6f;
	UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>();
	const FVector Dir = Fling.GetSafeNormal2D();
	PlayCrunch(Closing, Dir);
	if (Closing >= 750.f)
	{
		// About 27 km/h and up: Bayu comes off and tumbles, with a beat of slow motion on the hit.
		ShakeCamera(14.f, 0.6f);
		UGameplayStatics::SetGlobalTimeDilation(this, 0.25f);
		HitStopEnd = FPlatformTime::Seconds() + 0.09;
		bPendingKnock = true;
		PendingKnock = Dir * Closing * 0.55f + FVector(0.f, 0.f, 260.f);
		if (Story)
		{
			Story->AddEnergi(-8.f);
			Story->Run(bRiding ? TEXT("notifburuk Bayu jatuh dari motor! Energi -8.") : TEXT("notifburuk Bayu tertabrak mobil! Energi -8."));
		}
		return;
	}
	const bool bHard = Closing >= 450.f;
	ShakeCamera(bHard ? 9.f : 4.f, bHard ? 0.45f : 0.25f);
	if (bRiding)
	{
		// The scooter rebounds off the car (rammed) or gets knocked off its line and loses speed (rammed by it).
		RideSpeedNow = bRammed ? -RideSpeedNow * 0.2f : RideSpeedNow * 0.3f;
	}
	else
	{
		LaunchCharacter(Dir * Closing * 0.7f + FVector(0.f, 0.f, 200.f), true, true);
	}
	if (bHard && Story)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 0.4f);
		HitStopEnd = FPlatformTime::Seconds() + 0.06;
		Story->AddEnergi(-3.f);
		Story->Run(bRammed ? TEXT("notif Bayu menyerempet mobil. Energi -3.") : TEXT("notif Bayu terserempet mobil. Energi -3."));
	}
}

bool AGRCharacter::HandleWallImpact(float Closing, const FVector& Heading)
{
	if (bKnockedDown || bPendingKnock || ImpactCooldown > 0.f || Closing < WallBounceSpeed)
	{
		return false;
	}
	ImpactCooldown = 0.6f;
	UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>();
	UE_LOG(LogTemp, Display, TEXT("Ride: hit a wall head-on at %.0f (%s)"), Closing, Closing >= WallCrashSpeed ? TEXT("crash") : TEXT("bounce"));
	PlayCrunch(Closing, Heading);
	if (Closing >= WallCrashSpeed)
	{
		// Over the bars: the scooter stops dead and Bayu is thrown up and forward against it, with slow motion.
		ShakeCamera(15.f, 0.6f);
		UGameplayStatics::SetGlobalTimeDilation(this, 0.25f);
		HitStopEnd = FPlatformTime::Seconds() + 0.09;
		bPendingKnock = true;
		PendingKnock = Heading.GetSafeNormal2D() * Closing * 0.25f + FVector(0.f, 0.f, 320.f);
		if (Story)
		{
			Story->AddEnergi(-8.f);
			Story->Run(TEXT("notifburuk Bayu ngebut lalu menabrak! Jatuh dari motor. Energi -8."));
		}
		return true;
	}
	// A knock that throws the scooter back a little; Bayu stays on.
	ShakeCamera(7.f, 0.35f);
	RideSpeedNow = -RideSpeedNow * 0.22f;
	if (Story)
	{
		Story->AddEnergi(-2.f);
		Story->Run(TEXT("notif Motor Bayu membentur. Energi -2."));
	}
	return true;
}

bool AGRCharacter::RideIntoThing(const FHitResult& Hit, const FVector& Ahead, float SpeedAbs)
{
	AActor* Thing = Hit.GetActor();
	UPrimitiveComponent* Part = Hit.GetComponent();
	UGRKnockAway* Knock = GetWorld()->GetSubsystem<UGRKnockAway>();
	if (Knock && Knock->IsFlying(Thing, Part))
	{
		// Already in the air from the last hit: ride on through.
		return true;
	}
	// What gets knocked: a loose prop (chairs, pots, boxes) when it is small, else only the small part of it that was
	// hit (a crate on a stall). Buildings, their facades and everything fixed in place are walls.
	auto IsSmall = [](const FBox& Box)
	{
		const FVector Size = Box.GetSize();
		return Box.IsValid && Size.Z <= 130.f && FMath::Max(Size.X, Size.Y) <= 140.f;
	};
	const bool bLoose = Cast<AGRStoryProp>(Thing) || (Thing && Thing->ActorHasTag(TEXT("GRLepas")));
	USceneComponent* Root = bLoose ? Thing->GetRootComponent() : nullptr;
	USceneComponent* Piece = nullptr;
	FBox Box(ForceInit);
	if (Root && Root->Mobility == EComponentMobility::Movable && IsSmall(Box = Thing->GetComponentsBoundingBox(false)))
	{
		Piece = Root;
	}
	else if (Root && Part && Part != Root && Part->Mobility == EComponentMobility::Movable && Part->GetNumChildrenComponents() == 0
		&& IsSmall(Box = Part->Bounds.GetBox()))
	{
		Piece = Part;
	}
	if (Piece && Knock)
	{
		GRSfx::PlayAt(this, EGRSfx::TabrakBarang, Piece->GetComponentLocation(), FMath::Clamp(SpeedAbs / 800.f, 0.4f, 1.f), FMath::FRandRange(0.9f, 1.1f));
		if (Box.GetSize().Z <= 45.f && SpeedAbs > 200.f)
		{
			// Low enough to go under the front wheel: flattened, with a jolt through the bars.
			Knock->Squash(Piece);
			UE_LOG(LogTemp, Display, TEXT("Ride: ran over %s at %.0f"), *GetNameSafe(Thing), SpeedAbs);
			ShakeCamera(3.f, 0.15f);
			RideSpeedNow *= 0.93f;
		}
		else
		{
			// Sent flying ahead and up, tumbling; the scooter loses a little speed (a lot when it was crawling).
			Knock->Launch(Piece, Ahead * SpeedAbs * 1.1f + FVector(0.f, 0.f, 120.f + SpeedAbs * 0.35f));
			UE_LOG(LogTemp, Display, TEXT("Ride: knocked %s at %.0f"), *GetNameSafe(Thing), SpeedAbs);
			ShakeCamera(SpeedAbs > 450.f ? 5.f : 2.f, 0.2f);
			RideSpeedNow *= SpeedAbs > 450.f ? 0.85f : 0.6f;
		}
		return true;
	}
	// Big or fixed: only the speed straight into it counts, so scraping along a wall is not a crash.
	const float HeadOn = SpeedAbs * FMath::Clamp(-FVector::DotProduct(Ahead, Hit.ImpactNormal.GetSafeNormal2D()), 0.f, 1.f);
	return HandleWallImpact(HeadOn, Ahead);
}

void AGRCharacter::ShakeCamera(float Amplitude, float Duration)
{
	ShakeAmp = Amplitude;
	ShakeLength = Duration;
	ShakeTime = Duration;
}

void AGRCharacter::StartKnockdown(const FVector& Velocity)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	GRSfx::PlayAt(this, EGRSfx::TabrakBadan, GetActorLocation(), 1.f, FMath::FRandRange(0.9f, 1.05f));
	if (UGRVfxSubsystem* Vfx = UGRVfxSubsystem::Get(this))
	{
		Vfx->Dust(Feet, 60.f);
	}
	if (bRiding)
	{
		// The scooter drops where it hit and skids a little way back; Bayu flies on over the bars.
		Motor->Crash(GetActorForwardVector() * RideSpeedNow * -0.12f, LeanAngle);
		bRiding = false;
		bWantDismount = false;
		RideInput = FVector2D::ZeroVector;
		RideSpeedNow = 0.f;
		SteerAngle = 0.f;
		LeanAngle = 0.f;
		Move->bOrientRotationToMovement = true;
		Move->MaxAcceleration = 2048.f;
		if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
		{
			Anim->StopSlotAnimation(0.1f, TEXT("DefaultSlot"));
		}
		SetSprinting(bSprinting);
	}
	bKnockedDown = true;
	bKnockFade = false;
	KnockTime = 0.f;
	Move->StopMovementImmediately();
	Move->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	USkeletalMeshComponent* Body = GetMesh();
	MeshProfile = Body->GetCollisionProfileName();
	Body->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Body->SetCollisionProfileName(TEXT("Ragdoll"));
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Body->SetAllBodiesSimulatePhysics(true);
	Body->SetSimulatePhysics(true);
	Body->WakeAllRigidBodies();
	Body->SetAllPhysicsLinearVelocity(Velocity);
}

void AGRCharacter::TickKnockdown(float DeltaSeconds)
{
	// The capsule, and with it the camera, keeps up with the tumbling body.
	KnockTime += DeltaSeconds;
	SetActorLocation(GetMesh()->GetSocketLocation(TEXT("pelvis")) + FVector(0.f, 0.f, 30.f));
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!bKnockFade && KnockTime > 2.3f)
	{
		bKnockFade = true;
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.35f, FLinearColor::Black, false, true);
		}
	}
	if (KnockTime > 2.75f)
	{
		EndKnockdown();
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.6f, FLinearColor::Black, false, false);
		}
	}
}

void AGRCharacter::EndKnockdown()
{
	if (!bKnockedDown)
	{
		return;
	}
	bKnockedDown = false;
	USkeletalMeshComponent* Body = GetMesh();
	const FVector Pelvis = Body->GetSocketLocation(TEXT("pelvis"));
	Body->SetSimulatePhysics(false);
	Body->SetAllBodiesSimulatePhysics(false);
	Body->SetCollisionProfileName(MeshProfile);
	const float HalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	Body->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Body->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -HalfHeight), FRotator(0.f, -90.f, 0.f));
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	// Up on his feet where he landed; TeleportTo finds room beside a car or a wall.
	FHitResult Hit;
	const bool bGround = GetWorld()->LineTraceSingleByObjectType(Hit, Pelvis + FVector(0.f, 0.f, 100.f), Pelvis - FVector(0.f, 0.f, 600.f),
		FCollisionObjectQueryParams(ECC_WorldStatic));
	const FVector Stand = (bGround ? Hit.ImpactPoint : Pelvis) + FVector(0.f, 0.f, HalfHeight + 5.f);
	if (!TeleportTo(Stand, FRotator(0.f, GetActorRotation().Yaw, 0.f)))
	{
		SetActorLocation(Stand);
	}
	if (Motor)
	{
		Motor->FinishCrash();
	}
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
			bool bRideOn = false;
			if (AGRWarga* Person = Cast<AGRWarga>(Hit.GetActor()))
			{
				// Rode into someone on foot: a hard hit throws them down and the scooter rolls on, much slower; a
				// slow one shoves them and the scooter stops.
				const float Closing = FVector::DotProduct(Heading * RideSpeedNow - Person->GetVelocity(), Heading * Dir);
				const bool bHard = Closing >= 450.f;
				if (Closing > 120.f && Person->KnockedBy(this, Closing, Heading * Dir, bHard ? 6.f : 3.f))
				{
					ShakeCamera(bHard ? 7.f : 3.f, bHard ? 0.35f : 0.2f);
					if (bHard)
					{
						UGameplayStatics::SetGlobalTimeDilation(this, 0.35f);
						HitStopEnd = FPlatformTime::Seconds() + 0.07;
						RideSpeedNow *= 0.45f;
						bRideOn = true;
					}
				}
			}
			else if (AGRTrafficCar* Car = Cast<AGRTrafficCar>(Hit.GetActor()); Car && ImpactCooldown <= 0.f)
			{
				// Rode into a car: it gets shoved, the scooter bounces back or goes down with Bayu.
				const float Closing = FVector::DotProduct(Heading * RideSpeedNow - Car->GetMoveVelocity(), Heading * Dir);
				if (Closing > 150.f)
				{
					Car->HitByRider(Closing, Heading * Dir, Hit.ImpactPoint);
					HandleCarImpact(Closing, Heading * Dir, true);
					if (bPendingKnock)
					{
						return;
					}
				}
			}
			else if (!Cast<APawn>(Hit.GetActor()) && Hit.GetActor() != Motor)
			{
				bRideOn = RideIntoThing(Hit, Heading * Dir, SpeedAbs);
				if (bPendingKnock)
				{
					return;
				}
			}
			if (!bRideOn)
			{
				RideSpeedNow = Dir * FMath::Min(SpeedAbs, FMath::Max(Hit.Distance - 42.f, 0.f) / DeltaSeconds);
			}
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
