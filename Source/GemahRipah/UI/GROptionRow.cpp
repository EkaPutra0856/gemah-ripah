#include "UI/GROptionRow.h"
#include "UI/GRUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

void UGROptionRow::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button = WidgetTree->ConstructWidget<UButton>();
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	LabelText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 18);
	ValueText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 18, GRStyle::ColorAccent, true);

	UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText);
	LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LabelSlot->SetVerticalAlignment(VAlign_Center);

	UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueText);
	ValueSlot->SetVerticalAlignment(VAlign_Center);

	Button->SetContent(Row);
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Row->Slot))
	{
		ButtonSlot->SetPadding(FMargin(18.f, 8.f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	}
	WidgetTree->RootWidget = Button;

	Button->OnClicked.AddDynamic(this, &UGROptionRow::HandleClicked);
	Button->OnHovered.AddDynamic(this, &UGROptionRow::HandleHovered);
	RefreshStyle();
}

void UGROptionRow::Setup(const FText& Label, const TArray<FText>& InOptions, int32 InIndex, TFunction<void(int32)> InOnChanged)
{
	LabelText->SetText(Label);
	Options = InOptions;
	OnChanged = MoveTemp(InOnChanged);
	SetIndex(InIndex, false);
}

void UGROptionRow::SetIndex(int32 NewIndex, bool bNotify)
{
	if (Options.Num() == 0)
	{
		return;
	}
	Index = FMath::Clamp(NewIndex, 0, Options.Num() - 1);
	ValueText->SetText(FText::Format(NSLOCTEXT("GemahRipah", "OptionValue", "<  {0}  >"), Options[Index]));
	if (bNotify && OnChanged)
	{
		OnChanged(Index);
	}
}

void UGROptionRow::Step(int32 Direction)
{
	if (Options.Num() > 0)
	{
		SetIndex((Index + Direction + Options.Num()) % Options.Num(), true);
	}
}

void UGROptionRow::FocusRow()
{
	if (Button && GetOwningPlayer())
	{
		Button->SetUserFocus(GetOwningPlayer());
	}
}

FReply UGROptionRow::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left || Key == EKeys::A)
	{
		Step(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right || Key == EKeys::D)
	{
		Step(1);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGROptionRow::HandleClicked()
{
	Step(1);
}

void UGROptionRow::HandleHovered()
{
	FocusRow();
}

void UGROptionRow::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bFocused = true;
	RefreshStyle();
}

void UGROptionRow::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bFocused = false;
	RefreshStyle();
}

void UGROptionRow::RefreshStyle()
{
	if (!Button)
	{
		return;
	}
	const FLinearColor Base = bFocused ? GRStyle::ColorButtonFocus : GRStyle::ColorButton;
	FButtonStyle Style;
	Style.SetNormal(GRStyle::RoundedBrush(Base));
	Style.SetHovered(GRStyle::RoundedBrush(GRStyle::ColorButtonFocus));
	Style.SetPressed(GRStyle::RoundedBrush(GRStyle::ColorButtonFocus));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	Button->SetStyle(Style);
}
