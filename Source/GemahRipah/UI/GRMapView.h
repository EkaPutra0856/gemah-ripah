#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GRMapView.generated.h"

/**
 * Paints the city map: captured top-down image, districts, markers and the player arrow.
 * Follow mode (minimap) centers on the player; otherwise the whole map is fitted.
 */
UCLASS()
class GEMAHRIPAH_API UGRMapView : public UUserWidget
{
	GENERATED_BODY()

public:
	UGRMapView(const FObjectInitializer& ObjectInitializer);

	/** Minimap mode: centered on the player, showing ViewSize world units across. */
	bool bFollowPlayer = false;
	float ViewSize = 30000.f;

	bool bShowDistrictNames = true;
	bool bShowMarkerLabels = false;

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
};
