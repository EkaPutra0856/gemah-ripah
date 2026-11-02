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

	ContinueButton = AddIconButton(Box, LOCTEXT("Continue", "Lanjutkan"), TEXT("menu_lanjut"), [this]()
	{
		if (UGRGameFlowSubsystem* Flow = GetFlow(this)) { Flow->ContinueGame(); }
	});
	AddIconButton(Box, LOCTEXT("NewGame", "Game Baru"), TEXT("menu_baru"), [this]()
	{
		if (UGRGameFlowSubsystem* Flow = GetFlow(this)) { Flow->StartNewGame(); }
	});
	AddIconButton(Box, LOCTEXT("Load", "Muat"), TEXT("menu_muat"), [this]() { GetUI()->PushScreen<UGRLoadScreen>(); });
	AddIconButton(Box, LOCTEXT("Settings", "Pengaturan"), TEXT("menu_pengaturan"), [this]() { GetUI()->PushScreen<UGRSettingsScreen>(); });
	AddIconButton(Box, LOCTEXT("Credits", "Kredit"), TEXT("menu_kredit"), [this]() { GetUI()->PushScreen<UGRCreditsScreen>(); });
	AddIconButton(Box, LOCTEXT("Quit", "Keluar"), TEXT("menu_keluar"), [this]()
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

	AddIconButton(Box, LOCTEXT("Resume", "Lanjut"), TEXT("menu_lanjut"), [this]() { Close(); });
	AddIconButton(Box, LOCTEXT("PauseMap", "Peta"), TEXT("app_peta"), [this]() { GetUI()->PushScreen<UGRMapScreen>(); });
	AddIconButton(Box, LOCTEXT("PauseChapters", "Peta Bab"), TEXT("menu_petabab"), [this]() { UGRChapterMapScreen::Open(GetUI()); });
	AddIconButton(Box, LOCTEXT("Save", "Simpan"), TEXT("menu_simpan"), [this]() { GetUI()->PushScreen<UGRSaveScreen>(); });
	AddIconButton(Box, LOCTEXT("LoadPause", "Muat"), TEXT("menu_muat"), [this]() { GetUI()->PushScreen<UGRLoadScreen>(); });
	AddIconButton(Box, LOCTEXT("SettingsPause", "Pengaturan"), TEXT("menu_pengaturan"), [this]() { GetUI()->PushScreen<UGRSettingsScreen>(); });
	AddIconButton(Box, LOCTEXT("MainMenu", "Menu Utama"), TEXT("menu_utama"), [this]()
	{
		UGRConfirmScreen::Ask(GetUI(), LOCTEXT("ConfirmMenu", "Kembali ke menu utama?\nProgres sejak simpanan terakhir akan hilang."), [this]()
		{
			if (UGRGameFlowSubsystem* Flow = GetFlow(this)) { Flow->ReturnToMainMenu(); }
		});
	});
	AddIconButton(Box, LOCTEXT("QuitGame", "Keluar Game"), TEXT("menu_keluar"), [this]()
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
