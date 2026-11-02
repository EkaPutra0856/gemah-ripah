#include "World/GRDistrictVolume.h"
#include "World/GRMapSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

AGRDistrictVolume::AGRDistrictVolume()
{
	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	Area->SetBoxExtent(FVector(5000.f, 5000.f, 2000.f));
	Area->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Area->SetCollisionResponseToAllChannels(ECR_Ignore);
	Area->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Area->SetGenerateOverlapEvents(true);
	Area->ShapeColor = FColor(232, 163, 58);
	RootComponent = Area;
}

FBox2D AGRDistrictVolume::GetArea2D() const
{
	const FVector Center = Area->GetComponentLocation();
	const FVector Extent = Area->GetScaledBoxExtent();
	return FBox2D(FVector2D(Center.X - Extent.X, Center.Y - Extent.Y), FVector2D(Center.X + Extent.X, Center.Y + Extent.Y));
}

void AGRDistrictVolume::BeginPlay()
{
	Super::BeginPlay();
	Area->OnComponentBeginOverlap.AddDynamic(this, &AGRDistrictVolume::HandleBeginOverlap);
	Area->OnComponentEndOverlap.AddDynamic(this, &AGRDistrictVolume::HandleEndOverlap);
	if (UGRMapSubsystem* Map = GetWorld()->GetSubsystem<UGRMapSubsystem>())
	{
		Map->RegisterDistrict(this);
	}
}

void AGRDistrictVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGRMapSubsystem* Map = GetWorld() ? GetWorld()->GetSubsystem<UGRMapSubsystem>() : nullptr)
	{
		Map->UnregisterDistrict(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool AGRDistrictVolume::IsLocalPlayerRoot(const AActor* Actor, const UPrimitiveComponent* Comp)
{
	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && Pawn->IsPlayerControlled() && Pawn->IsLocallyControlled() && Comp == Pawn->GetRootComponent();
}

void AGRDistrictVolume::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsLocalPlayerRoot(OtherActor, OtherComp))
	{
		GetWorld()->GetSubsystem<UGRMapSubsystem>()->EnterDistrict(this);
	}
}

void AGRDistrictVolume::HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsLocalPlayerRoot(OtherActor, OtherComp))
	{
		GetWorld()->GetSubsystem<UGRMapSubsystem>()->LeaveDistrict(this);
	}
}
