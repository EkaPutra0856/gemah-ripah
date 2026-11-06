#include "World/GRMotor.h"
#include "Core/GRPlayerController.h"
#include "Story/GRStorySubsystem.h"
#include "Story/GRStoryUtil.h"
#include "World/GRMapMarkerComponent.h"
#include "World/GRMapSubsystem.h"
#include "World/GRPropModels.h"
#include "World/GRSfx.h"
#include "World/GRVfx.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

const FVector2D AGRMotor::HomeSpot(-7700.f, -8400.f);

namespace
{
	// Model spots (GRPropModels "motor"): steering head on the fork, wheel centres.
	const FTransform SteerHead(FRotator(20.f, 0.f, 0.f), FVector(52.f, 0.f, 62.f));
	const FVector FrontAxle(58.f, 0.f, 30.f);
	const FVector RearAxle(-58.f, 0.f, 30.f);
	constexpr float WheelRadius = 30.f;
	// On the side stand: leaning left, bars turned left.
	constexpr float StandLean = -7.f;
	constexpr float StandSteer = -18.f;
	/** Lying on its side after a crash (a little short of flat, so the side panels stay above the road). */
	constexpr float FallenLean = 78.f;
}

AGRMotor::AGRMotor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Root->SetMobility(EComponentMobility::Movable);

	Blocker = CreateDefaultSubobject<UBoxComponent>(TEXT("Blocker"));
	Blocker->SetupAttachment(Root);
	Blocker->SetBoxExtent(FVector(86.f, 22.f, 42.f));
	Blocker->SetRelativeLocation(FVector(0.f, 0.f, 44.f));
	Blocker->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Blocker->CanCharacterStepUpOn = ECB_No;
	Blocker->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Blocker->SetCanEverAffectNavigation(false);

	LeanPivot = CreateDefaultSubobject<USceneComponent>(TEXT("LeanPivot"));
	LeanPivot->SetupAttachment(Root);
	SteerPivot = CreateDefaultSubobject<USceneComponent>(TEXT("SteerPivot"));
	SteerPivot->SetupAttachment(LeanPivot);
	SteerPivot->SetRelativeTransform(SteerHead);
	SteerTurn = CreateDefaultSubobject<USceneComponent>(TEXT("SteerTurn"));
	SteerTurn->SetupAttachment(SteerPivot);
	FrontHub = CreateDefaultSubobject<USceneComponent>(TEXT("FrontHub"));
	FrontHub->SetupAttachment(SteerTurn);
	FrontHub->SetRelativeTransform(FTransform(FrontAxle).GetRelativeTransform(SteerHead));
	FrontSpin = CreateDefaultSubobject<USceneComponent>(TEXT("FrontSpin"));
	FrontSpin->SetupAttachment(FrontHub);
	RearSpin = CreateDefaultSubobject<USceneComponent>(TEXT("RearSpin"));
	RearSpin->SetupAttachment(LeanPivot);
	RearSpin->SetRelativeLocation(RearAxle);

	Marker = CreateDefaultSubobject<UGRMapMarkerComponent>(TEXT("Marker"));
	Marker->SetupAttachment(Root);
	Marker->Label = FText::FromString(TEXT("Motor"));
	Marker->Glyph = TEXT("M");
	Marker->Color = FLinearColor(0.2f, 0.75f, 0.45f);

	SetActorHiddenInGame(true);
}

void AGRMotor::BeginPlay()
{
	Super::BeginPlay();
	// The marker registered itself in BeginPlay; SyncFromStory takes it off the map until Bayu owns the scooter.
	bOnMap = true;
	BuildModel();
	SetRidePose(StandLean, StandSteer, 0.f);
	SyncFromStory();
}

void AGRMotor::BuildModel()
{
	TArray<FGRPropPart> Parts;
	GRPropModels::Build(TEXT("motor"), {}, TEXT("#2E8B57"), 0, Parts);
	TMap<FColor, UMaterialInstanceDynamic*> Materials;
	for (const FGRPropPart& Part : Parts)
	{
		// Each moving group hangs under its own pivot; part transforms are in model space, so make them relative.
		USceneComponent* Parent = LeanPivot;
		FTransform Frame = FTransform::Identity;
		switch (Part.Group)
		{
		case 1: Parent = SteerTurn; Frame = SteerHead; break;
		case 2: Parent = FrontSpin; Frame = FTransform(FrontAxle); break;
		case 3: Parent = RearSpin; Frame = FTransform(RearAxle); break;
		default: break;
		}
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetupAttachment(Parent);
		Comp->SetStaticMesh(GRStoryUtil::LoadShape(Part.Shape));
		Comp->SetRelativeTransform(FTransform(Part.Rot, Part.Loc, Part.Size / 100.f).GetRelativeTransform(Frame));
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCanEverAffectNavigation(false);
		UMaterialInstanceDynamic*& Material = Materials.FindOrAdd(Part.Color.ToFColor(true));
		if (!Material)
		{
			Material = GRStoryUtil::MakeColorMaterial(this, Part.Color, 0.9f);
		}
		Comp->SetMaterial(0, Material);
		Comp->RegisterComponent();
		if (Part.Group == 4)
		{
			StandParts.Add(Comp);
		}
	}
}

void AGRMotor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCrashed)
	{
		// Toppling over faster and faster, then scraping along on its side.
		const float Fall = CrashLean >= 0.f ? FallenLean : -FallenLean;
		CrashFallRate += 320.f * DeltaSeconds;
		CrashLean = FMath::FInterpConstantTo(CrashLean, Fall, DeltaSeconds, CrashFallRate);
		const float Speed = CrashVelocity.Size();
		const float Slowed = FMath::Max(Speed - 700.f * DeltaSeconds, 0.f);
		CrashVelocity = Speed > 0.f ? CrashVelocity * (Slowed / Speed) : FVector::ZeroVector;
		const FVector Spot = GetActorLocation() + CrashVelocity * DeltaSeconds;
		SetActorLocation(FVector(Spot.X, Spot.Y, GroundBelow(Spot + FVector(0.f, 0.f, 100.f)).Z));
		SetRidePose(CrashLean, 0.f, Slowed * DeltaSeconds);
		return;
	}
	SyncFromStory();
}

void AGRMotor::Crash(const FVector& Velocity, float Lean)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	bRidden = false;
	bCrashed = true;
	CrashVelocity = FVector(Velocity.X, Velocity.Y, 0.f);
	// Leaning either way, it falls that way; upright, onto the left (stand) side.
	CrashLean = FMath::Abs(Lean) > 1.f ? Lean : -1.f;
	CrashFallRate = 0.f;
	SetActorEnableCollision(false);
	SetActorTickInterval(0.f);
	StopEngine();
	if (UGRVfxSubsystem* Vfx = UGRVfxSubsystem::Get(this))
	{
		Vfx->Sparks(GetActorLocation() + FVector(0.f, 0.f, 20.f), GetActorForwardVector(), 14);
	}
}

void AGRMotor::TickEngine(float Speed, float TopSpeed, float DeltaSeconds)
{
	const float Ratio = FMath::Clamp(FMath::Abs(Speed) / TopSpeed, 0.f, 1.2f);
	if (!Engine)
	{
		Engine = GRSfx::MakeLoop(Root, EGRSfx::MesinMotor, 3000.f);
	}
	if (Engine)
	{
		if (!Engine->IsPlaying())
		{
			Engine->Play();
		}
		Engine->SetPitchMultiplier(0.72f + 0.95f * Ratio);
		Engine->SetVolumeMultiplier((0.5f + 0.4f * Ratio) * GRSfx::Setting());
	}
	ExhaustWait -= DeltaSeconds;
	if (ExhaustWait <= 0.f)
	{
		ExhaustWait = FMath::Lerp(0.2f, 0.1f, FMath::Min(Ratio, 1.f));
		if (UGRVfxSubsystem* Vfx = UGRVfxSubsystem::Get(this))
		{
			Vfx->Exhaust(GetActorTransform().TransformPosition(FVector(-88.f, 12.f, 32.f)), -GetActorForwardVector(), 0.9f);
		}
	}
}

void AGRMotor::StopEngine()
{
	if (Engine)
	{
		Engine->FadeOut(0.25f, 0.f);
	}
}

void AGRMotor::FinishCrash()
{
	if (!bCrashed)
	{
		return;
	}
	bCrashed = false;
	SetActorTickInterval(0.25f);
	Park(GroundBelow(GetActorLocation() + FVector(0.f, 0.f, 100.f)), GetActorRotation().Yaw);
}

void AGRMotor::SyncFromStory()
{
	UGRStorySubsystem* Story = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGRStorySubsystem>() : nullptr;
	if (!Story || bRidden || bCrashed)
	{
		return;
	}
	const bool bOwned = Story->GetVar(TEXT("punya_motor")) != 0;
	SetActorHiddenInGame(!bOwned);
	SetActorEnableCollision(bOwned);
	SetOnMap(bOwned);
	if (!bOwned)
	{
		return;
	}
	const int32 Mode = Story->GetVar(TEXT("motor_parkir"));
	if (Mode == 2)
	{
		// Saved while riding: after loading, Bayu is on foot, so the scooter waits right beside him.
		if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			const ACharacter* Character = Cast<ACharacter>(Player);
			const float HalfHeight = Character ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.f;
			Park(Player->GetActorLocation() - FVector(0.f, 0.f, HalfHeight) + Player->GetActorRightVector() * 120.f, Player->GetActorRotation().Yaw);
		}
		return;
	}
	const FVector Spot = Mode == 1
		? FVector(Story->GetVar(TEXT("motor_x")), Story->GetVar(TEXT("motor_y")), Story->GetVar(TEXT("motor_z")))
		: GroundBelow(FVector(HomeSpot, 3000.f));
	const float Yaw = Mode == 1 ? Story->GetVar(TEXT("motor_yaw")) : 0.f;
	if (!GetActorLocation().Equals(Spot, 5.f) || !FMath::IsNearlyEqual(FRotator::NormalizeAxis(GetActorRotation().Yaw - Yaw), 0.f, 1.f))
	{
		SetActorLocationAndRotation(Spot, FRotator(0.f, Yaw, 0.f));
	}
}

FVector AGRMotor::GroundBelow(const FVector& Spot) const
{
	FHitResult Hit;
	FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Spot, Spot - FVector(0.f, 0.f, 6000.f), Objects))
	{
		return Hit.ImpactPoint;
	}
	return FVector(Spot.X, Spot.Y, 0.f);
}

void AGRMotor::SetOnMap(bool bShow)
{
	if (bOnMap == bShow)
	{
		return;
	}
	bOnMap = bShow;
	if (UGRMapSubsystem* Map = GetWorld()->GetSubsystem<UGRMapSubsystem>())
	{
		if (bShow)
		{
			Map->RegisterMarker(Marker);
		}
		else
		{
			Map->UnregisterMarker(Marker);
		}
	}
}

void AGRMotor::AttachToRider(ACharacter* Rider)
{
	bRidden = true;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(false);
	SetOnMap(false);
	const float HalfHeight = Rider->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	AttachToComponent(Rider->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeLocation(FVector(0.f, 0.f, -HalfHeight));
	SetActorRelativeRotation(FRotator::ZeroRotator);
	for (UStaticMeshComponent* Stand : StandParts)
	{
		Stand->SetVisibility(false);
	}
	SetRidePose(0.f, 0.f, 0.f);
	if (UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>())
	{
		Story->SetVar(TEXT("motor_parkir"), 2);
	}
}

void AGRMotor::Park(const FVector& Ground, float Yaw)
{
	StopEngine();
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	bRidden = false;
	SetActorLocationAndRotation(Ground, FRotator(0.f, Yaw, 0.f));
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetOnMap(true);
	for (UStaticMeshComponent* Stand : StandParts)
	{
		Stand->SetVisibility(true);
	}
	SetRidePose(StandLean, StandSteer, 0.f);
	if (UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>())
	{
		Story->SetVar(TEXT("motor_parkir"), 1);
		Story->SetVar(TEXT("motor_x"), FMath::RoundToInt(Ground.X));
		Story->SetVar(TEXT("motor_y"), FMath::RoundToInt(Ground.Y));
		Story->SetVar(TEXT("motor_z"), FMath::RoundToInt(Ground.Z));
		Story->SetVar(TEXT("motor_yaw"), FMath::RoundToInt(FRotator::NormalizeAxis(Yaw)));
	}
	// Stored as whole cm and degrees; stand exactly there so the next sync does not nudge it.
	SetActorLocationAndRotation(FVector(FMath::RoundToFloat(Ground.X), FMath::RoundToFloat(Ground.Y), FMath::RoundToFloat(Ground.Z)),
		FRotator(0.f, FMath::RoundToFloat(FRotator::NormalizeAxis(Yaw)), 0.f));
}

void AGRMotor::SetRidePose(float Lean, float Steer, float Rolled)
{
	LeanPivot->SetRelativeRotation(FRotator(0.f, 0.f, Lean));
	SteerTurn->SetRelativeRotation(FRotator(0.f, Steer, 0.f));
	// Rolling forward turns the top of the wheel forward, which is negative pitch.
	WheelAngle = FMath::Fmod(WheelAngle + FMath::RadiansToDegrees(Rolled / WheelRadius), 360.f);
	FrontSpin->SetRelativeRotation(FRotator(-WheelAngle, 0.f, 0.f));
	RearSpin->SetRelativeRotation(FRotator(-WheelAngle, 0.f, 0.f));
}

FText AGRMotor::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return FText::FromString(TEXT("Naik motor (F)"));
}

bool AGRMotor::CanInteract_Implementation(AActor* Interactor) const
{
	return IsParked();
}

void AGRMotor::Interact_Implementation(AActor* Interactor)
{
	const APawn* Pawn = Cast<APawn>(Interactor);
	if (AGRPlayerController* PC = Pawn ? Cast<AGRPlayerController>(Pawn->GetController()) : nullptr)
	{
		PC->HandleMotor();
	}
}
