#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GRBubbles.generated.h"

/** One speech bubble over a speaker's head. */
struct FGRBubble
{
	TWeakObjectPtr<AActor> Who;
	FString Text;
	FLinearColor Color = FLinearColor::White;
	/** Height above the actor's origin, in cm. */
	float Height = 120.f;
	double Start = 0.0;
	float Duration = 3.f;
	bool bShout = false;
};

/**
 * Speech bubbles for barks (story NPCs, townspeople, drivers). Speakers call Show instead of
 * floating 3D text; the HUD draws the bubbles in screen space so they stay readable at any
 * camera angle. One bubble per speaker: a new line replaces the old one.
 */
UCLASS()
class GEMAHRIPAH_API UGRBubbleSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static void Show(AActor* Who, const FString& Text, float Duration, const FLinearColor& Color = FLinearColor::White, float Height = 120.f, bool bShout = false);
	static void Hide(AActor* Who);

	/** Drops finished bubbles and bubbles of destroyed or hidden speakers. */
	void Prune();
	const TArray<FGRBubble>& GetBubbles() const { return Bubbles; }

private:
	TArray<FGRBubble> Bubbles;
};
