#pragma once

#include "CoreMinimal.h"
#include "UI/GRScreenWidget.h"
#include "GRMenuScreens.generated.h"

class UGRMenuButton;
class UTextBlock;

/** Title screen in L_MainMenu. */
UCLASS()
class GEMAHRIPAH_API UGRMainMenuScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRMainMenuScreen(const FObjectInitializer& ObjectInitializer);
	virtual void OnShown() override;

protected:
	virtual void BuildScreen(UOverlay* Root) override;

private:
	UPROPERTY()
	TObjectPtr<UGRMenuButton> ContinueButton;
};

/** In-game pause menu (Escape / Start). */
UCLASS()
class GEMAHRIPAH_API UGRPauseScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRPauseScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BuildScreen(UOverlay* Root) override;
};

/** Yes/No question. Focus starts on "Tidak" so an accidental confirm is harder. */
UCLASS()
class GEMAHRIPAH_API UGRConfirmScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	static void Ask(UGRUISubsystem* UI, const FText& Message, TFunction<void()> OnYes);

protected:
	virtual void BuildScreen(UOverlay* Root) override;

private:
	UPROPERTY()
	TObjectPtr<UTextBlock> MessageText;

	TFunction<void()> OnYes;
};

/** Credits from Content/Data/credits.json. */
UCLASS()
class GEMAHRIPAH_API UGRCreditsScreen : public UGRScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void BuildScreen(UOverlay* Root) override;
};
