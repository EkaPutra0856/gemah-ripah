#include "Player/GRInteractionComponent.h"
#include "Interaction/GRInteractable.h"
#include "Player/GRCharacter.h"
#include "World/GRMusic.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

UGRInteractionComponent::UGRInteractionComponent()
{
	// Every frame for a smooth turn; the focus search itself runs ten times a second.
	PrimaryComponentTick.bCanEverTick = true;
}

void UGRInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (TurnTimeLeft > 0.f)
	{
		TurnTimeLeft -= DeltaTime;
		if (AActor* Owner = GetOwner())
		{
			Owner->SetActorRotation(FMath::RInterpTo(Owner->GetActorRotation(), FRotator(0.f, TurnYaw, 0.f), DeltaTime, 10.f));
		}
	}

	FocusTimer -= DeltaTime;
	if (FocusTimer > 0.f)
	{
		return;
	}
	FocusTimer = 0.1f;
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
		// Prefer what the player faces, then what is closest. Speakers only when nothing else is near, so a radio
		// on an office desk never takes the key from the person sitting behind it.
		const float Score = Facing * 100.f - Distance - (Candidate->IsA<AGRSpeaker>() ? 100000.f : 0.f);
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
	// Turn (over a few frames, not in one snap) to face whoever or whatever Bayu is talking to. Not on the
	// scooter: it cannot turn on the spot.
	const AGRCharacter* Rider = Cast<AGRCharacter>(GetOwner());
	if (AActor* Owner = Rider && Rider->IsRiding() ? nullptr : GetOwner())
	{
		const FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
		if (ToTarget.SizeSquared2D() > 1.f)
		{
			TurnYaw = ToTarget.Rotation().Yaw;
			TurnTimeLeft = 0.6f;
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
