#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GRScreenWidget.h"
#include "GRChapterScreens.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UVerticalBox;
struct FGRChapterDef;

enum class EGRChapterState : uint8
{
	/** No story file yet. */
	Soon,
	/** The chapter before it is not done. */
	Locked,
	/** Unlocked and not started. */
	Open,
	Current,
	Done,
};

/** Round chapter button on the chapter map: icon, status badge, number and title under it. */
UCLASS()
class GEMAHRIPAH_API UGRChapterNode : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FGRChapterDef& Def, EGRChapterState InState);
	UButton* GetButton() const { return Button; }

	TFunction<void()> OnFocused;
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
	UPROPERTY() TObjectPtr<UBorder> Ring;
	UPROPERTY() TObjectPtr<UImage> IconImage;
	UPROPERTY() TObjectPtr<UBorder> Badge;
	UPROPERTY() TObjectPtr<UImage> BadgeImage;
	UPROPERTY() TObjectPtr<UTextBlock> NumberText;
	UPROPERTY() TObjectPtr<UTextBlock> TitleText;
	UPROPERTY() TObjectPtr<UWidget> Disc;

	EGRChapterState State = EGRChapterState::Locked;
	FLinearColor Color = FLinearColor::White;
	float Highlight = 0.f;
	float Time = 0.f;
	bool bFocused = false;
};

/**
 * Chapter map ("Peta Bab"): every chapter as an icon on a map of Republik Gemah Ripah, joined by a dotted
 * road. Focusing a chapter shows its story so far; the next unlocked chapter can be started from here.
 */
UCLASS()
class GEMAHRIPAH_API UGRChapterMapScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRChapterMapScreen(const FObjectInitializer& ObjectInitializer);
	/** Opens the map with the given chapter selected (current chapter when empty). */
	static UGRChapterMapScreen* Open(class UGRUISubsystem* UI, const FString& SelectId = FString());

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual void PlayIntro() override;

private:
	EGRChapterState GetState(const FGRChapterDef& Def) const;
	void ShowDetails(int32 Index);

	static FString PendingSelect;
	int32 Selected = INDEX_NONE;

	UPROPERTY() TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY() TObjectPtr<UVerticalBox> Details;
	UPROPERTY() TArray<TObjectPtr<UGRChapterNode>> Nodes;
	UPROPERTY() TArray<TObjectPtr<UWidget>> Islands;
	UPROPERTY() TArray<TObjectPtr<UWidget>> Dots;
};
