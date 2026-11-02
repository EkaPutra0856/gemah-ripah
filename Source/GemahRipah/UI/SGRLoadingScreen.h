#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/** Slate loading screen (UMG cannot run while a map loads). */
class SGRLoadingScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGRLoadingScreen) {}
		SLATE_ARGUMENT(FText, Tip)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};
