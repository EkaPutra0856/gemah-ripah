#pragma once

#include "CoreMinimal.h"
#include "UI/GRScreenWidget.h"
#include "GRMapScreen.generated.h"

/** Full city map with a legend. Opened with M, or "Peta" in the pause menu. */
UCLASS()
class GEMAHRIPAH_API UGRMapScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRMapScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
};
