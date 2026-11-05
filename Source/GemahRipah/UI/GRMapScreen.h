#pragma once

#include "CoreMinimal.h"
#include "UI/GRScreenWidget.h"
#include "GRMapScreen.generated.h"

class UGRMapView;
class UTextBlock;

/**
 * The city map on Bayu's phone, held sideways: the phone fills about 60% of the screen while the camera looks over
 * his shoulder at it (AGRCharacter::SetPhoneView), and the game keeps running behind it. With an OjekKu trip under
 * way the map follows Bayu with the route drawn on it (navigation); otherwise it shows the whole town with a legend.
 * Opened with M, the Peta app, OjekKu's Navigasi button, or "Peta" in the pause menu.
 */
UCLASS()
class GEMAHRIPAH_API UGRMapScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRMapScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void SetNavigation(bool bOn);

	UPROPERTY()
	TObjectPtr<UGRMapView> View;

	UPROPERTY()
	TObjectPtr<UTextBlock> TripTitle;

	UPROPERTY()
	TObjectPtr<UTextBlock> TripBody;

	UPROPERTY()
	TObjectPtr<UTextBlock> TopBar;

	bool bNavigation = false;
	float RefreshTimer = 0.f;
};
