#include "World/GRStoryProp.h"
#include "Story/GRStorySubsystem.h"
#include "Story/GRStoryUtil.h"
#include "World/GRPropModels.h"
#include "World/GRTripoProps.h"
#include "Engine/StaticMesh.h"
#include "Components/BoxComponent.h"
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
	// "tripo:R08": a Tripo mesh as it is, with its own collision; a size scales it to that height.
	if (Def.Model.StartsWith(TEXT("tripo:")))
	{
		if (UStaticMesh* Direct = GRTripoProps::Load(Def.Model.Mid(6)))
		{
			Mesh->SetStaticMesh(Direct);
			const float Height = FMath::Max(Direct->GetBoundingBox().GetSize().Z, 1.f);
			const float Scale = Def.Size.IsValidIndex(2) ? Def.Size[2] / Height : 1.f;
			Mesh->SetWorldScale3D(FVector(Scale));
			Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			Mesh->CanCharacterStepUpOn = ECB_No;
			Label->SetText(FText::FromString(Def.Name));
			Label->SetWorldLocation(GetActorLocation() + FVector(0.f, 0.f, Height * Scale + 30.f));
			Label->SetWorldRotation(FRotator(0.f, Def.Yaw + 180.f, 0.f));
			Label->SetWorldScale3D(FVector::OneVector);
			return;
		}
	}
	TArray<FGRPropPart> Parts;
	if (!Def.Model.IsEmpty() && GRPropModels::Build(Def.Model, Def.Size, Def.Color, GetTypeHash(Def.Id), Parts))
	{
		float Top = BuildParts(Parts);
		float YawOffset = 0.f;
		const FString Code = GRTripoProps::CodeForModel(Def.Model, YawOffset);
		if (UStaticMesh* TripoMesh = Code.IsEmpty() ? nullptr : GRTripoProps::Load(Code))
		{
			Top = ShowTripo(TripoMesh, Parts, YawOffset);
		}
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
		// A low solid part with open space under it (seat, table top) collides as an invisible block down to
		// the ground, 10 cm wider on each side. Two dining chairs 18 cm apart squeezed Bayu's capsule, and
		// the way out of that overlap was up: onto the seats, over the table, down the other side.
		const float PartBottom = Part.Loc.Z - Part.Size.Z * 0.5f;
		const float PartTop = Part.Loc.Z + Part.Size.Z * 0.5f;
		const bool bGrounded = Part.bCollide && PartBottom > 1.f && PartBottom < 90.f;
		Comp->SetCollisionProfileName(Part.bCollide && !bGrounded ? TEXT("BlockAllDynamic") : TEXT("NoCollision"));
		if (bGrounded)
		{
			UBoxComponent* Block = NewObject<UBoxComponent>(this);
			Block->SetupAttachment(Mesh);
			Block->SetBoxExtent(FVector(Part.Size.X * 0.5f + 10.f, Part.Size.Y * 0.5f + 10.f, PartTop * 0.5f));
			Block->SetRelativeLocationAndRotation(FVector(Part.Loc.X, Part.Loc.Y, PartTop * 0.5f), FRotator(0.f, Part.Rot.Yaw, 0.f));
			Block->SetCollisionProfileName(TEXT("BlockAllDynamic"));
			Block->CanCharacterStepUpOn = ECB_No;
			Block->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			Block->SetCanEverAffectNavigation(false);
			Block->RegisterComponent();
		}
		// Furniture is not a step, and the camera passes through it instead of snapping in close behind a chair back.
		Comp->CanCharacterStepUpOn = ECB_No;
		Comp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
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

float AGRStoryProp::ShowTripo(UStaticMesh* TripoMesh, const TArray<FGRPropPart>& Parts, float YawOffset)
{
	FBox Fit(ForceInit);
	for (const FGRPropPart& Part : Parts)
	{
		Fit += FBox(Part.Loc - Part.Size * 0.5f, Part.Loc + Part.Size * 0.5f);
	}
	const FBox MeshBox = TripoMesh->GetBoundingBox();
	const FVector FitSize = Fit.GetSize();
	FVector MeshSize = MeshBox.GetSize();
	// The Tripo meshes lie along X; turn a quarter when the basic model is long the other way.
	const bool bFitLong = FMath::Max(FitSize.X, FitSize.Y) > FMath::Min(FitSize.X, FitSize.Y) * 1.2f;
	const bool bMeshLong = FMath::Max(MeshSize.X, MeshSize.Y) > FMath::Min(MeshSize.X, MeshSize.Y) * 1.2f;
	if (bFitLong && bMeshLong && (FitSize.X > FitSize.Y) != (MeshSize.X > MeshSize.Y))
	{
		YawOffset += 90.f;
	}
	const FRotator Turn(0.f, YawOffset, 0.f);
	const FVector Turned = Turn.RotateVector(MeshSize).GetAbs();
	// Uniform scale that keeps it inside the basic model (height ignored for flat things like a carpet).
	float Scale = FMath::Min(FitSize.X / FMath::Max(Turned.X, 1.f), FitSize.Y / FMath::Max(Turned.Y, 1.f));
	if (FitSize.Z > 15.f)
	{
		Scale = FMath::Min(Scale, FitSize.Z / FMath::Max(Turned.Z, 1.f));
	}
	Scale = FMath::Clamp(Scale, 0.4f, 2.5f);
	const FVector Center = Turn.RotateVector(MeshBox.GetCenter() * Scale);
	const FVector Where(Fit.GetCenter().X - Center.X, Fit.GetCenter().Y - Center.Y, Fit.Min.Z - MeshBox.Min.Z * Scale);

	// The basic shapes go invisible but keep colliding; roofed models swap back to them while the player is under.
	const TArray<TObjectPtr<UStaticMeshComponent>> Roofs = RoofParts;
	RoofParts.Reset();
	TArray<UStaticMeshComponent*> Shapes;
	GetComponents<UStaticMeshComponent>(Shapes);
	for (UStaticMeshComponent* Shape : Shapes)
	{
		if (Shape != Mesh)
		{
			Shape->SetVisibility(false);
			if (!Roofs.Contains(Shape))
			{
				UnderParts.Add(Shape);
			}
		}
	}
	if (Roofs.IsEmpty())
	{
		UnderParts.Reset();
	}

	UStaticMeshComponent* Tripo = NewObject<UStaticMeshComponent>(this);
	Tripo->SetMobility(EComponentMobility::Movable);
	Tripo->SetupAttachment(Mesh);
	Tripo->SetStaticMesh(TripoMesh);
	Tripo->SetRelativeLocationAndRotation(Where, Turn);
	Tripo->SetRelativeScale3D(FVector(Scale));
	Tripo->SetCollisionProfileName(TEXT("NoCollision"));
	Tripo->SetCanEverAffectNavigation(false);
	Tripo->RegisterComponent();
	if (!Roofs.IsEmpty())
	{
		RoofParts.Add(Tripo);
	}
	return Where.Z + MeshBox.Max.Z * Scale;
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
	for (UStaticMeshComponent* Part : UnderParts)
	{
		Part->SetVisibility(bUnder);
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
