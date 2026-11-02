#include "UI/GRUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

namespace GRStyle
{
	FSlateFontInfo Font(int32 Size, bool bBold)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	FSlateBrush RoundedBrush(const FLinearColor& Color, float Radius)
	{
		return FSlateRoundedBoxBrush(FSlateColor(Color), Radius);
	}

	UTextBlock* MakeText(UWidgetTree* Tree, const FText& Text, int32 Size, const FLinearColor& Color, bool bBold)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>();
		Block->SetText(Text);
		Block->SetFont(Font(Size, bBold));
		Block->SetColorAndOpacity(FSlateColor(Color));
		return Block;
	}

	UBorder* MakePanel(UWidgetTree* Tree, const FLinearColor& Color, const FMargin& Padding, float Radius)
	{
		UBorder* Panel = Tree->ConstructWidget<UBorder>();
		Panel->SetBrush(RoundedBrush(Color, Radius));
		Panel->SetPadding(Padding);
		return Panel;
	}
}
