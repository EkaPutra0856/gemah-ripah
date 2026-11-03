#include "World/GRStoryProp.h"
#include "Story/GRStorySubsystem.h"
#include "Story/GRStoryUtil.h"
#include "World/GRPropModels.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

AGRStoryProp::AGRStoryProp()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.2f;

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
	TArray<FGRPropPart> Parts;
	if (!Def.Model.IsEmpty() && GRPropModels::Build(Def.Model, Def.Size, Def.Color, GetTypeHash(Def.Id), Parts))
	{
		const float Top = BuildParts(Parts);
		Label->SetText(FText::FromString(Def.Name));
		Label->SetWorldLocation(GetActorLocation() + FVector(0.f, 0.f, Top + 30.f));
		Label->SetWorldRotation(FRotator(0.f, Def.Yaw + 180.f, 0.f));
		Label->SetWorldScale3D(FVector::OneVector);
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

float AGRStoryProp::BuildParts(const TArray<FGRPropPart>& Parts)
{
	Mesh->SetStaticMesh(nullptr);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// One material per color: a desk has a dozen parts but only a few colors.
	TMap<FColor, UMaterialInstanceDynamic*> Materials;
	float Top = 0.f;
	for (const FGRPropPart& Part : Parts)
	{
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetupAttachment(Mesh);
		Comp->SetStaticMesh(GRStoryUtil::LoadShape(Part.Shape));
		Comp->SetRelativeLocationAndRotation(Part.Loc, Part.Rot);
		Comp->SetRelativeScale3D(Part.Size / 100.f);
		Comp->SetCollisionProfileName(Part.bCollide ? TEXT("BlockAllDynamic") : TEXT("NoCollision"));
		Comp->SetCanEverAffectNavigation(false);
		UMaterialInstanceDynamic*& Material = Materials.FindOrAdd(Part.Color.ToFColor(true));
		if (!Material)
		{
			Material = GRStoryUtil::MakeColorMaterial(this, Part.Color, 0.9f);
		}
		Comp->SetMaterial(0, Material);
		Comp->RegisterComponent();
		if (Part.bRoof)
		{
			RoofParts.Add(Comp);
			RoofExtent.X = FMath::Max(RoofExtent.X, FMath::Abs(Part.Loc.X) + Part.Size.X * 0.5f);
			RoofExtent.Y = FMath::Max(RoofExtent.Y, FMath::Abs(Part.Loc.Y) + Part.Size.Y * 0.5f);
		}
		Top = FMath::Max(Top, Part.Loc.Z + Part.Size.Z * 0.5f);
	}
	SetActorTickEnabled(RoofParts.Num() > 0);
	return Top;
}

void AGRStoryProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	const FVector Local = GetActorTransform().InverseTransformPosition(Player->GetActorLocation());
	const bool bUnder = FMath::Abs(Local.X) < RoofExtent.X && FMath::Abs(Local.Y) < RoofExtent.Y;
	for (UStaticMeshComponent* Roof : RoofParts)
	{
		Roof->SetVisibility(!bUnder);
	}
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
