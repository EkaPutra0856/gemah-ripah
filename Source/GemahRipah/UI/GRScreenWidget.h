#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GRUIAnim.h"
#include "GRScreenWidget.generated.h"

class UOverlay;
class UPanelWidget;
class UVerticalBox;
class UGRMenuButton;
class UGRUISubsystem;

/**
 * Base for full-screen menus pushed on UGRUISubsystem's stack.
 * Subclasses build their layout in BuildScreen(); Escape / gamepad B calls HandleBack().
 */
UCLASS(Abstract)
class GEMAHRIPAH_API UGRScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGRScreenWidget(const FObjectInitializer& ObjectInitializer);

	/** Called after the screen is pushed or becomes the top again. */
	virtual void OnShown();

	void Close();

	UGRUISubsystem* GetUI() const;

	bool bPausesGame = false;
	bool bCanClose = true;
	bool bDimBackground = true;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual void BuildScreen(UOverlay* Root) {}
	/** Runs once, the first time the screen is shown: dim fades, panels pop in, the centered panel's rows stagger. */
	virtual void PlayIntro();
	virtual void HandleBack();

	/** Centered panel with a title; returns the box to add rows to. */
	UVerticalBox* MakeCenteredPanel(UOverlay* Root, const FText& Title, float Width);

	UGRMenuButton* AddButton(UPanelWidget* Parent, const FText& Label, TFunction<void()> OnClicked);

	/** Widget focused when the screen is shown (first button by default). */
	UPROPERTY()
	TObjectPtr<UWidget> DefaultFocus;

	GRAnim::FPlayer Anim;

	UPROPERTY()
	TObjectPtr<UOverlay> RootOverlay;

	UPROPERTY()
	TObjectPtr<UWidget> DimWidget;

	/** Rows of the panel from MakeCenteredPanel, staggered by the intro. */
	UPROPERTY()
	TObjectPtr<UVerticalBox> IntroList;

private:
	bool bIntroPlayed = false;
};
