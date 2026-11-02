#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"

class UWidgetTree;
class UTextBlock;
class UBorder;
class UTexture2D;
class UImage;
class UWidget;
struct FGRItemDef;
struct FSlateBrush;

/** Shared look for all code-built UI: smoky orange accent on dark panels. */
namespace GRStyle
{
	inline const FLinearColor ColorBackground = FLinearColor(FColor(14, 12, 11));
	inline const FLinearColor ColorPanel = FLinearColor(FColor(28, 24, 21, 235));
	inline const FLinearColor ColorPanelSoft = FLinearColor(FColor(20, 17, 15, 190));
	inline const FLinearColor ColorButton = FLinearColor(FColor(48, 41, 36, 230));
	inline const FLinearColor ColorButtonFocus = FLinearColor(FColor(96, 66, 30, 255));
	inline const FLinearColor ColorAccent = FLinearColor(FColor(232, 163, 58));
	inline const FLinearColor ColorDanger = FLinearColor(FColor(226, 78, 60));
	inline const FLinearColor ColorGood = FLinearColor(FColor(110, 200, 120));
	inline const FLinearColor ColorInfo = FLinearColor(FColor(90, 180, 235));
	inline const FLinearColor ColorText = FLinearColor(FColor(240, 234, 224));
	inline const FLinearColor ColorTextDim = FLinearColor(FColor(160, 150, 140));
	inline const FLinearColor ColorDim = FLinearColor(0.f, 0.f, 0.f, 0.6f);

	FSlateFontInfo Font(int32 Size, bool bBold = false);
	FSlateBrush RoundedBrush(const FLinearColor& Color, float Radius = 6.f);

	UTextBlock* MakeText(UWidgetTree* Tree, const FText& Text, int32 Size, const FLinearColor& Color = ColorText, bool bBold = false);
	UBorder* MakePanel(UWidgetTree* Tree, const FLinearColor& Color = ColorPanel, const FMargin& Padding = FMargin(24.f), float Radius = 8.f);

	/** Full-colour icon tile from Content/Data/Icons/<Name>.png (see Docs/CREDITS.md), cached; null if missing. */
	UTexture2D* Icon(const FString& Name);
	/** Icon of a story item: its "icon" field, else item_<id>. */
	UTexture2D* ItemIcon(const FGRItemDef& Item);
	/** Image of the given icon in a fixed-size box (empty when the icon is missing). */
	UWidget* MakeIcon(UWidgetTree* Tree, UTexture2D* Texture, float Size);
}
