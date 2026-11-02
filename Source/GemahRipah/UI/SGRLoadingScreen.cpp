#include "UI/SGRLoadingScreen.h"
#include "UI/GRUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void SGRLoadingScreen::Construct(const FArguments& InArgs)
{
	static const FSlateColorBrush Background(GRStyle::ColorBackground);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(&Background)
		.Padding(FMargin(80.f, 60.f))
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("GemahRipah", "LoadingTitle", "GEMAH RIPAH"))
				.Font(GRStyle::Font(40, true))
				.ColorAndOpacity(GRStyle::ColorAccent)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Bottom)
			[
				SNew(SBox)
				.MaxDesiredWidth(1000.f)
				[
					SNew(STextBlock)
					.Text(InArgs._Tip)
					.Font(GRStyle::Font(20, false))
					.ColorAndOpacity(GRStyle::ColorText)
					.AutoWrapText(true)
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 16.f, 0.f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("GemahRipah", "Loading", "Memuat..."))
					.Font(GRStyle::Font(20, false))
					.ColorAndOpacity(GRStyle::ColorTextDim)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SThrobber)
				]
			]
		]
	];
}
