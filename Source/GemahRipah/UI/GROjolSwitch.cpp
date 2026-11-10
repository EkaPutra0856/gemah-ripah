#include "UI/GROjolSwitch.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "UI/GRMenuButton.h"
#include "UI/GRUIAnim.h"
#include "UI/GRUIStyle.h"
#include "World/GRSfx.h"

namespace
{
	constexpr float CardHeight = 196.f;
	constexpr float CardRadius = 18.f;
	constexpr float Pad = 16.f;
	constexpr float TrackTop = 96.f;
	constexpr float TrackHeight = 64.f;
	constexpr float KnobInset = 4.f;
	constexpr float KnobSize = TrackHeight - KnobInset * 2.f;

	constexpr float SlideTime = 0.48f;
	/** The burst starts as the knob arrives. */
	constexpr float BurstAt = 0.3f;
	constexpr float BurstTime = 0.8f;
	constexpr float LightTime = 0.4f;
	constexpr float RefuseTime = 0.55f;
	constexpr float RadarPeriod = 1.8f;
	constexpr int32 Sparks = 16;

	const FLinearColor CardOff(0.085f, 0.085f, 0.10f, 1.f);
	const FLinearColor CardOn(0.012f, 0.13f, 0.065f, 1.f);
	const FLinearColor TrackOff(0.03f, 0.03f, 0.035f, 1.f);
	const FLinearColor Green(0.10f, 0.78f, 0.34f, 1.f);
	const FLinearColor Mint(0.62f, 1.f, 0.76f, 1.f);

	FSlateBrush TextureBrush(const TCHAR* Name)
	{
		FSlateBrush Brush;
		if (UTexture2D* Texture = GRStyle::Icon(Name))
		{
			Brush.SetResourceObject(Texture);
			Brush.ImageSize = FVector2D(128.f, 128.f);
		}
		return Brush;
	}

	FLinearColor Faded(const FLinearColor& Color, float Alpha)
	{
		return Color.CopyWithNewOpacity(Color.A * Alpha);
	}
}

void UGROjolSwitch::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CardBrush = FSlateRoundedBoxBrush(FLinearColor::White, CardRadius);
	FSlateRoundedBoxBrush Round(FLinearColor::White, 1.f);
	Round.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	RoundBrush = Round;
	HelmetBrush = TextureBrush(TEXT("ui_helm"));
	ScooterBrush = TextureBrush(TEXT("app_ojekku"));

	Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style;
	const FSlateBrush Clear = GRStyle::RoundedBrush(FLinearColor::Transparent, CardRadius);
	Style.SetNormal(Clear);
	Style.SetHovered(Clear);
	Style.SetPressed(Clear);
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	Button->SetStyle(Style);
	WidgetTree->RootWidget = Button;

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetHeightOverride(CardHeight);
	Button->SetContent(Size);

	Button->OnClicked.AddDynamic(this, &UGROjolSwitch::HandleClicked);
	Button->OnHovered.AddDynamic(this, &UGROjolSwitch::HandleHovered);
}

void UGROjolSwitch::SetOn(bool bOn)
{
	bTarget = bOn;
	Knob = KnobFrom = bOn ? 1.f : 0.f;
	Lit = Knob;
	SinceFlip = 100.f;
	SinceRefuse = 100.f;
}

void UGROjolSwitch::Flip(bool bOn)
{
	bTarget = bOn;
	KnobFrom = Knob;
	SinceFlip = 0.f;
	SinceRefuse = 100.f;
	GRSfx::Play(this, bOn ? EGRSfx::UIBuka : EGRSfx::UITutup);
}

void UGROjolSwitch::Refuse()
{
	SinceRefuse = 0.f;
}

void UGROjolSwitch::HandleClicked()
{
	GRSfx::Play(this, EGRSfx::UIKlik);
	if (OnClicked)
	{
		OnClicked();
	}
}

void UGROjolSwitch::HandleHovered()
{
	if (!UGRMenuButton::IsMouseResting() && GetOwningPlayer())
	{
		Button->SetUserFocus(GetOwningPlayer());
	}
}

void UGROjolSwitch::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bFocused = true;
}

void UGROjolSwitch::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bFocused = false;
}

void UGROjolSwitch::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Time += InDeltaTime;
	SinceFlip += InDeltaTime;
	SinceRefuse += InDeltaTime;

	const float To = bTarget ? 1.f : 0.f;
	const float Slide = FMath::Clamp(SinceFlip / SlideTime, 0.f, 1.f);
	Knob = FMath::Lerp(KnobFrom, To, GRAnim::Ease(bTarget ? GRAnim::EEase::OutBack : GRAnim::EEase::OutCubic, Slide));
	if (SinceRefuse < RefuseTime)
	{
		const float T = SinceRefuse / RefuseTime;
		Knob = To + 0.2f * FMath::Sin(T * PI) * (bTarget ? -1.f : 1.f) + 0.03f * FMath::Sin(T * 40.f) * (1.f - T);
	}
	Lit = FMath::FInterpConstantTo(Lit, To, InDeltaTime, 1.f / LightTime);
}

int32 UGROjolSwitch::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;
	const FVector2f Size(AllottedGeometry.GetLocalSize());
	// The phone's page transitions fade this widget too.
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	const auto Box = [&](const FSlateBrush& Brush, const FVector2f& At, const FVector2f& BoxSize, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(At)), &Brush, ESlateDrawEffect::None, Faded(Color, Opacity));
	};
	const auto Circle = [&](const FVector2f& Center, float Radius, const FLinearColor& Color)
	{
		Box(RoundBrush, Center - FVector2f(Radius, Radius), FVector2f(Radius * 2.f, Radius * 2.f), Color);
	};
	const auto Ring = [&](const FVector2f& Center, float Radius, const FLinearColor& Color, float Thickness)
	{
		TArray<FVector2f> Points;
		for (int32 i = 0; i <= 48; ++i)
		{
			const float Angle = i * 2.f * PI / 48.f;
			Points.Add(Center + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Faded(Color, Opacity), true, Thickness);
	};
	/** Align: 0 from the left of At, 0.5 centered on it. Scaled text keeps that anchor and its vertical middle. */
	const auto Label = [&](const FString& Text, const FVector2f& At, int32 FontSize, bool bBold, const FLinearColor& Color, float Align = 0.f, float Scale = 1.f)
	{
		const FSlateFontInfo Font = GRStyle::Font(FontSize, bBold);
		const FVector2f TextSize(Measure->Measure(Text, Font));
		const FVector2f Corner = At - FVector2f(TextSize.X * Scale * Align, TextSize.Y * (Scale - 1.f) * 0.5f);
		FSlateDrawElement::MakeText(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(Scale, Corner)), Text, Font, ESlateDrawEffect::None, Faded(Color, Opacity));
	};

	const float Burst = FMath::Clamp((SinceFlip - BurstAt) / BurstTime, 0.f, 1.f);
	const bool bBursting = bTarget && Burst > 0.f && Burst < 1.f;
	const float Breath = 0.5f + 0.5f * FMath::Sin(Time * 2.4f);

	// Card.
	Box(CardBrush, FVector2f::ZeroVector, Size, FMath::Lerp(CardOff, CardOn, Lit * (0.88f + 0.12f * Breath)));
	++Layer;

	const float TrackWidth = Size.X - Pad * 2.f;
	const float KnobLeft = Pad + KnobInset + (TrackWidth - KnobSize - KnobInset * 2.f) * Knob;
	const FVector2f KnobCenter(KnobLeft + KnobSize * 0.5f, TrackTop + TrackHeight * 0.5f);

	// Rings stay inside the card.
	OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));
	if (Lit > 0.f && bTarget)
	{
		for (int32 Wave = 0; Wave < 2; ++Wave)
		{
			const float T = FMath::Fractional(Time / RadarPeriod + Wave * 0.5f);
			Ring(KnobCenter, KnobSize * 0.5f + T * 130.f, Faded(Green, 0.45f * (1.f - T) * Lit), 2.f);
		}
	}
	if (bBursting)
	{
		for (int32 Wave = 0; Wave < 3; ++Wave)
		{
			const float T = FMath::Clamp(Burst * 1.5f - Wave * 0.2f, 0.f, 1.f);
			if (T > 0.f && T < 1.f)
			{
				Ring(KnobCenter, KnobSize * 0.5f + GRAnim::Ease(GRAnim::EEase::OutCubic, T) * 210.f, Faded(Wave == 0 ? FLinearColor::White : Mint, 0.85f * (1.f - T)), 4.f * (1.f - T) + 1.f);
			}
		}
		for (int32 Spark = 0; Spark < Sparks; ++Spark)
		{
			const float Angle = Spark * 2.f * PI / Sparks + (Spark % 3) * 0.16f;
			const float Reach = 70.f + (Spark * 37 % 5) * 26.f;
			const float Way = GRAnim::Ease(GRAnim::EEase::OutCubic, Burst);
			Circle(KnobCenter + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * (KnobSize * 0.5f + Reach * Way), 4.5f * (1.f - Burst) + 0.8f, Faded(Spark % 2 ? Mint : GRStyle::ColorAccent, 1.f - Burst * Burst));
		}
	}
	++Layer;
	OutDrawElements.PopClip();

	// Track, filling up behind the knob.
	Box(RoundBrush, FVector2f(Pad, TrackTop), FVector2f(TrackWidth, TrackHeight), TrackOff);
	++Layer;
	const float FillWidth = FMath::Max(KnobLeft + KnobSize + KnobInset - Pad, TrackHeight);
	Box(RoundBrush, FVector2f(Pad, TrackTop), FVector2f(FMath::Min(FillWidth, TrackWidth), TrackHeight), Faded(Green, FMath::Clamp(Knob * 1.6f, 0.f, 1.f)));
	++Layer;

	// What the track says, going away as the knob covers it.
	const float Clear = FMath::Clamp(1.f - Knob * 2.2f, 0.f, 1.f);
	if (Clear > 0.f)
	{
		const float Middle = Pad + KnobSize + (TrackWidth - KnobSize) * 0.5f;
		Label(TEXT("GESER UNTUK ONLINE"), FVector2f(Middle - 12.f, TrackTop + 22.f), 13, true, Faded(GRStyle::ColorTextDim, Clear), 0.5f);
		for (int32 Mark = 0; Mark < 3; ++Mark)
		{
			const float Wave = 0.5f + 0.5f * FMath::Sin(Time * 5.f - Mark * 0.9f);
			Label(TEXT(">"), FVector2f(Size.X - Pad - 50.f + Mark * 11.f, TrackTop + 20.f), 15, true, Faded(GRStyle::ColorAccent, Clear * (0.25f + 0.75f * Wave)));
		}
	}
	const float Ready = FMath::Clamp(Knob * 2.2f - 1.2f, 0.f, 1.f);
	if (Ready > 0.f)
	{
		Label(TEXT("SIAP NARIK"), FVector2f(Pad + (TrackWidth - KnobSize) * 0.5f, TrackTop + 19.f), 17, true, Faded(FLinearColor(0.01f, 0.1f, 0.04f, 1.f), Ready), 0.5f);
	}

	// Knob: it stretches while it moves.
	const float Speed = SinceFlip < SlideTime ? FMath::Sin(SinceFlip / SlideTime * PI) : 0.f;
	const float Pop = bBursting ? 1.f + 0.16f * FMath::Sin(FMath::Min(Burst * 4.f, 1.f) * PI) : 1.f;
	const FVector2f KnobBox(KnobSize * (1.f + 0.2f * Speed) * Pop, KnobSize * (1.f - 0.1f * Speed) * Pop);
	Box(RoundBrush, KnobCenter - KnobBox * 0.5f + FVector2f(0.f, 2.f), KnobBox, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	++Layer;
	Box(RoundBrush, KnobCenter - KnobBox * 0.5f, KnobBox, FLinearColor(0.94f, 0.94f, 0.92f, 1.f));
	++Layer;
	const FVector2f Face = KnobBox * 0.72f;
	Box(ScooterBrush, KnobCenter - Face * 0.5f, Face, FMath::Lerp(FLinearColor(0.42f, 0.42f, 0.42f, 1.f), FLinearColor::White, Lit));
	++Layer;

	// Header.
	Box(HelmetBrush, FVector2f(Pad, 16.f), FVector2f(60.f, 60.f), FMath::Lerp(FLinearColor(0.55f, 0.55f, 0.55f, 1.f), FLinearColor::White, Lit));
	const float Grow = bBursting ? 0.24f * (1.f - Burst) * (1.f - Burst) : 0.f;
	const FVector2f Word(Pad + 74.f, 14.f);
	const FString Dots = FString::ChrN(int32(Time * 2.5f) % 4, TEXT('.'));
	if (Knob >= 0.5f)
	{
		Label(TEXT("ONLINE"), Word, 28, true, FMath::Lerp(GRStyle::ColorText, Mint, Lit), 0.f, 1.f + Grow);
		Label(TEXT("Mencari order") + Dots, Word + FVector2f(0.f, 44.f), 14, false, Mint);
	}
	else
	{
		Label(TEXT("OFFLINE"), Word, 28, true, GRStyle::ColorTextDim);
		Label(TEXT("Istirahat dulu, Bay"), Word + FVector2f(0.f, 44.f), 14, false, GRStyle::ColorTextDim);
	}
	const FVector2f Lamp(Size.X - Pad - 10.f, 32.f);
	if (bTarget)
	{
		Circle(Lamp, 9.f + 5.f * Breath, Faded(Green, 0.3f * Lit));
	}
	Circle(Lamp, 6.f, FMath::Lerp(FLinearColor(0.3f, 0.28f, 0.27f, 1.f), Mint, Lit));
	++Layer;

	Label(bTarget ? TEXT("Klik atau Enter untuk berhenti") : TEXT("Klik atau Enter untuk mulai narik"), FVector2f(Size.X * 0.5f, TrackTop + TrackHeight + 10.f), 12, false, GRStyle::ColorTextDim, 0.5f);

	// Flash and a sheen across the card as it lights up.
	if (bBursting)
	{
		const float Flash = FMath::Clamp(1.f - Burst * 3.f, 0.f, 1.f);
		if (Flash > 0.f)
		{
			++Layer;
			Box(CardBrush, FVector2f::ZeroVector, Size, FLinearColor(1.f, 1.f, 1.f, 0.4f * Flash));
		}
		++Layer;
		const FVector2f Inset(0.f, CardRadius * 0.5f);
		const FVector2f Band(120.f, Size.Y - Inset.Y * 2.f);
		const FVector2f BandAt(FMath::Lerp(-Band.X, Size.X, Burst), Inset.Y);
		const FGeometry Whole = AllottedGeometry.MakeChild(FVector2D(Size), FSlateLayoutTransform());
		OutDrawElements.PushClip(FSlateClippingZone(Whole));
		TArray<FSlateGradientStop> Stops;
		Stops.Add(FSlateGradientStop(FVector2D(0.f, 0.f), FLinearColor(1.f, 1.f, 1.f, 0.f)));
		Stops.Add(FSlateGradientStop(FVector2D(Band.X * 0.5f, 0.f), FLinearColor(1.f, 1.f, 1.f, 0.22f * Opacity)));
		Stops.Add(FSlateGradientStop(FVector2D(Band.X, 0.f), FLinearColor(1.f, 1.f, 1.f, 0.f)));
		FSlateDrawElement::MakeGradient(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(Band, FSlateLayoutTransform(BandAt)), Stops, Orient_Vertical);
		OutDrawElements.PopClip();
	}

	// The red of a refusal.
	if (SinceRefuse < RefuseTime)
	{
		++Layer;
		Box(CardBrush, FVector2f::ZeroVector, Size, Faded(GRStyle::ColorDanger, 0.35f * (1.f - SinceRefuse / RefuseTime)));
	}

	if (bFocused)
	{
		++Layer;
		TArray<FVector2f> Outline;
		const FVector2f Corners[] = { { Size.X - CardRadius, CardRadius }, { Size.X - CardRadius, Size.Y - CardRadius }, { CardRadius, Size.Y - CardRadius }, { CardRadius, CardRadius } };
		for (int32 Corner = 0; Corner < 4; ++Corner)
		{
			for (int32 Step = 0; Step <= 8; ++Step)
			{
				const float Angle = (Corner - 1 + Step / 8.f) * PI * 0.5f;
				Outline.Add(Corners[Corner] + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * (CardRadius - 1.f));
			}
		}
		const FVector2f Start = Outline[0];
		Outline.Add(Start);
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), Outline, ESlateDrawEffect::None, Faded(GRStyle::ColorAccent, Opacity), true, 2.f);
	}
	return Layer + 1;
}
