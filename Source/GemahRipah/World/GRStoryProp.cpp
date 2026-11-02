#include "World/GRStoryProp.h"
#include "Story/GRStorySubsystem.h"
#include "Story/GRStoryUtil.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

AGRStoryProp::AGRStoryProp()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetMobility(EComponentMobility::Movable);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(RootComponent);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(22.f);
	Label->SetTextRenderColor(FColor(240, 200, 120));
	Label->SetAbsolute(false, false, true);
}

void AGRStoryProp::Setup(const FGRSpawnDef& Def)
{
	SpawnId = Def.Id;
	Prompt = Def.Prompt;
	Actions = Def.Do;
	DialogId = Def.Dialog;
	Radius = Def.Radius;
	bZone = Def.Kind == TEXT("zone") || Def.Shape == TEXT("none");

	if (bZone)
	{
		Mesh->SetStaticMesh(nullptr);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Label->SetHiddenInGame(true);
		return;
	}
	const FVector Size(Def.Size.IsValidIndex(0) ? Def.Size[0] : 80.f, Def.Size.IsValidIndex(1) ? Def.Size[1] : 80.f, Def.Size.IsValidIndex(2) ? Def.Size[2] : 100.f);
	Mesh->SetStaticMesh(GRStoryUtil::LoadShape(Def.Shape));
	Mesh->SetWorldScale3D(Size / 100.f);
	// Basic shapes are centered; lift so the object stands on the ground.
	if (!Def.Pos.IsValidIndex(2))
	{
		AddActorWorldOffset(FVector(0.f, 0.f, Size.Z * 0.5f));
	}
	GRStoryUtil::ApplyColor(Mesh, GRStoryUtil::ParseColor(Def.Color));
	Label->SetText(FText::FromString(Def.Name));
	Label->SetWorldLocation(GetActorLocation() + FVector(0.f, 0.f, Size.Z * 0.5f + 30.f));
	Label->SetWorldRotation(FRotator(0.f, Def.Yaw + 180.f, 0.f));
	Label->SetWorldScale3D(FVector::OneVector);
}

void AGRStoryProp::SetActive(bool bActive)
{
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive && !bZone);
	if (!bActive)
	{
		bPlayerInside = false;
	}
}

FText AGRStoryProp::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return FText::FromString(Prompt.IsEmpty() ? TEXT("Periksa") : Prompt);
}

bool AGRStoryProp::CanInteract_Implementation(AActor* Interactor) const
{
	return !bZone && !IsHidden() && (!Actions.IsEmpty() || !DialogId.IsEmpty());
}

void AGRStoryProp::Interact_Implementation(AActor* Interactor)
{
	if (UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>())
	{
		Story->Run(Actions.IsEmpty() ? TEXT("dialog ") + DialogId : Actions);
	}
}
