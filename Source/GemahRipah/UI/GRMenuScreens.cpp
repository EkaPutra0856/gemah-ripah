#include "UI/GRMenuScreens.h"
#include "UI/GRChapterScreens.h"
#include "UI/GRMenuButton.h"
#include "UI/GRSaveLoadScreen.h"
#include "UI/GRSettingsScreen.h"
#include "UI/GRMapScreen.h"
#include "UI/GRUISubsystem.h"
#include "UI/GRUIStyle.h"
#include "Data/GRDataSubsystem.h"
#include "Systems/GRGameFlowSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Misc/ConfigCacheIni.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

namespace
{
	UGRGameFlowSubsystem* GetFlow(const UUserWidget* Widget)
	{
		const UGameInstance* GI = Widget->GetGameInstance();
		return GI ? GI->GetSubsystem<UGRGameFlowSubsystem>() : nullptr;
	}
}

// ---------------------------------------------------------------- Main menu

UGRMainMenuScreen::UGRMainMenuScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanClose = false;
	bDimBackground = false;
}

void UGRMainMenuScreen::BuildScreen(UOverlay* Root)
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(460.f);
	UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size);
	SizeSlot->SetHorizontalAlignment(HAlign_Left);
	SizeSlot->SetVerticalAlignment(VAlign_Center);
	SizeSlot->SetPadding(FMargin(90.f, 0.f, 0.f, 0.f));

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Size->SetContent(Box);

	Box->AddChildToVerticalBox(GRStyle::MakeText(WidgetTree, LOCTEXT("GameTitle", "GEMAH RIPAH"), 56, GRStyle::ColorAccent, true));
	Box->AddChildToVerticalBox(GRStyle::MakeText(WidgetTree, LOCTEXT("ChapterTitle", "Bab 1: Asap dan Piring"), 22, GRStyle::ColorText))
		->SetPadding(FMargin(0.f, 0.f, 0.f, 36.f));

	ContinueButton = AddButton(Box, LOCTEXT("Continue", "Lanjutkan"), [this]()
	{
		if (UGRGameFlowSubsystem* Flow = GetFlow(this)) { Flow->ContinueGame(); }
	});
	AddButton(Box, LOCTEXT("NewGame", "Game Baru"), [this]()
	{
		if (UGRGameFlowSubsystem* Flow = GetFlow(this)) { Flow->StartNewGame(); }
	});
	AddButton(Box, LOCTEXT("Load", "Muat"), [this]() { GetUI()->PushScreen<UGRLoadScreen>(); });
	AddButton(Box, LOCTEXT("Settings", "Pengaturan"), [this]() { GetUI()->PushScreen<UGRSettingsScreen>(); });
	AddButton(Box, LOCTEXT("Credits", "Kredit"), [this]() { GetUI()->PushScreen<UGRCreditsScreen>(); });
	AddButton(Box, LOCTEXT("Quit", "Keluar"), [this]()
	{
		if (UGRGameFlowSubsystem* Flow = GetFlow(this)) { Flow->QuitGame(); }
	});

	FString Version;
	GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), Version, GGameIni);
	UTextBlock* VersionText = GRStyle::MakeText(WidgetTree, FText::Format(LOCTEXT("Version", "v{0} - prototipe, aset sementara"), FText::FromString(Version)), 14, GRStyle::ColorTextDim);
	UOverlaySlot* VersionSlot = Root->AddChildToOverlay(VersionText);
	VersionSlot->SetHorizontalAlignment(HAlign_Right);
	VersionSlot->SetVerticalAlignment(VAlign_Bottom);
	VersionSlot->SetPadding(FMargin(0.f, 0.f, 30.f, 24.f));
}

void UGRMainMenuScreen::OnShown()
{
	const UGRGameFlowSubsystem* Flow = GetFlow(this);
	const bool bCanContinue = Flow && Flow->CanContinue();
	ContinueButton->SetIsEnabled(bCanContinue);
	ContinueButton->SetVisibility(bCanContinue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	Super::OnShown();
	if (!bCanContinue)
	{
		// Skip the hidden "Lanjutkan" button.
		if (UVerticalBox* Box = Cast<UVerticalBox>(ContinueButton->GetParent()))
		{
			const int32 Next = Box->GetChildIndex(ContinueButton) + 1;
			if (UGRMenuButton* NewGame = Cast<UGRMenuButton>(Box->GetChildAt(Next)))
			{
				NewGame->FocusButton();
			}
		}
	}
}

// ---------------------------------------------------------------- Pause

UGRPauseScreen::UGRPauseScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPausesGame = true;
}

void UGRPauseScreen::BuildScreen(UOverlay* Root)
{
	UVerticalBox* Box = MakeCenteredPanel(Root, LOCTEXT("PauseTitle", "Jeda"), 420.f);

	AddButton(Box, LOCTEXT("Resume", "Lanjut"), [this]() { Close(); });
	AddButton(Box, LOCTEXT("PauseMap", "Peta"), [this]() { GetUI()->PushScreen<UGRMapScreen>(); });
	AddButton(Box, LOCTEXT("PauseChapters", "Peta Bab"), [this]() { UGRChapterMapScreen::Open(GetUI()); });
	AddButton(Box, LOCTEXT("Save", "Simpan"), [this]() { GetUI()->PushScreen<UGRSaveScreen>(); });
	AddButton(Box, LOCTEXT("LoadPause", "Muat"), [this]() { GetUI()->PushScreen<UGRLoadScreen>(); });
	AddButton(Box, LOCTEXT("SettingsPause", "Pengaturan"), [this]() { GetUI()->PushScreen<UGRSettingsScreen>(); });
	AddButton(Box, LOCTEXT("MainMenu", "Menu Utama"), [this]()
	{
		UGRConfirmScreen::Ask(GetUI(), LOCTEXT("ConfirmMenu", "Kembali ke menu utama?\nProgres sejak simpanan terakhir akan hilang."), [this]()
		{
			if (UGRGameFlowSubsystem* Flow = GetFlow(this)) { Flow->ReturnToMainMenu(); }
		});
	});
	AddButton(Box, LOCTEXT("QuitGame", "Keluar Game"), [this]()
	{
		UGRConfirmScreen::Ask(GetUI(), LOCTEXT("ConfirmQuit", "Keluar dari game?\nProgres sejak simpanan terakhir akan hilang."), [this]()
		{
			if (UGRGameFlowSubsystem* Flow = GetFlow(this)) { Flow->QuitGame(); }
		});
	})->SetDanger(true);
}

// ---------------------------------------------------------------- Confirm

void UGRConfirmScreen::Ask(UGRUISubsystem* UI, const FText& Message, TFunction<void()> OnYes)
{
	if (!UI)
	{
		return;
	}
	if (UGRConfirmScreen* Screen = UI->PushScreen<UGRConfirmScreen>())
	{
		Screen->MessageText->SetText(Message);
		Screen->OnYes = MoveTemp(OnYes);
	}
}

void UGRConfirmScreen::BuildScreen(UOverlay* Root)
{
	UVerticalBox* Box = MakeCenteredPanel(Root, LOCTEXT("ConfirmTitle", "Yakin?"), 520.f);

	MessageText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 18);
	MessageText->SetAutoWrapText(true);
	Box->AddChildToVerticalBox(MessageText)->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));

	AddButton(Box, LOCTEXT("Yes", "Ya"), [this]()
	{
		TFunction<void()> Callback = MoveTemp(OnYes);
		Close();
		if (Callback) { Callback(); }
	});
	DefaultFocus = AddButton(Box, LOCTEXT("No", "Tidak"), [this]() { Close(); });
}

// ---------------------------------------------------------------- Credits

void UGRCreditsScreen::BuildScreen(UOverlay* Root)
{
	UVerticalBox* Box = MakeCenteredPanel(Root, LOCTEXT("CreditsTitle", "Kredit"), 640.f);

	USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>();
	ListSize->SetMaxDesiredHeight(460.f);
	UScrollBox* List = WidgetTree->ConstructWidget<UScrollBox>();
	ListSize->SetContent(List);
	Box->AddChildToVerticalBox(ListSize);

	if (const UGRDataSubsystem* Data = GetGameInstance()->GetSubsystem<UGRDataSubsystem>())
	{
		for (const FGRCreditEntry& Entry : Data->GetCredits())
		{
			List->AddChild(GRStyle::MakeText(WidgetTree, FText::FromString(Entry.Role), 14, GRStyle::ColorTextDim));
			FString Line = Entry.Name;
			if (!Entry.Detail.IsEmpty())
			{
				Line += TEXT("  -  ") + Entry.Detail;
			}
			if (UScrollBoxSlot* NameSlot = Cast<UScrollBoxSlot>(List->AddChild(GRStyle::MakeText(WidgetTree, FText::FromString(Line), 18))))
			{
				NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
			}
		}
	}

	UVerticalBoxSlot* BackSlot = Cast<UVerticalBoxSlot>(AddButton(Box, LOCTEXT("Back", "Kembali"), [this]() { Close(); })->Slot);
	if (BackSlot)
	{
		BackSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	}
}

#undef LOCTEXT_NAMESPACE
