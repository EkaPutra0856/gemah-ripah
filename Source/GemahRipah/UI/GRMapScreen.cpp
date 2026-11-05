#include "UI/GRMapScreen.h"
#include "UI/GRMapView.h"
#include "UI/GRMenuButton.h"
#include "UI/GRUIStyle.h"
#include "Player/GRCharacter.h"
#include "Systems/GRClockSubsystem.h"
#include "Systems/GRProgressSubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "World/GRDistrictVolume.h"
#include "World/GRMapMarkerComponent.h"
#include "World/GRMapSubsystem.h"
#include "World/GROjol.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

namespace
{
	bool HasTrip(const UGROjolSubsystem* Ojol)
	{
		const EGROjolStage Stage = Ojol ? Ojol->GetStage() : EGROjolStage::Offline;
		return Stage == EGROjolStage::ToPickup || Stage == EGROjolStage::Boarding || Stage == EGROjolStage::Riding;
	}
}

UGRMapScreen::UGRMapScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Looking at the phone does not stop the town: the camera shows Bayu holding it.
	bPausesGame = false;
	bDimBackground = false;
}

void UGRMapScreen::BuildScreen(UOverlay* Root)
{
	const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.01f);
	FVector2D Viewport = UWidgetLayoutLibrary::GetViewportSize(this) / Scale;
	if (Viewport.X < 200.0 || Viewport.Y < 200.0)
	{
		Viewport = FVector2D(1920.0, 1080.0);
	}
	// A phone on its side: about 60% of the screen, a little right of centre so Bayu stays in view on the left.
	const float PhoneW = Viewport.X * 0.6f;
	const float PhoneH = FMath::Min(Viewport.Y * 0.66f, PhoneW * 0.56f);

	UBorder* Bezel = GRStyle::MakePanel(WidgetTree, FLinearColor(0.025f, 0.025f, 0.03f, 1.f), FMargin(16.f, 14.f), 36.f);
	UOverlaySlot* PhoneSlot = Root->AddChildToOverlay(Bezel);
	PhoneSlot->SetHorizontalAlignment(HAlign_Center);
	PhoneSlot->SetVerticalAlignment(VAlign_Center);
	PhoneSlot->SetPadding(FMargin(Viewport.X * 0.16f, Viewport.Y * 0.14f, 0.f, 0.f));
	USizeBox* PhoneSize = WidgetTree->ConstructWidget<USizeBox>();
	PhoneSize->SetWidthOverride(PhoneW);
	PhoneSize->SetHeightOverride(PhoneH);
	Bezel->SetContent(PhoneSize);
	UBorder* Glass = GRStyle::MakePanel(WidgetTree, GRStyle::ColorBackground, FMargin(0.f), 20.f);
	PhoneSize->SetContent(Glass);
	UVerticalBox* ScreenBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Glass->SetContent(ScreenBox);

	// Status bar.
	TopBar = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 13, GRStyle::ColorTextDim, true);
	ScreenBox->AddChildToVerticalBox(TopBar)->SetPadding(FMargin(22.f, 8.f, 22.f, 4.f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	UVerticalBoxSlot* RowSlot = ScreenBox->AddChildToVerticalBox(Row);
	RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RowSlot->SetPadding(FMargin(10.f, 0.f, 10.f, 10.f));

	View = CreateWidget<UGRMapView>(this, UGRMapView::StaticClass());
	View->bShowMarkerLabels = true;
	UHorizontalBoxSlot* MapSlot = Row->AddChildToHorizontalBox(View);
	MapSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	USizeBox* SideSize = WidgetTree->ConstructWidget<USizeBox>();
	SideSize->SetWidthOverride(FMath::Clamp(PhoneW * 0.3f, 230.f, 320.f));
	UHorizontalBoxSlot* SideSlot = Row->AddChildToHorizontalBox(SideSize);
	SideSlot->SetPadding(FMargin(14.f, 0.f, 4.f, 0.f));
	UVerticalBox* Side = WidgetTree->ConstructWidget<UVerticalBox>();
	SideSize->SetContent(Side);

	const UGameInstance* GI = GetGameInstance();
	const UGRProgressSubsystem* Progress = GI ? GI->GetSubsystem<UGRProgressSubsystem>() : nullptr;
	const FText City = Progress ? Progress->GetCityDisplayName(Progress->GetCurrentCityId()) : FText::GetEmpty();
	const UGROjolSubsystem* Ojol = UGROjolSubsystem::Get(this);
	const bool bTrip = HasTrip(Ojol);

	TripTitle = GRStyle::MakeText(WidgetTree, FText::Format(LOCTEXT("MapTitle", "Peta {0}"), City), 20, GRStyle::ColorAccent, true);
	TripTitle->SetAutoWrapText(true);
	Side->AddChildToVerticalBox(TripTitle);
	TripBody = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 14, GRStyle::ColorText);
	TripBody->SetAutoWrapText(true);
	Side->AddChildToVerticalBox(TripBody)->SetPadding(FMargin(0.f, 6.f, 0.f, 8.f));

	USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>();
	ListSize->SetHeightOverride(FMath::Max(120.f, PhoneH - (bTrip ? 330.f : 250.f)));
	UScrollBox* List = WidgetTree->ConstructWidget<UScrollBox>();
	ListSize->SetContent(List);
	Side->AddChildToVerticalBox(ListSize);
	if (const UGRMapSubsystem* Map = GetWorld() ? GetWorld()->GetSubsystem<UGRMapSubsystem>() : nullptr)
	{
		List->AddChild(GRStyle::MakeText(WidgetTree, LOCTEXT("Places", "TEMPAT"), 12, GRStyle::ColorTextDim, true));
		for (const TWeakObjectPtr<UGRMapMarkerComponent>& Weak : Map->GetMarkers())
		{
			const UGRMapMarkerComponent* Marker = Weak.Get();
			if (Marker && Marker->bInLegend)
			{
				List->AddChild(GRStyle::MakeText(WidgetTree, FText::Format(LOCTEXT("LegendLine", "[{0}]  {1}"), FText::FromString(Marker->Glyph), Marker->Label), 14, Marker->Color));
			}
		}
		List->AddChild(GRStyle::MakeText(WidgetTree, FText::FromString(TEXT(" ")), 8));
		List->AddChild(GRStyle::MakeText(WidgetTree, LOCTEXT("Districts", "KAWASAN"), 12, GRStyle::ColorTextDim, true));
		for (const TWeakObjectPtr<AGRDistrictVolume>& Weak : Map->GetDistricts())
		{
			if (const AGRDistrictVolume* District = Weak.Get())
			{
				List->AddChild(GRStyle::MakeText(WidgetTree, District->DisplayName, 14, District->MapColor));
			}
		}
	}

	if (bTrip)
	{
		UGRMenuButton* NavButton = AddButton(Side, LOCTEXT("MapNavToggle", "Navigasi / peta penuh"), [this]() { SetNavigation(!bNavigation); });
		Cast<UVerticalBoxSlot>(NavButton->Slot)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
	UGRMenuButton* CloseButton = AddButton(Side, LOCTEXT("MapClose", "Tutup (M)"), [this]() { Close(); });
	Cast<UVerticalBoxSlot>(CloseButton->Slot)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	DefaultFocus = CloseButton;
	SetNavigation(bTrip);
	RefreshTimer = 0.f;
}

void UGRMapScreen::SetNavigation(bool bOn)
{
	bNavigation = bOn;
	if (View)
	{
		// Navigation: a few hundred metres around Bayu, the way a phone shows it.
		View->bFollowPlayer = bOn;
		View->ViewSize = 14000.f;
		View->bShowMarkerLabels = !bOn;
	}
}

void UGRMapScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if ((RefreshTimer -= InDeltaTime) > 0.f)
	{
		return;
	}
	RefreshTimer = 0.25f;
	const UGameInstance* GI = GetGameInstance();
	const UGRClockSubsystem* Clock = GI ? GI->GetSubsystem<UGRClockSubsystem>() : nullptr;
	const UGROjolSubsystem* Ojol = UGROjolSubsystem::Get(this);
	if (TopBar && Clock)
	{
		TopBar->SetText(FText::FromString(FString::Printf(TEXT("%s   |   Hari %d   |   ISPU %d   |   %s"), *Clock->GetClockText().ToString(), Clock->GetDay(),
			FMath::RoundToInt(Clock->GetIspu()), Ojol && Ojol->IsOnline() ? TEXT("OjekKu ONLINE") : TEXT("OjekKu offline"))));
	}
	if (!TripBody || !Ojol)
	{
		return;
	}
	const FGROjolTrip& Trip = Ojol->GetTrip();
	if (HasTrip(Ojol))
	{
		TripTitle->SetText(FText::FromString(Ojol->GetStage() == EGROjolStage::Riding ? FString::Printf(TEXT("Antar ke %s"), *Trip.DestLabel)
			: FString::Printf(TEXT("Jemput %s %s"), *Trip.Honorific, *Trip.Name)));
		const float Meters = Ojol->GetRouteMeters();
		TripBody->SetText(FText::FromString(FString::Printf(TEXT("%s\nSisa %s\nTarif %s (kamu terima %s)"),
			*Ojol->GetTurnText(), Meters >= 1000.f ? *FString::Printf(TEXT("%.1f km"), Meters / 1000.f) : *FString::Printf(TEXT("%d m"), FMath::RoundToInt(Meters)),
			*UGRStatusSubsystem::FormatRupiah(Trip.Fare), *UGRStatusSubsystem::FormatRupiah(FMath::RoundToInt(Trip.Fare * 0.8f)))));
	}
	else if (Ojol->GetStage() == EGROjolStage::Offer)
	{
		TripBody->SetText(FText::FromString(FString::Printf(TEXT("ORDER MASUK: %s %s ke %s. Tutup peta lalu tekan Y."), *Trip.Honorific, *Trip.Name, *Trip.DestLabel)));
	}
	else
	{
		TripBody->SetText(FText::FromString(Ojol->IsOnline() ? Ojol->GetStatusLine() : FString(TEXT("Hidupkan OjekKu di ponsel (Tab) untuk menerima order."))));
	}
}

void UGRMapScreen::NativeConstruct()
{
	Super::NativeConstruct();
	if (AGRCharacter* Bayu = Cast<AGRCharacter>(GetOwningPlayerPawn()))
	{
		Bayu->SetPhoneView(true);
	}
}

void UGRMapScreen::NativeDestruct()
{
	if (AGRCharacter* Bayu = Cast<AGRCharacter>(GetOwningPlayerPawn()))
	{
		Bayu->SetPhoneView(false);
	}
	Super::NativeDestruct();
}

FReply UGRMapScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::M || InKeyEvent.GetKey() == EKeys::Gamepad_Special_Left)
	{
		Close();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
