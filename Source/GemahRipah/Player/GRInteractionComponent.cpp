#include "Player/GRInteractionComponent.h"
#include "Interaction/GRInteractable.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

UGRInteractionComponent::UGRInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UGRInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Best = FindBestInteractable();
	if (Best != FocusedActor.Get())
	{
		FocusedActor = Best;
		OnFocusChanged.Broadcast(Best);
	}
}

AActor* UGRInteractionComponent::FindBestInteractable() const
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return nullptr;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GRInteraction), false, Owner);

	TArray<FOverlapResult> Overlaps;
	const FVector Origin = Owner->GetActorLocation();
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Radius), QueryParams);

	AActor* Best = nullptr;
	float BestScore = -FLT_MAX;
	const FVector Forward = Owner->GetActorForwardVector();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || !Candidate->Implements<UGRInteractable>())
		{
			continue;
		}
		if (!IGRInteractable::Execute_CanInteract(Candidate, const_cast<AActor*>(Owner)))
		{
			continue;
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - Origin;
		const float Distance = ToCandidate.Size2D();
		const float Facing = FVector::DotProduct(Forward, ToCandidate.GetSafeNormal2D());
		// Prefer what the player faces, then what is closest.
		const float Score = Facing * 100.f - Distance;
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}
	return Best;
}

bool UGRInteractionComponent::TryInteract()
{
	AActor* Target = FocusedActor.Get();
	if (!Target || !IGRInteractable::Execute_CanInteract(Target, GetOwner()))
	{
		return false;
	}
	// Turn to face whoever (or whatever) Bayu is talking to.
	if (AActor* Owner = GetOwner())
	{
		const FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
		if (ToTarget.SizeSquared2D() > 1.f)
		{
			Owner->SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
		}
	}
	IGRInteractable::Execute_Interact(Target, GetOwner());
	return true;
}

FText UGRInteractionComponent::GetFocusedPrompt() const
{
	AActor* Target = FocusedActor.Get();
	return Target ? IGRInteractable::Execute_GetInteractionPrompt(Target, GetOwner()) : FText::GetEmpty();
}
