#include "UI/GRMenuButton.h"
#include "UI/GRUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
	FVector2D RestingCursor(-1.f, -1.f);
}

void UGRMenuButton::IgnoreHoverUntilMouseMoves()
{
	if (FSlateApplication::IsInitialized())
	{
		RestingCursor = FSlateApplication::Get().GetCursorPos();
	}
}

bool UGRMenuButton::IsMouseResting()
{
	return FSlateApplication::IsInitialized() && FVector2D::Distance(FSlateApplication::Get().GetCursorPos(), RestingCursor) < 2.f;
}
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UGRMenuButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button = WidgetTree->ConstructWidget<UButton>();
	Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Button->SetContent(Row);
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Row->Slot))
	{
		ButtonSlot->SetPadding(FMargin(14.f, 10.f, 20.f, 10.f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	}
	// Accent bar on the left; grows in when the button gets focus.
	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>();
	BarSize->SetWidthOverride(4.f);
	Bar = WidgetTree->ConstructWidget<UBorder>();
	Bar->SetBrush(GRStyle::RoundedBrush(FLinearColor::White, 2.f));
	BarSize->SetContent(Bar);
	UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(BarSize);
	BarSlot->SetVerticalAlignment(VAlign_Fill);
	BarSlot->SetPadding(FMargin(0.f, 2.f, 10.f, 2.f));
	Text = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 20);
	Row->AddChildToHorizontalBox(Text)->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Button;

	Button->OnClicked.AddDynamic(this, &UGRMenuButton::HandleClicked);
	Button->OnHovered.AddDynamic(this, &UGRMenuButton::HandleHovered);
	RefreshStyle();
}

void UGRMenuButton::SetLabel(const FText& Label)
{
	Text->SetText(Label);
}

void UGRMenuButton::SetIcon(const FString& Glyph, const FLinearColor& Color, UTexture2D* Texture)
{
	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
	IconSize->SetWidthOverride(40.f);
	IconSize->SetHeightOverride(40.f);
	UBorder* Icon = WidgetTree->ConstructWidget<UBorder>();
	// Icon tiles bring their own rounded background; text glyphs get a coloured one.
	Icon->SetBrush(GRStyle::RoundedBrush(Texture ? FLinearColor::Transparent : Color, 10.f));
	Icon->SetHorizontalAlignment(HAlign_Center);
	Icon->SetVerticalAlignment(VAlign_Center);
	Icon->SetPadding(FMargin(Texture ? 0.f : 6.f));
	IconSize->SetContent(Icon);
	if (Texture)
	{
		Icon->SetContent(GRStyle::MakeIcon(WidgetTree, Texture, 40.f));
	}
	else
	{
		Icon->SetContent(GRStyle::MakeText(WidgetTree, FText::FromString(Glyph), 18, FLinearColor::White, true));
	}
	// InsertChildAt does not reorder the live Slate box, so re-add the label after the icon.
	Row->RemoveChild(Text);
	UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconSize);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	IconSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
	Row->AddChildToHorizontalBox(Text)->SetVerticalAlignment(VAlign_Center);
}

void UGRMenuButton::FocusButton()
{
	if (Button && GetOwningPlayer())
	{
		Button->SetUserFocus(GetOwningPlayer());
	}
}

void UGRMenuButton::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bFocused = true;
	RefreshStyle();
}

void UGRMenuButton::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bFocused = false;
	RefreshStyle();
}

void UGRMenuButton::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Button)
	{
		return;
	}
	const float Dt = FMath::Min(InDeltaTime, 1.f / 20.f);
	Highlight = FMath::FInterpTo(Highlight, bFocused ? 1.f : 0.f, Dt, 16.f);
	Press = FMath::FInterpTo(Press, Button->IsPressed() ? 1.f : 0.f, Dt, 30.f);

	FWidgetTransform Transform;
	const float Scale = 1.f + 0.02f * Highlight - 0.04f * Press;
	Transform.Scale = FVector2D(Scale, Scale);
	Button->SetRenderTransform(Transform);

	Bar->SetRenderOpacity(Highlight);
	FWidgetTransform BarTransform;
	BarTransform.Scale = FVector2D(1.f, FMath::Max(Highlight, 0.01f));
	Bar->SetRenderTransform(BarTransform);

	FWidgetTransform TextTransform;
	TextTransform.Translation = FVector2D(6.f * Highlight - 8.f, 0.f);
	Text->SetRenderTransform(TextTransform);
}

void UGRMenuButton::HandleClicked()
{
	OnClicked.ExecuteIfBound();
}

void UGRMenuButton::HandleHovered()
{
	if (IsMouseResting())
	{
		return;
	}
	// Keep mouse and keyboard/gamepad focus in sync.
	FocusButton();
}

void UGRMenuButton::RefreshStyle()
{
	if (!Button)
	{
		return;
	}

	const FLinearColor Base = bFocused ? GRStyle::ColorButtonFocus : GRStyle::ColorButton;
	FButtonStyle Style;
	Style.SetNormal(GRStyle::RoundedBrush(Base));
	Style.SetHovered(GRStyle::RoundedBrush(GRStyle::ColorButtonFocus));
	// Dark accent, so the accent-colored label stays readable while held.
	const FLinearColor Pressed(GRStyle::ColorAccent.R * 0.45f, GRStyle::ColorAccent.G * 0.45f, GRStyle::ColorAccent.B * 0.45f, 1.f);
	Style.SetPressed(GRStyle::RoundedBrush(Pressed));
	Style.SetDisabled(GRStyle::RoundedBrush(GRStyle::ColorPanelSoft));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	Button->SetStyle(Style);

	const FLinearColor TextColor = !GetIsEnabled() ? GRStyle::ColorTextDim : (bDanger ? GRStyle::ColorDanger : (bFocused ? GRStyle::ColorAccent : GRStyle::ColorText));
	Text->SetColorAndOpacity(FSlateColor(TextColor));
	Bar->SetBrushColor(bDanger ? GRStyle::ColorDanger : GRStyle::ColorAccent);
}
