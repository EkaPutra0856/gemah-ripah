#include "UI/GRAppTile.h"
#include "UI/GRUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

namespace
{
	constexpr float TileSize = 76.f;
}

void UGRAppTile::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style;
	Style.SetNormal(GRStyle::RoundedBrush(FLinearColor::Transparent, 18.f));
	Style.SetHovered(GRStyle::RoundedBrush(FLinearColor::Transparent, 18.f));
	Style.SetPressed(GRStyle::RoundedBrush(FLinearColor(1.f, 1.f, 1.f, 0.06f), 18.f));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	Button->SetStyle(Style);
	WidgetTree->RootWidget = Button;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Button->SetContent(Column);

	UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>();
	Column->AddChildToVerticalBox(Stack)->SetHorizontalAlignment(HAlign_Center);
	IconStack = Stack;

	// Soft glow behind the icon when focused.
	USizeBox* GlowSize = WidgetTree->ConstructWidget<USizeBox>();
	GlowSize->SetWidthOverride(TileSize + 12.f);
	GlowSize->SetHeightOverride(TileSize + 12.f);
	Stack->AddChildToOverlay(GlowSize);
	Glow = WidgetTree->ConstructWidget<UBorder>();
	Glow->SetBrush(GRStyle::RoundedBrush(GRStyle::ColorAccent, 24.f));
	Glow->SetRenderOpacity(0.f);
	GlowSize->SetContent(Glow);

	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
	IconSize->SetWidthOverride(TileSize);
	IconSize->SetHeightOverride(TileSize);
	IconImage = WidgetTree->ConstructWidget<UImage>();
	IconSize->SetContent(IconImage);
	UOverlaySlot* IconSlot = Stack->AddChildToOverlay(IconSize);
	IconSlot->SetHorizontalAlignment(HAlign_Center);
	IconSlot->SetVerticalAlignment(VAlign_Center);

	Badge = WidgetTree->ConstructWidget<UBorder>();
	Badge->SetBrush(GRStyle::RoundedBrush(GRStyle::ColorDanger, 11.f));
	Badge->SetPadding(FMargin(7.f, 1.f));
	Badge->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* BadgeSlot = Stack->AddChildToOverlay(Badge);
	BadgeSlot->SetHorizontalAlignment(HAlign_Right);
	BadgeSlot->SetVerticalAlignment(VAlign_Top);
	BadgeText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 13, FLinearColor::White, true);
	Badge->SetContent(BadgeText);

	LabelText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 14, GRStyle::ColorText, true);
	LabelText->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(LabelText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

	Button->OnClicked.AddDynamic(this, &UGRAppTile::HandleClicked);
	Button->OnHovered.AddDynamic(this, &UGRAppTile::HandleHovered);
}

void UGRAppTile::Setup(UTexture2D* Icon, const FText& Label, const FString& InBadge)
{
	if (Icon)
	{
		IconImage->SetBrushFromTexture(Icon);
	}
	LabelText->SetText(Label);
	BadgeText->SetText(FText::FromString(InBadge));
	Badge->SetVisibility(InBadge.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UGRAppTile::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bFocused = true;
}

void UGRAppTile::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bFocused = false;
}

void UGRAppTile::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const float Dt = FMath::Min(InDeltaTime, 1.f / 20.f);
	Time += Dt;
	Highlight = FMath::FInterpTo(Highlight, bFocused ? 1.f : 0.f, Dt, 14.f);
	const float Press = Button && Button->IsPressed() ? 0.08f : 0.f;
	// The focused app lifts and sways a little.
	FWidgetTransform Transform;
	const float Scale = 1.f + 0.12f * Highlight - Press;
	Transform.Scale = FVector2D(Scale, Scale);
	Transform.Translation = FVector2D(0.f, -6.f * Highlight);
	Transform.Angle = 2.f * Highlight * FMath::Sin(Time * 5.f);
	IconStack->SetRenderTransform(Transform);
	Glow->SetRenderOpacity(0.55f * Highlight);
	LabelText->SetColorAndOpacity(FMath::Lerp(GRStyle::ColorText, GRStyle::ColorAccent, Highlight));
	if (Badge->GetVisibility() != ESlateVisibility::Collapsed)
	{
		const float Pulse = 1.f + 0.08f * FMath::Sin(Time * 6.f);
		Badge->SetRenderScale(FVector2D(Pulse, Pulse));
	}
}

void UGRAppTile::HandleClicked()
{
	if (OnClicked)
	{
		OnClicked();
	}
}

void UGRAppTile::HandleHovered()
{
	if (GetOwningPlayer())
	{
		Button->SetUserFocus(GetOwningPlayer());
	}
}
