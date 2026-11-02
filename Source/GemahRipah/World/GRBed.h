#pragma once

#include "CoreMinimal.h"
#include "Interaction/GRInteractableActor.h"
#include "GRBed.generated.h"

/** Sleep until morning (allowed in the evening and at night) and autosave. */
UCLASS()
class GEMAHRIPAH_API AGRBed : public AGRInteractableActor
{
	GENERATED_BODY()

public:
	AGRBed();

	virtual void Interact_Implementation(AActor* Interactor) override;

	/** Fades out, jumps to the morning, autosaves and fades back in. */
	void Sleep(APlayerController* PC);

private:
	void WakeUp(APlayerController* PC);

	FTimerHandle SleepTimer;
};
