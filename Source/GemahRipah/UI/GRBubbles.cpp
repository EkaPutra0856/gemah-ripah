#include "UI/GRBubbles.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void UGRBubbleSubsystem::Show(AActor* Who, const FString& Text, float Duration, const FLinearColor& Color, float Height, bool bShout)
{
	UWorld* World = Who ? Who->GetWorld() : nullptr;
	UGRBubbleSubsystem* Self = World ? World->GetSubsystem<UGRBubbleSubsystem>() : nullptr;
	if (!Self || Text.IsEmpty())
	{
		return;
	}
	FGRBubble* Bubble = Self->Bubbles.FindByPredicate([Who](const FGRBubble& B) { return B.Who.Get() == Who; });
	if (!Bubble)
	{
		Bubble = &Self->Bubbles.AddDefaulted_GetRef();
		Bubble->Who = Who;
	}
	// The same line again (a repeated shout) keeps its pop-in time so it does not flicker.
	if (Bubble->Text != Text || World->GetTimeSeconds() - Bubble->Start > Bubble->Duration)
	{
		Bubble->Start = World->GetTimeSeconds();
	}
	Bubble->Text = Text;
	Bubble->Color = Color;
	Bubble->Height = Height;
	Bubble->Duration = Duration + float(World->GetTimeSeconds() - Bubble->Start);
	Bubble->bShout = bShout;
}

void UGRBubbleSubsystem::Hide(AActor* Who)
{
	UWorld* World = Who ? Who->GetWorld() : nullptr;
	if (UGRBubbleSubsystem* Self = World ? World->GetSubsystem<UGRBubbleSubsystem>() : nullptr)
	{
		Self->Bubbles.RemoveAll([Who](const FGRBubble& B) { return B.Who.Get() == Who; });
	}
}

void UGRBubbleSubsystem::Prune()
{
	const double Now = GetWorld()->GetTimeSeconds();
	Bubbles.RemoveAll([Now](const FGRBubble& B)
	{
		const AActor* Who = B.Who.Get();
		return !Who || Who->IsHidden() || Now - B.Start > B.Duration;
	});
}
