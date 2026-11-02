#include "World/GRMapMarkerComponent.h"
#include "World/GRMapSubsystem.h"
#include "Engine/World.h"

void UGRMapMarkerComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UGRMapSubsystem* Map = GetWorld()->GetSubsystem<UGRMapSubsystem>())
	{
		Map->RegisterMarker(this);
	}
}

void UGRMapMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGRMapSubsystem* Map = GetWorld() ? GetWorld()->GetSubsystem<UGRMapSubsystem>() : nullptr)
	{
		Map->UnregisterMarker(this);
	}
	Super::EndPlay(EndPlayReason);
}

AGRMapMarker::AGRMapMarker()
{
	Marker = CreateDefaultSubobject<UGRMapMarkerComponent>(TEXT("Marker"));
	RootComponent = Marker;
}
