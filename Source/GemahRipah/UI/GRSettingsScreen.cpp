#include "UI/GRSettingsScreen.h"
#include "UI/GRMenuButton.h"
#include "UI/GROptionRow.h"
#include "UI/GRUIStyle.h"
#include "Core/GRGameUserSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

namespace
{
	TArray<FText> MakeTexts(std::initializer_list<const TCHAR*> Items)
	{
		TArray<FText> Result;
		for (const TCHAR* Item : Items)
		{
			Result.Add(FText::FromString(Item));
		}
		return Result;
	}

	TArray<FText> Percent()
	{
		TArray<FText> Result;
		for (int32 Value = 0; Value <= 100; Value += 10)
		{
			Result.Add(FText::FromString(FString::Printf(TEXT("%d%%"), Value)));
		}
		return Result;
	}

	const TArray<int32> FrameLimits = { 30, 60, 120, 144, 0 };
	const TArray<float> TextSpeeds = { 25.f, 45.f, 80.f, 0.f };
	constexpr float SensitivityStep = 0.25f;
	constexpr int32 SensitivitySteps = 10;

	int32 IndexOfClosest(const TArray<float>& Values, float Value)
	{
		int32 Best = 0;
		for (int32 Index = 1; Index < Values.Num(); ++Index)
		{
			if (FMath::Abs(Values[Index] - Value) < FMath::Abs(Values[Best] - Value))
			{
				Best = Index;
			}
		}
		return Best;
	}
}

void UGRSettingsScreen::BuildScreen(UOverlay* Root)
{
	UVerticalBox* Box = MakeCenteredPanel(Root, LOCTEXT("SettingsTitle", "Pengaturan"), 760.f);

	USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>();
	ListSize->SetMaxDesiredHeight(520.f);
	RowList = WidgetTree->ConstructWidget<UScrollBox>();
	ListSize->SetContent(RowList);
	Box->AddChildToVerticalBox(ListSize);

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
	Box->AddChildToVerticalBox(Buttons)->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));

	TArray<TPair<FText, TFunction<void()>>> Actions;
	Actions.Add({ LOCTEXT("Apply", "Terapkan"), [this]() { Apply(); } });
	Actions.Add({ LOCTEXT("Defaults", "Bawaan"), [this]()
	{
		if (UGRGameUserSettings* Settings = UGRGameUserSettings::Get())
		{
			Settings->SetToDefaults();
			RebuildRows();
		}
	} });
	Actions.Add({ LOCTEXT("SettingsBack", "Kembali"), [this]() { HandleBack(); } });

	for (TPair<FText, TFunction<void()>>& Action : Actions)
	{
		UGRMenuButton* Button = WidgetTree->ConstructWidget<UGRMenuButton>(UGRMenuButton::StaticClass());
		Button->SetLabel(Action.Key);
		Button->OnClicked.BindLambda(MoveTemp(Action.Value));
		UHorizontalBoxSlot* ButtonSlot = Buttons->AddChildToHorizontalBox(Button);
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ButtonSlot->SetPadding(FMargin(4.f, 0.f));
	}

	RebuildRows();
}

UGROptionRow* UGRSettingsScreen::AddRow(const FText& Label, const TArray<FText>& Options, int32 Index, TFunction<void(int32)> OnChanged)
{
	UGROptionRow* Row = WidgetTree->ConstructWidget<UGROptionRow>(UGROptionRow::StaticClass());
	Row->Setup(Label, Options, Index, MoveTemp(OnChanged));
	if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(RowList->AddChild(Row)))
	{
		RowSlot->SetPadding(FMargin(0.f, 3.f));
	}
	if (!DefaultFocus)
	{
		DefaultFocus = Row;
	}
	return Row;
}

void UGRSettingsScreen::RebuildRows()
{
	UGRGameUserSettings* S = UGRGameUserSettings::Get();
	if (!S)
	{
		return;
	}
	RowList->ClearChildren();
	DefaultFocus = nullptr;

	// Graphics
	const int32 Quality = S->GetOverallScalabilityLevel();
	AddRow(LOCTEXT("Quality", "Kualitas grafis"), MakeTexts({ TEXT("Rendah"), TEXT("Sedang"), TEXT("Tinggi"), TEXT("Epik"), TEXT("Sinematik") }),
		Quality < 0 ? 2 : Quality, [S](int32 I) { S->SetOverallScalabilityLevel(I); });

	Resolutions.Reset();
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	const FIntPoint Current = S->GetScreenResolution();
	Resolutions.AddUnique(Current);
	Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X * A.Y < B.X * B.Y; });
	TArray<FText> ResolutionTexts;
	for (const FIntPoint& R : Resolutions)
	{
		ResolutionTexts.Add(FText::FromString(FString::Printf(TEXT("%d x %d"), R.X, R.Y)));
	}
	AddRow(LOCTEXT("Resolution", "Resolusi"), ResolutionTexts, Resolutions.IndexOfByKey(Current),
		[this, S](int32 I) { S->SetScreenResolution(Resolutions[I]); });

	// EWindowMode: Fullscreen = 0, WindowedFullscreen = 1, Windowed = 2
	AddRow(LOCTEXT("WindowMode", "Mode layar"), MakeTexts({ TEXT("Layar penuh"), TEXT("Jendela tanpa bingkai"), TEXT("Jendela") }),
		static_cast<int32>(S->GetFullscreenMode()), [S](int32 I) { S->SetFullscreenMode(static_cast<EWindowMode::Type>(I)); });

	AddRow(LOCTEXT("VSync", "VSync"), MakeTexts({ TEXT("Mati"), TEXT("Nyala") }), S->IsVSyncEnabled() ? 1 : 0,
		[S](int32 I) { S->SetVSyncEnabled(I == 1); });

	AddRow(LOCTEXT("FrameLimit", "Batas FPS"), MakeTexts({ TEXT("30"), TEXT("60"), TEXT("120"), TEXT("144"), TEXT("Tanpa batas") }),
		FMath::Max(0, FrameLimits.IndexOfByKey(FMath::RoundToInt(S->GetFrameRateLimit()))),
		[S](int32 I) { S->SetFrameRateLimit(FrameLimits[I]); });

	// Audio
	AddRow(LOCTEXT("MasterVolume", "Volume utama"), Percent(), FMath::RoundToInt(S->MasterVolume * 10.f),
		[S](int32 I) { S->MasterVolume = I / 10.f; });
	AddRow(LOCTEXT("MusicVolume", "Volume musik"), Percent(), FMath::RoundToInt(S->MusicVolume * 10.f),
		[S](int32 I) { S->MusicVolume = I / 10.f; });
	AddRow(LOCTEXT("SfxVolume", "Volume efek"), Percent(), FMath::RoundToInt(S->SfxVolume * 10.f),
		[S](int32 I) { S->SfxVolume = I / 10.f; });

	// Controls
	TArray<FText> Sensitivities;
	for (int32 Step = 1; Step <= SensitivitySteps; ++Step)
	{
		Sensitivities.Add(FText::AsNumber(Step * SensitivityStep));
	}
	AddRow(LOCTEXT("Sensitivity", "Sensitivitas mouse"), Sensitivities, FMath::RoundToInt(S->MouseSensitivity / SensitivityStep) - 1,
		[S](int32 I) { S->MouseSensitivity = (I + 1) * SensitivityStep; });
	AddRow(LOCTEXT("InvertY", "Balik sumbu Y kamera"), MakeTexts({ TEXT("Mati"), TEXT("Nyala") }), S->bInvertLookY ? 1 : 0,
		[S](int32 I) { S->bInvertLookY = I == 1; });

	// Gameplay
	AddRow(LOCTEXT("TextSpeed", "Kecepatan teks dialog"), MakeTexts({ TEXT("Lambat"), TEXT("Normal"), TEXT("Cepat"), TEXT("Langsung") }),
		IndexOfClosest(TextSpeeds, S->TextSpeed), [S](int32 I) { S->TextSpeed = TextSpeeds[I]; });
	AddRow(LOCTEXT("SubtitleSize", "Ukuran subtitle"), MakeTexts({ TEXT("Kecil"), TEXT("Sedang"), TEXT("Besar") }), S->SubtitleSize,
		[S](int32 I) { S->SubtitleSize = I; });
}

void UGRSettingsScreen::Apply()
{
	if (UGRGameUserSettings* Settings = UGRGameUserSettings::Get())
	{
		Settings->ApplySettings(false);
	}
}

void UGRSettingsScreen::HandleBack()
{
	// Discard anything not applied.
	if (UGRGameUserSettings* Settings = UGRGameUserSettings::Get())
	{
		Settings->LoadSettings(true);
	}
	Close();
}

#undef LOCTEXT_NAMESPACE
