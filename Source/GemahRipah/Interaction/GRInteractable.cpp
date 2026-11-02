#include "Interaction/GRInteractable.h"

FText IGRInteractable::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return NSLOCTEXT("GemahRipah", "DefaultPrompt", "Interaksi");
}

bool IGRInteractable::CanInteract_Implementation(AActor* Interactor) const
{
	return true;
}

void IGRInteractable::Interact_Implementation(AActor* Interactor)
{
}
