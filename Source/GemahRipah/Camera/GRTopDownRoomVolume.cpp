#include "Camera/GRTopDownRoomVolume.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Core/GRPlayerController.h"
#include "GameFramework/Pawn.h"

AGRTopDownRoomVolume::AGRTopDownRoomVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetBoxExtent(FVector(500.f, 500.f, 200.f));
	Trigger->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	RootComponent = Trigger;

	RoomCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("RoomCamera"));
	RoomCamera->SetupAttachment(Trigger);
	RoomCamera->SetRelativeLocation(FVector(-450.f, 0.f, 1100.f));
	RoomCamera->SetRelativeRotation(FRotator(-65.f, 0.f, 0.f));
	RoomCamera->SetFieldOfView(60.f);
}

void AGRTopDownRoomVolume::BeginPlay()
{
	Super::BeginPlay();

	CameraOffsetFromCenter = RoomCamera->GetComponentLocation() - GetActorLocation();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGRTopDownRoomVolume::HandleBeginOverlap);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &AGRTopDownRoomVolume::HandleEndOverlap);
}

void AGRTopDownRoomVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (const APawn* Player = ActivePlayer.Get())
	{
		RoomCamera->SetWorldLocation(Player->GetActorLocation() + CameraOffsetFromCenter);
	}
}

void AGRTopDownRoomVolume::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	// Only the pawn's root (capsule) counts, so extra components do not double-trigger.
	if (!Pawn || OtherComp != Pawn->GetRootComponent())
	{
		return;
	}

	AGRPlayerController* PC = Cast<AGRPlayerController>(Pawn->GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	ActivePlayer = Pawn;
	SetActorTickEnabled(bFollowPlayer);
	PC->EnterTopDownRoom(this);
	OnPlayerEntered(Pawn);
}

void AGRTopDownRoomVolume::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || Pawn != ActivePlayer.Get() || OtherComp != Pawn->GetRootComponent())
	{
		return;
	}

	ActivePlayer.Reset();
	SetActorTickEnabled(false);
	RoomCamera->SetWorldLocation(GetActorLocation() + CameraOffsetFromCenter);

	if (AGRPlayerController* PC = Cast<AGRPlayerController>(Pawn->GetController()))
	{
		PC->ExitTopDownRoom(this);
	}
	OnPlayerExited(Pawn);
}
