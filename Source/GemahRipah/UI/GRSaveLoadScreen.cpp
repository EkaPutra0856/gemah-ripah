#include "UI/GRSaveLoadScreen.h"
#include "UI/GRMenuButton.h"
#include "UI/GRMenuScreens.h"
#include "UI/GRUISubsystem.h"
#include "UI/GRUIStyle.h"
#include "Core/GRPlayerController.h"
#include "Systems/GRGameFlowSubsystem.h"
#include "Systems/GRSaveSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

void UGRSlotScreen::BuildScreen(UOverlay* Root)
{
	UVerticalBox* Box = MakeCenteredPanel(Root, IsSaveMode() ? LOCTEXT("SaveTitle", "Simpan Game") : LOCTEXT("LoadTitle", "Muat Game"), 900.f);

	SlotList = WidgetTree->ConstructWidget<UVerticalBox>();
	Box->AddChildToVerticalBox(SlotList);

	UGRMenuButton* Back = AddButton(Box, LOCTEXT("SlotBack", "Kembali"), [this]() { Close(); });
	if (UVerticalBoxSlot* BackSlot = Cast<UVerticalBoxSlot>(Back->Slot))
	{
		BackSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	}
}

void UGRSlotScreen::OnShown()
{
	RebuildSlots();
	Super::OnShown();
}

bool UGRSlotScreen::IsInGame() const
{
	return Cast<AGRPlayerController>(GetOwningPlayer()) != nullptr;
}

void UGRSlotScreen::RebuildSlots()
{
	SlotList->ClearChildren();
	DefaultFocus = nullptr;

	const UGRSaveSubsystem* Saves = GetGameInstance()->GetSubsystem<UGRSaveSubsystem>();
	TArray<FString> Slots;
	if (!IsSaveMode())
	{
		Slots.Add(UGRSaveSubsystem::AutoSlot);
	}
	Slots.Append(UGRSaveSubsystem::GetManualSlots());

	for (const FString& SlotName : Slots)
	{
		const FGRSaveSlotInfo Info = Saves->GetSlotInfo(SlotName);
		const FText Label = Info.bExists
			? FText::Format(LOCTEXT("SlotFilled", "{0}  |  {1}  |  {2}  |  {3}"),
				UGRSaveSubsystem::GetSlotLabel(SlotName), Info.CityName,
				FText::FromString(Info.SavedAt.ToString(TEXT("%d/%m/%Y %H:%M"))),
				FText::FromString(UGRSaveSubsystem::FormatPlayTime(Info.PlayTimeSeconds)))
			: FText::Format(LOCTEXT("SlotEmpty", "{0}  |  Kosong"), UGRSaveSubsystem::GetSlotLabel(SlotName));

		const bool bExists = Info.bExists;
		UGRMenuButton* Button = AddButton(SlotList, Label, [this, SlotName, bExists]() { HandleSlot(SlotName, bExists); });
		if (!IsSaveMode() && !bExists)
		{
			Button->SetIsEnabled(false);
			if (DefaultFocus == Button)
			{
				DefaultFocus = nullptr;
			}
		}
	}

	if (!DefaultFocus)
	{
		DefaultFocus = SlotList->GetChildrenCount() > 0 ? SlotList->GetChildAt(0) : nullptr;
	}
}

void UGRSlotScreen::HandleSlot(const FString& SlotName, bool bExists)
{
	UGameInstance* GI = GetGameInstance();
	UGRUISubsystem* UI = GetUI();

	if (IsSaveMode())
	{
		auto DoSave = [this, GI, SlotName]()
		{
			GI->GetSubsystem<UGRSaveSubsystem>()->SaveToSlot(SlotName);
			OnShown();
		};
		if (bExists)
		{
			UGRConfirmScreen::Ask(UI, LOCTEXT("ConfirmOverwrite", "Timpa simpanan di slot ini?"), DoSave);
		}
		else
		{
			DoSave();
		}
		return;
	}

	auto DoLoad = [GI, UI, SlotName]()
	{
		if (GI->GetSubsystem<UGRGameFlowSubsystem>()->LoadGame(SlotName) && UI)
		{
			UI->PopAll();
		}
	};
	if (IsInGame())
	{
		UGRConfirmScreen::Ask(UI, LOCTEXT("ConfirmLoad", "Muat simpanan ini?\nProgres yang belum disimpan akan hilang."), DoLoad);
	}
	else
	{
		DoLoad();
	}
}

#undef LOCTEXT_NAMESPACE
