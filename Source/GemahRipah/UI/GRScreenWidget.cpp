#include "UI/GRScreenWidget.h"
#include "UI/GRMenuButton.h"
#include "UI/GROptionRow.h"
#include "UI/GRUISubsystem.h"
#include "UI/GRUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"

UGRScreenWidget::UGRScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The UI subsystem focuses the screen itself; Escape must reach NativeOnKeyDown even with no button focused.
	SetIsFocusable(true);
}

void UGRScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;
	RootOverlay = Root;

	if (bDimBackground)
	{
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
		Dim->SetBrushColor(GRStyle::ColorDim);
		DimWidget = Dim;
		UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	BuildScreen(Root);
}

UGRUISubsystem* UGRScreenWidget::GetUI() const
{
	const ULocalPlayer* LP = GetOwningLocalPlayer();
	return LP ? LP->GetSubsystem<UGRUISubsystem>() : nullptr;
}

void UGRScreenWidget::OnShown()
{
	if (!bIntroPlayed)
	{
		bIntroPlayed = true;
		PlayIntro();
	}
	if (UGRMenuButton* Button = Cast<UGRMenuButton>(DefaultFocus))
	{
		Button->FocusButton();
	}
	else if (UGROptionRow* Row = Cast<UGROptionRow>(DefaultFocus))
	{
		Row->FocusRow();
	}
	else if (DefaultFocus && GetOwningPlayer())
	{
		DefaultFocus->SetUserFocus(GetOwningPlayer());
	}
}

void UGRScreenWidget::PlayIntro()
{
	if (!RootOverlay)
	{
		return;
	}
	for (int32 i = 0; i < RootOverlay->GetChildrenCount(); ++i)
	{
		UWidget* Child = RootOverlay->GetChildAt(i);
		if (Child == DimWidget)
		{
			Anim.Add(GRAnim::In(Child, 0.f, FVector2D::ZeroVector, 0.2f));
		}
		else
		{
			Anim.Add(GRAnim::Pop(Child, 0.02f, 0.94f, FVector2D(0.f, 24.f), 0.32f));
		}
	}
	if (IntroList)
	{
		Anim.Stagger(IntroList, 0.1f, 0.035f, FVector2D(-18.f, 0.f), 0.22f);
	}
}

void UGRScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Anim.Tick(InDeltaTime);
}

void UGRScreenWidget::Close()
{
	if (UGRUISubsystem* UI = GetUI())
	{
		UI->PopScreen(this);
	}
}

void UGRScreenWidget::HandleBack()
{
	if (bCanClose)
	{
		Close();
	}
}

FReply UGRScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right)
	{
		HandleBack();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

UVerticalBox* UGRScreenWidget::MakeCenteredPanel(UOverlay* Root, const FText& Title, float Width)
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(Width);
	UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size);
	SizeSlot->SetHorizontalAlignment(HAlign_Center);
	SizeSlot->SetVerticalAlignment(VAlign_Center);

	UBorder* Panel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanel, FMargin(32.f, 28.f));
	Size->SetContent(Panel);

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Box);

	UTextBlock* TitleText = GRStyle::MakeText(WidgetTree, Title, 30, GRStyle::ColorAccent, true);
	Box->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
	IntroList = Box;
	return Box;
}

UGRMenuButton* UGRScreenWidget::AddButton(UPanelWidget* Parent, const FText& Label, TFunction<void()> OnClicked)
{
	UGRMenuButton* Button = WidgetTree->ConstructWidget<UGRMenuButton>(UGRMenuButton::StaticClass());
	Button->SetLabel(Label);
	Button->OnClicked.BindLambda(MoveTemp(OnClicked));

	UPanelSlot* PanelSlot = Parent->AddChild(Button);
	if (UVerticalBoxSlot* BoxSlot = Cast<UVerticalBoxSlot>(PanelSlot))
	{
		BoxSlot->SetPadding(FMargin(0.f, 4.f));
	}

	if (!DefaultFocus)
	{
		DefaultFocus = Button;
	}
	return Button;
}
