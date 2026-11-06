#include "UI/GRHUDWidget.h"
#include "World/GRSfx.h"
#include "UI/GRBubbles.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "World/GRStealthLevel.h"
#include "UI/GRStoryScreens.h"
#include "UI/GRUISubsystem.h"
#include "Story/GRStorySubsystem.h"
#include "World/GRNpc.h"
#include "Engine/LocalPlayer.h"
#include "UI/GRUIStyle.h"
#include "Components/Image.h"
#include "Player/GRCharacter.h"
#include "Player/GRInteractionComponent.h"
#include "Systems/GRProgressSubsystem.h"
#include "Systems/GRRegulationData.h"
#include "Systems/GRRegulationSubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "Systems/GRClockSubsystem.h"
#include "UI/GRMapView.h"
#include "World/GRDistrictVolume.h"
#include "World/GRMapSubsystem.h"
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
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Story/GRStoryDirector.h"
#include "UI/GROjolHUD.h"
#include "World/GROjol.h"

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

	const TCHAR* ToastIcon(EGRNotifyType Type)
	{
		switch (Type)
		{
		case EGRNotifyType::Good: return TEXT("status_selesai");
		case EGRNotifyType::Bad: return TEXT("ui_peringatan");
		case EGRNotifyType::Warning: return TEXT("stat_diawasi");
		default: return TEXT("app_pesan");
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

	// Objective waypoint: a diamond over the current target with its distance. Positioned every tick, under the other panels.
	USizeBox* WaypointSize = WidgetTree->ConstructWidget<USizeBox>();
	WaypointSize->SetWidthOverride(140.f);
	Place(Root, WaypointSize, HAlign_Left, VAlign_Top, FMargin(0.f));
	UVerticalBox* WaypointBox = WidgetTree->ConstructWidget<UVerticalBox>();
	WaypointSize->SetContent(WaypointBox);
	USizeBox* DiamondSize = WidgetTree->ConstructWidget<USizeBox>();
	DiamondSize->SetWidthOverride(18.f);
	DiamondSize->SetHeightOverride(18.f);
	WaypointBox->AddChildToVerticalBox(DiamondSize)->SetHorizontalAlignment(HAlign_Center);
	WaypointDiamond = GRStyle::MakePanel(WidgetTree, GRStyle::ColorAccent, FMargin(0.f), 4.f);
	WaypointDiamond->SetRenderTransformAngle(45.f);
	DiamondSize->SetContent(WaypointDiamond);
	WaypointText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 14, GRStyle::ColorAccent, true);
	WaypointText->SetShadowOffset(FVector2D(1.f, 1.f));
	WaypointText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	WaypointBox->AddChildToVerticalBox(WaypointText)->SetHorizontalAlignment(HAlign_Center);
	Waypoint = WaypointSize;
	Waypoint->SetVisibility(ESlateVisibility::Collapsed);

	// Top-left: money, meters, active rules.
	UBorder* StatusPanel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanelSoft, FMargin(18.f, 14.f), 8.f);
	Place(Root, StatusPanel, HAlign_Left, VAlign_Top, FMargin(24.f, 24.f, 0.f, 0.f));
	USizeBox* StatusSize = WidgetTree->ConstructWidget<USizeBox>();
	StatusSize->SetWidthOverride(280.f);
	StatusPanel->SetContent(StatusSize);
	UVerticalBox* StatusBox = WidgetTree->ConstructWidget<UVerticalBox>();
	StatusSize->SetContent(StatusBox);

	UHorizontalBox* UangRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	StatusBox->AddChildToVerticalBox(UangRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	UangRow->AddChildToHorizontalBox(GRStyle::MakeIcon(WidgetTree, GRStyle::Icon(TEXT("stat_uang")), 34.f))->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	UangText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 24, GRStyle::ColorText, true);
	UangRow->AddChildToHorizontalBox(UangText)->SetVerticalAlignment(VAlign_Center);
	UTextBlock* Value = nullptr;
	SuaraBar = MakeMeter(StatusBox, LOCTEXT("Suara", "Suara"), TEXT("stat_suara"), GRStyle::ColorInfo, Value);
	SuaraValue = Value;
	DiawasiBar = MakeMeter(StatusBox, LOCTEXT("Diawasi", "Diawasi"), TEXT("stat_diawasi"), GRStyle::ColorDanger, Value);
	DiawasiValue = Value;
	EnergiBar = MakeMeter(StatusBox, LOCTEXT("Energi", "Energi"), TEXT("stat_energi"), GRStyle::ColorGood, Value);
	EnergiValue = Value;

	ObjectiveText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 15, GRStyle::ColorAccent, true);
	ObjectiveText->SetAutoWrapText(true);
	StatusBox->AddChildToVerticalBox(ObjectiveText)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	AlertText = GRStyle::MakeText(WidgetTree, LOCTEXT("Spotted", "SPN MELIHATMU! Menjauh dari pandangan."), 15, GRStyle::ColorDanger, true);
	AlertText->SetAutoWrapText(true);
	AlertText->SetVisibility(ESlateVisibility::Collapsed);
	StatusBox->AddChildToVerticalBox(AlertText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

	RuleList = WidgetTree->ConstructWidget<UVerticalBox>();
	StatusBox->AddChildToVerticalBox(RuleList)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

	// Top-right: location, clock, air quality and minimap.
	UVerticalBox* RightColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	Place(Root, RightColumn, HAlign_Right, VAlign_Top, FMargin(0.f, 24.f, 24.f, 0.f));
	CityText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 15, GRStyle::ColorTextDim, true);
	CityText->SetJustification(ETextJustify::Right);
	RightColumn->AddChildToVerticalBox(CityText)->SetHorizontalAlignment(HAlign_Right);
	ClockText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 24, GRStyle::ColorText, true);
	RightColumn->AddChildToVerticalBox(ClockText)->SetHorizontalAlignment(HAlign_Right);
	SmokeText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 14, GRStyle::ColorAccent, true);
	RightColumn->AddChildToVerticalBox(SmokeText)->SetHorizontalAlignment(HAlign_Right);
	USizeBox* MinimapSize = WidgetTree->ConstructWidget<USizeBox>();
	MinimapSize->SetWidthOverride(230.f);
	MinimapSize->SetHeightOverride(230.f);
	Minimap = CreateWidget<UGRMapView>(this, UGRMapView::StaticClass());
	Minimap->bFollowPlayer = true;
	Minimap->ViewSize = 32000.f;
	Minimap->bShowDistrictNames = false;
	MinimapSize->SetContent(Minimap);
	RightColumn->AddChildToVerticalBox(MinimapSize)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

	// Upper-center: district name when entering an area.
	DistrictPopup = WidgetTree->ConstructWidget<UVerticalBox>();
	Place(Root, DistrictPopup, HAlign_Center, VAlign_Top, FMargin(0.f, 230.f, 0.f, 0.f));
	DistrictTitle = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 40, GRStyle::ColorText, true);
	DistrictTitle->SetShadowOffset(FVector2D(2.f, 2.f));
	DistrictTitle->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	DistrictPopup->AddChildToVerticalBox(DistrictTitle)->SetHorizontalAlignment(HAlign_Center);
	DistrictSubtitle = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 18, GRStyle::ColorAccent);
	DistrictSubtitle->SetShadowOffset(FVector2D(1.f, 1.f));
	DistrictSubtitle->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	DistrictPopup->AddChildToVerticalBox(DistrictSubtitle)->SetHorizontalAlignment(HAlign_Center);
	DistrictPopup->SetRenderOpacity(0.f);

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


	// OjekKu: order card, trip bar, passenger chat, end-of-trip stars.
	Place(Root, CreateWidget<UGROjolHUD>(this, UGROjolHUD::StaticClass()), HAlign_Fill, VAlign_Fill, FMargin(0.f));

	// Right side: toasts.
	ToastList = WidgetTree->ConstructWidget<UVerticalBox>();
	Place(Root, ToastList, HAlign_Right, VAlign_Top, FMargin(0.f, 400.f, 24.f, 0.f));

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
	if (UGRMapSubsystem* Map = GetWorld() ? GetWorld()->GetSubsystem<UGRMapSubsystem>() : nullptr)
	{
		DistrictHandle = Map->OnDistrictChanged.AddUObject(this, &UGRHUDWidget::HandleDistrictChanged);
	}
	RefreshLocation();
	RefreshClock();

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
	if (UGRMapSubsystem* Map = GetWorld() ? GetWorld()->GetSubsystem<UGRMapSubsystem>() : nullptr)
	{
		Map->OnDistrictChanged.Remove(DistrictHandle);
	}
	Super::NativeDestruct();
}

UProgressBar* UGRHUDWidget::MakeMeter(UVerticalBox* Parent, const FText& Label, const TCHAR* IconName, const FLinearColor& Color, UTextBlock*& OutValue)
{
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	Parent->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.f, 4.f, 0.f, 2.f));
	Header->AddChildToHorizontalBox(GRStyle::MakeIcon(WidgetTree, GRStyle::Icon(IconName), 20.f))->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
	UHorizontalBoxSlot* LabelSlot = Header->AddChildToHorizontalBox(GRStyle::MakeText(WidgetTree, Label, 14, GRStyle::ColorTextDim));
	LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LabelSlot->SetVerticalAlignment(VAlign_Center);
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
	const int32 Uang = Status->GetUang();
	if (ShownUang < 0.f)
	{
		ShownUang = Uang;
		UangText->SetText(FText::FromString(UGRStatusSubsystem::FormatRupiah(Uang)));
	}
	else if (Uang != TargetUang)
	{
		UangFlash = 1.f;
		UangFlashColor = Uang > TargetUang ? GRStyle::ColorGood : GRStyle::ColorDanger;
		UangText->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
		Anim.Add(GRAnim::Pop(UangText, 0.f, 1.18f, FVector2D::ZeroVector, 0.35f));
	}
	TargetUang = Uang;
	SuaraBar->SetPercent(Status->GetSuara() / UGRStatusSubsystem::MeterMax);
	SuaraValue->SetText(FText::AsNumber(FMath::RoundToInt(Status->GetSuara())));
	DiawasiBar->SetPercent(Status->GetDiawasi() / UGRStatusSubsystem::MeterMax);
	DiawasiValue->SetText(FText::AsNumber(FMath::RoundToInt(Status->GetDiawasi())));
}

void UGRHUDWidget::RefreshLocation()
{
	const UGameInstance* GI = GetGameInstance();
	const UGRProgressSubsystem* Progress = GI ? GI->GetSubsystem<UGRProgressSubsystem>() : nullptr;
	const FText City = Progress ? Progress->GetCityDisplayName(Progress->GetCurrentCityId()) : FText::GetEmpty();
	const UGRMapSubsystem* Map = GetWorld() ? GetWorld()->GetSubsystem<UGRMapSubsystem>() : nullptr;
	const AGRDistrictVolume* District = Map ? Map->GetCurrentDistrict() : nullptr;
	CityText->SetText(District ? FText::Format(LOCTEXT("Location", "{0}  |  {1}"), District->DisplayName, City) : City);
}

void UGRHUDWidget::RefreshClock()
{
	const UGRClockSubsystem* Clock = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGRClockSubsystem>() : nullptr;
	if (!Clock)
	{
		return;
	}
	ClockText->SetText(FText::Format(LOCTEXT("Clock", "Hari {0}  {1}"), FText::AsNumber(Clock->GetDay()), Clock->GetClockText()));
	SmokeText->SetText(FText::Format(LOCTEXT("Smoke", "Udara: {0} (ISPU {1})"), Clock->GetSmokeCategory(), FText::AsNumber(FMath::RoundToInt(Clock->GetIspu()))));
	SmokeText->SetColorAndOpacity(FSlateColor(Clock->GetSmokeColor()));

	if (const UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>())
	{
		const float Energi = Story->GetEnergi();
		EnergiBar->SetPercent(Energi / 100.f);
		EnergiBar->SetFillColorAndOpacity(Energi < 25.f ? GRStyle::ColorDanger : GRStyle::ColorGood);
		EnergiValue->SetText(FText::AsNumber(FMath::RoundToInt(Energi)));
		FString Objective = Story->GetObjectiveText();
		if (Story->GetOrderStage() == 2)
		{
			Objective += FString::Printf(TEXT(" (%d dtk)"), FMath::Max(0, FMath::RoundToInt(Story->GetOrderTimeLeft())));
		}
		ObjectiveText->SetText(FText::FromString(Objective.IsEmpty() ? FString() : TEXT("> ") + Objective));
		ObjectiveText->SetVisibility(Objective.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	const bool bSpotted = GetWorld() && GetWorld()->GetTimeSeconds() - AGRNpc::LastSpottedTime < 1.0;
	if (const AGRStealthLevel* Stealth = AGRStealthLevel::GetActive(GetWorld()))
	{
		// Inside the stealth level the alert line becomes the evidence and suspicion meter.
		const int32 Percent = FMath::RoundToInt(Stealth->GetSuspicion());
		AlertText->SetText(FText::FromString(FString::Printf(TEXT("MENYUSUP  |  Bukti %d/%d  |  Kecurigaan %d%%%s"),
			Stealth->GetTaken(), Stealth->GetTotal(), Percent, bSpotted ? TEXT("  - TERLIHAT!") : TEXT(""))));
		AlertText->SetColorAndOpacity(Percent > 0 ? GRStyle::ColorDanger : GRStyle::ColorAccent);
		AlertText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		AlertText->SetText(LOCTEXT("Spotted", "SPN MELIHATMU! Menjauh dari pandangan."));
		AlertText->SetColorAndOpacity(GRStyle::ColorDanger);
		AlertText->SetVisibility(bSpotted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// Cutscenes get the whole screen.
	const ULocalPlayer* LP = GetOwningLocalPlayer();
	const UGRUISubsystem* UI = LP ? LP->GetSubsystem<UGRUISubsystem>() : nullptr;
	const bool bCutscene = UI && Cast<UGRCutsceneScreen>(UI->GetTopScreen());
	SetRenderOpacity(bCutscene ? 0.f : 1.f);
	// The minimap paints itself and ignores render opacity.
	Minimap->SetVisibility(bCutscene ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
}

void UGRHUDWidget::HandleDistrictChanged(AGRDistrictVolume* District, bool bEntered)
{
	RefreshLocation();
	if (District && bEntered)
	{
		DistrictTitle->SetText(FText::FromString(District->DisplayName.ToString().ToUpper()));
		DistrictSubtitle->SetText(District->Subtitle);
		DistrictPopupStart = FPlatformTime::Seconds();
	}
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
	// Several toasts at once (money and energy from one event) ring once.
	const double Now = FPlatformTime::Seconds();
	if (Now - LastNotifySound > 0.3)
	{
		LastNotifySound = Now;
		GRSfx::Play(this, Type == EGRNotifyType::Good ? EGRSfx::NotifBaik : Type == EGRNotifyType::Info ? EGRSfx::NotifInfo : EGRSfx::NotifBuruk);
	}
	// Colored stripe on the left says good / bad / warning at a glance.
	UBorder* Panel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanelSoft, FMargin(0.f), 6.f);
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Panel->SetContent(Row);
	USizeBox* Stripe = WidgetTree->ConstructWidget<USizeBox>();
	Stripe->SetWidthOverride(4.f);
	UBorder* StripeFill = WidgetTree->ConstructWidget<UBorder>();
	StripeFill->SetBrush(GRStyle::RoundedBrush(ToastColor(Type), 2.f));
	Stripe->SetContent(StripeFill);
	Row->AddChildToHorizontalBox(Stripe)->SetVerticalAlignment(VAlign_Fill);
	UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(GRStyle::MakeIcon(WidgetTree, GRStyle::Icon(ToastIcon(Type)), 26.f));
	IconSlot->SetPadding(FMargin(10.f, 6.f, 0.f, 6.f));
	IconSlot->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(GRStyle::MakeText(WidgetTree, Message, 16, ToastColor(Type), true));
	TextSlot->SetPadding(FMargin(10.f, 8.f, 14.f, 8.f));
	TextSlot->SetVerticalAlignment(VAlign_Center);
	UVerticalBoxSlot* ToastSlot = ToastList->AddChildToVerticalBox(Panel);
	ToastSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	ToastSlot->SetHorizontalAlignment(HAlign_Right);
	Panel->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
	Anim.Add(GRAnim::Pop(Panel, 0.f, 0.9f, FVector2D(80.f, 0.f), 0.32f));

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
	Anim.Add(GRAnim::Pop(BannerPanel, 0.f, 0.85f, FVector2D(0.f, -20.f), 0.4f));
	BannerEndTime = FPlatformTime::Seconds() + Duration;
}

void UGRHUDWidget::UpdateWaypoint(float DeltaTime)
{
	APlayerController* PC = GetOwningPlayer();
	const AGRStoryDirector* Director = AGRStoryDirector::Get(this);
	const ULocalPlayer* LP = GetOwningLocalPlayer();
	const UGRUISubsystem* UI = LP ? LP->GetSubsystem<UGRUISubsystem>() : nullptr;
	FVector Target;
	FText Label;
	// An OjekKu trip under way points at the passenger or the drop-off; otherwise the story's goal.
	const UGROjolSubsystem* Ojol = UGROjolSubsystem::Get(this);
	const bool bOjol = Ojol && Ojol->GetTarget(Target, Label);
	// Hidden in menus, dialogs, cutscenes and the stealth rooms (those have their own goals).
	if (!PC || !PC->PlayerCameraManager || !PC->GetPawn() || (!Director && !bOjol) || (UI && UI->HasScreens())
		|| AGRStealthLevel::GetActive(GetWorld()) || (!bOjol && !Director->GetTrackedTarget(Target, Label)))
	{
		Waypoint->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const float Meters = FVector::Dist2D(PC->GetPawn()->GetActorLocation(), Target) / 100.f;
	if (Meters < 4.f)
	{
		Waypoint->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	Target.Z += 260.f;
	const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.01f);
	const FVector2D View = UWidgetLayoutLibrary::GetViewportSize(this) / Scale;
	const FVector2D Half = View * 0.5f;
	constexpr float Margin = 70.f;
	const FVector Local = PC->PlayerCameraManager->GetCameraRotation().UnrotateVector(Target - PC->PlayerCameraManager->GetCameraLocation());
	FVector2D Pos = FVector2D::ZeroVector;
	FVector2D Screen;
	bool bOnScreen = Local.X > 1.f && PC->ProjectWorldLocationToScreen(Target, Screen, true);
	if (bOnScreen)
	{
		Pos = Screen / Scale;
		bOnScreen = Pos.X > Margin && Pos.X < View.X - Margin && Pos.Y > Margin && Pos.Y < View.Y - Margin;
	}
	if (!bOnScreen)
	{
		// Off-screen or behind the camera: pin it to the screen edge in the target's direction.
		FVector2D Dir = FVector2D(Local.Y, -Local.Z).GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			Dir = FVector2D(0.f, 1.f);
		}
		const FVector2D Box = Half - FVector2D(Margin, Margin);
		const float T = FMath::Min(Box.X / FMath::Max(FMath::Abs(Dir.X), 0.001f), Box.Y / FMath::Max(FMath::Abs(Dir.Y), 0.001f));
		Pos = Half + Dir * T;
	}
	WaypointTime += DeltaTime;
	Waypoint->SetVisibility(ESlateVisibility::HitTestInvisible);
	Waypoint->SetRenderTranslation(Pos - FVector2D(70.f, 9.f));
	Waypoint->SetRenderOpacity(bOnScreen ? 1.f : 0.8f);
	const float Pulse = 1.f + 0.18f * FMath::Sin(WaypointTime * 5.f);
	WaypointDiamond->SetRenderScale(FVector2D(Pulse, Pulse));
	const FString Distance = Meters >= 1000.f ? FString::Printf(TEXT("%.1f km"), Meters / 1000.f) : FString::Printf(TEXT("%d m"), FMath::RoundToInt(Meters));
	WaypointText->SetText(FText::FromString(bOnScreen ? Distance : Label.ToString() + TEXT("\n") + Distance));
	WaypointText->SetJustification(ETextJustify::Center);
}

void UGRHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateWaypoint(InDeltaTime);

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
				Anim.Add(GRAnim::Out(Old, FVector2D(60.f, 0.f), 0.25f));
			}
			Toasts.RemoveAt(Index);
		}
	}
	Anim.Tick(InDeltaTime);

	if (ShownUang >= 0.f && (!FMath::IsNearlyEqual(ShownUang, static_cast<float>(TargetUang)) || UangFlash > 0.f))
	{
		ShownUang = FMath::Abs(ShownUang - TargetUang) < 50.f ? TargetUang : FMath::FInterpTo(ShownUang, static_cast<float>(TargetUang), InDeltaTime, 7.f);
		const bool bSettled = FMath::IsNearlyEqual(ShownUang, static_cast<float>(TargetUang));
		UangText->SetText(FText::FromString(UGRStatusSubsystem::FormatRupiah(bSettled ? TargetUang : FMath::RoundToInt(ShownUang / 100.f) * 100)));
		UangFlash = FMath::Max(0.f, UangFlash - InDeltaTime * 1.2f);
		UangText->SetColorAndOpacity(FSlateColor(FMath::Lerp(GRStyle::ColorText, UangFlashColor, UangFlash)));
	}

	// District name: fade in 0.5 s, hold, fade out by 3.5 s.
	const double Since = Now - DistrictPopupStart;
	const float PopupAlpha = Since < 0.5 ? Since / 0.5 : (Since < 2.8 ? 1.0 : FMath::Max(0.0, 1.0 - (Since - 2.8) / 0.7));
	DistrictPopup->SetRenderOpacity(PopupAlpha);

	if (Now >= NextClockRefresh)
	{
		NextClockRefresh = Now + 0.25;
		RefreshClock();
		UGRMapSubsystem* Map = GetWorld() ? GetWorld()->GetSubsystem<UGRMapSubsystem>() : nullptr;
		const APawn* Pawn = GetOwningPlayerPawn();
		if (Map && Pawn && !Map->GetCurrentDistrict())
		{
			Map->UpdateFromLocation(Pawn->GetActorLocation());
		}
	}

	UpdatePrompt(InDeltaTime);
	if (UGRBubbleSubsystem* Bubbles = GetWorld() ? GetWorld()->GetSubsystem<UGRBubbleSubsystem>() : nullptr)
	{
		Bubbles->Prune();
	}
}

void UGRHUDWidget::UpdatePrompt(float DeltaTime)
{
	AActor* Focused = nullptr;
	FText Prompt;
	if (const AGRCharacter* Character = Cast<AGRCharacter>(GetOwningPlayerPawn()))
	{
		Focused = Character->GetInteraction()->GetFocusedActor();
		Prompt = Character->GetInteraction()->GetFocusedPrompt();
	}
	const UGRUISubsystem* PromptUI = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UGRUISubsystem>() : nullptr;
	if (Prompt.IsEmpty() || (PromptUI && PromptUI->GetTopScreen()))
	{
		Focused = nullptr;
	}
	// A new target or a new label pops in again.
	if (Focused != PromptActor.Get() || !Prompt.EqualTo(PromptLabel))
	{
		PromptAge = 0.f;
	}
	PromptActor = Focused;
	PromptLabel = Prompt;
	PromptAge += DeltaTime;
}

namespace
{
	/** Word-wraps Text into lines no wider than MaxWidth. */
	TArray<FString> HudWrapLines(const FString& Text, const FSlateFontInfo& Font, float MaxWidth, const TSharedRef<FSlateFontMeasure>& Measure)
	{
		TArray<FString> Words;
		Text.ParseIntoArrayWS(Words);
		TArray<FString> Lines;
		FString Line;
		for (const FString& Word : Words)
		{
			const FString Try = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
			if (!Line.IsEmpty() && Measure->Measure(Try, Font).X > MaxWidth)
			{
				Lines.Add(Line);
				Line = Word;
			}
			else
			{
				Line = Try;
			}
		}
		if (!Line.IsEmpty())
		{
			Lines.Add(Line);
		}
		return Lines;
	}

	float HudEaseOutBack(float T)
	{
		const float U = T - 1.f;
		return 1.f + 2.7f * U * U * U + 1.7f * U * U;
	}
}

int32 UGRHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	return PaintWorldLabels(AllottedGeometry, OutDrawElements, Layer + 1);
}

int32 UGRHUDWidget::PaintWorldLabels(const FGeometry& Geometry, FSlateWindowElementList& Out, int32 LayerId) const
{
	const APlayerController* PC = GetOwningPlayer();
	const UWorld* World = GetWorld();
	if (!PC || !PC->PlayerCameraManager || !World)
	{
		return LayerId;
	}
	static const FSlateRoundedBoxBrush Rounded(FLinearColor::White, 7.f);
	static const FSlateRoundedBoxBrush Key(FLinearColor::White, 5.f);
	static const FSlateBrush Solid;
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.01f);
	const FVector Camera = PC->PlayerCameraManager->GetCameraLocation();
	auto Project = [&](const FVector& World3, FVector2D& OutPos)
	{
		FVector2D Screen;
		if (!PC->ProjectWorldLocationToScreen(World3, Screen, true))
		{
			return false;
		}
		OutPos = Screen / Scale;
		return true;
	};
	auto Box = [&](int32 Layer, const FSlateBrush* Brush, const FVector2D& Pos, const FVector2D& Size, const FLinearColor& Color, float Angle = 0.f)
	{
		const FPaintGeometry Paint = Angle == 0.f
			? Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Pos)))
			: Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Pos)), FSlateRenderTransform(FQuat2f(FMath::DegreesToRadians(Angle))));
		FSlateDrawElement::MakeBox(Out, Layer, Paint, Brush, ESlateDrawEffect::None, Color);
	};
	auto Text = [&](int32 Layer, const FString& String, const FSlateFontInfo& Font, const FVector2D& Pos, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeText(Out, Layer, Geometry.ToPaintGeometry(FVector2f(Measure->Measure(String, Font)), FSlateLayoutTransform(FVector2f(Pos))), String, Font, ESlateDrawEffect::None, Color);
	};

	// Speech bubbles, far ones first so near ones draw on top.
	if (const UGRBubbleSubsystem* Bubbles = World->GetSubsystem<UGRBubbleSubsystem>())
	{
		struct FDrawn { const FGRBubble* Bubble; FVector2D Pos; float Dist; };
		TArray<FDrawn> Drawn;
		for (const FGRBubble& Bubble : Bubbles->GetBubbles())
		{
			const AActor* Who = Bubble.Who.Get();
			if (!Who || Who->IsHidden())
			{
				continue;
			}
			const FVector Spot = Who->GetActorLocation() + FVector(0.f, 0.f, Bubble.Height);
			const float Dist = FVector::Dist(Camera, Spot);
			FVector2D Pos;
			if (Dist < 4200.f && Project(Spot, Pos))
			{
				Drawn.Add({ &Bubble, Pos, Dist });
			}
		}
		Drawn.Sort([](const FDrawn& A, const FDrawn& B) { return A.Dist > B.Dist; });
		const double Now = World->GetTimeSeconds();
		int32 Layer = LayerId;
		for (const FDrawn& D : Drawn)
		{
			const FGRBubble& B = *D.Bubble;
			const float Age = float(Now - B.Start);
			const float Near = FMath::GetMappedRangeValueClamped(FVector2f(500.f, 3500.f), FVector2f(1.f, 0.f), D.Dist);
			const float Alpha = FMath::Clamp(Age / 0.12f, 0.f, 1.f) * FMath::Clamp((B.Duration - Age) / 0.35f, 0.f, 1.f)
				* FMath::GetMappedRangeValueClamped(FVector2f(3400.f, 4200.f), FVector2f(1.f, 0.f), D.Dist);
			if (Alpha <= 0.01f)
			{
				continue;
			}
			const FSlateFontInfo Font = GRStyle::Font(FMath::RoundToInt((B.bShout ? 15.f : 12.f) + 4.f * Near), B.bShout);
			const TArray<FString> Lines = HudWrapLines(B.Text, Font, 250.f, Measure);
			const float LineH = Measure->Measure(TEXT("Ag"), Font).Y;
			float Width = 0.f;
			for (const FString& Line : Lines)
			{
				Width = FMath::Max(Width, float(Measure->Measure(Line, Font).X));
			}
			const FVector2D Size(Width + 22.f, Lines.Num() * LineH + 12.f);
			// Pops up from below; a shout shakes for half a second.
			FVector2D Anchor = D.Pos + FVector2D(0.f, (1.f - HudEaseOutBack(FMath::Clamp(Age / 0.2f, 0.f, 1.f))) * 14.f);
			if (B.bShout && Age < 0.5f)
			{
				Anchor.X += FMath::Sin(Age * 70.f) * 4.f * (1.f - Age / 0.5f);
			}
			const FVector2D TopLeft = Anchor - FVector2D(Size.X * 0.5f, Size.Y + 9.f);
			const bool bPlain = B.Color.Equals(FLinearColor::White, 0.02f);
			const FLinearColor Edge = (bPlain ? GRStyle::ColorAccent : B.Color).CopyWithNewOpacity(Alpha * (B.bShout ? 1.f : 0.75f));
			const FLinearColor Fill = GRStyle::ColorPanel.CopyWithNewOpacity(0.93f * Alpha);
			const float Border = B.bShout ? 2.5f : 1.5f;
			Box(Layer, &Solid, Anchor - FVector2D(7.f, 16.f), FVector2D(14.f, 14.f), Edge, 45.f);
			Box(Layer, &Rounded, TopLeft - FVector2D(Border, Border), Size + FVector2D(Border * 2.f, Border * 2.f), Edge);
			Box(Layer, &Solid, Anchor - FVector2D(5.5f, 16.f - Border), FVector2D(11.f, 11.f), Fill, 45.f);
			Box(Layer, &Rounded, TopLeft, Size, Fill);
			const FLinearColor Ink = (bPlain ? GRStyle::ColorText : B.Color).CopyWithNewOpacity(Alpha);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				const float LineW = Measure->Measure(Lines[i], Font).X;
				Text(Layer, Lines[i], Font, FVector2D(Anchor.X - LineW * 0.5f, TopLeft.Y + 6.f + i * LineH), Ink);
			}
			++Layer;
		}
		LayerId = Layer;
	}

	// [E] prompt beside the focused thing: a key cap and its label, sliding in.
	const AActor* Target = PromptActor.Get();
	if (!Target || PromptLabel.IsEmpty())
	{
		return LayerId;
	}
	const FBox Bounds = Target->GetComponentsBoundingBox(false);
	const FVector Center = Bounds.IsValid ? Bounds.GetCenter() : Target->GetActorLocation();
	const float Top = Bounds.IsValid ? FMath::Min(Bounds.Max.Z, Bounds.Min.Z + 200.f) : Center.Z + 60.f;
	FVector2D Pos;
	const FVector2D View = UWidgetLayoutLibrary::GetViewportSize(this) / Scale;
	if (!Project(FVector(Center.X, Center.Y, (Center.Z + Top) * 0.5f), Pos) || Pos.X < 40.f || Pos.X > View.X - 300.f || Pos.Y < 40.f || Pos.Y > View.Y - 40.f)
	{
		Pos = FVector2D(View.X * 0.5f - 60.f, View.Y - 170.f);
	}
	else
	{
		Pos.X += 34.f;
	}
	const float In = FMath::Clamp(PromptAge / 0.22f, 0.f, 1.f);
	const float Alpha = In;
	Pos.X += (1.f - HudEaseOutBack(In)) * -18.f;
	const FString Label = PromptLabel.ToString();
	const FSlateFontInfo LabelFont = GRStyle::Font(16);
	const FSlateFontInfo KeyFont = GRStyle::Font(16, true);
	const FVector2D LabelSize = FVector2D(Measure->Measure(Label, LabelFont)) + FVector2D(26.f, 12.f);
	const float KeySize = 30.f;
	const FVector2D KeyPos = Pos - FVector2D(0.f, KeySize * 0.5f + FMath::Sin(PromptAge * 4.f) * 1.5f);
	// Key cap: a darker base under a lighter top, like a real key.
	Box(LayerId, &Rounded, FVector2D(Pos.X + KeySize * 0.5f, Pos.Y - LabelSize.Y * 0.5f), LabelSize + FVector2D(KeySize * 0.5f + 8.f, 0.f), GRStyle::ColorPanel.CopyWithNewOpacity(0.9f * Alpha));
	Box(LayerId, &Key, KeyPos + FVector2D(0.f, 3.f), FVector2D(KeySize, KeySize), (GRStyle::ColorAccent * 0.55f).CopyWithNewOpacity(Alpha));
	Box(LayerId, &Key, KeyPos, FVector2D(KeySize, KeySize), GRStyle::ColorAccent.CopyWithNewOpacity(Alpha));
	const FVector2D KeyText = FVector2D(Measure->Measure(TEXT("E"), KeyFont));
	Text(LayerId + 1, TEXT("E"), KeyFont, KeyPos + (FVector2D(KeySize, KeySize) - KeyText) * 0.5f, FLinearColor(0.08f, 0.06f, 0.04f, Alpha));
	Text(LayerId + 1, Label, LabelFont, FVector2D(Pos.X + KeySize + 13.f, Pos.Y - LabelSize.Y * 0.5f + 6.f), GRStyle::ColorText.CopyWithNewOpacity(Alpha));
	return LayerId + 1;
}

#undef LOCTEXT_NAMESPACE
