#include "UI/GRMapScreen.h"
#include "UI/GRMapView.h"
#include "UI/GRMenuButton.h"
#include "UI/GRUIStyle.h"
#include "Systems/GRClockSubsystem.h"
#include "Systems/GRProgressSubsystem.h"
#include "World/GRDistrictVolume.h"
#include "World/GRMapMarkerComponent.h"
#include "World/GRMapSubsystem.h"
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

UGRMapScreen::UGRMapScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPausesGame = true;
}

void UGRMapScreen::BuildScreen(UOverlay* Root)
{
	UBorder* Panel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanel, FMargin(24.f), 10.f);
	UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel);
	PanelSlot->SetHorizontalAlignment(HAlign_Center);
	PanelSlot->SetVerticalAlignment(VAlign_Center);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Panel->SetContent(Row);

	USizeBox* MapSize = WidgetTree->ConstructWidget<USizeBox>();
	MapSize->SetWidthOverride(720.f);
	MapSize->SetHeightOverride(720.f);
	UGRMapView* View = CreateWidget<UGRMapView>(this, UGRMapView::StaticClass());
	View->bShowMarkerLabels = true;
	MapSize->SetContent(View);
	Row->AddChildToHorizontalBox(MapSize);

	USizeBox* SideSize = WidgetTree->ConstructWidget<USizeBox>();
	SideSize->SetWidthOverride(300.f);
	UHorizontalBoxSlot* SideSlot = Row->AddChildToHorizontalBox(SideSize);
	SideSlot->SetPadding(FMargin(24.f, 0.f, 0.f, 0.f));
	UVerticalBox* Side = WidgetTree->ConstructWidget<UVerticalBox>();
	SideSize->SetContent(Side);

	const UGameInstance* GI = GetGameInstance();
	const UGRProgressSubsystem* Progress = GI ? GI->GetSubsystem<UGRProgressSubsystem>() : nullptr;
	const UGRClockSubsystem* Clock = GI ? GI->GetSubsystem<UGRClockSubsystem>() : nullptr;
	const FText City = Progress ? Progress->GetCityDisplayName(Progress->GetCurrentCityId()) : FText::GetEmpty();
	Side->AddChildToVerticalBox(GRStyle::MakeText(WidgetTree, FText::Format(LOCTEXT("MapTitle", "Peta {0}"), City), 28, GRStyle::ColorAccent, true));
	if (Clock)
	{
		Side->AddChildToVerticalBox(GRStyle::MakeText(WidgetTree,
			FText::Format(LOCTEXT("MapClock", "Hari {0}, {1}  |  Udara: {2} (ISPU {3})"), FText::AsNumber(Clock->GetDay()), Clock->GetClockText(),
				Clock->GetSmokeCategory(), FText::AsNumber(FMath::RoundToInt(Clock->GetIspu()))), 14, GRStyle::ColorTextDim))
			->SetPadding(FMargin(0.f, 4.f, 0.f, 14.f));
	}

	USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>();
	ListSize->SetHeightOverride(560.f);
	UScrollBox* List = WidgetTree->ConstructWidget<UScrollBox>();
	ListSize->SetContent(List);
	Side->AddChildToVerticalBox(ListSize);

	if (const UGRMapSubsystem* Map = GetWorld() ? GetWorld()->GetSubsystem<UGRMapSubsystem>() : nullptr)
	{
		List->AddChild(GRStyle::MakeText(WidgetTree, LOCTEXT("Places", "TEMPAT"), 13, GRStyle::ColorTextDim, true));
		for (const TWeakObjectPtr<UGRMapMarkerComponent>& Weak : Map->GetMarkers())
		{
			const UGRMapMarkerComponent* Marker = Weak.Get();
			if (Marker && Marker->bInLegend)
			{
				List->AddChild(GRStyle::MakeText(WidgetTree, FText::Format(LOCTEXT("LegendLine", "[{0}]  {1}"), FText::FromString(Marker->Glyph), Marker->Label), 16, Marker->Color));
			}
		}
		List->AddChild(GRStyle::MakeText(WidgetTree, FText::FromString(TEXT(" ")), 10));
		List->AddChild(GRStyle::MakeText(WidgetTree, LOCTEXT("Districts", "KAWASAN"), 13, GRStyle::ColorTextDim, true));
		for (const TWeakObjectPtr<AGRDistrictVolume>& Weak : Map->GetDistricts())
		{
			if (const AGRDistrictVolume* District = Weak.Get())
			{
				List->AddChild(GRStyle::MakeText(WidgetTree, District->DisplayName, 16, District->MapColor));
			}
		}
	}

	DefaultFocus = AddButton(Side, LOCTEXT("MapClose", "Tutup (M)"), [this]() { Close(); });
	if (UVerticalBoxSlot* CloseSlot = Cast<UVerticalBoxSlot>(DefaultFocus->Slot))
	{
		CloseSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}
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
