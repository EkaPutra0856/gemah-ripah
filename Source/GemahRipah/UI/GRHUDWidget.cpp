#include "UI/GRHUDWidget.h"
#include "UI/GRUIStyle.h"
#include "Player/GRCharacter.h"
#include "Player/GRInteractionComponent.h"
#include "Systems/GRProgressSubsystem.h"
#include "Systems/GRRegulationData.h"
#include "Systems/GRRegulationSubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

namespace
{
	constexpr float ToastDuration = 3.5f;
	constexpr int32 MaxToasts = 5;

	FLinearColor ToastColor(EGRNotifyType Type)
	{
		switch (Type)
		{
		case EGRNotifyType::Good: return GRStyle::ColorGood;
		case EGRNotifyType::Bad: return GRStyle::ColorDanger;
		case EGRNotifyType::Warning: return GRStyle::ColorAccent;
		default: return GRStyle::ColorText;
		}
	}

	UOverlaySlot* Place(UOverlay* Root, UWidget* Widget, EHorizontalAlignment H, EVerticalAlignment V, const FMargin& Padding)
	{
		UOverlaySlot* WidgetSlot = Root->AddChildToOverlay(Widget);
		WidgetSlot->SetHorizontalAlignment(H);
		WidgetSlot->SetVerticalAlignment(V);
		WidgetSlot->SetPadding(Padding);
		return WidgetSlot;
	}
}

void UGRHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetVisibility(ESlateVisibility::HitTestInvisible);

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;

	// Top-left: money, meters, active rules.
	UBorder* StatusPanel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanelSoft, FMargin(18.f, 14.f), 8.f);
	Place(Root, StatusPanel, HAlign_Left, VAlign_Top, FMargin(24.f, 24.f, 0.f, 0.f));
	USizeBox* StatusSize = WidgetTree->ConstructWidget<USizeBox>();
	StatusSize->SetWidthOverride(280.f);
	StatusPanel->SetContent(StatusSize);
	UVerticalBox* StatusBox = WidgetTree->ConstructWidget<UVerticalBox>();
	StatusSize->SetContent(StatusBox);

	UangText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 24, GRStyle::ColorText, true);
	StatusBox->AddChildToVerticalBox(UangText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	UTextBlock* Value = nullptr;
	SuaraBar = MakeMeter(StatusBox, LOCTEXT("Suara", "Suara"), GRStyle::ColorInfo, Value);
	SuaraValue = Value;
	DiawasiBar = MakeMeter(StatusBox, LOCTEXT("Diawasi", "Diawasi"), GRStyle::ColorDanger, Value);
	DiawasiValue = Value;

	RuleList = WidgetTree->ConstructWidget<UVerticalBox>();
	StatusBox->AddChildToVerticalBox(RuleList)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

	// Top-right: city.
	CityText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 18, GRStyle::ColorTextDim, true);
	Place(Root, CityText, HAlign_Right, VAlign_Top, FMargin(0.f, 28.f, 32.f, 0.f));

	// Top-center: government banner.
	BannerPanel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanel, FMargin(28.f, 16.f), 8.f);
	Place(Root, BannerPanel, HAlign_Center, VAlign_Top, FMargin(0.f, 110.f, 0.f, 0.f));
	UVerticalBox* BannerBox = WidgetTree->ConstructWidget<UVerticalBox>();
	BannerPanel->SetContent(BannerBox);
	BannerTitle = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 22, GRStyle::ColorAccent, true);
	BannerTitle->SetJustification(ETextJustify::Center);
	BannerBox->AddChildToVerticalBox(BannerTitle)->SetHorizontalAlignment(HAlign_Center);
	BannerBody = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 17);
	BannerBody->SetJustification(ETextJustify::Center);
	BannerBody->SetAutoWrapText(true);
	BannerBox->AddChildToVerticalBox(BannerBody)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	BannerPanel->SetVisibility(ESlateVisibility::Collapsed);

	// Bottom-center: interaction prompt.
	PromptPanel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanelSoft, FMargin(18.f, 10.f), 6.f);
	Place(Root, PromptPanel, HAlign_Center, VAlign_Bottom, FMargin(0.f, 0.f, 0.f, 140.f));
	PromptText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 18);
	PromptPanel->SetContent(PromptText);
	PromptPanel->SetVisibility(ESlateVisibility::Collapsed);

	// Right side: toasts.
	ToastList = WidgetTree->ConstructWidget<UVerticalBox>();
	Place(Root, ToastList, HAlign_Right, VAlign_Top, FMargin(0.f, 70.f, 24.f, 0.f));

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	if (UGRStatusSubsystem* Status = GI->GetSubsystem<UGRStatusSubsystem>())
	{
		Status->OnStatusChanged.AddDynamic(this, &UGRHUDWidget::HandleStatusChanged);
	}
	if (UGRRegulationSubsystem* Regulations = GI->GetSubsystem<UGRRegulationSubsystem>())
	{
		Regulations->OnRegulationEnacted.AddDynamic(this, &UGRHUDWidget::HandleRegulationEnacted);
		Regulations->OnRegulationRevoked.AddDynamic(this, &UGRHUDWidget::HandleRegulationRevoked);
		Regulations->OnRegulationViolated.AddDynamic(this, &UGRHUDWidget::HandleRegulationViolated);
	}
	if (UGRNotificationSubsystem* Notifications = GI->GetSubsystem<UGRNotificationSubsystem>())
	{
		NotifyHandle = Notifications->OnNotify.AddUObject(this, &UGRHUDWidget::HandleNotify);
	}
	if (const UGRProgressSubsystem* Progress = GI->GetSubsystem<UGRProgressSubsystem>())
	{
		CityText->SetText(Progress->GetCityDisplayName(Progress->GetCurrentCityId()));
	}

	RefreshStatus();
	RefreshRules();
}

void UGRHUDWidget::NativeDestruct()
{
	// Subsystems outlive the HUD across map travel.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGRStatusSubsystem* Status = GI->GetSubsystem<UGRStatusSubsystem>())
		{
			Status->OnStatusChanged.RemoveAll(this);
		}
		if (UGRRegulationSubsystem* Regulations = GI->GetSubsystem<UGRRegulationSubsystem>())
		{
			Regulations->OnRegulationEnacted.RemoveAll(this);
			Regulations->OnRegulationRevoked.RemoveAll(this);
			Regulations->OnRegulationViolated.RemoveAll(this);
		}
		if (UGRNotificationSubsystem* Notifications = GI->GetSubsystem<UGRNotificationSubsystem>())
		{
			Notifications->OnNotify.Remove(NotifyHandle);
		}
	}
	Super::NativeDestruct();
}

UProgressBar* UGRHUDWidget::MakeMeter(UVerticalBox* Parent, const FText& Label, const FLinearColor& Color, UTextBlock*& OutValue)
{
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	Parent->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.f, 4.f, 0.f, 2.f));
	Header->AddChildToHorizontalBox(GRStyle::MakeText(WidgetTree, Label, 14, GRStyle::ColorTextDim))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	OutValue = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 14, GRStyle::ColorText, true);
	Header->AddChildToHorizontalBox(OutValue);

	UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>();
	FProgressBarStyle Style = Bar->GetWidgetStyle();
	Style.BackgroundImage = GRStyle::RoundedBrush(FLinearColor(0.f, 0.f, 0.f, 0.55f), 4.f);
	Style.FillImage = GRStyle::RoundedBrush(FLinearColor::White, 4.f);
	Bar->SetWidgetStyle(Style);
	Bar->SetFillColorAndOpacity(Color);
	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>();
	BarSize->SetHeightOverride(12.f);
	BarSize->SetContent(Bar);
	Parent->AddChildToVerticalBox(BarSize);
	return Bar;
}

void UGRHUDWidget::RefreshStatus()
{
	const UGRStatusSubsystem* Status = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGRStatusSubsystem>() : nullptr;
	if (!Status)
	{
		return;
	}
	UangText->SetText(FText::FromString(UGRStatusSubsystem::FormatRupiah(Status->GetUang())));
	SuaraBar->SetPercent(Status->GetSuara() / UGRStatusSubsystem::MeterMax);
	SuaraValue->SetText(FText::AsNumber(FMath::RoundToInt(Status->GetSuara())));
	DiawasiBar->SetPercent(Status->GetDiawasi() / UGRStatusSubsystem::MeterMax);
	DiawasiValue->SetText(FText::AsNumber(FMath::RoundToInt(Status->GetDiawasi())));
}

void UGRHUDWidget::RefreshRules()
{
	RuleList->ClearChildren();
	const UGRRegulationSubsystem* Regulations = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGRRegulationSubsystem>() : nullptr;
	if (!Regulations)
	{
		return;
	}
	for (const UGRRegulationData* Regulation : Regulations->GetActiveRegulations())
	{
		UTextBlock* Line = GRStyle::MakeText(WidgetTree, FText::Format(LOCTEXT("RuleLine", "! {0}"), Regulation->Title), 14, GRStyle::ColorAccent);
		Line->SetAutoWrapText(true);
		RuleList->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 2.f));
	}
}

void UGRHUDWidget::HandleStatusChanged()
{
	RefreshStatus();
}

void UGRHUDWidget::HandleRegulationEnacted(UGRRegulationData* Regulation)
{
	RefreshRules();
	if (Regulation)
	{
		ShowBanner(FText::Format(LOCTEXT("Announcement", "PENGUMUMAN PEMERINTAH: {0}"), Regulation->Title), Regulation->Announcement, GRStyle::ColorAccent, 7.f);
	}
}

void UGRHUDWidget::HandleRegulationRevoked(FGameplayTag RegulationId)
{
	RefreshRules();
}

void UGRHUDWidget::HandleRegulationViolated(UGRRegulationData* Regulation, FGameplayTag Action)
{
	if (Regulation)
	{
		ShowBanner(FText::Format(LOCTEXT("Violation", "MELANGGAR ATURAN: {0}"), Regulation->Title),
			FText::Format(LOCTEXT("ViolationBody", "Diawasi +{0}"), FText::AsNumber(FMath::RoundToInt(Regulation->DiawasiPenalty))), GRStyle::ColorDanger, 4.f);
	}
}

void UGRHUDWidget::HandleNotify(const FText& Message, EGRNotifyType Type)
{
	UBorder* Panel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanelSoft, FMargin(14.f, 8.f), 6.f);
	Panel->SetContent(GRStyle::MakeText(WidgetTree, Message, 16, ToastColor(Type), true));
	UVerticalBoxSlot* ToastSlot = ToastList->AddChildToVerticalBox(Panel);
	ToastSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	ToastSlot->SetHorizontalAlignment(HAlign_Right);

	Toasts.Add({ Panel, FPlatformTime::Seconds() + ToastDuration });
	while (Toasts.Num() > MaxToasts)
	{
		if (UWidget* Old = Toasts[0].Widget.Get())
		{
			Old->RemoveFromParent();
		}
		Toasts.RemoveAt(0);
	}
}

void UGRHUDWidget::ShowBanner(const FText& Title, const FText& Body, const FLinearColor& Color, float Duration)
{
	BannerTitle->SetText(Title);
	BannerTitle->SetColorAndOpacity(FSlateColor(Color));
	BannerBody->SetText(Body);
	BannerPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	BannerEndTime = FPlatformTime::Seconds() + Duration;
}

void UGRHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Real time, so toasts and banners still expire while the game is paused.
	const double Now = FPlatformTime::Seconds();
	if (BannerPanel->GetVisibility() != ESlateVisibility::Collapsed && Now > BannerEndTime)
	{
		BannerPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (int32 Index = Toasts.Num() - 1; Index >= 0; --Index)
	{
		if (Now > Toasts[Index].EndTime)
		{
			if (UWidget* Old = Toasts[Index].Widget.Get())
			{
				Old->RemoveFromParent();
			}
			Toasts.RemoveAt(Index);
		}
	}

	FText Prompt;
	if (const AGRCharacter* Character = Cast<AGRCharacter>(GetOwningPlayerPawn()))
	{
		Prompt = Character->GetInteraction()->GetFocusedPrompt();
	}
	if (Prompt.IsEmpty())
	{
		PromptPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		PromptText->SetText(FText::Format(LOCTEXT("Prompt", "[E]  {0}"), Prompt));
		PromptPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

#undef LOCTEXT_NAMESPACE
