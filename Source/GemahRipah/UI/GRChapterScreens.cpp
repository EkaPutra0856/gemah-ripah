#include "UI/GRChapterScreens.h"
#include "UI/GRMenuButton.h"
#include "UI/GRUIStyle.h"
#include "UI/GRUISubsystem.h"
#include "Story/GRStorySubsystem.h"
#include "Story/GRStoryUtil.h"
#include "Systems/GRGameFlowSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

namespace
{
	constexpr float MapWidth = 900.f;
	constexpr float MapHeight = 660.f;
	constexpr float NodeSize = 92.f;

	UGRStorySubsystem* StoryFrom(const UUserWidget* Widget)
	{
		const UGameInstance* GI = Widget ? Widget->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UGRStorySubsystem>() : nullptr;
	}

	FVector2D MapPos(const FGRChapterDef& Def)
	{
		return FVector2D(Def.Pos.Num() > 0 ? Def.Pos[0] : 0.5f, Def.Pos.Num() > 1 ? Def.Pos[1] : 0.5f) * FVector2D(MapWidth, MapHeight);
	}

	const TCHAR* StateIcon(EGRChapterState State)
	{
		switch (State)
		{
		case EGRChapterState::Done: return TEXT("status_selesai");
		case EGRChapterState::Current: return TEXT("status_sekarang");
		case EGRChapterState::Locked:
		case EGRChapterState::Soon: return TEXT("status_terkunci");
		default: return nullptr;
		}
	}

	FText StateLabel(EGRChapterState State)
	{
		switch (State)
		{
		case EGRChapterState::Done: return LOCTEXT("ChapDone", "SELESAI");
		case EGRChapterState::Current: return LOCTEXT("ChapNow", "SEDANG DIMAINKAN");
		case EGRChapterState::Open: return LOCTEXT("ChapOpen", "TERBUKA");
		case EGRChapterState::Locked: return LOCTEXT("ChapLocked", "TERKUNCI");
		default: return LOCTEXT("ChapSoon", "SEGERA HADIR");
		}
	}

	FLinearColor StateColor(EGRChapterState State)
	{
		switch (State)
		{
		case EGRChapterState::Done: return GRStyle::ColorGood;
		case EGRChapterState::Current: return GRStyle::ColorAccent;
		case EGRChapterState::Open: return GRStyle::ColorInfo;
		default: return GRStyle::ColorTextDim;
		}
	}

	UImage* MakeIcon(UWidgetTree* Tree, const FString& Name, float Size, const FLinearColor& Tint = FLinearColor::White)
	{
		UImage* Image = Tree->ConstructWidget<UImage>();
		if (UTexture2D* Texture = GRStyle::Icon(Name))
		{
			Image->SetBrushFromTexture(Texture);
		}
		Image->SetDesiredSizeOverride(FVector2D(Size, Size));
		Image->SetColorAndOpacity(Tint);
		return Image;
	}
}

// ---------------------------------------------------------------------------------------------
// Node

void UGRChapterNode::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	WidgetTree->RootWidget = Column;

	UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>();
	Column->AddChildToVerticalBox(Stack)->SetHorizontalAlignment(HAlign_Center);
	Disc = Stack;

	USizeBox* RingSize = WidgetTree->ConstructWidget<USizeBox>();
	RingSize->SetWidthOverride(NodeSize);
	RingSize->SetHeightOverride(NodeSize);
	Stack->AddChildToOverlay(RingSize);
	Ring = WidgetTree->ConstructWidget<UBorder>();
	Ring->SetBrush(GRStyle::RoundedBrush(FLinearColor::White, NodeSize * 0.5f));
	Ring->SetPadding(FMargin(5.f));
	RingSize->SetContent(Ring);

	Button = WidgetTree->ConstructWidget<UButton>();
	Ring->SetContent(Button);
	IconImage = WidgetTree->ConstructWidget<UImage>();
	IconImage->SetDesiredSizeOverride(FVector2D(48.f, 48.f));
	Button->SetContent(IconImage);

	USizeBox* BadgeSize = WidgetTree->ConstructWidget<USizeBox>();
	BadgeSize->SetWidthOverride(30.f);
	BadgeSize->SetHeightOverride(30.f);
	UOverlaySlot* BadgeSlot = Stack->AddChildToOverlay(BadgeSize);
	BadgeSlot->SetHorizontalAlignment(HAlign_Right);
	BadgeSlot->SetVerticalAlignment(VAlign_Bottom);
	Badge = WidgetTree->ConstructWidget<UBorder>();
	Badge->SetBrush(GRStyle::RoundedBrush(FLinearColor::White, 15.f));
	Badge->SetPadding(FMargin(6.f));
	BadgeSize->SetContent(Badge);
	BadgeImage = WidgetTree->ConstructWidget<UImage>();
	Badge->SetContent(BadgeImage);

	NumberText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 13, GRStyle::ColorTextDim, true);
	Column->AddChildToVerticalBox(NumberText)->SetHorizontalAlignment(HAlign_Center);
	TitleText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 16, GRStyle::ColorText, true);
	Column->AddChildToVerticalBox(TitleText)->SetHorizontalAlignment(HAlign_Center);

	Button->OnClicked.AddDynamic(this, &UGRChapterNode::HandleClicked);
	Button->OnHovered.AddDynamic(this, &UGRChapterNode::HandleHovered);
}

void UGRChapterNode::Setup(const FGRChapterDef& Def, EGRChapterState InState)
{
	State = InState;
	Color = GRStoryUtil::ParseColor(Def.Color);
	const bool bLocked = State == EGRChapterState::Locked || State == EGRChapterState::Soon;
	const FLinearColor Fill = bLocked ? FLinearColor(0.12f, 0.12f, 0.13f, 1.f) : Color * FLinearColor(0.55f, 0.55f, 0.55f, 1.f);

	FButtonStyle Style;
	Style.SetNormal(GRStyle::RoundedBrush(Fill, NodeSize * 0.5f));
	Style.SetHovered(GRStyle::RoundedBrush(Fill * 1.25f, NodeSize * 0.5f));
	Style.SetPressed(GRStyle::RoundedBrush(Fill * 0.7f, NodeSize * 0.5f));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	Button->SetStyle(Style);

	if (UTexture2D* Texture = GRStyle::Icon(Def.Icon))
	{
		IconImage->SetBrushFromTexture(Texture);
		IconImage->SetDesiredSizeOverride(FVector2D(48.f, 48.f));
	}
	IconImage->SetColorAndOpacity(bLocked ? FLinearColor(1.f, 1.f, 1.f, 0.35f) : FLinearColor::White);
	Ring->SetBrushColor(bLocked ? FLinearColor(0.25f, 0.25f, 0.27f, 1.f) : StateColor(State));

	const TCHAR* BadgeIcon = StateIcon(State);
	Badge->SetVisibility(BadgeIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (BadgeIcon)
	{
		Badge->SetBrushColor(StateColor(State));
		if (UTexture2D* Texture = GRStyle::Icon(BadgeIcon))
		{
			BadgeImage->SetBrushFromTexture(Texture);
		}
		BadgeImage->SetColorAndOpacity(State == EGRChapterState::Done || State == EGRChapterState::Current ? FLinearColor(0.05f, 0.05f, 0.05f, 1.f) : FLinearColor::White);
	}
	NumberText->SetText(FText::Format(LOCTEXT("ChapNum", "BAB {0}"), Def.Number));
	TitleText->SetText(FText::FromString(Def.Title));
	TitleText->SetColorAndOpacity(bLocked ? GRStyle::ColorTextDim : GRStyle::ColorText);
}

void UGRChapterNode::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bFocused = true;
	if (OnFocused)
	{
		OnFocused();
	}
}

void UGRChapterNode::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bFocused = false;
}

void UGRChapterNode::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const float Dt = FMath::Min(InDeltaTime, 1.f / 20.f);
	Time += Dt;
	Highlight = FMath::FInterpTo(Highlight, bFocused ? 1.f : 0.f, Dt, 14.f);
	const float Press = Button && Button->IsPressed() ? 0.06f : 0.f;
	// The chapter being played breathes; the focused one grows.
	const float Breathe = State == EGRChapterState::Current ? 0.04f * FMath::Sin(Time * 3.f) : 0.f;
	FWidgetTransform Transform;
	const float Scale = 1.f + 0.14f * Highlight + Breathe - Press;
	Transform.Scale = FVector2D(Scale, Scale);
	Disc->SetRenderTransform(Transform);
	TitleText->SetColorAndOpacity(bFocused ? GRStyle::ColorAccent : (State == EGRChapterState::Locked || State == EGRChapterState::Soon ? GRStyle::ColorTextDim : GRStyle::ColorText));
}

void UGRChapterNode::HandleClicked()
{
	if (OnClicked)
	{
		OnClicked();
	}
}

void UGRChapterNode::HandleHovered()
{
	if (GetOwningPlayer())
	{
		Button->SetUserFocus(GetOwningPlayer());
	}
}

// ---------------------------------------------------------------------------------------------
// Map

FString UGRChapterMapScreen::PendingSelect;

UGRChapterMapScreen::UGRChapterMapScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPausesGame = true;
}

UGRChapterMapScreen* UGRChapterMapScreen::Open(UGRUISubsystem* UI, const FString& SelectId)
{
	if (!UI)
	{
		return nullptr;
	}
	PendingSelect = SelectId;
	return UI->PushScreen<UGRChapterMapScreen>();
}

EGRChapterState UGRChapterMapScreen::GetState(const FGRChapterDef& Def) const
{
	const UGRStorySubsystem* Story = StoryFrom(this);
	if (Def.File.IsEmpty())
	{
		return EGRChapterState::Soon;
	}
	if (Story->IsChapterDone(Def.Id))
	{
		return EGRChapterState::Done;
	}
	if (Def.Id == Story->GetChapterId())
	{
		return EGRChapterState::Current;
	}
	return Story->IsChapterUnlocked(Def.Id) ? EGRChapterState::Open : EGRChapterState::Locked;
}

void UGRChapterMapScreen::BuildScreen(UOverlay* Root)
{
	UGRStorySubsystem* Story = StoryFrom(this);
	const TArray<FGRChapterDef>& Chapters = Story->GetChapters();

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(MapWidth + 480.f);
	UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size);
	SizeSlot->SetHorizontalAlignment(HAlign_Center);
	SizeSlot->SetVerticalAlignment(VAlign_Center);
	UBorder* Panel = GRStyle::MakePanel(WidgetTree, GRStyle::ColorPanel, FMargin(24.f), 14.f);
	Size->SetContent(Panel);
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Panel->SetContent(Row);

	// Left: the map. Sea, one island per chapter, a dotted road from chapter to chapter.
	USizeBox* MapSize = WidgetTree->ConstructWidget<USizeBox>();
	MapSize->SetWidthOverride(MapWidth);
	MapSize->SetHeightOverride(MapHeight);
	Row->AddChildToHorizontalBox(MapSize);
	UOverlay* MapStack = WidgetTree->ConstructWidget<UOverlay>();
	MapSize->SetContent(MapStack);
	UBorder* Sea = GRStyle::MakePanel(WidgetTree, FLinearColor(FColor(18, 34, 40)), FMargin(0.f), 12.f);
	MapStack->AddChildToOverlay(Sea)->SetHorizontalAlignment(HAlign_Fill);
	Cast<UOverlaySlot>(Sea->Slot)->SetVerticalAlignment(VAlign_Fill);
	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	UOverlaySlot* CanvasSlot = MapStack->AddChildToOverlay(Canvas);
	CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
	CanvasSlot->SetVerticalAlignment(VAlign_Fill);
	UTextBlock* MapTitle = GRStyle::MakeText(WidgetTree, LOCTEXT("ChapMapTitle", "Peta Perjalanan Bayu"), 26, GRStyle::ColorAccent, true);
	UOverlaySlot* TitleSlot = MapStack->AddChildToOverlay(MapTitle);
	TitleSlot->SetPadding(FMargin(22.f, 16.f));
	UTextBlock* MapSub = GRStyle::MakeText(WidgetTree, LOCTEXT("ChapMapSub", "Republik Gemah Ripah  |  panah: pilih bab  |  Esc: tutup"), 14, GRStyle::ColorTextDim);
	UOverlaySlot* SubSlot = MapStack->AddChildToOverlay(MapSub);
	SubSlot->SetPadding(FMargin(22.f, 52.f));

	// Road first, so the islands cover it and it only shows across the sea.
	for (int32 i = 0; i + 1 < Chapters.Num(); ++i)
	{
		const FVector2D From = MapPos(Chapters[i]);
		const FVector2D To = MapPos(Chapters[i + 1]);
		const bool bWalked = Story->IsChapterDone(Chapters[i].Id);
		const int32 Count = FMath::Max(3, FMath::RoundToInt(FVector2D::Distance(From, To) / 22.f));
		for (int32 k = 1; k < Count; ++k)
		{
			const FVector2D At = FMath::Lerp(From, To, static_cast<float>(k) / Count);
			UBorder* Dot = GRStyle::MakePanel(WidgetTree, bWalked ? GRStyle::ColorAccent : FLinearColor(0.4f, 0.42f, 0.4f, 0.8f), FMargin(0.f), 4.f);
			UCanvasPanelSlot* DotSlot = Canvas->AddChildToCanvas(Dot);
			DotSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			DotSlot->SetPosition(At);
			DotSlot->SetSize(FVector2D(8.f, 8.f));
			Dots.Add(Dot);
		}
	}

	const FLinearColor Land(FColor(52, 60, 48));
	for (const FGRChapterDef& Def : Chapters)
	{
		const FVector2D At = MapPos(Def);
		UBorder* Island = GRStyle::MakePanel(WidgetTree, Land, FMargin(12.f, 8.f), 40.f);
		Island->SetVerticalAlignment(VAlign_Bottom);
		Island->SetHorizontalAlignment(HAlign_Center);
		Island->SetContent(GRStyle::MakeText(WidgetTree, FText::FromString(Def.Place), 12, FLinearColor(0.75f, 0.8f, 0.7f, 0.8f), true));
		UCanvasPanelSlot* IslandSlot = Canvas->AddChildToCanvas(Island);
		IslandSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		IslandSlot->SetPosition(At + FVector2D(0.f, 22.f));
		IslandSlot->SetSize(FVector2D(230.f, 200.f));
		Islands.Add(Island);
	}

	const FString SelectId = PendingSelect.IsEmpty() ? Story->GetChapterId() : PendingSelect;
	for (int32 i = 0; i < Chapters.Num(); ++i)
	{
		UGRChapterNode* Node = WidgetTree->ConstructWidget<UGRChapterNode>(UGRChapterNode::StaticClass());
		Node->Setup(Chapters[i], GetState(Chapters[i]));
		UCanvasPanelSlot* NodeSlot = Canvas->AddChildToCanvas(Node);
		NodeSlot->SetAutoSize(true);
		NodeSlot->SetAlignment(FVector2D(0.5f, 0.3f));
		NodeSlot->SetPosition(MapPos(Chapters[i]));
		NodeSlot->SetZOrder(10);
		Node->OnFocused = [this, i]() { ShowDetails(i); };
		Node->OnClicked = [this]()
		{
			// Enter on a chapter jumps to its first action (Mainkan / Tutup).
			for (int32 c = 0; c < Details->GetChildrenCount(); ++c)
			{
				if (UGRMenuButton* Button = Cast<UGRMenuButton>(Details->GetChildAt(c)))
				{
					Button->FocusButton();
					return;
				}
			}
		};
		Nodes.Add(Node);
		if (Chapters[i].Id == SelectId)
		{
			Selected = i;
		}
	}

	// Right: details of the selected chapter.
	USizeBox* DetailSize = WidgetTree->ConstructWidget<USizeBox>();
	DetailSize->SetWidthOverride(430.f);
	UHorizontalBoxSlot* DetailSlot = Row->AddChildToHorizontalBox(DetailSize);
	DetailSlot->SetPadding(FMargin(26.f, 8.f, 4.f, 8.f));
	Details = WidgetTree->ConstructWidget<UVerticalBox>();
	DetailSize->SetContent(Details);

	// Arrow keys walk the road in order instead of jumping to whatever is spatially closest.
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		UButton* Prev = Nodes[FMath::Max(i - 1, 0)]->GetButton();
		UButton* Next = Nodes[FMath::Min(i + 1, Nodes.Num() - 1)]->GetButton();
		UButton* Own = Nodes[i]->GetButton();
		Own->SetNavigationRuleExplicit(EUINavigation::Left, Prev);
		Own->SetNavigationRuleExplicit(EUINavigation::Up, Prev);
		Own->SetNavigationRuleExplicit(EUINavigation::Right, Next);
		Own->SetNavigationRuleExplicit(EUINavigation::Down, Next);
	}

	if (Nodes.Num() > 0)
	{
		const int32 First = FMath::Max(Selected, 0);
		Selected = INDEX_NONE;
		DefaultFocus = Nodes[First]->GetButton();
		ShowDetails(First);
	}
}

void UGRChapterMapScreen::ShowDetails(int32 Index)
{
	UGRStorySubsystem* Story = StoryFrom(this);
	const TArray<FGRChapterDef>& Chapters = Story->GetChapters();
	if (!Chapters.IsValidIndex(Index) || !Details)
	{
		return;
	}
	if (Index == Selected && Details->GetChildrenCount() > 0)
	{
		return;
	}
	Selected = Index;
	const FGRChapterDef& Def = Chapters[Index];
	const EGRChapterState State = GetState(Def);
	const FLinearColor Color = GRStoryUtil::ParseColor(Def.Color);
	Details->ClearChildren();

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	Details->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
	IconSize->SetWidthOverride(84.f);
	IconSize->SetHeightOverride(84.f);
	UBorder* IconBox = GRStyle::MakePanel(WidgetTree, State == EGRChapterState::Soon || State == EGRChapterState::Locked ? FLinearColor(0.2f, 0.2f, 0.22f, 1.f) : Color, FMargin(14.f), 18.f);
	IconBox->SetContent(MakeIcon(WidgetTree, Def.Icon, 56.f));
	IconSize->SetContent(IconBox);
	Header->AddChildToHorizontalBox(IconSize)->SetVerticalAlignment(VAlign_Center);
	UVerticalBox* Names = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBoxSlot* NamesSlot = Header->AddChildToHorizontalBox(Names);
	NamesSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	NamesSlot->SetVerticalAlignment(VAlign_Center);
	NamesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Names->AddChildToVerticalBox(GRStyle::MakeText(WidgetTree, FText::Format(LOCTEXT("ChapHeader", "BAB {0}"), Def.Number), 15, GRStyle::ColorTextDim, true));
	UTextBlock* Title = GRStyle::MakeText(WidgetTree, FText::FromString(Def.Title), 28, GRStyle::ColorText, true);
	Title->SetAutoWrapText(true);
	Names->AddChildToVerticalBox(Title);

	UBorder* Chip = GRStyle::MakePanel(WidgetTree, StateColor(State) * FLinearColor(1.f, 1.f, 1.f, 0.22f), FMargin(10.f, 4.f), 10.f);
	Chip->SetContent(GRStyle::MakeText(WidgetTree, StateLabel(State), 13, StateColor(State), true));
	Details->AddChildToVerticalBox(Chip)->SetHorizontalAlignment(HAlign_Left);

	auto Line = [this](const FString& Text, int32 Size, const FLinearColor& TextColor, float Top)
	{
		UTextBlock* Block = GRStyle::MakeText(WidgetTree, FText::FromString(Text), Size, TextColor);
		Block->SetAutoWrapText(true);
		Details->AddChildToVerticalBox(Block)->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
	};
	Line(Def.Subtitle, 18, Color, 14.f);
	Line(Def.Summary, 16, GRStyle::ColorText, 10.f);
	if (!Def.Place.IsEmpty())
	{
		Line(FString::Printf(TEXT("Tempat: %s"), *Def.Place), 14, GRStyle::ColorTextDim, 10.f);
	}

	FString Note;
	switch (State)
	{
	case EGRChapterState::Done: Note = TEXT("Bab ini sudah kamu selesaikan. Pilihanmu di sini terbawa ke bab berikutnya."); break;
	case EGRChapterState::Current: Note = TEXT("Kamu sedang memainkan bab ini. Cek jurnal (J) untuk tugas berikutnya."); break;
	case EGRChapterState::Open: Note = TEXT("Uang, Suara, Diawasi, aturan yang berlaku, dan pilihan-pilihanmu ikut terbawa."); break;
	case EGRChapterState::Locked: Note = TEXT("Selesaikan bab sebelumnya untuk membuka bab ini."); break;
	default: Note = TEXT("Masih ditulis. Pantau terus Kicau."); break;
	}
	Line(Note, 14, GRStyle::ColorTextDim, 14.f);

	UVerticalBox* Spacer = WidgetTree->ConstructWidget<UVerticalBox>();
	Details->AddChildToVerticalBox(Spacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	if (State == EGRChapterState::Open)
	{
		const FString Id = Def.Id;
		UGRMenuButton* Play = AddButton(Details, FText::Format(LOCTEXT("ChapPlay", "Mainkan Bab {0}"), Def.Number), [this, Id]()
		{
			GetGameInstance()->GetSubsystem<UGRGameFlowSubsystem>()->StartChapter(Id);
		});
		Play->SetIcon(FString(), Color, GRStyle::Icon(Def.Icon));
		Cast<UVerticalBoxSlot>(Play->Slot)->SetPadding(FMargin(0.f, 16.f, 0.f, 4.f));
	}
	AddButton(Details, LOCTEXT("ChapClose", "Tutup"), [this]() { Close(); });

	Anim.Stagger(Details, 0.f, 0.025f, FVector2D(18.f, 0.f), 0.2f);
}

void UGRChapterMapScreen::PlayIntro()
{
	Super::PlayIntro();
	for (int32 i = 0; i < Islands.Num(); ++i)
	{
		Anim.Add(GRAnim::Pop(Islands[i], 0.08f + 0.06f * i, 0.7f, FVector2D::ZeroVector, 0.4f));
	}
	// The road draws itself, then the chapters pop up one by one.
	for (int32 i = 0; i < Dots.Num(); ++i)
	{
		Anim.Add(GRAnim::Pop(Dots[i], 0.25f + 0.012f * i, 0.2f, FVector2D::ZeroVector, 0.2f));
	}
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		Anim.Add(GRAnim::Pop(Nodes[i], 0.2f + 0.09f * i, 0.5f, FVector2D(0.f, 20.f), 0.4f));
	}
}

#undef LOCTEXT_NAMESPACE
