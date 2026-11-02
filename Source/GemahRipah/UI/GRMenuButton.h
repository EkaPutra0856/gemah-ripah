#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GRMenuButton.generated.h"

class UButton;
class UTextBlock;

/** Menu button usable with mouse, keyboard and gamepad. Highlights when hovered or focused. */
UCLASS()
class GEMAHRIPAH_API UGRMenuButton : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetLabel(const FText& Label);
	void SetDanger(bool bInDanger) { bDanger = bInDanger; RefreshStyle(); }
	void FocusButton();
	UButton* GetButton() const { return Button; }

	FSimpleDelegate OnClicked;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

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

	bool bFocused = false;
	bool bDanger = false;
};
