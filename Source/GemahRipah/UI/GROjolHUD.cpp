#include "UI/GROjolHUD.h"

#include "UI/GRUIStyle.h"
#include "UI/GRUISubsystem.h"
#include "World/GROjol.h"
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
#include "Engine/LocalPlayer.h"
#include "Rendering/DrawElements.h"

namespace
{
	const FLinearColor OjolGreen = FLinearColor(FColor(0, 170, 90));
	const FLinearColor StarGold = FLinearColor(FColor(255, 196, 40));

	UOverlaySlot* PlaceIn(UOverlay* Root, UWidget* Widget, EHorizontalAlignment H, EVerticalAlignment V, const FMargin& Padding)
	{
		UOverlaySlot* WidgetSlot = Root->AddChildToOverlay(Widget);
		WidgetSlot->SetHorizontalAlignment(H);
		WidgetSlot->SetVerticalAlignment(V);
		WidgetSlot->SetPadding(Padding);
		return WidgetSlot;
	}

	FString Distance(float Meters)
	{
		return Meters >= 1000.f ? FString::Printf(TEXT("%.1f km"), Meters / 1000.f) : FString::Printf(TEXT("%d m"), FMath::RoundToInt(Meters));
	}
}

UGROjolStars::UGROjolStars(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UGROjolStars::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Size(AllottedGeometry.GetLocalSize());
	const float R = FMath::Min(Size.Y * 0.5f, Size.X / 10.f);
	const float Step = Size.X / 5.f;
	for (int32 Star = 0; Star < 5; ++Star)
	{
		const bool bLit = Value - Star >= 0.5f;
		const FLinearColor Color = bLit ? StarGold : FLinearColor(0.35f, 0.32f, 0.28f, 0.9f);
		const FVector2f Center(Step * (Star + 0.5f), Size.Y * 0.5f);
		// Nested outlines fill the star; an unlit one keeps only the outer ring and a faint fill.
		for (float Scale = 1.f; Scale > 0.05f; Scale -= 0.16f)
		{
			TArray<FVector2f> Points;
			for (int32 i = 0; i <= 10; ++i)
			{
				const float Angle = -PI * 0.5f + i * PI / 5.f;
				const float Radius = (i % 2 == 0 ? R : R * 0.45f) * Scale;
				Points.Add(Center + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
			}
			const FLinearColor Draw = bLit || Scale >= 0.99f ? Color : Color.CopyWithNewOpacity(0.25f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Draw, true, FMath::Max(1.5f, R * 0.22f));
		}
	}
	return LayerId + 1;
}

void UGROjolHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;

	auto MakeStars = [this](float StarSize, TObjectPtr<UGROjolStars>& Out)
	{
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
		Box->SetWidthOverride(StarSize * 6.f);
		Box->SetHeightOverride(StarSize * 1.1f);
		Out = CreateWidget<UGROjolStars>(this, UGROjolStars::StaticClass());
		Out->StarSize = StarSize;
		Box->SetContent(Out);
		return Box;
	};
	auto MakeBar = [this](const FLinearColor& Color)
	{
		UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>();
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		Style.BackgroundImage = GRStyle::RoundedBrush(FLinearColor(0.f, 0.f, 0.f, 0.55f), 3.f);
		Style.FillImage = GRStyle::RoundedBrush(FLinearColor::White, 3.f);
		Bar->SetWidgetStyle(Style);
		Bar->SetFillColorAndOpacity(Color);
		return Bar;
	};

	// Incoming order, lower middle.
	OfferPanel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanel, FMargin(22.f, 16.f), 10.f);
	PlaceIn(Root, OfferPanel, HAlign_Center, VAlign_Bottom, FMargin(0.f, 0.f, 0.f, 120.f));
	USizeBox* OfferSize = WidgetTree->ConstructWidget<USizeBox>();
	OfferSize->SetWidthOverride(430.f);
	OfferPanel->SetContent(OfferSize);
	UVerticalBox* OfferBox = WidgetTree->ConstructWidget<UVerticalBox>();
	OfferSize->SetContent(OfferBox);
	UHorizontalBox* OfferHead = WidgetTree->ConstructWidget<UHorizontalBox>();
	OfferBox->AddChildToVerticalBox(OfferHead);
	OfferHead->AddChildToHorizontalBox(GRStyle::MakeText(WidgetTree, FText::FromString(TEXT("OjekKu  |  ORDER MASUK")), 15, OjolGreen, true))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	OfferTimer = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 15, GRStyle::ColorAccent, true);
	OfferHead->AddChildToHorizontalBox(OfferTimer);
	UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	OfferBox->AddChildToVerticalBox(NameRow)->SetPadding(FMargin(0.f, 8.f, 0.f, 4.f));
	OfferName = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 20, GRStyle::ColorText, true);
	NameRow->AddChildToHorizontalBox(OfferName)->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	NameRow->AddChildToHorizontalBox(MakeStars(14.f, OfferStars))->SetVerticalAlignment(VAlign_Center);
	OfferBody = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 15);
	OfferBody->SetAutoWrapText(true);
	OfferBox->AddChildToVerticalBox(OfferBody);
	OfferBar = MakeBar(GRStyle::ColorAccent);
	USizeBox* OfferBarSize = WidgetTree->ConstructWidget<USizeBox>();
	OfferBarSize->SetHeightOverride(8.f);
	OfferBarSize->SetContent(OfferBar);
	OfferBox->AddChildToVerticalBox(OfferBarSize)->SetPadding(FMargin(0.f, 10.f, 0.f, 8.f));
	UTextBlock* Keys = GRStyle::MakeText(WidgetTree, FText::FromString(TEXT("[Y] Terima        [N] Tolak")), 16, GRStyle::ColorAccent, true);
	OfferBox->AddChildToVerticalBox(Keys)->SetHorizontalAlignment(HAlign_Center);

	// Trip bar, top middle under the district title.
	TripPanel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanelSoft, FMargin(20.f, 10.f), 10.f);
	PlaceIn(Root, TripPanel, HAlign_Center, VAlign_Top, FMargin(0.f, 24.f, 0.f, 0.f));
	UVerticalBox* TripBox = WidgetTree->ConstructWidget<UVerticalBox>();
	TripPanel->SetContent(TripBox);
	TripStatus = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 15, OjolGreen, true);
	TripBox->AddChildToVerticalBox(TripStatus)->SetHorizontalAlignment(HAlign_Center);
	UHorizontalBox* TurnRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	TripBox->AddChildToVerticalBox(TurnRow)->SetHorizontalAlignment(HAlign_Center);
	TripTurn = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 22, GRStyle::ColorText, true);
	TurnRow->AddChildToHorizontalBox(TripTurn)->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
	TurnRow->AddChildToHorizontalBox(MakeStars(13.f, TripComfort))->SetVerticalAlignment(VAlign_Center);

	// Chat with the passenger, lower middle.
	ChatPanel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanel, FMargin(20.f, 14.f), 10.f);
	PlaceIn(Root, ChatPanel, HAlign_Center, VAlign_Bottom, FMargin(0.f, 0.f, 0.f, 110.f));
	USizeBox* ChatSize = WidgetTree->ConstructWidget<USizeBox>();
	ChatSize->SetWidthOverride(520.f);
	ChatPanel->SetContent(ChatSize);
	UVerticalBox* ChatBox = WidgetTree->ConstructWidget<UVerticalBox>();
	ChatSize->SetContent(ChatBox);
	ChatPrompt = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 15, GRStyle::ColorAccent, true);
	ChatPrompt->SetAutoWrapText(true);
	ChatBox->AddChildToVerticalBox(ChatPrompt);
	ChatList = WidgetTree->ConstructWidget<UVerticalBox>();
	ChatBox->AddChildToVerticalBox(ChatList)->SetPadding(FMargin(0.f, 6.f, 0.f, 6.f));
	ChatBar = MakeBar(GRStyle::ColorInfo);
	USizeBox* ChatBarSize = WidgetTree->ConstructWidget<USizeBox>();
	ChatBarSize->SetHeightOverride(6.f);
	ChatBarSize->SetContent(ChatBar);
	ChatBox->AddChildToVerticalBox(ChatBarSize);

	// End of the trip, centre.
	SummaryPanel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanel, FMargin(28.f, 20.f), 12.f);
	PlaceIn(Root, SummaryPanel, HAlign_Center, VAlign_Center, FMargin(0.f, 0.f, 0.f, 80.f));
	USizeBox* SummarySize = WidgetTree->ConstructWidget<USizeBox>();
	SummarySize->SetWidthOverride(420.f);
	SummaryPanel->SetContent(SummarySize);
	UVerticalBox* SummaryBox = WidgetTree->ConstructWidget<UVerticalBox>();
	SummarySize->SetContent(SummaryBox);
	SummaryTitle = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 18, OjolGreen, true);
	SummaryBox->AddChildToVerticalBox(SummaryTitle)->SetHorizontalAlignment(HAlign_Center);
	SummaryBox->AddChildToVerticalBox(MakeStars(30.f, SummaryStars))->SetHorizontalAlignment(HAlign_Center);
	SummaryReview = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 15, GRStyle::ColorText);
	SummaryReview->SetAutoWrapText(true);
	SummaryReview->SetJustification(ETextJustify::Center);
	SummaryBox->AddChildToVerticalBox(SummaryReview)->SetPadding(FMargin(0.f, 10.f, 0.f, 8.f));
	SummaryMoney = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 15, GRStyle::ColorAccent, true);
	SummaryMoney->SetJustification(ETextJustify::Center);
	SummaryBox->AddChildToVerticalBox(SummaryMoney)->SetHorizontalAlignment(HAlign_Center);

	// Online pill, bottom left.
	OnlinePill = GRStyle::MakePanel(WidgetTree, FLinearColor(0.f, 0.35f, 0.18f, 0.85f), FMargin(14.f, 6.f), 14.f);
	PlaceIn(Root, OnlinePill, HAlign_Left, VAlign_Bottom, FMargin(24.f, 0.f, 0.f, 24.f));
	OnlineText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 14, GRStyle::ColorText, true);
	OnlinePill->SetContent(OnlineText);

	for (UWidget* Panel : { (UWidget*)OfferPanel, (UWidget*)TripPanel, (UWidget*)ChatPanel, (UWidget*)SummaryPanel, (UWidget*)OnlinePill })
	{
		Panel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGROjolHUD::Refill()
{
	const UGROjolSubsystem* Ojol = UGROjolSubsystem::Get(this);
	if (!Ojol)
	{
		return;
	}
	const FGROjolTrip& Trip = Ojol->GetTrip();
	const EGROjolStage Stage = Ojol->GetStage();
	const bool bOffer = Stage == EGROjolStage::Offer;
	const bool bTrip = Stage == EGROjolStage::ToPickup || Stage == EGROjolStage::Boarding || Stage == EGROjolStage::Riding;
	const bool bSummary = Stage == EGROjolStage::Summary;

	auto Show = [this](UWidget* Panel, bool bOn, bool bPop)
	{
		const bool bWas = Panel->GetVisibility() != ESlateVisibility::Collapsed;
		Panel->SetVisibility(bOn ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bOn && !bWas)
		{
			Anim.Add(bPop ? GRAnim::Pop(Panel) : GRAnim::In(Panel));
		}
	};

	if (bOffer)
	{
		OfferName->SetText(FText::FromString(FString::Printf(TEXT("%s %s"), *Trip.Honorific, *Trip.Name)));
		OfferStars->Value = Trip.PaxRating;
		const float ToPickup = UGROjolSubsystem::RouteLength([&]()
		{
			TArray<FVector2D> Path;
			if (const APawn* Pawn = GetOwningPlayerPawn())
			{
				UGROjolSubsystem::BuildRoute(FVector2D(Pawn->GetActorLocation()), FVector2D(Trip.Pickup), Path);
			}
			return Path;
		}()) / 100.f;
		OfferBody->SetText(FText::FromString(FString::Printf(TEXT("Jemput:  %s  (%s dari sini)\nTujuan:  %s  (%s)\nTarif:  %s   |   kamu terima %s"),
			*Trip.PickupLabel, *Distance(ToPickup), *Trip.DestLabel, *Distance(Trip.Meters),
			*UGRStatusSubsystem::FormatRupiah(Trip.Fare), *UGRStatusSubsystem::FormatRupiah(FMath::RoundToInt(Trip.Fare * 0.8f)))));
	}
	Show(OfferPanel, bOffer, true);
	Show(TripPanel, bTrip, false);

	const bool bChat = Ojol->IsChatOpen();
	if (bChat)
	{
		ChatPrompt->SetText(FText::FromString(FString::Printf(TEXT("%s %s:  \"%s\""), *Trip.Honorific, *Trip.Name, *Ojol->GetChatPrompt())));
		if (Ojol->GetChatPrompt() == TEXT("Sapa penumpang"))
		{
			ChatPrompt->SetText(FText::FromString(FString::Printf(TEXT("Sapa %s %s:"), *Trip.Honorific, *Trip.Name)));
		}
		ChatList->ClearChildren();
		const TArray<FString>& Choices = Ojol->GetChatChoices();
		for (int32 i = 0; i < Choices.Num(); ++i)
		{
			UTextBlock* Line = GRStyle::MakeText(WidgetTree, FText::FromString(FString::Printf(TEXT("[%d]  %s"), i + 1, *Choices[i])), 16, GRStyle::ColorText);
			Line->SetAutoWrapText(true);
			ChatList->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 3.f));
		}
		ChatSeconds = FMath::Max(Ojol->GetChatTimeLeft(), 1.f);
	}
	Show(ChatPanel, bChat, false);

	if (bSummary)
	{
		SummaryTitle->SetText(FText::FromString(Trip.Paid > 0 ? FString::Printf(TEXT("SAMPAI DI %s"), *Trip.DestLabel.ToUpper()) : FString(TEXT("PERJALANAN GAGAL"))));
		SummaryStars->Value = 0.f;
		SummaryReview->SetText(FText::FromString(FString::Printf(TEXT("%s %s memberi %d bintang\n%s"), *Trip.Honorific, *Trip.Name, Trip.Stars, *Trip.Review)));
		SummaryMoney->SetText(FText::FromString(Trip.Paid > 0
			? FString::Printf(TEXT("Tarif %s  -  potongan 20%%  =  %s%s"), *UGRStatusSubsystem::FormatRupiah(Trip.Fare), *UGRStatusSubsystem::FormatRupiah(Trip.Paid),
				Trip.Tip > 0 ? *FString::Printf(TEXT("\n+ tip %s"), *UGRStatusSubsystem::FormatRupiah(Trip.Tip)) : TEXT(""))
			: FString(TEXT("Tidak dibayar."))));
	}
	Show(SummaryPanel, bSummary, true);
}

void UGROjolHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Anim.Tick(InDeltaTime);
	const UGROjolSubsystem* Ojol = UGROjolSubsystem::Get(this);
	const ULocalPlayer* LP = GetOwningLocalPlayer();
	const UGRUISubsystem* UI = LP ? LP->GetSubsystem<UGRUISubsystem>() : nullptr;
	// Menus and dialogs on top: the phone map shows the trip itself.
	SetRenderOpacity(UI && UI->HasScreens() ? 0.f : 1.f);
	if (!Ojol)
	{
		return;
	}
	if (Ojol->GetRevision() != ShownRevision)
	{
		ShownRevision = Ojol->GetRevision();
		Refill();
	}
	const EGROjolStage Stage = Ojol->GetStage();
	const FGROjolTrip& Trip = Ojol->GetTrip();
	if (Stage == EGROjolStage::Offer)
	{
		const float Left = Ojol->GetOfferTimeLeft();
		OfferTimer->SetText(FText::FromString(FString::Printf(TEXT("%d dtk"), FMath::CeilToInt(FMath::Max(Left, 0.f)))));
		OfferBar->SetPercent(Left / UGROjolSubsystem::OfferSeconds);
	}
	if (Stage == EGROjolStage::ToPickup || Stage == EGROjolStage::Boarding || Stage == EGROjolStage::Riding)
	{
		TripStatus->SetText(FText::FromString(Ojol->GetStatusLine()));
		const FString Turn = Ojol->GetTurnText();
		TripTurn->SetText(FText::FromString(Turn.IsEmpty() ? FString() : FString::Printf(TEXT("%s   |   sisa %s"), *Turn, *Distance(Ojol->GetRouteMeters()))));
		TripComfort->Value = Trip.Comfort;
		TripComfort->SetVisibility(Stage == EGROjolStage::Riding ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (Ojol->IsChatOpen())
	{
		ChatBar->SetPercent(Ojol->GetChatTimeLeft() / ChatSeconds);
	}
	if (Stage == EGROjolStage::Summary)
	{
		// Stars light up one by one.
		SummaryStars->Value = FMath::Min(static_cast<float>(Trip.Stars), FMath::FloorToFloat(Ojol->GetSummaryTime() / 0.3f));
	}
	const bool bPill = Stage == EGROjolStage::Waiting;
	if (bPill && OnlinePill->GetVisibility() == ESlateVisibility::Collapsed)
	{
		Anim.Add(GRAnim::Pop(OnlinePill, 0.f, 0.7f, FVector2D(0.f, 18.f), 0.34f));
	}
	OnlinePill->SetVisibility(bPill ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bPill)
	{
		OnlineText->SetText(FText::FromString(TEXT("OjekKu ONLINE  |  ") + Ojol->GetStatusLine()));
	}
}
