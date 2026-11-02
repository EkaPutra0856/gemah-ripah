#include "Interaction/GRInteractableActor.h"
#include "Components/StaticMeshComponent.h"
#include "Systems/GRStatusSubsystem.h"
#include "Systems/GRProgressSubsystem.h"
#include "Systems/GRRegulationSubsystem.h"

AGRInteractableActor::AGRInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Mesh;
}

FText AGRInteractableActor::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return Prompt;
}

bool AGRInteractableActor::CanInteract_Implementation(AActor* Interactor) const
{
	return !(bSingleUse && bUsed);
}

void AGRInteractableActor::Interact_Implementation(AActor* Interactor)
{
	if (!Execute_CanInteract(this, Interactor))
	{
		return;
	}
	bUsed = true;

	UGameInstance* GI = GetGameInstance();
	bool bBrokeRule = false;

	if (UGRRegulationSubsystem* Regulations = GI->GetSubsystem<UGRRegulationSubsystem>())
	{
		bBrokeRule = Regulations->ReportAction(ActionTag);
	}

	if (UGRStatusSubsystem* Status = GI->GetSubsystem<UGRStatusSubsystem>())
	{
		if (UangDelta != 0) { Status->AddUang(UangDelta); }
		if (SuaraDelta != 0.f) { Status->AddSuara(SuaraDelta); }
		if (DiawasiDelta != 0.f) { Status->AddDiawasi(DiawasiDelta); }
	}

	if (StoryFlagToAdd.IsValid())
	{
		GI->GetSubsystem<UGRProgressSubsystem>()->AddStoryFlag(StoryFlagToAdd);
	}

	OnInteracted(Interactor, bBrokeRule);
}
