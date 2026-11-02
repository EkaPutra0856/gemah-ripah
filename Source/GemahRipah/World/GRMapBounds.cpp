#include "World/GRMapBounds.h"
#include "World/GRMapSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

AGRMapBounds::AGRMapBounds()
{
	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	Area->SetBoxExtent(FVector(50000.f, 50000.f, 5000.f));
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Area->ShapeColor = FColor(90, 180, 235);
	RootComponent = Area;
}

FBox2D AGRMapBounds::GetArea2D() const
{
	const FVector Center = Area->GetComponentLocation();
	const FVector Extent = Area->GetScaledBoxExtent();
	const double Half = FMath::Max(Extent.X, Extent.Y);
	return FBox2D(FVector2D(Center.X - Half, Center.Y - Half), FVector2D(Center.X + Half, Center.Y + Half));
}

void AGRMapBounds::BeginPlay()
{
	Super::BeginPlay();
	if (UGRMapSubsystem* Map = GetWorld()->GetSubsystem<UGRMapSubsystem>())
	{
		Map->SetMapBounds(this);
	}
}
