#include "UI/GRStoryScreens.h"
#include "World/GRStealthLevel.h"
#include "UI/GRMenuButton.h"
#include "UI/GRUIStyle.h"
#include "UI/GRUISubsystem.h"
#include "Story/GRStorySubsystem.h"
#include "Story/GRStoryUtil.h"
#include "Systems/GRClockSubsystem.h"
#include "Systems/GRGameFlowSubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

namespace
{
	UGRStorySubsystem* StoryOf(const UUserWidget* Widget)
	{
		const UGameInstance* GI = Widget ? Widget->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UGRStorySubsystem>() : nullptr;
	}

	bool IsAdvanceKey(const FKey& Key)
	{
		return Key == EKeys::SpaceBar || Key == EKeys::Enter || Key == EKeys::E || Key == EKeys::Gamepad_FaceButton_Bottom;
	}

	UTextBlock* Wrapped(UTextBlock* Text)
	{
		Text->SetAutoWrapText(true);
		return Text;
	}
}

// ---------------------------------------------------------------------------------------------
// Dialog

FString UGRDialogScreen::PendingId;

UGRDialogScreen::UGRDialogScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanClose = false;
}

bool UGRDialogScreen::Open(UGRUISubsystem* UI, const FString& DialogId)
{
	if (!UI || !UI->GetLocalPlayer()->GetGameInstance()->GetSubsystem<UGRStorySubsystem>()->FindDialog(DialogId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Dialog tidak ditemukan: %s"), *DialogId);
		return false;
	}
	PendingId = DialogId;
	return UI->PushScreen<UGRDialogScreen>() != nullptr;
}

UGRStorySubsystem* UGRDialogScreen::GetStory() const
{
	return StoryOf(this);
}

void UGRDialogScreen::NativeOnInitialized()
{
	DialogId = PendingId;
	const UGRStorySubsystem* Story = GetStory();
	const FGRDialogDef* Def = Story ? Story->FindDialog(DialogId) : nullptr;
	Style = Def ? Def->Style : TEXT("3d");
	bDimBackground = Style == TEXT("vn");
	Super::NativeOnInitialized();
}

void UGRDialogScreen::BuildScreen(UOverlay* Root)
{
	UGRStorySubsystem* Story = GetStory();
	const FGRDialogDef* Def = Story ? Story->FindDialog(DialogId) : nullptr;
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("Dialog tidak ditemukan: %s"), *DialogId);
		bFinished = true;
		return;
	}

	if (Style == TEXT("chat"))
	{
		Story->SetVar(TEXT("baca.") + DialogId, 1);
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(440.f);
		Size->SetHeightOverride(760.f);
		UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size);
		SizeSlot->SetHorizontalAlignment(HAlign_Right);
		SizeSlot->SetVerticalAlignment(VAlign_Center);
		SizeSlot->SetPadding(FMargin(0.f, 0.f, 120.f, 0.f));
		UBorder* Phone = GRStyle::MakePanel(WidgetTree, FLinearColor(0.04f, 0.04f, 0.05f, 0.97f), FMargin(18.f), 26.f);
		Size->SetContent(Phone);
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		Phone->SetContent(Box);
		const FGRSpeakerDef* First = Def->Nodes.Num() > 0 ? Story->FindSpeaker(Def->Nodes[0].Speaker) : nullptr;
		NameText = GRStyle::MakeText(WidgetTree, FText::FromString(FString::Printf(TEXT("Pesan  |  %s"), First ? *First->Name : TEXT("?"))), 20, GRStyle::ColorAccent, true);
		Box->AddChildToVerticalBox(NameText)->SetPadding(FMargin(4.f, 0.f, 0.f, 10.f));
		ChatList = WidgetTree->ConstructWidget<UScrollBox>();
		UVerticalBoxSlot* ListSlot = Box->AddChildToVerticalBox(ChatList);
		ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ChoiceBox = WidgetTree->ConstructWidget<UVerticalBox>();
		Box->AddChildToVerticalBox(ChoiceBox);
		HintText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 12, GRStyle::ColorTextDim);
		Box->AddChildToVerticalBox(HintText)->SetHorizontalAlignment(HAlign_Right);
	}
	else
	{
		if (Style == TEXT("vn"))
		{
			USizeBox* PortraitSize = WidgetTree->ConstructWidget<USizeBox>();
			PortraitSize->SetWidthOverride(280.f);
			PortraitSize->SetHeightOverride(340.f);
			UOverlaySlot* PortraitSlot = Root->AddChildToOverlay(PortraitSize);
			PortraitSlot->SetHorizontalAlignment(HAlign_Left);
			PortraitSlot->SetVerticalAlignment(VAlign_Bottom);
			PortraitSlot->SetPadding(FMargin(150.f, 0.f, 0.f, 250.f));
			Portrait = GRStyle::MakePanel(WidgetTree, FLinearColor::White, FMargin(12.f), 16.f);
			PortraitSize->SetContent(Portrait);
			UVerticalBox* PortraitBox = WidgetTree->ConstructWidget<UVerticalBox>();
			Portrait->SetContent(PortraitBox);
			Portrait->SetVerticalAlignment(VAlign_Center);
			Portrait->SetHorizontalAlignment(HAlign_Center);
			PortraitGlyph = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 120, FLinearColor(0.f, 0.f, 0.f, 0.55f), true);
			PortraitBox->AddChildToVerticalBox(PortraitGlyph)->SetHorizontalAlignment(HAlign_Center);
			PortraitName = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 16, FLinearColor(0.f, 0.f, 0.f, 0.7f), true);
			PortraitBox->AddChildToVerticalBox(PortraitName)->SetHorizontalAlignment(HAlign_Center);
		}

		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(1100.f);
		Size->SetMinDesiredHeight(200.f);
		UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size);
		SizeSlot->SetHorizontalAlignment(HAlign_Center);
		SizeSlot->SetVerticalAlignment(VAlign_Bottom);
		SizeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 40.f));
		UBorder* Panel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanel, FMargin(30.f, 22.f), 10.f);
		Size->SetContent(Panel);
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(Box);
		NameText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 22, GRStyle::ColorAccent, true);
		Box->AddChildToVerticalBox(NameText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		BodyText = Wrapped(GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 21, GRStyle::ColorText));
		Box->AddChildToVerticalBox(BodyText);
		ChoiceBox = WidgetTree->ConstructWidget<UVerticalBox>();
		Box->AddChildToVerticalBox(ChoiceBox)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		HintText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 13, GRStyle::ColorTextDim);
		Box->AddChildToVerticalBox(HintText)->SetHorizontalAlignment(HAlign_Right);
	}

	if (!Def->Meter.IsEmpty())
	{
		USizeBox* MeterSize = WidgetTree->ConstructWidget<USizeBox>();
		MeterSize->SetWidthOverride(460.f);
		UOverlaySlot* MeterSlot = Root->AddChildToOverlay(MeterSize);
		MeterSlot->SetHorizontalAlignment(HAlign_Center);
		MeterSlot->SetVerticalAlignment(VAlign_Top);
		MeterSlot->SetPadding(FMargin(0.f, 60.f, 0.f, 0.f));
		UBorder* MeterPanel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanel, FMargin(16.f, 10.f));
		MeterSize->SetContent(MeterPanel);
		UVerticalBox* MeterBox = WidgetTree->ConstructWidget<UVerticalBox>();
		MeterPanel->SetContent(MeterBox);
		MeterText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 15, GRStyle::ColorDanger, true);
		MeterBox->AddChildToVerticalBox(MeterText);
		MeterBar = WidgetTree->ConstructWidget<UProgressBar>();
		MeterBar->SetFillColorAndOpacity(GRStyle::ColorDanger);
		MeterBox->AddChildToVerticalBox(MeterBar)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}

	ShowNode(0);
}

void UGRDialogScreen::RefreshMeter()
{
	const UGRStorySubsystem* Story = GetStory();
	const FGRDialogDef* Def = Story ? Story->FindDialog(DialogId) : nullptr;
	if (!MeterBar || !Def)
	{
		return;
	}
	const int32 Value = FMath::Clamp(Story->GetVar(Def->Meter), 0, 100);
	MeterBar->SetPercent(Value / 100.f);
	MeterText->SetText(FText::FromString(FString::Printf(TEXT("%s: %d%%"), *Def->MeterLabel, Value)));
}

void UGRDialogScreen::ShowNode(int32 Index)
{
	UGRStorySubsystem* Story = GetStory();
	const FGRDialogDef* Def = Story ? Story->FindDialog(DialogId) : nullptr;
	if (!Def)
	{
		Finish();
		return;
	}
	// Nodes whose condition fails are skipped (always to the next node in the list).
	while (Def->Nodes.IsValidIndex(Index) && !Story->Check(Def->Nodes[Index].If))
	{
		++Index;
	}
	if (!Def->Nodes.IsValidIndex(Index))
	{
		Finish();
		return;
	}

	NodeIndex = Index;
	const FGRDialogNode& Node = Def->Nodes[Index];
	const FString DoActions = Node.Do;
	const FGRSpeakerDef* Speaker = Story->FindSpeaker(Node.Speaker);
	const FString SpeakerName = Speaker ? Speaker->Name : FString();
	const FLinearColor SpeakerColor = Speaker ? GRStoryUtil::ParseColor(Speaker->Color) : GRStyle::ColorTextDim;
	FullText = Node.Text;

	ChoiceBox->ClearChildren();
	DefaultFocus = nullptr;
	bHasChoices = Node.Choices.ContainsByPredicate([Story](const FGRDialogChoice& C) { return Story->Check(C.If); });

	if (ChatList)
	{
		const bool bMine = Node.Speaker == TEXT("bayu");
		UBorder* Bubble = GRStyle::MakePanel(WidgetTree, bMine ? FLinearColor(0.12f, 0.32f, 0.2f, 1.f) : FLinearColor(0.16f, 0.16f, 0.18f, 1.f), FMargin(12.f, 8.f), 12.f);
		UVerticalBox* BubbleBox = WidgetTree->ConstructWidget<UVerticalBox>();
		Bubble->SetContent(BubbleBox);
		if (!bMine && !SpeakerName.IsEmpty())
		{
			BubbleBox->AddChildToVerticalBox(GRStyle::MakeText(WidgetTree, FText::FromString(SpeakerName), 12, SpeakerColor, true));
		}
		BubbleBox->AddChildToVerticalBox(Wrapped(GRStyle::MakeText(WidgetTree, FText::FromString(FullText), 16, GRStyle::ColorText)));
		UScrollBoxSlot* BubbleSlot = Cast<UScrollBoxSlot>(ChatList->AddChild(Bubble));
		BubbleSlot->SetHorizontalAlignment(bMine ? HAlign_Right : HAlign_Left);
		BubbleSlot->SetPadding(FMargin(bMine ? 50.f : 0.f, 4.f, bMine ? 0.f : 50.f, 4.f));
		ChatList->ScrollToEnd();
		Reveal = FullText.Len();
	}
	else
	{
		NameText->SetText(FText::FromString(SpeakerName));
		NameText->SetColorAndOpacity(SpeakerColor);
		NameText->SetVisibility(SpeakerName.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		BodyText->SetColorAndOpacity(SpeakerName.IsEmpty() ? GRStyle::ColorTextDim : GRStyle::ColorText);
		Reveal = 0.f;
		BodyText->SetText(FText::GetEmpty());
		if (Portrait)
		{
			const bool bShow = Speaker != nullptr;
			Portrait->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
			if (bShow)
			{
				Portrait->SetBrushColor(SpeakerColor);
				PortraitGlyph->SetText(FText::FromString(SpeakerName.Left(1)));
				PortraitName->SetText(FText::FromString(Speaker->Role.IsEmpty() ? SpeakerName : Speaker->Role));
			}
		}
	}

	HintText->SetText(bHasChoices ? FText::GetEmpty() : LOCTEXT("DialogHint", "E / Spasi / klik: lanjut"));
	Story->Run(DoActions);
	RefreshMeter();

	if (bHasChoices && Reveal >= FullText.Len())
	{
		// Chat shows its choices right away; the other styles after the text finishes typing.
		Reveal = FullText.Len() + 0.5f;
		NativeTick(GetCachedGeometry(), 0.f);
	}
}

void UGRDialogScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bFinished)
	{
		// Finished while being built (before it was on the UI stack).
		Close();
		return;
	}
	if (NodeIndex == INDEX_NONE)
	{
		return;
	}
	const bool bWasRevealing = Reveal < FullText.Len() + 0.5f;
	if (Reveal < FullText.Len())
	{
		Reveal = FMath::Min<float>(Reveal + InDeltaTime * 55.f, FullText.Len());
		if (BodyText)
		{
			BodyText->SetText(FText::FromString(FullText.Left(FMath::FloorToInt(Reveal))));
		}
		return;
	}
	if (!bWasRevealing && ChoiceBox->GetChildrenCount() > 0)
	{
		return;
	}
	if (bHasChoices && ChoiceBox->GetChildrenCount() == 0)
	{
		UGRStorySubsystem* Story = GetStory();
		const FGRDialogDef* Def = Story->FindDialog(DialogId);
		for (const FGRDialogChoice& Choice : Def->Nodes[NodeIndex].Choices)
		{
			if (!Story->Check(Choice.If))
			{
				continue;
			}
			const FGRDialogChoice Copy = Choice;
			AddButton(ChoiceBox, FText::FromString(Choice.Text), [this, Copy]()
			{
				if (bFinished)
				{
					return;
				}
				if (ChatList)
				{
					UBorder* Bubble = GRStyle::MakePanel(WidgetTree, FLinearColor(0.12f, 0.32f, 0.2f, 1.f), FMargin(12.f, 8.f), 12.f);
					Bubble->SetContent(Wrapped(GRStyle::MakeText(WidgetTree, FText::FromString(Copy.Text), 16, GRStyle::ColorText)));
					UScrollBoxSlot* BubbleSlot = Cast<UScrollBoxSlot>(ChatList->AddChild(Bubble));
					BubbleSlot->SetHorizontalAlignment(HAlign_Right);
					BubbleSlot->SetPadding(FMargin(50.f, 4.f, 0.f, 4.f));
				}
				UGRStorySubsystem* S = GetStory();
				S->Run(Copy.Do);
				GoTo(Copy.Next, NodeIndex + 1);
			});
		}
		OnShown();
	}
	Reveal = FullText.Len() + 1.f;
}

void UGRDialogScreen::GoTo(const FString& NodeId, int32 Fallback)
{
	const UGRStorySubsystem* Story = GetStory();
	const FGRDialogDef* Def = Story ? Story->FindDialog(DialogId) : nullptr;
	if (!Def || NodeId == TEXT("end"))
	{
		Finish();
		return;
	}
	if (NodeId.IsEmpty())
	{
		ShowNode(Fallback);
		return;
	}
	const int32 Found = Def->Nodes.IndexOfByPredicate([&NodeId](const FGRDialogNode& N) { return N.Id == NodeId; });
	if (Found == INDEX_NONE)
	{
		Finish();
		return;
	}
	ShowNode(Found);
}

void UGRDialogScreen::Advance()
{
	if (bFinished || NodeIndex == INDEX_NONE)
	{
		return;
	}
	if (Reveal < FullText.Len())
	{
		Reveal = FullText.Len();
		if (BodyText)
		{
			BodyText->SetText(FText::FromString(FullText));
		}
		return;
	}
	if (bHasChoices)
	{
		return;
	}
	const FGRDialogDef* Def = GetStory()->FindDialog(DialogId);
	GoTo(Def->Nodes[NodeIndex].Next, NodeIndex + 1);
}

void UGRDialogScreen::Finish()
{
	if (!bFinished)
	{
		bFinished = true;
		Close();
	}
}

FReply UGRDialogScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsAdvanceKey(InKeyEvent.GetKey()) && !bHasChoices)
	{
		Advance();
		return FReply::Handled();
	}
	if (IsAdvanceKey(InKeyEvent.GetKey()) && Reveal < FullText.Len())
	{
		Advance();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UGRDialogScreen::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bHasChoices || Reveal < FullText.Len())
	{
		Advance();
	}
	return FReply::Handled();
}

// ---------------------------------------------------------------------------------------------
// Cutscene

FString UGRCutsceneScreen::PendingId;

UGRCutsceneScreen::UGRCutsceneScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bDimBackground = false;
}

bool UGRCutsceneScreen::Open(UGRUISubsystem* UI, const FString& CutsceneId)
{
	if (!UI || !UI->GetLocalPlayer()->GetGameInstance()->GetSubsystem<UGRStorySubsystem>()->FindCutscene(CutsceneId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Cutscene tidak ditemukan: %s"), *CutsceneId);
		return false;
	}
	PendingId = CutsceneId;
	return UI->PushScreen<UGRCutsceneScreen>() != nullptr;
}

void UGRCutsceneScreen::BuildScreen(UOverlay* Root)
{
	CutsceneId = PendingId;
	for (int32 i = 0; i < 2; ++i)
	{
		USizeBox* Bar = WidgetTree->ConstructWidget<USizeBox>();
		Bar->SetHeightOverride(i == 0 ? 90.f : 170.f);
		UBorder* Black = WidgetTree->ConstructWidget<UBorder>();
		Black->SetBrushColor(FLinearColor::Black);
		Bar->SetContent(Black);
		UOverlaySlot* BarSlot = Root->AddChildToOverlay(Bar);
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetVerticalAlignment(i == 0 ? VAlign_Top : VAlign_Bottom);
		if (i == 1)
		{
			UVerticalBox* CaptionBox = WidgetTree->ConstructWidget<UVerticalBox>();
			USizeBox* CaptionSize = WidgetTree->ConstructWidget<USizeBox>();
			CaptionSize->SetWidthOverride(1100.f);
			CaptionSize->SetContent(CaptionBox);
			Black->SetContent(CaptionSize);
			Black->SetHorizontalAlignment(HAlign_Center);
			Black->SetVerticalAlignment(VAlign_Center);
			Black->SetPadding(FMargin(20.f, 10.f));
			SpeakerText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 18, GRStyle::ColorAccent, true);
			CaptionBox->AddChildToVerticalBox(SpeakerText)->SetHorizontalAlignment(HAlign_Center);
			CaptionText = Wrapped(GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 22, GRStyle::ColorText));
			CaptionText->SetJustification(ETextJustify::Center);
			CaptionBox->AddChildToVerticalBox(CaptionText)->SetHorizontalAlignment(HAlign_Fill);
		}
	}
	TitleText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 46, GRStyle::ColorAccent, true);
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetShadowOffset(FVector2D(2.f, 2.f));
	TitleText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	UOverlaySlot* TitleSlot = Root->AddChildToOverlay(TitleText);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetVerticalAlignment(VAlign_Center);
	UTextBlock* Skip = GRStyle::MakeText(WidgetTree, LOCTEXT("CutSkip", "Esc: lewati"), 13, GRStyle::ColorTextDim);
	UOverlaySlot* SkipSlot = Root->AddChildToOverlay(Skip);
	SkipSlot->SetHorizontalAlignment(HAlign_Right);
	SkipSlot->SetVerticalAlignment(VAlign_Top);
	SkipSlot->SetPadding(FMargin(0.f, 34.f, 40.f, 0.f));

	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->PlayerCameraManager)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Camera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), PC->PlayerCameraManager->GetCameraLocation(), PC->PlayerCameraManager->GetCameraRotation(), Params);
		if (Camera)
		{
			PC->SetViewTargetWithBlend(Camera, 0.f);
		}
	}
	StartStep(0);
}

void UGRCutsceneScreen::StartStep(int32 Index)
{
	UGRStorySubsystem* Story = StoryOf(this);
	const FGRCutsceneDef* Def = Story ? Story->FindCutscene(CutsceneId) : nullptr;
	if (!Def || !Def->Steps.IsValidIndex(Index))
	{
		Finish();
		return;
	}
	StepIndex = Index;
	StepTime = 0.f;
	FadeFrom = FadeNow;
	const FGRCutStep& Step = Def->Steps[Index];
	if (Camera)
	{
		CamFrom = Camera->GetActorLocation();
		RotFrom = Camera->GetActorRotation();
	}
	if (Step.Cam.Num() >= 5)
	{
		CamFrom = FVector(Step.Cam[0], Step.Cam[1], Step.Cam[2]);
		RotFrom = FRotator(Step.Cam[3], Step.Cam[4], 0.f);
	}
	CamTo = CamFrom;
	RotTo = RotFrom;
	if (Step.Cam2.Num() >= 5)
	{
		CamTo = FVector(Step.Cam2[0], Step.Cam2[1], Step.Cam2[2]);
		RotTo = FRotator(Step.Cam2[3], Step.Cam2[4], 0.f);
	}
	TitleText->SetText(FText::FromString(Step.Title));
	CaptionText->SetText(FText::FromString(Step.Text));
	const FGRSpeakerDef* Speaker = Story->FindSpeaker(Step.Speaker);
	SpeakerText->SetText(Speaker ? FText::FromString(Speaker->Name) : FText::GetEmpty());
	Story->Run(Step.Do);
}

void UGRCutsceneScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bDone)
	{
		Close();
		return;
	}
	const UGRStorySubsystem* Story = StoryOf(this);
	const FGRCutsceneDef* Def = Story ? Story->FindCutscene(CutsceneId) : nullptr;
	if (!Def || !Def->Steps.IsValidIndex(StepIndex))
	{
		return;
	}
	const FGRCutStep& Step = Def->Steps[StepIndex];
	StepTime += InDeltaTime;
	const float Alpha = FMath::Clamp(StepTime / FMath::Max(Step.T, 0.01f), 0.f, 1.f);
	const float Smooth = FMath::SmoothStep(0.f, 1.f, Alpha);
	if (Camera)
	{
		Camera->SetActorLocationAndRotation(FMath::Lerp(CamFrom, CamTo, Smooth), FMath::Lerp(RotFrom, RotTo, Smooth));
	}
	if (Step.Fade >= 0.f)
	{
		FadeNow = FMath::Lerp(FadeFrom, Step.Fade, FMath::Clamp(StepTime / FMath::Min(Step.T, 1.2f), 0.f, 1.f));
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC->PlayerCameraManager->SetManualCameraFade(FadeNow, FLinearColor::Black, false);
		}
	}
	if (StepTime >= Step.T)
	{
		StartStep(StepIndex + 1);
	}
}

void UGRCutsceneScreen::HandleBack()
{
	UGRStorySubsystem* Story = StoryOf(this);
	const FGRCutsceneDef* Def = Story ? Story->FindCutscene(CutsceneId) : nullptr;
	if (bDone || !Def)
	{
		return;
	}
	const TArray<FGRCutStep> Remaining(Def->Steps.GetData() + StepIndex + 1, FMath::Max(0, Def->Steps.Num() - StepIndex - 1));
	StepIndex = Def->Steps.Num();
	for (const FGRCutStep& Step : Remaining)
	{
		Story->Run(Step.Do);
	}
	Finish();
}

void UGRCutsceneScreen::Finish()
{
	if (bDone)
	{
		return;
	}
	bDone = true;
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (PC->GetPawn())
		{
			PC->SetViewTargetWithBlend(PC->GetPawn(), 0.f);
		}
		PC->PlayerCameraManager->StartCameraFade(FadeNow, 0.f, FadeNow > 0.f ? 0.8f : 0.01f, FLinearColor::Black, false, false);
	}
	if (Camera)
	{
		Camera->Destroy();
		Camera = nullptr;
	}
	UGRStorySubsystem* Story = StoryOf(this);
	const FGRCutsceneDef* Def = Story ? Story->FindCutscene(CutsceneId) : nullptr;
	const FString EndActions = Def ? Def->End : FString();
	Close();
	if (Story)
	{
		Story->Run(EndActions);
	}
}

void UGRCutsceneScreen::NativeDestruct()
{
	if (Camera)
	{
		Camera->Destroy();
		Camera = nullptr;
	}
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------
// Shop

FString UGRShopScreen::PendingPlace;

bool UGRShopScreen::Open(UGRUISubsystem* UI, const FString& Place)
{
	if (!UI)
	{
		return false;
	}
	PendingPlace = Place;
	return UI->PushScreen<UGRShopScreen>() != nullptr;
}

void UGRShopScreen::BuildScreen(UOverlay* Root)
{
	Place = PendingPlace;
	UVerticalBox* Box = MakeCenteredPanel(Root, FText::FromString(Place == TEXT("pasar") ? TEXT("Pasar Sentosa") : TEXT("Warung Bu Tini")), 640.f);
	Box->AddChildToVerticalBox(Wrapped(GRStyle::MakeText(WidgetTree, LOCTEXT("ShopNote", "Harga hari ini. Besok mungkin naik lagi, kata pedagang \"karena asap\"."), 14, GRStyle::ColorTextDim)));
	MoneyText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 18, GRStyle::ColorGood, true);
	Box->AddChildToVerticalBox(MoneyText)->SetPadding(FMargin(0.f, 8.f));
	List = WidgetTree->ConstructWidget<UVerticalBox>();
	Box->AddChildToVerticalBox(List);
	AddButton(Box, LOCTEXT("ShopClose", "Selesai belanja"), [this]() { Close(); });
	Rebuild();
}

void UGRShopScreen::Rebuild()
{
	UGRStorySubsystem* Story = StoryOf(this);
	UGRStatusSubsystem* Status = GetGameInstance()->GetSubsystem<UGRStatusSubsystem>();
	MoneyText->SetText(FText::FromString(FString::Printf(TEXT("Uang: %s"), *UGRStatusSubsystem::FormatRupiah(Status->GetUang()))));
	List->ClearChildren();
	DefaultFocus = nullptr;
	for (const FGRShopEntry& Entry : Story->GetData().Shop)
	{
		if (Entry.Place != Place)
		{
			continue;
		}
		const FGRItemDef* Item = Story->FindItem(Entry.Item);
		const int32 Price = Story->GetShopPrice(Entry);
		const FString Label = FString::Printf(TEXT("%s  -  %s   (punya %d)"), Item ? *Item->Name : *Entry.Item, *UGRStatusSubsystem::FormatRupiah(Price), Story->GetItemCount(Entry.Item));
		const FString ItemId = Entry.Item;
		const FString ItemName = Item ? Item->Name : Entry.Item;
		AddButton(List, FText::FromString(Label), [this, ItemId, ItemName, Price]()
		{
			UGRStorySubsystem* S = StoryOf(this);
			if (GetGameInstance()->GetSubsystem<UGRStatusSubsystem>()->SpendUang(Price))
			{
				S->AddLedger(-Price, FString::Printf(TEXT("Beli %s"), *ItemName));
				S->Run(FString::Printf(TEXT("item %s 1"), *ItemId));
			}
			else
			{
				S->Run(TEXT("notifburuk Uangmu tidak cukup."));
			}
			Rebuild();
			OnShown();
		});
	}
}

// ---------------------------------------------------------------------------------------------
// Journal

UGRJournalScreen::UGRJournalScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPausesGame = true;
}

void UGRJournalScreen::BuildScreen(UOverlay* Root)
{
	UVerticalBox* Box = MakeCenteredPanel(Root, LOCTEXT("JournalTitle", "Jurnal Bayu"), 980.f);
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Box->AddChildToVerticalBox(Row);
	USizeBox* Left = WidgetTree->ConstructWidget<USizeBox>();
	Left->SetWidthOverride(220.f);
	Row->AddChildToHorizontalBox(Left);
	Buttons = WidgetTree->ConstructWidget<UVerticalBox>();
	Left->SetContent(Buttons);
	AddButton(Buttons, LOCTEXT("JTasks", "Tugas"), [this]() { ShowTab(0); });
	AddButton(Buttons, LOCTEXT("JItems", "Barang"), [this]() { ShowTab(1); });
	AddButton(Buttons, LOCTEXT("JFamily", "Bayu & Keluarga"), [this]() { ShowTab(2); });
	AddButton(Buttons, LOCTEXT("JClose", "Tutup (J)"), [this]() { Close(); });

	USizeBox* Right = WidgetTree->ConstructWidget<USizeBox>();
	Right->SetHeightOverride(540.f);
	Right->SetWidthOverride(680.f);
	UHorizontalBoxSlot* RightSlot = Row->AddChildToHorizontalBox(Right);
	RightSlot->SetPadding(FMargin(24.f, 0.f, 0.f, 0.f));
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Right->SetContent(Scroll);
	Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(Content);
	ShowTab(0);
}

void UGRJournalScreen::ShowTab(int32 Tab)
{
	UGRStorySubsystem* Story = StoryOf(this);
	const UGameInstance* GI = GetGameInstance();
	Content->ClearChildren();
	auto Line = [this](const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = false, float Top = 2.f)
	{
		Content->AddChildToVerticalBox(Wrapped(GRStyle::MakeText(WidgetTree, FText::FromString(Text), Size, Color, bBold)))->SetPadding(FMargin(0.f, Top, 0.f, 2.f));
	};

	if (Tab == 0)
	{
		const TArray<const FGRQuestDef*> Active = Story->GetActiveQuests();
		if (Active.Num() == 0)
		{
			Line(TEXT("Tidak ada tugas aktif."), 16, GRStyle::ColorTextDim);
		}
		for (const FGRQuestDef* Quest : Active)
		{
			const int32 Step = Story->GetQuestStep(Quest->Id);
			Line(Quest->Title, 22, GRStyle::ColorAccent, true, 10.f);
			Line(Quest->Desc, 14, GRStyle::ColorTextDim);
			for (int32 i = 0; i < FMath::Min(Step, Quest->Steps.Num()); ++i)
			{
				const bool bCurrent = i == Step - 1;
				Line(FString::Printf(TEXT("%s %s"), bCurrent ? TEXT(">") : TEXT("v"), *Quest->Steps[i]), 16, bCurrent ? GRStyle::ColorText : GRStyle::ColorTextDim, bCurrent);
			}
		}
		const TArray<const FGRQuestDef*> Done = Story->GetDoneQuests();
		if (Done.Num() > 0)
		{
			Line(TEXT("SELESAI"), 13, GRStyle::ColorTextDim, true, 16.f);
			for (const FGRQuestDef* Quest : Done)
			{
				Line(Quest->Title, 15, GRStyle::ColorGood);
			}
		}
	}
	else if (Tab == 1)
	{
		bool bAny = false;
		for (const FGRItemDef& Item : Story->GetData().Items)
		{
			const int32 Count = Story->GetItemCount(Item.Id);
			if (Count <= 0)
			{
				continue;
			}
			bAny = true;
			Line(FString::Printf(TEXT("%s%s  x%d"), Item.Bukti ? TEXT("[Bukti] ") : TEXT(""), *Item.Name, Count), 18, Item.Bukti ? GRStyle::ColorDanger : GRStyle::ColorText, true, 8.f);
			Line(Item.Desc, 14, GRStyle::ColorTextDim);
			if (!Item.Use.IsEmpty())
			{
				const FString Id = Item.Id;
				const FString Use = Item.Use;
				AddButton(Content, FText::FromString(FString::Printf(TEXT("Pakai %s"), *Item.Name)), [this, Id, Use]()
				{
					UGRStorySubsystem* S = StoryOf(this);
					S->SetVar(TEXT("item.") + Id, FMath::Max(0, S->GetItemCount(Id) - 1));
					S->Run(Use);
					ShowTab(1);
				});
			}
		}
		if (!bAny)
		{
			Line(TEXT("Tas Bayu kosong."), 16, GRStyle::ColorTextDim);
		}
	}
	else
	{
		const UGRStatusSubsystem* Status = GI->GetSubsystem<UGRStatusSubsystem>();
		const UGRClockSubsystem* Clock = GI->GetSubsystem<UGRClockSubsystem>();
		Line(TEXT("Bayu"), 22, GRStyle::ColorAccent, true);
		Line(FString::Printf(TEXT("Energi: %d / 100"), FMath::RoundToInt(Story->GetEnergi())), 16, GRStyle::ColorText);
		Line(FString::Printf(TEXT("Uang: %s"), *UGRStatusSubsystem::FormatRupiah(Status->GetUang())), 16, GRStyle::ColorText);
		Line(FString::Printf(TEXT("Suara warga: %d   |   Diawasi: %d"), FMath::RoundToInt(Status->GetSuara()), FMath::RoundToInt(Status->GetDiawasi())), 16, GRStyle::ColorText);
		Line(FString::Printf(TEXT("Hari %d, %s. Udara: %s (ISPU %d). Klaim resmi: \"%s\""), Clock->GetDay(), *Clock->GetClockText().ToString(),
			*Clock->GetSmokeCategory().ToString(), FMath::RoundToInt(Clock->GetIspu()), *Clock->GetOfficialClaim()), 14, GRStyle::ColorTextDim);
		Line(TEXT("Keluarga"), 22, GRStyle::ColorAccent, true, 14.f);
		Line(TEXT("Ibu (Bu Sri): menjahit di rumah, batuk sejak asap datang."), 15, GRStyle::ColorText);
		Line(Story->GetVar(TEXT("dinda_sakit")) > 0 ? TEXT("Dinda: sakit setelah makan PPR. Perlu obat dan makan teratur.") : TEXT("Dinda: kelas 4 SD, makan siang dari PPR."), 15, GRStyle::ColorText);
		Line(TEXT("Kebutuhan tiap pagi:"), 15, GRStyle::ColorTextDim, true, 10.f);
		for (const FGRNeedDef& Need : Story->GetData().Needs)
		{
			if (!Story->Check(Need.If))
			{
				continue;
			}
			const FGRItemDef* Item = Story->FindItem(Need.Item);
			const int32 Have = Story->GetItemCount(Need.Item);
			Line(FString::Printf(TEXT("- %s (punya %d)"), Item ? *Item->Name : *Need.Item, Have), 15, Have > 0 ? GRStyle::ColorGood : GRStyle::ColorDanger);
		}
		Line(TEXT("Tips: tidur hanya bisa pukul 18.00-04.00 di kasur rumah. Masker mengurangi lelah karena asap."), 13, GRStyle::ColorTextDim, false, 12.f);
	}
}

FReply UGRJournalScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::J)
	{
		Close();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ---------------------------------------------------------------------------------------------
// Chapter end

UGRChapterEndScreen::UGRChapterEndScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPausesGame = true;
	bCanClose = false;
}

void UGRChapterEndScreen::BuildScreen(UOverlay* Root)
{
	UGRStorySubsystem* Story = StoryOf(this);
	const UGRStatusSubsystem* Status = GetGameInstance()->GetSubsystem<UGRStatusSubsystem>();
	UVerticalBox* Box = MakeCenteredPanel(Root, LOCTEXT("EndTitle", "Bab 1 Selesai: Asap dan Piring"), 820.f);
	auto Line = [this, Box](const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = false)
	{
		Box->AddChildToVerticalBox(Wrapped(GRStyle::MakeText(WidgetTree, FText::FromString(Text), Size, Color, bBold)))->SetPadding(FMargin(0.f, 3.f));
	};
	const int32 Suara = FMath::RoundToInt(Status->GetSuara());
	Line(Suara >= 40 ? TEXT("Kota Sentosa mulai bersuara. Unggahanmu dibicarakan di pos ronda sampai pasar.")
		: TEXT("Sebagian warga mendengar ceritamu. Sebagian lagi masih takut."), 18, GRStyle::ColorText);
	Line(Story->GetVar(TEXT("guru_dilindungi")) > 0 ? TEXT("Kamu melindungi Bu Ratna. Namanya tidak pernah keluar dari mulutmu.")
		: TEXT("Nama Bu Ratna sampai ke telinga SPN. Ia dipindahtugaskan ke pulau seberang."), 16, GRStyle::ColorTextDim);
	Line(Story->GetVar(TEXT("pungli_lapor")) > 0 ? TEXT("Rekaman pungli kelurahan ikut tersebar.") : TEXT("Pungli di kelurahan masih berjalan seperti biasa."), 16, GRStyle::ColorTextDim);
	Line(FString::Printf(TEXT("Suara warga %d  |  Diawasi %d  |  Uang %s  |  Order selesai %d"), Suara, FMath::RoundToInt(Status->GetDiawasi()),
		*UGRStatusSubsystem::FormatRupiah(Status->GetUang()), Story->GetVar(TEXT("orders_done"))), 15, GRStyle::ColorAccent, true);
	Line(TEXT("Bersambung di Bab 2: Turun ke Jalan."), 20, GRStyle::ColorAccent, true);
	AddButton(Box, LOCTEXT("EndContinue", "Lanjut menjelajah Kota Sentosa"), [this]() { Close(); });
	AddButton(Box, LOCTEXT("EndMenu", "Kembali ke menu utama"), [this]()
	{
		GetGameInstance()->GetSubsystem<UGRGameFlowSubsystem>()->ReturnToMainMenu();
	});
}

TWeakObjectPtr<AGRStealthLevel> UGRStealthCaughtScreen::PendingLevel;

UGRStealthCaughtScreen::UGRStealthCaughtScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPausesGame = true;
	bCanClose = false;
}

bool UGRStealthCaughtScreen::Open(UGRUISubsystem* UI, AGRStealthLevel* InLevel)
{
	if (!UI)
	{
		return false;
	}
	PendingLevel = InLevel;
	return UI->PushScreen<UGRStealthCaughtScreen>() != nullptr;
}

void UGRStealthCaughtScreen::BuildScreen(UOverlay* Root)
{
	Level = PendingLevel;
	UVerticalBox* Box = MakeCenteredPanel(Root, LOCTEXT("CaughtTitle", "Ketahuan penjaga dapur!"), 560.f);
	Box->AddChildToVerticalBox(Wrapped(GRStyle::MakeText(WidgetTree, LOCTEXT("CaughtBody", "Senter satpam menyorot wajahmu. Ulangi dari pintu belakang, atau kabur tanpa membawa apa-apa."), 17, GRStyle::ColorText)))->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	AddButton(Box, LOCTEXT("CaughtRetry", "Coba lagi"), [this]()
	{
		TWeakObjectPtr<AGRStealthLevel> Target = Level;
		Close();
		if (Target.IsValid())
		{
			Target->Retry();
		}
	});
	AddButton(Box, LOCTEXT("CaughtGiveUp", "Kabur tanpa bukti"), [this]()
	{
		TWeakObjectPtr<AGRStealthLevel> Target = Level;
		Close();
		if (Target.IsValid())
		{
			Target->GiveUp();
		}
	});
}

#undef LOCTEXT_NAMESPACE
