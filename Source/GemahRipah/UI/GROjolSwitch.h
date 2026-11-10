#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "GROjolSwitch.generated.h"

class UButton;

/**
 * OjekKu's big online switch in the phone. Everything but the click area is painted, because the card animates as a whole:
 * the knob slides along a track that fills up behind it, then rings and sparks burst from it and the card lights up.
 * While online, radar rings keep going out from the knob.
 */
UCLASS()
class GEMAHRIPAH_API UGROjolSwitch : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows a state without animating. */
	void SetOn(bool bOn);
	/** Animates to a state. */
	void Flip(bool bOn);
	/** The app refused: the knob tries, shakes and falls back. */
	void Refuse();

	bool IsOn() const { return bTarget; }
	UButton* GetButton() const { return Button; }

	TFunction<void()> OnClicked;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UPROPERTY() TObjectPtr<UButton> Button;

	FSlateBrush CardBrush;
	FSlateBrush RoundBrush;
	FSlateBrush HelmetBrush;
	FSlateBrush ScooterBrush;

	bool bTarget = false;
	bool bFocused = false;
	/** Where the knob is, 0 left to 1 right; it overshoots a little. */
	float Knob = 0.f;
	float KnobFrom = 0.f;
	/** How far the card has lit up, 0 to 1. */
	float Lit = 0.f;
	float Time = 0.f;
	float SinceFlip = 100.f;
	float SinceRefuse = 100.f;
};
