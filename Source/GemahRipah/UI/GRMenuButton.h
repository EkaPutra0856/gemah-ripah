#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GRMenuButton.generated.h"

class UButton;
class UBorder;
class UHorizontalBox;
class UTextBlock;
class UTexture2D;

/** Menu button usable with mouse, keyboard and gamepad. Highlights when hovered or focused: grows a little, shows an accent bar, squishes when pressed. */
UCLASS()
class GEMAHRIPAH_API UGRMenuButton : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetLabel(const FText& Label);
	void SetDanger(bool bInDanger) { bDanger = bInDanger; RefreshStyle(); }
	void FocusButton();
	/** Rounded colored icon left of the label: a texture if given, else the glyph. */
	void SetIcon(const FString& Glyph, const FLinearColor& Color, UTexture2D* Texture = nullptr);
	UButton* GetButton() const { return Button; }

	FSimpleDelegate OnClicked;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	void RefreshStyle();

	UPROPERTY()
	TObjectPtr<UButton> Button;

	UPROPERTY()
	TObjectPtr<UTextBlock> Text;

	UPROPERTY()
	TObjectPtr<UBorder> Bar;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> Row;

	float Highlight = 0.f;
	float Press = 0.f;
	bool bFocused = false;
	bool bDanger = false;
};
