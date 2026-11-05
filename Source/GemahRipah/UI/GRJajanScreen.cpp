#include "UI/GRJajanScreen.h"
#include "UI/GRMenuButton.h"
#include "UI/GRUIStyle.h"
#include "Story/GRStorySubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "World/GRCrowd.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

namespace
{
	struct FDish
	{
		const TCHAR* Name;
		const TCHAR* Desc;
		const TCHAR* Icon;
		int32 Price;
		int32 Energi;
		/** Story item it becomes when bagged (its "use" gives the same energy), or null: eaten on the spot anyway. */
		const TCHAR* Item;
		bool bDrink;
		int32 Max;
	};
	const FDish Dishes[] = {
		{ TEXT("Gorengan"), TEXT("Bakwan, tahu isi, tempe."), TEXT("item_gorengan"), 1000, 3, nullptr, false, 10 },
		{ TEXT("Kopi sachet"), TEXT("Diseduh, gelas plastik."), TEXT("item_kopi"), 3000, 18, TEXT("kopi"), true, 3 },
		{ TEXT("Nasi bungkus"), TEXT("Nasi, tempe, sambal."), TEXT("item_nasi_bungkus"), 10000, 40, TEXT("nasi_bungkus"), false, 3 },
	};
	constexpr int32 DishCount = UE_ARRAY_COUNT(Dishes);
	constexpr float PanelWidth = 470.f;
	constexpr float BarWidth = PanelWidth - 44.f;

	FText Rupiah(int32 Amount)
	{
		return FText::FromString(UGRStatusSubsystem::FormatRupiah(Amount));
	}

	void SetBarWidth(USizeBox* Box, float Fraction)
	{
		if (Box)
		{
			Box->SetWidthOverride(FMath::Max(0.f, BarWidth * FMath::Clamp(Fraction, 0.f, 1.f)));
		}
	}

	UOverlay* MakeBar(UWidgetTree* Tree, float Height, const FLinearColor& Back, TArray<TPair<USizeBox*, FLinearColor>> Fills)
	{
		UOverlay* Bar = Tree->ConstructWidget<UOverlay>();
		USizeBox* BackBox = Tree->ConstructWidget<USizeBox>();
		BackBox->SetWidthOverride(BarWidth);
		BackBox->SetHeightOverride(Height);
		UBorder* BackFill = GRStyle::MakePanel(Tree, Back, FMargin(0.f), Height * 0.5f);
		BackBox->SetContent(BackFill);
		Bar->AddChildToOverlay(BackBox);
		for (const TPair<USizeBox*, FLinearColor>& Fill : Fills)
		{
			Fill.Key->SetHeightOverride(Height);
			Fill.Key->SetWidthOverride(0.f);
			Fill.Key->SetContent(GRStyle::MakePanel(Tree, Fill.Value, FMargin(0.f), Height * 0.5f));
			Bar->AddChildToOverlay(Fill.Key)->SetHorizontalAlignment(HAlign_Left);
		}
		return Bar;
	}
}

UGRJajanScreen::UGRJajanScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPausesGame = false;
	bDimBackground = false;
	Amounts.Init(0, DishCount);
}

UBorder* UGRJajanScreen::MakeRoundButton(const FText& Glyph)
{
	// A 30 px round button: size box > rounded border > centered glyph (the stepper adds the size box).
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(30.f);
	Size->SetHeightOverride(30.f);
	UBorder* Button = GRStyle::MakePanel(WidgetTree, FLinearColor::White, FMargin(0.f), 15.f);
	Button->SetBrushColor(GRStyle::ColorPanelSoft);
	Button->SetHorizontalAlignment(HAlign_Center);
	Button->SetVerticalAlignment(VAlign_Center);
	Button->SetContent(GRStyle::MakeText(WidgetTree, Glyph, 18, GRStyle::ColorText, true));
	Size->SetContent(Button);
	return Button;
}

UBorder* UGRJajanScreen::MakePill(const FText& Label, TObjectPtr<UTextBlock>& OutText)
{
	UBorder* Pill = GRStyle::MakePanel(WidgetTree, FLinearColor::White, FMargin(10.f, 7.f), 16.f);
	Pill->SetHorizontalAlignment(HAlign_Center);
	OutText = GRStyle::MakeText(WidgetTree, Label, 14, GRStyle::ColorText, true);
	Pill->SetContent(OutText);
	return Pill;
}

void UGRJajanScreen::BuildScreen(UOverlay* Root)
{
	// Right of center, like the talk menu, so the cart and Bayu stay in view for the hand-over.
	PanelBox = WidgetTree->ConstructWidget<USizeBox>();
	PanelBox->SetWidthOverride(PanelWidth);
	UOverlaySlot* SizeSlot = Root->AddChildToOverlay(PanelBox);
	SizeSlot->SetHorizontalAlignment(HAlign_Right);
	// Low on the right, under where the toasts stack.
	SizeSlot->SetVerticalAlignment(VAlign_Bottom);
	SizeSlot->SetPadding(FMargin(0.f, 0.f, 70.f, 50.f));

	UBorder* Panel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanel, FMargin(22.f, 18.f));
	PanelBox->SetContent(Panel);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Box);

	// Header: cart, name, wallet.
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	Box->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	Header->AddChildToHorizontalBox(GRStyle::MakeIcon(WidgetTree, GRStyle::Icon(TEXT("item_gorengan")), 46.f))->SetVerticalAlignment(VAlign_Center);
	UVerticalBox* Titles = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBoxSlot* TitlesSlot = Header->AddChildToHorizontalBox(Titles);
	TitlesSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	TitlesSlot->SetVerticalAlignment(VAlign_Center);
	TitlesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleText = GRStyle::MakeText(WidgetTree, LOCTEXT("JajanTitle", "Gerobak Jajanan"), 22, GRStyle::ColorAccent, true);
	Titles->AddChildToVerticalBox(TitleText);
	HintText = GRStyle::MakeText(WidgetTree, LOCTEXT("JajanHint", "W/S pilih  ·  A/D jumlah  ·  Tab makan/bungkus"), 12, GRStyle::ColorTextDim);
	Titles->AddChildToVerticalBox(HintText);
	WalletText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 15, GRStyle::ColorText, true);
	Header->AddChildToHorizontalBox(WalletText)->SetVerticalAlignment(VAlign_Top);

	// --- Ordering ----------------------------------------------------------------------------------------------
	OrderBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Box->AddChildToVerticalBox(OrderBox);
	UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>();
	OrderBox->AddChildToVerticalBox(Menu);
	IntroList = Menu;
	for (int32 i = 0; i < DishCount; ++i)
	{
		const FDish& Dish = Dishes[i];
		FRow Row;
		UBorder* Card = GRStyle::MakePanel(WidgetTree, FLinearColor::White, FMargin(10.f, 8.f), 8.f);
		Card->SetBrushColor(GRStyle::ColorButton);
		Menu->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.f, 3.f));
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>();
		Card->SetContent(Line);
		UWidget* Icon = GRStyle::MakeIcon(WidgetTree, GRStyle::Icon(Dish.Icon), 52.f);
		Line->AddChildToHorizontalBox(Icon)->SetVerticalAlignment(VAlign_Center);
		UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* TextsSlot = Line->AddChildToHorizontalBox(Texts);
		TextsSlot->SetPadding(FMargin(12.f, 0.f, 8.f, 0.f));
		TextsSlot->SetVerticalAlignment(VAlign_Center);
		TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Texts->AddChildToVerticalBox(GRStyle::MakeText(WidgetTree, FText::FromString(Dish.Name), 17, GRStyle::ColorText, true));
		Texts->AddChildToVerticalBox(GRStyle::MakeText(WidgetTree, FText::FromString(Dish.Desc), 12, GRStyle::ColorTextDim));
		Texts->AddChildToVerticalBox(GRStyle::MakeText(WidgetTree, FText::Format(LOCTEXT("JajanMeta", "{0}  ·  +{1} energi"),
			Rupiah(Dish.Price), FText::AsNumber(Dish.Energi)), 13, GRStyle::ColorAccent, true));

		UHorizontalBox* Stepper = WidgetTree->ConstructWidget<UHorizontalBox>();
		Line->AddChildToHorizontalBox(Stepper)->SetVerticalAlignment(VAlign_Center);
		UBorder* Minus = MakeRoundButton(LOCTEXT("JajanMinus", "-"));
		Stepper->AddChildToHorizontalBox(Minus->GetParent())->SetVerticalAlignment(VAlign_Center);
		USizeBox* AmountBox = WidgetTree->ConstructWidget<USizeBox>();
		AmountBox->SetWidthOverride(36.f);
		UTextBlock* Amount = GRStyle::MakeText(WidgetTree, FText::AsNumber(0), 20, GRStyle::ColorText, true);
		Amount->SetJustification(ETextJustify::Center);
		Amount->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		AmountBox->SetContent(Amount);
		Stepper->AddChildToHorizontalBox(AmountBox)->SetVerticalAlignment(VAlign_Center);
		UBorder* Plus = MakeRoundButton(LOCTEXT("JajanPlus", "+"));
		Stepper->AddChildToHorizontalBox(Plus->GetParent())->SetVerticalAlignment(VAlign_Center);

		Icon->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Row.Card = Card;
		Row.Icon = Icon;
		Row.Amount = Amount;
		Row.Minus = Minus;
		Row.Plus = Plus;
		Rows.Add(Row);
	}

	// Eat here or take away.
	UHorizontalBox* Pills = WidgetTree->ConstructWidget<UHorizontalBox>();
	OrderBox->AddChildToVerticalBox(Pills)->SetPadding(FMargin(0.f, 10.f, 0.f, 4.f));
	PillEat = MakePill(LOCTEXT("JajanEatHere", "Makan di sini"), PillEatText);
	PillTake = MakePill(LOCTEXT("JajanTakeAway", "Bungkus"), PillTakeText);
	UHorizontalBoxSlot* EatSlot = Pills->AddChildToHorizontalBox(PillEat);
	EatSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	EatSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
	UHorizontalBoxSlot* TakeSlot = Pills->AddChildToHorizontalBox(PillTake);
	TakeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TakeSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 0.f));

	// Total, money left, energy before and after.
	USizeBox* Rule = WidgetTree->ConstructWidget<USizeBox>();
	Rule->SetHeightOverride(1.f);
	Rule->SetContent(GRStyle::MakePanel(WidgetTree, FLinearColor(1.f, 1.f, 1.f, 0.12f), FMargin(0.f), 0.f));
	OrderBox->AddChildToVerticalBox(Rule)->SetPadding(FMargin(0.f, 8.f));
	UHorizontalBox* TotalRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	OrderBox->AddChildToVerticalBox(TotalRow);
	UHorizontalBoxSlot* TotalLabel = TotalRow->AddChildToHorizontalBox(GRStyle::MakeText(WidgetTree, LOCTEXT("JajanTotal", "Total"), 16, GRStyle::ColorTextDim, true));
	TotalLabel->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TotalLabel->SetVerticalAlignment(VAlign_Center);
	TotalText = GRStyle::MakeText(WidgetTree, Rupiah(0), 26, GRStyle::ColorAccent, true);
	TotalText->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
	TotalRow->AddChildToHorizontalBox(TotalText);
	UHorizontalBox* InfoRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	OrderBox->AddChildToVerticalBox(InfoRow)->SetPadding(FMargin(0.f, 2.f, 0.f, 4.f));
	LeftText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 13, GRStyle::ColorTextDim);
	InfoRow->AddChildToHorizontalBox(LeftText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	EnergyText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 13, GRStyle::ColorGood, true);
	InfoRow->AddChildToHorizontalBox(EnergyText);
	EnergyNow = WidgetTree->ConstructWidget<USizeBox>();
	EnergyAfter = WidgetTree->ConstructWidget<USizeBox>();
	OrderBox->AddChildToVerticalBox(MakeBar(WidgetTree, 8.f, GRStyle::ColorPanelSoft,
		{ { EnergyAfter, FLinearColor(GRStyle::ColorGood.R, GRStyle::ColorGood.G, GRStyle::ColorGood.B, 0.4f) }, { EnergyNow, GRStyle::ColorGood } }));
	WarnText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 13, GRStyle::ColorDanger, true);
	WarnText->SetVisibility(ESlateVisibility::Collapsed);
	OrderBox->AddChildToVerticalBox(WarnText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

	USizeBox* Gap = WidgetTree->ConstructWidget<USizeBox>();
	Gap->SetHeightOverride(6.f);
	OrderBox->AddChildToVerticalBox(Gap);
	PayButton = AddIconButton(OrderBox, LOCTEXT("JajanPay", "Bayar  (Enter)"), TEXT("item_amplop"), [this]() { Pay(); });
	AddIconButton(OrderBox, LOCTEXT("JajanCancel", "Nggak jadi  (Esc)"), TEXT("menu_keluar"), [this]() { HandleBack(); });
	// Keys go to the panel itself (W/S, A/D, Enter), not to the first button.
	DefaultFocus = this;

	// --- The sale in progress, then the receipt ---------------------------------------------------------------
	ServeBox = WidgetTree->ConstructWidget<UVerticalBox>();
	ServeBox->SetVisibility(ESlateVisibility::Collapsed);
	Box->AddChildToVerticalBox(ServeBox);
	StageText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 20, GRStyle::ColorText, true);
	ServeBox->AddChildToVerticalBox(StageText);
	StageSub = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 13, GRStyle::ColorTextDim);
	StageSub->SetAutoWrapText(true);
	ServeBox->AddChildToVerticalBox(StageSub)->SetPadding(FMargin(0.f, 2.f, 0.f, 10.f));
	ProgressFill = WidgetTree->ConstructWidget<USizeBox>();
	ServeBox->AddChildToVerticalBox(MakeBar(WidgetTree, 10.f, GRStyle::ColorPanelSoft, { { ProgressFill, GRStyle::ColorAccent } }));
	Basket = WidgetTree->ConstructWidget<UHorizontalBox>();
	ServeBox->AddChildToVerticalBox(Basket)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	ReceiptBox = WidgetTree->ConstructWidget<UVerticalBox>();
	ReceiptBox->SetVisibility(ESlateVisibility::Collapsed);
	ServeBox->AddChildToVerticalBox(ReceiptBox)->SetPadding(FMargin(0.f, 10.f, 0.f, 6.f));
	DoneButton = AddIconButton(ServeBox, LOCTEXT("JajanDone", "Makasih, Pak!  (Enter)"), TEXT("app_pesan"), [this]() { Close(); });
	DoneButton->SetVisibility(ESlateVisibility::Collapsed);
	DefaultFocus = this;

	Refresh();
}

void UGRJajanScreen::SetWarga(AGRWarga* InWarga)
{
	Warga = InWarga;
	if (InWarga)
	{
		InWarga->HoldChat(6.f);
		InWarga->Speak(TEXT("Mau apa, Mas? Gorengan anget, kopi, nasi bungkus ada."), 3.f);
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		SetUserFocus(PC);
	}
	Refresh();
}

void UGRJajanScreen::SetOrder(const TArray<int32>& InAmounts)
{
	for (int32 i = 0; i < DishCount; ++i)
	{
		Change(i, (InAmounts.IsValidIndex(i) ? InAmounts[i] : 0) - Amounts[i]);
	}
}

void UGRJajanScreen::SetTakeAway(bool bInTakeAway)
{
	if (bPaid || bTakeAway == bInTakeAway)
	{
		return;
	}
	bTakeAway = bInTakeAway;
	Anim.Add(GRAnim::Pop(bTakeAway ? PillTake : PillEat, 0.f, 0.9f));
	Refresh();
}

void UGRJajanScreen::Select(int32 Row)
{
	if (Rows.IsValidIndex(Row))
	{
		Selected = Row;
	}
}

void UGRJajanScreen::Change(int32 Row, int32 Delta)
{
	if (bPaid || !Rows.IsValidIndex(Row) || Delta == 0)
	{
		return;
	}
	Select(Row);
	const int32 Before = Amounts[Row];
	Amounts[Row] = FMath::Clamp(Before + Delta, 0, Dishes[Row].Max);
	if (Amounts[Row] == Before)
	{
		// At the limit: a little nudge on the number instead.
		Anim.Add(GRAnim::Pop(Rows[Row].Amount.Get(), 0.f, 0.8f));
		return;
	}
	Anim.Add(GRAnim::Pop(Rows[Row].Amount.Get(), 0.f, Delta > 0 ? 1.5f : 0.6f));
	if (Delta > 0)
	{
		Anim.Add(GRAnim::Pop(Rows[Row].Icon.Get(), 0.f, 1.22f));
	}
	if (WarnText)
	{
		WarnText->SetVisibility(ESlateVisibility::Collapsed);
	}
	Refresh();
}

int32 UGRJajanScreen::Total() const
{
	int32 Sum = 0;
	for (int32 i = 0; i < DishCount; ++i)
	{
		Sum += Amounts[i] * Dishes[i].Price;
	}
	return Sum;
}

int32 UGRJajanScreen::EnergyGain() const
{
	int32 Sum = 0;
	for (int32 i = 0; i < DishCount; ++i)
	{
		if (!bTakeAway || !Dishes[i].Item)
		{
			Sum += Amounts[i] * Dishes[i].Energi;
		}
	}
	return Sum;
}

void UGRJajanScreen::Refresh()
{
	if (!TotalText)
	{
		return;
	}
	const UGameInstance* GI = GetGameInstance();
	const UGRStatusSubsystem* Status = GI ? GI->GetSubsystem<UGRStatusSubsystem>() : nullptr;
	const UGRStorySubsystem* Story = GI ? GI->GetSubsystem<UGRStorySubsystem>() : nullptr;
	const int32 Money = Status ? Status->GetUang() : 0;
	const float Energi = Story ? Story->GetEnergi() : 0.f;
	if (const AGRWarga* Person = Warga.Get())
	{
		TitleText->SetText(Person->GetChatTitle());
	}
	WalletText->SetText(Rupiah(Money));
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		if (UTextBlock* Amount = Rows[i].Amount.Get())
		{
			Amount->SetText(FText::AsNumber(Amounts[i]));
			Amount->SetColorAndOpacity(FSlateColor(Amounts[i] > 0 ? GRStyle::ColorAccent : GRStyle::ColorTextDim));
		}
	}
	const int32 Sum = Total();
	const bool bShort = Sum > Money;
	LeftText->SetText(bShort ? FText::Format(LOCTEXT("JajanShort", "Kurang {0}"), Rupiah(Sum - Money))
		: FText::Format(LOCTEXT("JajanLeft", "Sisa uang {0}"), Rupiah(Money - Sum)));
	LeftText->SetColorAndOpacity(FSlateColor(bShort ? GRStyle::ColorDanger : GRStyle::ColorTextDim));
	const int32 Gain = EnergyGain();
	const int32 After = FMath::Min(100, FMath::RoundToInt(Energi) + Gain);
	EnergyText->SetText(Gain > 0 ? FText::Format(LOCTEXT("JajanEnergy", "Energi {0} > {1}"), FText::AsNumber(FMath::RoundToInt(Energi)), FText::AsNumber(After))
		: FText::Format(LOCTEXT("JajanEnergyNow", "Energi {0}"), FText::AsNumber(FMath::RoundToInt(Energi))));
	PayButton->SetIsEnabled(Sum > 0);

	// Selected pill: accent with dark text.
	for (int32 k = 0; k < 2; ++k)
	{
		const bool bOn = (k == 1) == bTakeAway;
		UBorder* Pill = k == 0 ? PillEat : PillTake;
		UTextBlock* Text = k == 0 ? PillEatText : PillTakeText;
		Pill->SetBrushColor(bOn ? GRStyle::ColorAccent : GRStyle::ColorButton);
		Text->SetColorAndOpacity(FSlateColor(bOn ? GRStyle::ColorBackground : GRStyle::ColorText));
	}
}

void UGRJajanScreen::Shake(const FText& Why)
{
	ShakeTime = 0.45f;
	WarnText->SetText(Why);
	WarnText->SetVisibility(ESlateVisibility::HitTestInvisible);
	Anim.Add(GRAnim::In(WarnText, 0.f, FVector2D(0.f, -6.f), 0.18f));
}

bool UGRJajanScreen::Pay()
{
	if (bPaid)
	{
		return false;
	}
	UGameInstance* GI = GetGameInstance();
	UGRStorySubsystem* Story = GI ? GI->GetSubsystem<UGRStorySubsystem>() : nullptr;
	const UGRStatusSubsystem* Status = GI ? GI->GetSubsystem<UGRStatusSubsystem>() : nullptr;
	AGRWarga* Person = Warga.Get();
	const int32 Sum = Total();
	if (Sum <= 0)
	{
		Shake(LOCTEXT("JajanNothing", "Pilih dulu jajanannya, Mas."));
		return false;
	}
	if (!Story || !Status || !Person || Status->GetUang() < Sum)
	{
		Shake(LOCTEXT("JajanNoMoney", "Uangnya kurang. Kurangi pesanan dulu."));
		if (Person)
		{
			Person->Speak(TEXT("Uangnya kurang, Mas. Ngutang? Wah, jangan deh."), 2.5f);
		}
		return false;
	}
	Story->Run(FString::Printf(TEXT("uang -%d Jajan di gerobak"), Sum));
	bPaid = true;
	PaidTotal = Sum;
	PaidEnergy = EnergyGain();
	bool bOnlyDrink = true;
	for (int32 i = 0; i < DishCount; ++i)
	{
		if (Amounts[i] > 0 && !Dishes[i].bDrink)
		{
			bOnlyDrink = false;
		}
	}
	Person->StartServe(!bTakeAway, bOnlyDrink);

	// What was bought, as icons with their amounts; they pop in again when it is handed over.
	for (int32 i = 0; i < DishCount; ++i)
	{
		if (Amounts[i] <= 0)
		{
			continue;
		}
		UVerticalBox* Item = WidgetTree->ConstructWidget<UVerticalBox>();
		Item->AddChildToVerticalBox(GRStyle::MakeIcon(WidgetTree, GRStyle::Icon(Dishes[i].Icon), 44.f))->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* Count = GRStyle::MakeText(WidgetTree, FText::Format(LOCTEXT("JajanCount", "x{0}"), FText::AsNumber(Amounts[i])), 13, GRStyle::ColorText, true);
		Count->SetJustification(ETextJustify::Center);
		Item->AddChildToVerticalBox(Count)->SetHorizontalAlignment(HAlign_Center);
		Item->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Basket->AddChildToHorizontalBox(Item)->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
	}

	OrderBox->SetVisibility(ESlateVisibility::Collapsed);
	ServeBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Anim.Add(GRAnim::In(ServeBox, 0.05f, FVector2D(24.f, 0.f), 0.25f));
	Anim.Stagger(Basket, 0.15f, 0.06f, FVector2D(0.f, 10.f), 0.22f);
	Anim.Add(GRAnim::Pop(WalletText, 0.f, 1.3f));
	WalletText->SetText(Rupiah(Status->GetUang()));
	return true;
}

void UGRJajanScreen::Deliver()
{
	if (!bPaid || bDelivered)
	{
		return;
	}
	bDelivered = true;
	UGameInstance* GI = GetGameInstance();
	UGRStorySubsystem* Story = GI ? GI->GetSubsystem<UGRStorySubsystem>() : nullptr;
	if (!Story)
	{
		return;
	}
	EnergyBefore = FMath::RoundToInt(Story->GetEnergi());
	TArray<FString> Actions;
	if (PaidEnergy > 0)
	{
		Actions.Add(FString::Printf(TEXT("energi %d"), PaidEnergy));
	}
	if (bTakeAway)
	{
		for (int32 i = 0; i < DishCount; ++i)
		{
			if (Amounts[i] > 0 && Dishes[i].Item)
			{
				Actions.Add(FString::Printf(TEXT("item %s %d"), Dishes[i].Item, Amounts[i]));
			}
		}
	}
	if (!Actions.IsEmpty())
	{
		Story->Run(FString::Join(Actions, TEXT("; ")));
	}
	EnergyAfterMeal = FMath::RoundToInt(Story->GetEnergi());
}

void UGRJajanScreen::ShowReceipt()
{
	bReceipt = true;
	Deliver();
	if (HintText)
	{
		HintText->SetText(LOCTEXT("JajanThanks", "Struk belanja"));
	}
	// Receipt: one line per dish, name left and price right, then the total and what it did.
	auto AddLine = [this](const FText& Left, const FText& Right, const FLinearColor& Color, int32 Size, bool bBold)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>();
		Line->AddChildToHorizontalBox(GRStyle::MakeText(WidgetTree, Left, Size, Color, bBold))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Line->AddChildToHorizontalBox(GRStyle::MakeText(WidgetTree, Right, Size, Color, bBold));
		ReceiptBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 1.f));
	};
	for (int32 i = 0; i < DishCount; ++i)
	{
		if (Amounts[i] > 0)
		{
			AddLine(FText::Format(LOCTEXT("JajanReceiptRow", "{0}  x{1}"), FText::FromString(Dishes[i].Name), FText::AsNumber(Amounts[i])),
				Rupiah(Amounts[i] * Dishes[i].Price), GRStyle::ColorText, 14, false);
		}
	}
	USizeBox* Rule = WidgetTree->ConstructWidget<USizeBox>();
	Rule->SetHeightOverride(1.f);
	Rule->SetContent(GRStyle::MakePanel(WidgetTree, FLinearColor(1.f, 1.f, 1.f, 0.12f), FMargin(0.f), 0.f));
	ReceiptBox->AddChildToVerticalBox(Rule)->SetPadding(FMargin(0.f, 5.f));
	AddLine(LOCTEXT("JajanReceiptTotal", "Total dibayar"), Rupiah(PaidTotal), GRStyle::ColorAccent, 16, true);
	if (PaidEnergy > 0)
	{
		AddLine(LOCTEXT("JajanReceiptEnergy", "Energi"), EnergyAfterMeal > EnergyBefore
			? FText::Format(LOCTEXT("JajanReceiptEnergyUp", "{0} > {1}"), FText::AsNumber(EnergyBefore), FText::AsNumber(EnergyAfterMeal))
			: LOCTEXT("JajanReceiptEnergyFull", "sudah penuh"), GRStyle::ColorGood, 14, true);
	}
	if (bTakeAway)
	{
		UTextBlock* Note = GRStyle::MakeText(WidgetTree, LOCTEXT("JajanReceiptBag", "Yang dibungkus masuk tas. Makan nanti dari Tas."), 12, GRStyle::ColorTextDim);
		Note->SetAutoWrapText(true);
		ReceiptBox->AddChildToVerticalBox(Note)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	ReceiptBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	Anim.Stagger(ReceiptBox, 0.05f, 0.05f, FVector2D(0.f, 8.f), 0.22f);
	DoneButton->SetVisibility(ESlateVisibility::Visible);
	Anim.Add(GRAnim::Pop(DoneButton, 0.15f, 0.9f));
	for (int32 i = 0; i < Basket->GetChildrenCount(); ++i)
	{
		Anim.Add(GRAnim::Pop(Basket->GetChildAt(i), 0.05f * i, 1.35f));
	}
	CloseIn = 6.f;
}

void UGRJajanScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AGRWarga* Person = Warga.Get();
	const APawn* Bayu = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Person || !Bayu || FVector::Dist2D(Person->GetActorLocation(), Bayu->GetActorLocation()) > 450.f || (!bPaid && !Person->CanChat()))
	{
		Close();
		return;
	}
	if (!bPaid)
	{
		Person->HoldChat(1.f);
	}

	// Cards: the picked one lights up and slides a little; hover lights it halfway.
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		FRow& Row = Rows[i];
		const float Want = i == Selected ? 1.f : i == Hovered ? 0.45f : 0.f;
		Row.Highlight = FMath::FInterpTo(Row.Highlight, Want, InDeltaTime, 14.f);
		if (UBorder* Card = Row.Card.Get())
		{
			Card->SetBrushColor(FMath::Lerp(GRStyle::ColorButton, GRStyle::ColorButtonFocus, Row.Highlight));
			Card->SetRenderTranslation(FVector2D(6.f * Row.Highlight, 0.f));
		}
		const bool bRowSelected = i == Selected;
		if (UBorder* Minus = Row.Minus.Get())
		{
			Minus->SetBrushColor(bRowSelected && Amounts[i] > 0 ? GRStyle::ColorAccent * FLinearColor(1.f, 1.f, 1.f, 0.55f) : GRStyle::ColorPanelSoft);
		}
		if (UBorder* Plus = Row.Plus.Get())
		{
			Plus->SetBrushColor(bRowSelected && Amounts[i] < Dishes[i].Max ? GRStyle::ColorAccent : GRStyle::ColorPanelSoft);
		}
	}

	// The total counts up and down instead of jumping.
	const float Target = static_cast<float>(bPaid ? PaidTotal : Total());
	ShownTotal = FMath::Abs(ShownTotal - Target) < 50.f ? Target : FMath::FInterpTo(ShownTotal, Target, InDeltaTime, 12.f);
	if (TotalText)
	{
		TotalText->SetText(Rupiah(FMath::RoundToInt(ShownTotal / 100.f) * 100));
	}
	if (!bPaid)
	{
		const UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>();
		const float Energi = Story ? Story->GetEnergi() / 100.f : 0.f;
		SetBarWidth(EnergyNow, Energi);
		const float After = FMath::Min(1.f, Energi + EnergyGain() / 100.f);
		const float Shown = EnergyAfter ? EnergyAfter->GetWidthOverride() / BarWidth : 0.f;
		SetBarWidth(EnergyAfter, FMath::FInterpTo(Shown, After, InDeltaTime, 10.f));
	}

	if (ShakeTime > 0.f && PanelBox)
	{
		ShakeTime = FMath::Max(0.f, ShakeTime - InDeltaTime);
		PanelBox->SetRenderTranslation(FVector2D(FMath::Sin(ShakeTime * 70.f) * 9.f * (ShakeTime / 0.45f), 0.f));
	}

	if (!bPaid)
	{
		return;
	}
	// Following the sale in the world.
	ServeClock += InDeltaTime;
	const int32 Stage = Person->GetServeStage();
	static const float StageProgress[] = { 0.f, 0.35f, 0.55f, 0.75f, 0.9f, 1.f };
	float Want = StageProgress[FMath::Clamp(Stage, 0, 5)];
	if (Stage == 1)
	{
		// Creeping on while it is made, so the bar never sits still.
		Want = FMath::Min(0.5f, 0.05f + ServeClock * 0.2f);
	}
	Progress = FMath::FInterpTo(Progress, FMath::Max(Progress, Want), InDeltaTime, 6.f);
	SetBarWidth(ProgressFill, Progress);
	if (Stage != LastStage)
	{
		LastStage = Stage;
		FText Title, Sub;
		switch (Stage)
		{
		case 1: Title = LOCTEXT("JajanStage1", "Lagi dibikinin"); Sub = LOCTEXT("JajanStage1Sub", "Pedagang menyiapkan pesananmu di gerobak."); break;
		case 2: Title = LOCTEXT("JajanStage2", "Siap!"); Sub = LOCTEXT("JajanStage2Sub", "Pesanan diangkat dari gerobak..."); break;
		case 3: Title = LOCTEXT("JajanStage3", "Tangkap!"); Sub = LOCTEXT("JajanStage3Sub", "Dilempar lewat atas gerobak."); break;
		case 4: Title = bTakeAway ? LOCTEXT("JajanStage4Take", "Masuk tas") : LOCTEXT("JajanStage4Eat", "Nyam... nyam...");
			Sub = bTakeAway ? LOCTEXT("JajanStage4TakeSub", "Disimpan buat nanti.") : LOCTEXT("JajanStage4EatSub", "Dimakan selagi hangat."); break;
		case 5: Title = bTakeAway ? LOCTEXT("JajanStage5Take", "Beres, sudah dibungkus") : LOCTEXT("JajanStage5Eat", "Kenyang!"); Sub = FText::GetEmpty(); break;
		default: break;
		}
		if (!Title.IsEmpty())
		{
			StageText->SetText(Title);
			StageSub->SetText(Sub);
			Anim.Add(GRAnim::Pop(StageText, 0.f, 1.15f));
		}
		if (Stage == 5)
		{
			ShowReceipt();
		}
	}
	if (Stage == 1)
	{
		// "Lagi dibikinin..." with the dots ticking.
		const int32 Dots = 1 + static_cast<int32>(ServeClock * 3.f) % 3;
		StageText->SetText(FText::Format(LOCTEXT("JajanStage1Dots", "Lagi dibikinin{0}"), FText::FromString(FString::ChrN(Dots, TEXT('.')))));
	}
	if (Stage == 0 && !bReceipt && ServeClock > 0.5f)
	{
		// The sale was cut short (the vendor was bumped or Bayu stepped off): hand it over anyway.
		ShowReceipt();
	}
	if (CloseIn > 0.f)
	{
		CloseIn -= InDeltaTime;
		if (CloseIn <= 0.f)
		{
			Close();
		}
	}
}

void UGRJajanScreen::NativeDestruct()
{
	Deliver();
	if (AGRWarga* Person = Warga.Get())
	{
		Person->HoldChat(1.5f);
	}
	Super::NativeDestruct();
}

FReply UGRJajanScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (bPaid)
	{
		if (bReceipt && (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom))
		{
			Close();
			return FReply::Handled();
		}
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	if (Key == EKeys::W || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up)
	{
		Select((Selected + Rows.Num() - 1) % FMath::Max(1, Rows.Num()));
		return FReply::Handled();
	}
	if (Key == EKeys::S || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down)
	{
		Select((Selected + 1) % FMath::Max(1, Rows.Num()));
		return FReply::Handled();
	}
	if (Key == EKeys::A || Key == EKeys::Left || Key == EKeys::Hyphen || Key == EKeys::Subtract || Key == EKeys::Gamepad_DPad_Left)
	{
		Change(Selected, -1);
		return FReply::Handled();
	}
	if (Key == EKeys::D || Key == EKeys::Right || Key == EKeys::Equals || Key == EKeys::Add || Key == EKeys::Gamepad_DPad_Right)
	{
		Change(Selected, 1);
		return FReply::Handled();
	}
	if (Key == EKeys::Tab || Key == EKeys::Gamepad_FaceButton_Left)
	{
		SetTakeAway(!bTakeAway);
		return FReply::Handled();
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		Pay();
		return FReply::Handled();
	}
	static const FKey NumberKeys[] = { EKeys::One, EKeys::Two, EKeys::Three };
	for (int32 i = 0; i < UE_ARRAY_COUNT(NumberKeys) && i < Rows.Num(); ++i)
	{
		if (Key == NumberKeys[i])
		{
			Change(i, 1);
			return FReply::Handled();
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UGRJajanScreen::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bPaid && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D Point = InMouseEvent.GetScreenSpacePosition();
		auto Under = [&Point](const UWidget* Widget) { return Widget && Widget->GetCachedGeometry().IsUnderLocation(Point); };
		for (int32 i = 0; i < Rows.Num(); ++i)
		{
			if (Under(Rows[i].Minus.Get()))
			{
				Change(i, -1);
				return FReply::Handled();
			}
			if (Under(Rows[i].Plus.Get()))
			{
				Change(i, 1);
				return FReply::Handled();
			}
			if (Under(Rows[i].Card.Get()))
			{
				// A click on the card adds one, like most food apps.
				Change(i, 1);
				return FReply::Handled();
			}
		}
		if (Under(PillEat))
		{
			SetTakeAway(false);
			return FReply::Handled();
		}
		if (Under(PillTake))
		{
			SetTakeAway(true);
			return FReply::Handled();
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UGRJajanScreen::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Hovered = INDEX_NONE;
	const FVector2D Point = InMouseEvent.GetScreenSpacePosition();
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		if (const UBorder* Card = Rows[i].Card.Get(); Card && Card->GetCachedGeometry().IsUnderLocation(Point))
		{
			Hovered = i;
		}
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UGRJajanScreen::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bPaid && Hovered != INDEX_NONE)
	{
		Change(Hovered, InMouseEvent.GetWheelDelta() > 0.f ? 1 : -1);
		return FReply::Handled();
	}
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

#undef LOCTEXT_NAMESPACE
