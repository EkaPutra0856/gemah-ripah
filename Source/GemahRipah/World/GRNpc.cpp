#include "World/GRNpc.h"
#include "Story/GRStorySubsystem.h"
#include "Story/GRStoryUtil.h"
#include "Systems/GRClockSubsystem.h"
#include "UI/GRUISubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

double AGRNpc::LastSpottedTime = -100.0;

namespace
{
	constexpr float VisionRange = 1800.f;
	constexpr float VisionHalfAngle = 38.f;
	constexpr float BarkRange = 700.f;
	constexpr float NameRange = 1500.f;
}

AGRNpc::AGRNpc()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(40.f, 96.f);
	// The player's camera boom should pass through people instead of zooming into them.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	bUseControllerRotationYaw = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 360.f, 0.f);
	Move->MaxWalkSpeed = 160.f;
	Move->bRunPhysicsWithNoController = true;
	AutoPossessAI = EAutoPossessAI::Disabled;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
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

	Hat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hat"));
	Hat->SetupAttachment(RootComponent);
	Hat->SetRelativeLocation(FVector(0.f, 0.f, 92.f));
	Hat->SetRelativeScale3D(FVector(0.32f, 0.32f, 0.1f));
	Hat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hat->SetCastShadow(false);

	NameText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameText"));
	NameText->SetupAttachment(RootComponent);
	NameText->SetRelativeLocation(FVector(0.f, 0.f, 125.f));
	NameText->SetHorizontalAlignment(EHTA_Center);
	NameText->SetWorldSize(18.f);
	NameText->SetTextRenderColor(FColor(240, 220, 190));

	BarkText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BarkText"));
	BarkText->SetupAttachment(RootComponent);
	BarkText->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	BarkText->SetHorizontalAlignment(EHTA_Center);
	BarkText->SetWorldSize(16.f);
	BarkText->SetTextRenderColor(FColor::White);
	BarkText->SetHiddenInGame(true);

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(RootComponent);
	Flashlight->SetRelativeLocationAndRotation(FVector(20.f, 0.f, 40.f), FRotator(-12.f, 0.f, 0.f));
	Flashlight->SetIntensityUnits(ELightUnits::Candelas);
	Flashlight->SetIntensity(1200.f);
	Flashlight->SetAttenuationRadius(VisionRange);
	Flashlight->SetOuterConeAngle(VisionHalfAngle);
	Flashlight->SetInnerConeAngle(VisionHalfAngle - 8.f);
	Flashlight->SetLightColor(FLinearColor(1.f, 0.95f, 0.8f));
	Flashlight->SetCastShadows(false);
	Flashlight->SetVisibility(false);
}

void AGRNpc::Setup(const FGRSpawnDef& Def)
{
	SpawnId = Def.Id;
	Prompt = Def.Prompt;
	Actions = Def.Do;
	DialogId = Def.Dialog;
	Barks = Def.Barks;
	WanderRadius = Def.Wander;
	Home = GetActorLocation();
	HomeYaw = Def.Yaw;
	bSpn = Def.Kind == TEXT("spn");
	for (int32 i = 0; i + 1 < Def.Patrol.Num(); i += 2)
	{
		Patrol.Add(FVector(Def.Patrol[i], Def.Patrol[i + 1], Home.Z));
	}

	NameText->SetText(FText::FromString(Def.Name));
	Hat->SetStaticMesh(GRStoryUtil::LoadShape(bSpn ? TEXT("cube") : TEXT("cylinder")));
	GRStoryUtil::ApplyColor(Hat, GRStoryUtil::ParseColor(Def.Color));
	if (bSpn)
	{
		Hat->SetRelativeScale3D(FVector(0.34f, 0.34f, 0.22f));
		Flashlight->SetVisibility(true);
		GetCharacterMovement()->MaxWalkSpeed = 230.f;
	}
	BarkCooldown = FMath::FRandRange(2.f, 8.f);
}

void AGRNpc::SetActive(bool bActive)
{
	// Hidden NPCs have no collision: freeze movement too, or they fall through the ground into the void.
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (bActive && IsHidden())
	{
		SetActorLocation(Home, false, nullptr, ETeleportType::ResetPhysics);
		Move->Velocity = FVector::ZeroVector;
		bHasTarget = false;
		bChasing = false;
	}
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive);
	SetActorTickEnabled(bActive);
	Move->SetComponentTickEnabled(bActive);
	if (bActive)
	{
		Move->SetMovementMode(MOVE_Walking);
	}
	else
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	if (Flashlight)
	{
		Flashlight->SetVisibility(bActive && bSpn);
	}
}

FText AGRNpc::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	if (!Prompt.IsEmpty())
	{
		return FText::FromString(Prompt);
	}
	return FText::Format(LOCTEXT("TalkTo", "Bicara dengan {0}"), NameText->Text);
}

bool AGRNpc::CanInteract_Implementation(AActor* Interactor) const
{
	return !IsHidden() && (!Actions.IsEmpty() || !DialogId.IsEmpty());
}

void AGRNpc::Interact_Implementation(AActor* Interactor)
{
	if (Interactor)
	{
		const FVector ToPlayer = Interactor->GetActorLocation() - GetActorLocation();
		SetActorRotation(FRotator(0.f, ToPlayer.Rotation().Yaw, 0.f));
	}
	TalkPause = 6.f;
	if (UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>())
	{
		Story->Run(Actions.IsEmpty() ? TEXT("dialog ") + DialogId : Actions);
	}
}

void AGRNpc::Say(const FString& Text, float Duration)
{
	BarkText->SetText(FText::FromString(Text));
	BarkText->SetHiddenInGame(false);
	BarkHideTime = Duration;
}

void AGRNpc::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);

	TickMovement(DeltaSeconds, Player);
	if (bSpn)
	{
		TickVision(DeltaSeconds, Player);
		// Flashlights only after dark.
		const UGRClockSubsystem* Clock = GetGameInstance()->GetSubsystem<UGRClockSubsystem>();
		Flashlight->SetVisibility(Clock && Clock->IsNight());
	}
	TickBarks(DeltaSeconds, Player);

	if (Camera)
	{
		const FVector CamLoc = Camera->GetCameraLocation();
		const float Dist = FVector::Dist(CamLoc, GetActorLocation());
		const FRotator Face(0.f, (CamLoc - GetActorLocation()).Rotation().Yaw, 0.f);
		NameText->SetWorldRotation(Face);
		BarkText->SetWorldRotation(Face);
		NameText->SetHiddenInGame(Dist > NameRange);
	}
}

void AGRNpc::PickWanderTarget()
{
	const FVector2D Offset = FMath::RandPointInCircle(WanderRadius);
	MoveTarget = Home + FVector(Offset, 0.f);
	bHasTarget = true;
}

void AGRNpc::TickMovement(float DeltaSeconds, const APawn* Player)
{
	if (TalkPause > 0.f)
	{
		// Keep facing the player for the whole conversation, not just the first seconds.
		const ULocalPlayer* LP = GetWorld()->GetFirstLocalPlayerFromController();
		const UGRUISubsystem* UI = LP ? LP->GetSubsystem<UGRUISubsystem>() : nullptr;
		if (!UI || !UI->HasScreens())
		{
			TalkPause -= DeltaSeconds;
		}
		if (Player)
		{
			const FRotator Want(0.f, (Player->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0.f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), Want, DeltaSeconds, 8.f));
		}
		return;
	}
	if (bChasing && Player)
	{
		MoveTarget = Player->GetActorLocation();
		bHasTarget = true;
	}
	else if (!bHasTarget)
	{
		WaitTime -= DeltaSeconds;
		if (WaitTime > 0.f)
		{
			return;
		}
		if (Patrol.Num() > 0)
		{
			MoveTarget = Patrol[PatrolIndex];
			PatrolIndex = (PatrolIndex + 1) % Patrol.Num();
			bHasTarget = true;
		}
		else if (WanderRadius > 0.f)
		{
			PickWanderTarget();
		}
		else
		{
			// Stand still facing the spawn direction.
			const FRotator Want(0.f, HomeYaw, 0.f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), Want, DeltaSeconds, 3.f));
			return;
		}
	}

	FVector To = MoveTarget - GetActorLocation();
	To.Z = 0.f;
	if (To.Size() < (bChasing ? 150.f : 120.f))
	{
		if (bChasing)
		{
			SetActorRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
			return;
		}
		bHasTarget = false;
		WaitTime = Patrol.Num() > 0 ? 2.f : FMath::FRandRange(2.f, 6.f);
		StuckTime = 0.f;
		return;
	}
	AddMovementInput(To.GetSafeNormal(), 1.f);
	if (GetVelocity().Size2D() < 20.f)
	{
		StuckTime += DeltaSeconds;
		if (StuckTime > 1.5f)
		{
			// Blocked by a wall: give up on this target.
			StuckTime = 0.f;
			bHasTarget = false;
			WaitTime = 1.f;
			if (Patrol.Num() == 0 && WanderRadius <= 0.f)
			{
				SetActorLocation(Home);
			}
		}
	}
	else
	{
		StuckTime = 0.f;
	}
}

void AGRNpc::TickVision(float DeltaSeconds, APawn* Player)
{
	const UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>();
	bool bSees = false;
	if (Player && !Player->IsHidden())
	{
		const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 60.f);
		const FVector To = Player->GetActorLocation() - Eye;
		const float Dist = To.Size();
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(GetActorForwardVector(), To.GetSafeNormal2D())));
		// Close by, a guard notices you even without looking.
		if (Dist < VisionRange && (Angle < VisionHalfAngle || Dist < 350.f || (bChasing && Dist < VisionRange * 0.6f)))
		{
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(GRNpcVision), false, this);
			bSees = !GetWorld()->LineTraceSingleByChannel(Hit, Eye, Player->GetActorLocation(), ECC_Visibility, Params) || Hit.GetActor() == Player;
		}
	}

	const bool bSuspicious = bSees && Story && Story->Check(Story->GetData().Suspicious);
	if (bSuspicious)
	{
		LastSpottedTime = GetWorld()->GetTimeSeconds();
		UGRStatusSubsystem* Status = GetGameInstance()->GetSubsystem<UGRStatusSubsystem>();
		const float Dist = FVector::Dist2D(Player->GetActorLocation(), GetActorLocation());
		Status->AddDiawasi((Dist < 160.f ? 60.f : 14.f) * DeltaSeconds, true);
		if (!bChasing)
		{
			Say(TEXT("Hei! Berhenti!"), 2.5f);
		}
		bChasing = true;
		GetCharacterMovement()->MaxWalkSpeed = 520.f;
	}
	else if (bChasing && !bSees)
	{
		bChasing = false;
		bHasTarget = false;
		GetCharacterMovement()->MaxWalkSpeed = 230.f;
		Say(TEXT("Ke mana dia..."), 2.5f);
	}
}

void AGRNpc::TickBarks(float DeltaSeconds, const APawn* Player)
{
	if (BarkHideTime > 0.f)
	{
		BarkHideTime -= DeltaSeconds;
		if (BarkHideTime <= 0.f)
		{
			BarkText->SetHiddenInGame(true);
		}
	}
	BarkCooldown -= DeltaSeconds;
	if (BarkCooldown > 0.f || !Player || FVector::Dist(Player->GetActorLocation(), GetActorLocation()) > BarkRange)
	{
		return;
	}
	BarkCooldown = FMath::FRandRange(9.f, 16.f);

	const UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>();
	TArray<FString> Options = Barks;
	if (Story && !bSpn)
	{
		for (const FGRBarkDef& Bark : Story->GetData().Barks)
		{
			if (Story->Check(Bark.If))
			{
				Options.Add(Bark.Text);
			}
		}
	}
	if (Options.Num() > 0)
	{
		Say(Options[FMath::RandRange(0, Options.Num() - 1)]);
	}
}

#undef LOCTEXT_NAMESPACE
