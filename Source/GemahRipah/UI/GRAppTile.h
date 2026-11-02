#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GRAppTile.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

/** Phone home-screen app: big icon tile, label under it, optional red badge. Lifts when focused. */
UCLASS()
class GEMAHRIPAH_API UGRAppTile : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UTexture2D* Icon, const FText& Label, const FString& BadgeText = FString());
	UButton* GetButton() const { return Button; }

	TFunction<void()> OnClicked;

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

	UPROPERTY() TObjectPtr<UButton> Button;
	UPROPERTY() TObjectPtr<UWidget> IconStack;
	UPROPERTY() TObjectPtr<UBorder> Glow;
	UPROPERTY() TObjectPtr<UImage> IconImage;
	UPROPERTY() TObjectPtr<UBorder> Badge;
	UPROPERTY() TObjectPtr<UTextBlock> BadgeText;
	UPROPERTY() TObjectPtr<UTextBlock> LabelText;

	float Highlight = 0.f;
	float Time = 0.f;
	bool bFocused = false;
};
