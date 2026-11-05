#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GRUIAnim.h"
#include "GROjolHUD.generated.h"

class UBorder;
class UProgressBar;
class UTextBlock;
class UVerticalBox;

/** A row of five stars painted in place; Value 0..5 (whole stars light up, the rest stay dim). */
UCLASS()
class GEMAHRIPAH_API UGROjolStars : public UUserWidget
{
	GENERATED_BODY()

public:
	UGROjolStars(const FObjectInitializer& ObjectInitializer);

	float Value = 5.f;
	float StarSize = 18.f;

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
};

/**
 * The OjekKu overlay on the HUD (World/GROjol.h): the incoming order card with its countdown (Y / N), the trip bar
 * at the top with the next turn, the chat choices with the passenger (1 / 2 / 3) and the end-of-trip card with the
 * stars and the money.
 */
UCLASS()
class GEMAHRIPAH_API UGROjolHUD : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void Refill();

	UPROPERTY() TObjectPtr<UBorder> OfferPanel;
	UPROPERTY() TObjectPtr<UTextBlock> OfferTimer;
	UPROPERTY() TObjectPtr<UTextBlock> OfferName;
	UPROPERTY() TObjectPtr<UGROjolStars> OfferStars;
	UPROPERTY() TObjectPtr<UTextBlock> OfferBody;
	UPROPERTY() TObjectPtr<UProgressBar> OfferBar;

	UPROPERTY() TObjectPtr<UBorder> TripPanel;
	UPROPERTY() TObjectPtr<UTextBlock> TripStatus;
	UPROPERTY() TObjectPtr<UTextBlock> TripTurn;
	UPROPERTY() TObjectPtr<UGROjolStars> TripComfort;

	UPROPERTY() TObjectPtr<UBorder> ChatPanel;
	UPROPERTY() TObjectPtr<UTextBlock> ChatPrompt;
	UPROPERTY() TObjectPtr<UVerticalBox> ChatList;
	UPROPERTY() TObjectPtr<UProgressBar> ChatBar;

	UPROPERTY() TObjectPtr<UBorder> SummaryPanel;
	UPROPERTY() TObjectPtr<UTextBlock> SummaryTitle;
	UPROPERTY() TObjectPtr<UGROjolStars> SummaryStars;
	UPROPERTY() TObjectPtr<UTextBlock> SummaryReview;
	UPROPERTY() TObjectPtr<UTextBlock> SummaryMoney;

	UPROPERTY() TObjectPtr<UBorder> OnlinePill;
	UPROPERTY() TObjectPtr<UTextBlock> OnlineText;

	GRAnim::FPlayer Anim;
	int32 ShownRevision = -1;
	float ChatSeconds = 1.f;
};
