#include "UI/GRStoryScreens.h"
#include "UI/GRMapScreen.h"
#include "UI/GRMenuButton.h"
#include "UI/GRUIStyle.h"
#include "UI/GRUISubsystem.h"
#include "Story/GRStorySubsystem.h"
#include "Systems/GRClockSubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

namespace
{
	UGRStorySubsystem* StoryFor(const UUserWidget* Widget)
	{
		const UGameInstance* GI = Widget ? Widget->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UGRStorySubsystem>() : nullptr;
	}

	UGRStorySubsystem* StoryFor(const UGRUISubsystem* UI)
	{
		const ULocalPlayer* LP = UI ? UI->GetLocalPlayer() : nullptr;
		return LP && LP->GetGameInstance() ? LP->GetGameInstance()->GetSubsystem<UGRStorySubsystem>() : nullptr;
	}

	UTextBlock* WrapText(UTextBlock* Text)
	{
		Text->SetAutoWrapText(true);
		return Text;
	}
}

// ---------------------------------------------------------------------------------------------
// Phone

UGRPhoneScreen::UGRPhoneScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bDimBackground = false;
}

void UGRPhoneScreen::BuildScreen(UOverlay* Root)
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(440.f);
	Size->SetHeightOverride(780.f);
	UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size);
	SizeSlot->SetHorizontalAlignment(HAlign_Right);
	SizeSlot->SetVerticalAlignment(VAlign_Center);
	SizeSlot->SetPadding(FMargin(0.f, 0.f, 120.f, 0.f));
	UBorder* Phone = GRStyle::MakePanel(WidgetTree, FLinearColor(0.03f, 0.03f, 0.04f, 0.98f), FMargin(18.f, 14.f), 30.f);
	Size->SetContent(Phone);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Phone->SetContent(Box);
	StatusBar = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 12, GRStyle::ColorTextDim);
	Box->AddChildToVerticalBox(StatusBar)->SetHorizontalAlignment(HAlign_Center);
	AppTitle = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 24, GRStyle::ColorAccent, true);
	Box->AddChildToVerticalBox(AppTitle)->SetPadding(FMargin(4.f, 8.f, 0.f, 8.f));
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Box->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(Content);
	Box->AddChildToVerticalBox(GRStyle::MakeText(WidgetTree, LOCTEXT("PhoneHint", "Tab: tutup  |  Esc: kembali"), 12, GRStyle::ColorTextDim))->SetHorizontalAlignment(HAlign_Center);
}

void UGRPhoneScreen::OnShown()
{
	ShowApp(CurrentApp);
	Super::OnShown();
}

void UGRPhoneScreen::AddLine(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	Content->AddChildToVerticalBox(WrapText(GRStyle::MakeText(WidgetTree, FText::FromString(Text), Size, Color, bBold)))->SetPadding(FMargin(2.f, 2.f));
}

void UGRPhoneScreen::AddCard(const FString& Header, const FString& Body, const FLinearColor& HeaderColor)
{
	UBorder* Card = GRStyle::MakePanel(WidgetTree, FLinearColor(0.11f, 0.11f, 0.13f, 1.f), FMargin(12.f, 10.f), 10.f);
	UVerticalBox* CardBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Card->SetContent(CardBox);
	CardBox->AddChildToVerticalBox(WrapText(GRStyle::MakeText(WidgetTree, FText::FromString(Header), 13, HeaderColor, true)));
	CardBox->AddChildToVerticalBox(WrapText(GRStyle::MakeText(WidgetTree, FText::FromString(Body), 15, GRStyle::ColorText)))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	Content->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.f, 4.f));
}

void UGRPhoneScreen::ShowApp(const FString& App)
{
	UGRStorySubsystem* Story = StoryFor(this);
	const UGameInstance* GI = GetGameInstance();
	const UGRClockSubsystem* Clock = GI->GetSubsystem<UGRClockSubsystem>();
	UGRStatusSubsystem* Status = GI->GetSubsystem<UGRStatusSubsystem>();
	CurrentApp = App;
	Content->ClearChildren();
	DefaultFocus = nullptr;
	StatusBar->SetText(FText::FromString(FString::Printf(TEXT("%s   Hari %d   ISPU %d   Baterai %d%%"), *Clock->GetClockText().ToString(), Clock->GetDay(),
		FMath::RoundToInt(Clock->GetIspu()), FMath::Clamp(100 - Clock->GetHour() * 3, 12, 100))));

	if (App.IsEmpty())
	{
		AppTitle->SetText(LOCTEXT("PhoneHome", "Ponsel Bayu"));
		int32 Unread = 0;
		for (const FString& Id : Story->GetInbox())
		{
			Unread += Story->GetVar(TEXT("baca.") + Id) == 0 ? 1 : 0;
		}
		const TPair<FString, FString> Apps[] = {
			{ TEXT("ojekku"), Story->GetActiveOrder() ? TEXT("OjekKu (order aktif)") : TEXT("OjekKu") },
			{ TEXT("pesan"), Unread > 0 ? FString::Printf(TEXT("Pesan (%d baru)"), Unread) : TEXT("Pesan") },
			{ TEXT("kicau"), TEXT("Kicau") },
			{ TEXT("berita"), TEXT("Berita") },
			{ TEXT("dompet"), TEXT("Dompet") },
			{ TEXT("galeri"), TEXT("Galeri / Kamera") },
			{ TEXT("peta"), TEXT("Peta") },
		};
		for (const TPair<FString, FString>& Entry : Apps)
		{
			const FString Id = Entry.Key;
			AddButton(Content, FText::FromString(Entry.Value), [this, Id]()
			{
				if (Id == TEXT("peta"))
				{
					GetUI()->PushScreen<UGRMapScreen>();
					return;
				}
				ShowApp(Id);
				Super::OnShown();
			});
		}
		return;
	}

	AddButton(Content, LOCTEXT("PhoneBack", "< Kembali"), [this]() { ShowApp(FString()); Super::OnShown(); });

	if (App == TEXT("ojekku"))
	{
		AppTitle->SetText(LOCTEXT("AppOjek", "OjekKu Driver"));
		if (Story->GetVar(TEXT("ojek_blokir")) > 0)
		{
			AddLine(TEXT("Akun driver dibekukan sementara karena \"aktivitas mencurigakan\". Hubungi CS (antrean: 4.812)."), 15, GRStyle::ColorDanger, true);
			return;
		}
		if (const FGROrderDef* Order = Story->GetActiveOrder())
		{
			AddCard(FString::Printf(TEXT("ORDER AKTIF  |  %s"), *Order->Customer), Story->GetObjectiveText(), GRStyle::ColorGood);
			if (Story->GetOrderStage() == 2)
			{
				AddLine(FString::Printf(TEXT("Sisa waktu: %d detik"), FMath::Max(0, FMath::RoundToInt(Story->GetOrderTimeLeft()))), 15, GRStyle::ColorAccent);
			}
			AddButton(Content, LOCTEXT("OrderCancel", "Batalkan order"), [this]() { StoryFor(this)->CancelOrder(); ShowApp(TEXT("ojekku")); Super::OnShown(); });
			return;
		}
		AddLine(FString::Printf(TEXT("Order selesai: %d   |   Potongan aplikasi: 20%%"), Story->GetVar(TEXT("orders_done"))), 13, GRStyle::ColorTextDim);
		bool bAny = false;
		for (const FGROrderDef& Order : Story->GetData().Orders)
		{
			if (!Story->IsOrderAvailable(Order))
			{
				continue;
			}
			bAny = true;
			const FString Id = Order.Id;
			AddButton(Content, FText::FromString(FString::Printf(TEXT("%s  -  %s\n%s"), *Order.Title, *UGRStatusSubsystem::FormatRupiah(Order.Fare), *Order.Customer)), [this, Id]()
			{
				StoryFor(this)->StartOrder(Id);
				Close();
			});
		}
		if (!bAny)
		{
			AddLine(TEXT("Belum ada order di sekitarmu. Coba lagi besok, atau tunggu jam sibuk."), 15, GRStyle::ColorTextDim);
		}
	}
	else if (App == TEXT("pesan"))
	{
		AppTitle->SetText(LOCTEXT("AppMsg", "Pesan"));
		const TArray<FString>& Inbox = Story->GetInbox();
		if (Inbox.Num() == 0)
		{
			AddLine(TEXT("Belum ada pesan."), 15, GRStyle::ColorTextDim);
		}
		for (int32 i = Inbox.Num() - 1; i >= 0; --i)
		{
			const FString Id = Inbox[i];
			const FGRDialogDef* Dialog = Story->FindDialog(Id);
			if (!Dialog || Dialog->Nodes.Num() == 0)
			{
				continue;
			}
			const FGRSpeakerDef* Speaker = Story->FindSpeaker(Dialog->Nodes[0].Speaker);
			const bool bUnread = Story->GetVar(TEXT("baca.") + Id) == 0;
			const FString Preview = Dialog->Nodes[0].Text.Left(34);
			AddButton(Content, FText::FromString(FString::Printf(TEXT("%s%s\n%s..."), bUnread ? TEXT("(baru) ") : TEXT(""), Speaker ? *Speaker->Name : TEXT("?"), *Preview)), [this, Id]()
			{
				UGRDialogScreen::Open(GetUI(), Id);
			});
		}
	}
	else if (App == TEXT("kicau"))
	{
		AppTitle->SetText(LOCTEXT("AppKicau", "Kicau"));
		if (Story->GetVar(TEXT("kicau_beku")) > 0)
		{
			AddLine(TEXT("Akun @bayu_ngojek dibekukan: melanggar Pasal Penghinaan Pejabat. Kicauanmu tidak bisa dilihat lagi."), 14, GRStyle::ColorDanger, true);
		}
		const TArray<FGRPostDef>& Posts = Story->GetData().Kicau;
		for (int32 i = Posts.Num() - 1; i >= 0; --i)
		{
			const FGRPostDef& Post = Posts[i];
			if (Story->Check(Post.If))
			{
				AddCard(FString::Printf(TEXT("%s%s  @%s  |  %d suka"), *Post.Author, Post.Official ? TEXT(" [Resmi]") : TEXT(""), *Post.Handle, Post.Likes),
					Post.Text, Post.Official ? GRStyle::ColorInfo : GRStyle::ColorAccent);
			}
		}
	}
	else if (App == TEXT("berita"))
	{
		AppTitle->SetText(LOCTEXT("AppNews", "Berita"));
		const TArray<FGRNewsDef>& News = Story->GetData().Berita;
		for (int32 i = News.Num() - 1; i >= 0; --i)
		{
			if (Story->Check(News[i].If))
			{
				AddCard(FString::Printf(TEXT("%s  |  %s"), *News[i].Source, *News[i].Title), News[i].Body, GRStyle::ColorAccent);
			}
		}
	}
	else if (App == TEXT("dompet"))
	{
		AppTitle->SetText(LOCTEXT("AppWallet", "Dompet"));
		AddLine(UGRStatusSubsystem::FormatRupiah(Status->GetUang()), 30, GRStyle::ColorGood, true);
		const TArray<FString>& Ledger = Story->GetLedger();
		if (Ledger.Num() == 0)
		{
			AddLine(TEXT("Belum ada transaksi."), 14, GRStyle::ColorTextDim);
		}
		for (int32 i = Ledger.Num() - 1; i >= 0; --i)
		{
			TArray<FString> Parts;
			Ledger[i].ParseIntoArray(Parts, TEXT("|"), false);
			if (Parts.Num() == 3)
			{
				const int32 Amount = FCString::Atoi(*Parts[2]);
				AddLine(FString::Printf(TEXT("%s  %s  %s%s"), *Parts[0], *Parts[1], Amount > 0 ? TEXT("+") : TEXT(""), *UGRStatusSubsystem::FormatRupiah(Amount)), 14,
					Amount >= 0 ? GRStyle::ColorGood : GRStyle::ColorDanger);
			}
		}
	}
	else if (App == TEXT("galeri"))
	{
		AppTitle->SetText(LOCTEXT("AppGallery", "Galeri"));
		AddLine(TEXT("Dekati sesuatu yang janggal lalu tekan E untuk memotret atau mengambilnya."), 13, GRStyle::ColorTextDim);
		bool bAny = false;
		for (const FGRItemDef& Item : Story->GetData().Items)
		{
			if (Item.Bukti && Story->GetItemCount(Item.Id) > 0)
			{
				bAny = true;
				AddCard(FString::Printf(TEXT("[Bukti] %s"), *Item.Name), Item.Desc, GRStyle::ColorDanger);
			}
		}
		if (!bAny)
		{
			AddLine(TEXT("Galeri kosong. Isinya foto kucing dan tangkapan layar promo kuota."), 15, GRStyle::ColorText);
		}
	}

	// Inside an app, start on its first real entry rather than "< Kembali" (Esc already goes back).
	if (!App.IsEmpty())
	{
		for (int32 i = 1; i < Content->GetChildrenCount(); ++i)
		{
			if (UGRMenuButton* Button = Cast<UGRMenuButton>(Content->GetChildAt(i)))
			{
				DefaultFocus = Button;
				break;
			}
		}
	}
}

FReply UGRPhoneScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Tab || InKeyEvent.GetKey() == EKeys::Gamepad_DPad_Up)
	{
		Close();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGRPhoneScreen::HandleBack()
{
	if (!CurrentApp.IsEmpty())
	{
		ShowApp(FString());
		Super::OnShown();
		return;
	}
	Close();
}

// ---------------------------------------------------------------------------------------------
// Warnet post builder

FString UGRWarnetScreen::PendingId;

UGRWarnetScreen::UGRWarnetScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPausesGame = true;
}

bool UGRWarnetScreen::Open(UGRUISubsystem* UI, const FString& MinigameId)
{
	const UGRStorySubsystem* Story = StoryFor(UI);
	if (!Story || !Story->FindMinigame(MinigameId))
	{
		return false;
	}
	PendingId = MinigameId;
	return UI->PushScreen<UGRWarnetScreen>() != nullptr;
}

void UGRWarnetScreen::BuildScreen(UOverlay* Root)
{
	MinigameId = PendingId;
	const FGRMinigameDef* Def = StoryFor(this)->FindMinigame(MinigameId);
	UVerticalBox* Panel = MakeCenteredPanel(Root, FText::FromString(Def->Title), 940.f);
	// Up to five evidence items: scroll instead of running off the screen, and follow keyboard focus.
	USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>();
	ListSize->SetMaxDesiredHeight(640.f);
	Panel->AddChildToVerticalBox(ListSize);
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Scroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll);
	ListSize->SetContent(Scroll);
	Body = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(Body);
	Rebuild();
}

void UGRWarnetScreen::Rebuild()
{
	UGRStorySubsystem* Story = StoryFor(this);
	const FGRMinigameDef* Def = Story->FindMinigame(MinigameId);
	// Remember which button had focus, so a choice does not send keyboard focus back to the top.
	int32 FocusIndex = INDEX_NONE;
	int32 ButtonIndex = 0;
	for (UWidget* Child : Body->GetAllChildren())
	{
		if (const UGRMenuButton* Button = Cast<UGRMenuButton>(Child))
		{
			if (Button->HasFocusedDescendants())
			{
				FocusIndex = ButtonIndex;
			}
			++ButtonIndex;
		}
	}
	Body->ClearChildren();
	DefaultFocus = nullptr;
	auto Line = [this](const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = false)
	{
		Body->AddChildToVerticalBox(WrapText(GRStyle::MakeText(WidgetTree, FText::FromString(Text), Size, Color, bBold)))->SetPadding(FMargin(0.f, 4.f, 0.f, 2.f));
	};
	Line(Def->Help, 14, GRStyle::ColorTextDim);

	int32 Viral = 0;
	int32 Risk = 0;
	TArray<int32> HeadlineIdx, ToneIdx;
	for (int32 i = 0; i < Def->Options.Num(); ++i)
	{
		(Def->Options[i].Group == TEXT("judul") ? HeadlineIdx : ToneIdx).Add(i);
	}

	Line(TEXT("1. Judul"), 16, GRStyle::ColorAccent, true);
	for (int32 i : HeadlineIdx)
	{
		const FGRPostOption& Opt = Def->Options[i];
		if (Headline == i) { Viral += Opt.Viral; Risk += Opt.Risk; }
		AddButton(Body, FText::FromString(FString::Printf(TEXT("%s%s"), Headline == i ? TEXT("[x] ") : TEXT("[  ] "), *Opt.Text)), [this, i]() { Headline = i; Rebuild(); OnShown(); });
	}
	Line(TEXT("2. Lampiran bukti"), 16, GRStyle::ColorAccent, true);
	bool bAnyEvidence = false;
	for (const FGRItemDef& Item : Story->GetData().Items)
	{
		if (!Item.Bukti || Story->GetItemCount(Item.Id) <= 0)
		{
			continue;
		}
		bAnyEvidence = true;
		const FString Id = Item.Id;
		const bool bOn = Evidence.Contains(Id);
		if (bOn) { Viral += 25; Risk += 10; }
		AddButton(Body, FText::FromString(FString::Printf(TEXT("%s%s"), bOn ? TEXT("[x] ") : TEXT("[  ] "), *Item.Name)), [this, Id, bOn]()
		{
			if (bOn) { Evidence.Remove(Id); } else { Evidence.Add(Id); }
			Rebuild();
			OnShown();
		});
	}
	if (!bAnyEvidence)
	{
		Line(TEXT("Kamu belum punya bukti apa pun."), 14, GRStyle::ColorDanger);
	}
	Line(TEXT("3. Nada tulisan"), 16, GRStyle::ColorAccent, true);
	for (int32 i : ToneIdx)
	{
		const FGRPostOption& Opt = Def->Options[i];
		if (Tone == i) { Viral += Opt.Viral; Risk += Opt.Risk; }
		AddButton(Body, FText::FromString(FString::Printf(TEXT("%s%s"), Tone == i ? TEXT("[x] ") : TEXT("[  ] "), *Opt.Text)), [this, i]() { Tone = i; Rebuild(); OnShown(); });
	}

	Line(FString::Printf(TEXT("Perkiraan jangkauan: %d   |   Risiko: %d"), Viral, Risk), 18, GRStyle::ColorText, true);
	const bool bReady = Headline != INDEX_NONE && Tone != INDEX_NONE && Evidence.Num() > 0;
	UGRMenuButton* Post = AddButton(Body, LOCTEXT("WarnetPost", "Unggah ke Kicau"), [this, bReady]() { if (bReady) { Submit(); } });
	Post->SetDanger(!bReady);
	AddButton(Body, LOCTEXT("WarnetCancel", "Batal, belum siap"), [this]() { Close(); });

	ButtonIndex = 0;
	for (UWidget* Child : Body->GetAllChildren())
	{
		if (UGRMenuButton* Button = Cast<UGRMenuButton>(Child))
		{
			if (ButtonIndex++ == FocusIndex)
			{
				DefaultFocus = Button;
			}
		}
	}
}

void UGRWarnetScreen::Submit()
{
	UGRStorySubsystem* Story = StoryFor(this);
	const FGRMinigameDef* Def = Story->FindMinigame(MinigameId);
	int32 Viral = Evidence.Num() * 25;
	int32 Risk = Evidence.Num() * 10;
	for (int32 i : { Headline, Tone })
	{
		if (Def->Options.IsValidIndex(i))
		{
			Viral += Def->Options[i].Viral;
			Risk += Def->Options[i].Risk;
		}
	}
	Story->SetVar(TEXT("viral"), Viral);
	Story->SetVar(TEXT("risiko"), Risk);
	for (const FString& Id : Evidence)
	{
		Story->SetVar(TEXT("unggah.") + Id, 1);
	}
	const FString Win = Def->Win;
	Close();
	Story->Run(Win);
}

// ---------------------------------------------------------------------------------------------
// Stealth (dapur PPR)

FString UGRStealthScreen::PendingId;

UGRStealthScreen::UGRStealthScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPausesGame = true;
	bCanClose = false;
}

bool UGRStealthScreen::Open(UGRUISubsystem* UI, const FString& MinigameId)
{
	const UGRStorySubsystem* Story = StoryFor(UI);
	if (!Story || !Story->FindMinigame(MinigameId))
	{
		return false;
	}
	PendingId = MinigameId;
	return UI->PushScreen<UGRStealthScreen>() != nullptr;
}

void UGRStealthScreen::BuildScreen(UOverlay* Root)
{
	MinigameId = PendingId;
	const FGRMinigameDef* Def = StoryFor(this)->FindMinigame(MinigameId);
	UVerticalBox* Top = WidgetTree->ConstructWidget<UVerticalBox>();
	USizeBox* TopSize = WidgetTree->ConstructWidget<USizeBox>();
	TopSize->SetWidthOverride(1200.f);
	TopSize->SetContent(Top);
	UOverlaySlot* TopSlot = Root->AddChildToOverlay(TopSize);
	TopSlot->SetHorizontalAlignment(HAlign_Center);
	TopSlot->SetVerticalAlignment(VAlign_Top);
	TopSlot->SetPadding(FMargin(0.f, 24.f, 0.f, 0.f));
	Top->AddChildToVerticalBox(GRStyle::MakeText(WidgetTree, FText::FromString(Def->Title), 30, GRStyle::ColorAccent, true))->SetHorizontalAlignment(HAlign_Center);
	UTextBlock* Help = WrapText(GRStyle::MakeText(WidgetTree, FText::FromString(Def->Help), 15, GRStyle::ColorTextDim));
	Help->SetJustification(ETextJustify::Center);
	Top->AddChildToVerticalBox(Help)->SetHorizontalAlignment(HAlign_Center);
	StatusText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 18, GRStyle::ColorText, true);
	Top->AddChildToVerticalBox(StatusText)->SetHorizontalAlignment(HAlign_Center);

	USizeBox* EndSize = WidgetTree->ConstructWidget<USizeBox>();
	// Right of the board (the board itself is painted over the widgets).
	EndSize->SetWidthOverride(330.f);
	UOverlaySlot* EndSlot = Root->AddChildToOverlay(EndSize);
	EndSlot->SetHorizontalAlignment(HAlign_Right);
	EndSlot->SetVerticalAlignment(VAlign_Center);
	EndSlot->SetPadding(FMargin(0.f, 0.f, 30.f, 0.f));
	UBorder* EndPanel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanel, FMargin(24.f));
	EndSize->SetContent(EndPanel);
	EndBox = WidgetTree->ConstructWidget<UVerticalBox>();
	EndPanel->SetContent(EndBox);
	EndSize->SetVisibility(ESlateVisibility::Collapsed);
	Reset();
}

void UGRStealthScreen::Reset()
{
	const FGRMinigameDef* Def = StoryFor(this)->FindMinigame(MinigameId);
	Grid = Def->Grid;
	Guards.Reset();
	Loot.Reset();
	Taken.Reset();
	for (int32 Y = 0; Y < Grid.Num(); ++Y)
	{
		for (int32 X = 0; X < Grid[Y].Len(); ++X)
		{
			const TCHAR C = Grid[Y][X];
			const FIntPoint P(X, Y);
			if (C == 'P') { Start = P; Grid[Y][X] = '.'; }
			else if (C == 'G') { Guards.Add({ P, FIntPoint(1, 0), false }); Grid[Y][X] = '.'; }
			else if (C == 'H') { Guards.Add({ P, FIntPoint(0, 1), true }); Grid[Y][X] = '.'; }
			else if (C == 'B') { Loot.Add(P); Grid[Y][X] = '.'; }
		}
	}
	Player = Start;
	Suspicion = 0.f;
	GuardTimer = 0.f;
	bOver = false;
	if (EndBox && EndBox->GetParent())
	{
		EndBox->ClearChildren();
		EndBox->GetParent()->GetParent()->SetVisibility(ESlateVisibility::Collapsed);
	}
	DefaultFocus = nullptr;
	UpdateStatus();
	SetUserFocus(GetOwningPlayer());
}

bool UGRStealthScreen::IsWall(const FIntPoint& P) const
{
	return !Grid.IsValidIndex(P.Y) || P.X < 0 || P.X >= Grid[P.Y].Len() || Grid[P.Y][P.X] == '#';
}

bool UGRStealthScreen::IsCover(const FIntPoint& P) const
{
	return !IsWall(P) && Grid[P.Y][P.X] == 'C';
}

bool UGRStealthScreen::GuardSees(const FGuard& Guard, const FIntPoint& P) const
{
	const FIntPoint Side(Guard.Dir.Y, Guard.Dir.X);
	for (int32 D = 1; D <= 4; ++D)
	{
		const FIntPoint Center = Guard.Pos + Guard.Dir * D;
		if (IsWall(Center))
		{
			return false;
		}
		const int32 Width = (D - 1) / 2;
		for (int32 W = -Width; W <= Width; ++W)
		{
			const FIntPoint Cell = Center + Side * W;
			if (Cell == P && !IsWall(Cell) && (!IsCover(Cell) || D == 1))
			{
				return true;
			}
		}
	}
	return Guard.Pos == P;
}

void UGRStealthScreen::UpdateStatus()
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(FString::Printf(TEXT("Bukti: %d / %d     Kecurigaan: %d%%"), Taken.Num(), Loot.Num(), FMath::RoundToInt(Suspicion))));
		StatusText->SetColorAndOpacity(Suspicion > 50.f ? GRStyle::ColorDanger : GRStyle::ColorText);
	}
}

void UGRStealthScreen::TryMove(const FIntPoint& Delta)
{
	if (bOver)
	{
		return;
	}
	const FIntPoint Next = Player + Delta;
	if (IsWall(Next))
	{
		return;
	}
	Player = Next;
	if (Loot.Contains(Player) && !Taken.Contains(Player))
	{
		Taken.Add(Player);
	}
	if (Grid[Player.Y][Player.X] == 'X')
	{
		if (Taken.Num() > 0)
		{
			End(true);
			return;
		}
		StoryFor(this)->Run(TEXT("notif Belum ada bukti yang kamu bawa."));
	}
	UpdateStatus();
}

void UGRStealthScreen::StepGuards()
{
	for (FGuard& Guard : Guards)
	{
		const FIntPoint Next = Guard.Pos + Guard.Dir;
		if (IsWall(Next) || IsCover(Next))
		{
			Guard.Dir = FIntPoint(-Guard.Dir.X, -Guard.Dir.Y);
		}
		else
		{
			Guard.Pos = Next;
		}
	}
}

void UGRStealthScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bOver)
	{
		return;
	}
	GuardTimer += InDeltaTime;
	if (GuardTimer >= 0.55f)
	{
		GuardTimer = 0.f;
		StepGuards();
	}
	bool bSeen = false;
	for (const FGuard& Guard : Guards)
	{
		if (GuardSees(Guard, Player))
		{
			bSeen = true;
			if (FMath::Abs(Guard.Pos.X - Player.X) + FMath::Abs(Guard.Pos.Y - Player.Y) <= 1)
			{
				Suspicion = 100.f;
			}
		}
	}
	Suspicion = FMath::Clamp(Suspicion + (bSeen ? 140.f : -12.f) * InDeltaTime, 0.f, 100.f);
	UpdateStatus();
	if (Suspicion >= 100.f)
	{
		End(false);
	}
}

void UGRStealthScreen::End(bool bWin)
{
	bOver = true;
	EndBox->ClearChildren();
	DefaultFocus = nullptr;
	EndBox->GetParent()->GetParent()->SetVisibility(ESlateVisibility::Visible);
	if (bWin)
	{
		UTextBlock* Result = GRStyle::MakeText(WidgetTree, FText::FromString(FString::Printf(TEXT("Lolos! %d bukti terbawa."), Taken.Num())), 22, GRStyle::ColorGood, true);
		Result->SetAutoWrapText(true);
		EndBox->AddChildToVerticalBox(Result);
		AddButton(EndBox, LOCTEXT("StealthOk", "Pulang diam-diam"), [this]()
		{
			UGRStorySubsystem* Story = StoryFor(this);
			const FGRMinigameDef* Def = Story->FindMinigame(MinigameId);
			FString Actions;
			for (int32 i = 0; i < Taken.Num(); ++i)
			{
				const int32 LootIndex = Loot.IndexOfByKey(Taken[i]);
				if (Def->Loot.IsValidIndex(LootIndex))
				{
					Actions += FString::Printf(TEXT("item %s 1;"), *Def->Loot[LootIndex]);
				}
			}
			Actions += Def->Win;
			Close();
			Story->Run(Actions);
		});
	}
	else
	{
		UTextBlock* Result = GRStyle::MakeText(WidgetTree, LOCTEXT("StealthLose", "Ketahuan penjaga dapur!"), 22, GRStyle::ColorDanger, true);
		Result->SetAutoWrapText(true);
		EndBox->AddChildToVerticalBox(Result);
		AddButton(EndBox, LOCTEXT("StealthRetry", "Coba lagi"), [this]() { Reset(); });
		AddButton(EndBox, LOCTEXT("StealthGiveUp", "Kabur tanpa bukti"), [this]()
		{
			UGRStorySubsystem* Story = StoryFor(this);
			const FString Lose = Story->FindMinigame(MinigameId)->Lose;
			Close();
			Story->Run(Lose);
		});
	}
	OnShown();
}

FReply UGRStealthScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (!bOver)
	{
		if (Key == EKeys::W || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up) { TryMove(FIntPoint(0, -1)); return FReply::Handled(); }
		if (Key == EKeys::S || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down) { TryMove(FIntPoint(0, 1)); return FReply::Handled(); }
		if (Key == EKeys::A || Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left) { TryMove(FIntPoint(-1, 0)); return FReply::Handled(); }
		if (Key == EKeys::D || Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right) { TryMove(FIntPoint(1, 0)); return FReply::Handled(); }
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGRStealthScreen::HandleBack()
{
	if (!bOver)
	{
		End(false);
	}
}

int32 UGRStealthScreen::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// Called after the child widgets have painted, so the board is drawn on top of them.
	const FVector2D Size(AllottedGeometry.GetLocalSize());
	int32 Rows = Grid.Num();
	int32 Cols = 0;
	for (const FString& Row : Grid)
	{
		Cols = FMath::Max(Cols, Row.Len());
	}
	int32 Layer = LayerId + 1;
	if (Rows > 0 && Cols > 0)
	{
		const float Cell = FMath::Min3((Size.X - 120.0) / Cols, (Size.Y - 260.0) / Rows, 56.0);
		const FVector2D Origin((Size.X - Cell * Cols) * 0.5, 150.0 + (Size.Y - 200.0 - Cell * Rows) * 0.5);
		static const FSlateRoundedBoxBrush Round(FLinearColor::White, 6.f);
		FSlateBrush Solid;
		auto Box = [&](const FIntPoint& P, const FLinearColor& Color, float Inset, bool bRound, int32 L)
		{
			const FVector2D TopLeft = Origin + FVector2D(P.X * Cell + Inset, P.Y * Cell + Inset);
			FSlateDrawElement::MakeBox(OutDrawElements, L, AllottedGeometry.ToPaintGeometry(FVector2f(Cell - Inset * 2.f, Cell - Inset * 2.f), FSlateLayoutTransform(FVector2f(TopLeft))),
				bRound ? static_cast<const FSlateBrush*>(&Round) : &Solid, ESlateDrawEffect::None, Color);
		};
		for (int32 Y = 0; Y < Rows; ++Y)
		{
			for (int32 X = 0; X < Grid[Y].Len(); ++X)
			{
				const TCHAR C = Grid[Y][X];
				const FLinearColor Color = C == '#' ? FLinearColor(0.35f, 0.25f, 0.18f) : C == 'C' ? FLinearColor(0.14f, 0.3f, 0.2f) : C == 'X' ? FLinearColor(0.2f, 0.55f, 0.3f) : FLinearColor(0.12f, 0.11f, 0.1f);
				Box(FIntPoint(X, Y), Color, 1.f, false, Layer);
			}
		}
		++Layer;
		for (const FGuard& Guard : Guards)
		{
			for (int32 Y = 0; Y < Rows; ++Y)
			{
				for (int32 X = 0; X < Cols; ++X)
				{
					if (!IsWall(FIntPoint(X, Y)) && GuardSees(Guard, FIntPoint(X, Y)) && FIntPoint(X, Y) != Guard.Pos)
					{
						Box(FIntPoint(X, Y), FLinearColor(1.f, 0.85f, 0.3f, 0.28f), 1.f, false, Layer);
					}
				}
			}
		}
		++Layer;
		for (const FIntPoint& P : Loot)
		{
			if (!Taken.Contains(P))
			{
				Box(P, FLinearColor(0.95f, 0.8f, 0.2f), Cell * 0.28f, true, Layer);
			}
		}
		for (const FGuard& Guard : Guards)
		{
			Box(Guard.Pos, FLinearColor(0.85f, 0.2f, 0.15f), Cell * 0.14f, true, Layer);
		}
		Box(Player, FLinearColor(0.3f, 0.85f, 0.9f), Cell * 0.16f, true, Layer);
		++Layer;
	}
	return Layer;
}

#undef LOCTEXT_NAMESPACE
