#include "UI/GRWargaChat.h"
#include "UI/GRJajanScreen.h"
#include "UI/GRMenuButton.h"
#include "UI/GRUISubsystem.h"
#include "UI/GRUIStyle.h"
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
	enum EChatTopic : int32 { TopicGreet = 0, TopicNews = 1, TopicWay = 2, TopicBuy = 3, TopicLeave = 4 };
}

UGRWargaChatScreen::UGRWargaChatScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPausesGame = false;
	bDimBackground = false;
}

void UGRWargaChatScreen::BuildScreen(UOverlay* Root)
{
	// A side panel right of center, so the person and their speech bubble stay in view.
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(380.f);
	UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size);
	SizeSlot->SetHorizontalAlignment(HAlign_Right);
	SizeSlot->SetVerticalAlignment(VAlign_Center);
	SizeSlot->SetPadding(FMargin(0.f, 80.f, 90.f, 0.f));

	UBorder* Panel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanel, FMargin(24.f, 20.f));
	Size->SetContent(Panel);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Box);

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	Box->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	Header->AddChildToHorizontalBox(GRStyle::MakeIcon(WidgetTree, GRStyle::Icon(TEXT("app_pesan")), 40.f))->SetVerticalAlignment(VAlign_Center);
	UVerticalBox* Titles = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBoxSlot* TitlesSlot = Header->AddChildToHorizontalBox(Titles);
	TitlesSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	TitlesSlot->SetVerticalAlignment(VAlign_Center);
	TitleText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 22, GRStyle::ColorAccent, true);
	Titles->AddChildToVerticalBox(TitleText);
	SubtitleText = GRStyle::MakeText(WidgetTree, LOCTEXT("ChatHint", "Pilih obrolan: klik atau tekan angka"), 13, GRStyle::ColorTextDim);
	Titles->AddChildToVerticalBox(SubtitleText);

	Options = WidgetTree->ConstructWidget<UVerticalBox>();
	Box->AddChildToVerticalBox(Options)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	IntroList = Options;
}

void UGRWargaChatScreen::SetWarga(AGRWarga* InWarga)
{
	Warga = InWarga;
	if (!InWarga)
	{
		return;
	}
	TitleText->SetText(InWarga->GetChatTitle());
	struct FOption { int32 Topic; FText Label; const TCHAR* Icon; };
	TArray<FOption> List = {
		{ TopicGreet, LOCTEXT("ChatGreet", "Sapa"), TEXT("app_pesan") },
		{ TopicNews, LOCTEXT("ChatNews", "Tanya kabar"), TEXT("app_berita") },
		{ TopicWay, LOCTEXT("ChatWay", "Tanya jalan ke tujuan"), TEXT("app_peta") },
	};
	if (InWarga->IsVendor())
	{
		List.Add({ TopicBuy, LOCTEXT("ChatBuy", "Beli jajanan"), TEXT("item_gorengan") });
	}
	List.Add({ TopicLeave, LOCTEXT("ChatLeave", "Pamit"), TEXT("menu_keluar") });

	Options->ClearChildren();
	Topics.Reset();
	Buttons.Reset();
	for (const FOption& Option : List)
	{
		const int32 Topic = Option.Topic;
		const FText Label = FText::Format(LOCTEXT("ChatOption", "{0}.  {1}"), FText::AsNumber(Topics.Num() + 1), Option.Label);
		UGRMenuButton* Button = AddIconButton(Options, Label, Option.Icon, [this, Topic]() { Pick(Topic); });
		Topics.Add(Topic);
		Buttons.Add(Button);
	}
	DefaultFocus = Buttons.IsEmpty() ? nullptr : Buttons[0].Get();
	if (DefaultFocus)
	{
		DefaultFocus->SetFocus();
	}
}

void UGRWargaChatScreen::Pick(int32 Topic)
{
	AGRWarga* Person = Warga.Get();
	if (Topic == TopicLeave || !Person)
	{
		if (Person)
		{
			Person->HoldChat(1.5f);
		}
		Close();
		return;
	}
	if (Topic == TopicBuy)
	{
		// The cart's menu has a screen of its own (order, pay, the hand-over).
		UGRUISubsystem* UI = GetUI();
		Close();
		if (UGRJajanScreen* Screen = UI ? UI->PushScreen<UGRJajanScreen>() : nullptr)
		{
			Screen->SetWarga(Person);
		}
		return;
	}
	Person->Chat(Topic);
}

void UGRWargaChatScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AGRWarga* Person = Warga.Get();
	const APawn* Bayu = UGameplayStatics::GetPlayerPawn(this, 0);
	// They wander off when knocked over, angry or recycled, or once Bayu has walked away.
	if (!Person || !Person->CanChat() || !Bayu || FVector::Dist2D(Person->GetActorLocation(), Bayu->GetActorLocation()) > 450.f)
	{
		Close();
		return;
	}
	Person->HoldChat(1.f);
}

FReply UGRWargaChatScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	static const FKey NumberKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five };
	static const FKey NumPadKeys[] = { EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour, EKeys::NumPadFive };
	for (int32 Index = 0; Index < Topics.Num() && Index < UE_ARRAY_COUNT(NumberKeys); ++Index)
	{
		if (InKeyEvent.GetKey() == NumberKeys[Index] || InKeyEvent.GetKey() == NumPadKeys[Index])
		{
			if (Buttons.IsValidIndex(Index) && Buttons[Index]->GetIsEnabled())
			{
				Pick(Topics[Index]);
			}
			return FReply::Handled();
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
