#include "UI/GRUIStyle.h"
#include "Story/GRStoryTypes.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "UObject/StrongObjectPtr.h"

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

	UTexture2D* Icon(const FString& Name)
	{
		// Loaded from the staged Data folder at runtime, like the JSON, so no editor import step is needed.
		static TMap<FString, TStrongObjectPtr<UTexture2D>> Cache;
		if (const TStrongObjectPtr<UTexture2D>* Found = Cache.Find(Name))
		{
			return Found->Get();
		}
		UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(FPaths::ProjectContentDir() / TEXT("Data/Icons") / (Name + TEXT(".png")));
		Cache.Add(Name, TStrongObjectPtr<UTexture2D>(Texture));
		return Texture;
	}

	UTexture2D* ItemIcon(const FGRItemDef& Item)
	{
		return Icon(Item.Icon.IsEmpty() ? TEXT("item_") + Item.Id : Item.Icon);
	}

	UWidget* MakeIcon(UWidgetTree* Tree, UTexture2D* Texture, float Size)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>();
		Box->SetWidthOverride(Size);
		Box->SetHeightOverride(Size);
		UImage* Image = Tree->ConstructWidget<UImage>();
		Box->SetContent(Image);
		if (Texture)
		{
			Image->SetBrushFromTexture(Texture);
		}
		else
		{
			Image->SetColorAndOpacity(FLinearColor::Transparent);
		}
		return Box;
	}
}
