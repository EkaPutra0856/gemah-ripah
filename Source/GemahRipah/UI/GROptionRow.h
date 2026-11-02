#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GROptionRow.generated.h"

class UButton;
class UTextBlock;

/**
 * Settings row "Label   < Value >". Left/Right (keys, d-pad) change the value; click cycles forward.
 */
UCLASS()
class GEMAHRIPAH_API UGROptionRow : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FText& Label, const TArray<FText>& InOptions, int32 InIndex, TFunction<void(int32)> InOnChanged);
	void SetIndex(int32 NewIndex, bool bNotify);
	int32 GetIndex() const { return Index; }
	void FocusRow();

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	void Step(int32 Direction);
	void RefreshStyle();

	UPROPERTY()
	TObjectPtr<UButton> Button;

	UPROPERTY()
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ValueText;

	TArray<FText> Options;
	int32 Index = 0;
	TFunction<void(int32)> OnChanged;
	bool bFocused = false;
};
