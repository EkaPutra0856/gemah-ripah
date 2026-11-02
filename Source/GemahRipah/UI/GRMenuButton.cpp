#include "UI/GRMenuButton.h"
#include "UI/GRUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"

void UGRMenuButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button = WidgetTree->ConstructWidget<UButton>();
	Text = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 20);
	Button->SetContent(Text);
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Text->Slot))
	{
		ButtonSlot->SetPadding(FMargin(20.f, 10.f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Left);
	}
	WidgetTree->RootWidget = Button;

	Button->OnClicked.AddDynamic(this, &UGRMenuButton::HandleClicked);
	Button->OnHovered.AddDynamic(this, &UGRMenuButton::HandleHovered);
	RefreshStyle();
}

void UGRMenuButton::SetLabel(const FText& Label)
{
	Text->SetText(Label);
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

void UGRMenuButton::HandleClicked()
{
	OnClicked.ExecuteIfBound();
}

void UGRMenuButton::HandleHovered()
{
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
	Style.SetPressed(GRStyle::RoundedBrush(GRStyle::ColorAccent));
	Style.SetDisabled(GRStyle::RoundedBrush(GRStyle::ColorPanelSoft));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	Button->SetStyle(Style);

	const FLinearColor TextColor = !GetIsEnabled() ? GRStyle::ColorTextDim : (bDanger ? GRStyle::ColorDanger : (bFocused ? GRStyle::ColorAccent : GRStyle::ColorText));
	Text->SetColorAndOpacity(FSlateColor(TextColor));
}
