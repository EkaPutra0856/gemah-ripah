#include "UI/GRMapView.h"
#include "UI/GRUIStyle.h"
#include "World/GRDistrictVolume.h"
#include "World/GRMapMarkerComponent.h"
#include "World/GRMapSubsystem.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "Rendering/DrawElements.h"

namespace
{
	FVector2D MeasureText(const FString& Text, const FSlateFontInfo& Font)
	{
		return FVector2D(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font));
	}

	void DrawText(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const FString& Text, const FSlateFontInfo& Font,
		const FVector2D& Center, const FLinearColor& Color)
	{
		const FVector2D Size = MeasureText(Text, Font);
		const FVector2D TopLeft = Center - Size * 0.5;
		// Shadow for readability over the map image.
		FSlateDrawElement::MakeText(Out, Layer, Geo.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(TopLeft + FVector2D(1.0, 1.0)))),
			Text, Font, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.8f));
		FSlateDrawElement::MakeText(Out, Layer + 1, Geo.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(TopLeft))),
			Text, Font, ESlateDrawEffect::None, Color);
	}
}

UGRMapView::UGRMapView(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetClipping(EWidgetClipping::ClipToBounds);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UGRMapView::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Size(AllottedGeometry.GetLocalSize());
	const UWorld* World = GetWorld();
	const UGRMapSubsystem* Map = World ? World->GetSubsystem<UGRMapSubsystem>() : nullptr;

	static const FSlateRoundedBoxBrush Background(FSlateColor(FLinearColor(0.06f, 0.05f, 0.05f, 0.92f)), 6.f);
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), &Background, ESlateDrawEffect::None, FLinearColor(0.06f, 0.05f, 0.05f, 0.92f));
	if (!Map || Size.X < 4.0 || Size.Y < 4.0)
	{
		return LayerId + 1;
	}

	const APawn* Pawn = GetOwningPlayerPawn();
	const FBox2D Area = Map->GetMapArea();
	FVector2D Center = Area.GetCenter();
	double WorldAcross = FMath::Max(Area.GetSize().X, Area.GetSize().Y);
	if (bFollowPlayer && Pawn)
	{
		Center = FVector2D(Pawn->GetActorLocation());
		WorldAcross = ViewSize;
	}
	const double Scale = FMath::Min(Size.X, Size.Y) / WorldAcross;

	// World +X is map-up, world +Y is map-right.
	auto ToLocal = [&](const FVector2D& W)
	{
		return FVector2D(Size.X * 0.5 + (W.Y - Center.Y) * Scale, Size.Y * 0.5 - (W.X - Center.X) * Scale);
	};

	int32 Layer = LayerId + 1;

	// Ground, then building and road footprints.
	{
		const FVector2D GroundTopLeft = ToLocal(FVector2D(Area.Max.X, Area.Min.Y));
		const FVector2D GroundSize = ToLocal(FVector2D(Area.Min.X, Area.Max.Y)) - GroundTopLeft;
		FSlateBrush Ground;
		Ground.TintColor = FSlateColor(FLinearColor(0.16f, 0.15f, 0.12f, 1.f));
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(GroundSize), FSlateLayoutTransform(FVector2f(GroundTopLeft))), &Ground, ESlateDrawEffect::None, FLinearColor(0.16f, 0.15f, 0.12f, 1.f));
		++Layer;

		FSlateBrush Solid;
		for (const UGRMapSubsystem::FFootprint& Print : const_cast<UGRMapSubsystem*>(Map)->GetFootprints())
		{
			const FVector2D TopLeft = ToLocal(FVector2D(Print.Box.Max.X, Print.Box.Min.Y));
			FVector2D BoxSize = ToLocal(FVector2D(Print.Box.Min.X, Print.Box.Max.Y)) - TopLeft;
			if (TopLeft.X > Size.X || TopLeft.Y > Size.Y || TopLeft.X + BoxSize.X < 0.0 || TopLeft.Y + BoxSize.Y < 0.0)
			{
				continue;
			}
			BoxSize.X = FMath::Max(BoxSize.X, 1.0);
			BoxSize.Y = FMath::Max(BoxSize.Y, 1.0);
			Solid.TintColor = FSlateColor(Print.Color * 0.85f);
			FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(BoxSize), FSlateLayoutTransform(FVector2f(TopLeft))), &Solid, ESlateDrawEffect::None, Print.Color * 0.85f);
		}
		++Layer;
	}

	// Districts: tinted areas and names.
	const FSlateFontInfo DistrictFont = GRStyle::Font(bFollowPlayer ? 9 : 13, true);
	for (const TWeakObjectPtr<AGRDistrictVolume>& Weak : Map->GetDistricts())
	{
		const AGRDistrictVolume* District = Weak.Get();
		if (!District)
		{
			continue;
		}
		const FBox2D Box = District->GetArea2D();
		const FVector2D TopLeft = ToLocal(FVector2D(Box.Max.X, Box.Min.Y));
		const FVector2D BoxSize = ToLocal(FVector2D(Box.Min.X, Box.Max.Y)) - TopLeft;
		FLinearColor Tint = District->MapColor;
		Tint.A = 0.16f;
		FSlateBrush Fill;
		Fill.TintColor = FSlateColor(Tint);
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(BoxSize), FSlateLayoutTransform(FVector2f(TopLeft))), &Fill, ESlateDrawEffect::None, Tint);
		if (bShowDistrictNames)
		{
			DrawText(OutDrawElements, Layer + 2, AllottedGeometry, District->DisplayName.ToString().ToUpper(), DistrictFont,
				TopLeft + BoxSize * 0.5, District->MapColor.CopyWithNewOpacity(0.95f));
		}
	}
	Layer += 4;

	// Markers.
	const float MarkerSize = bFollowPlayer ? 16.f : 22.f;
	const FSlateFontInfo GlyphFont = GRStyle::Font(bFollowPlayer ? 8 : 10, true);
	const FSlateFontInfo LabelFont = GRStyle::Font(10);
	for (const TWeakObjectPtr<UGRMapMarkerComponent>& Weak : Map->GetMarkers())
	{
		const UGRMapMarkerComponent* Marker = Weak.Get();
		if (!Marker || (bFollowPlayer && !Marker->bShowOnMinimap))
		{
			continue;
		}
		FVector2D P = ToLocal(FVector2D(Marker->GetComponentLocation()));
		const double Margin = MarkerSize * 0.5 + 2.0;
		const bool bInside = P.X >= Margin && P.Y >= Margin && P.X <= Size.X - Margin && P.Y <= Size.Y - Margin;
		if (!bInside)
		{
			if (!(bFollowPlayer && Marker->bTracked))
			{
				continue;
			}
			P.X = FMath::Clamp(P.X, Margin, Size.X - Margin);
			P.Y = FMath::Clamp(P.Y, Margin, Size.Y - Margin);
		}
		const FSlateRoundedBoxBrush Circle(FSlateColor(Marker->Color), MarkerSize * 0.5f, FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.85f)), 2.f);
		const FVector2D TopLeft = P - FVector2D(MarkerSize * 0.5);
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(MarkerSize, MarkerSize), FSlateLayoutTransform(FVector2f(TopLeft))), &Circle, ESlateDrawEffect::None, Marker->Color);
		DrawText(OutDrawElements, Layer + 1, AllottedGeometry, Marker->Glyph, GlyphFont, P, FLinearColor(0.05f, 0.04f, 0.03f));
		if (bShowMarkerLabels && !Marker->Label.IsEmpty())
		{
			DrawText(OutDrawElements, Layer + 1, AllottedGeometry, Marker->Label.ToString(), LabelFont,
				P + FVector2D(0.0, MarkerSize * 0.5 + 9.0), GRStyle::ColorText);
		}
	}
	Layer += 3;

	// Player arrow (screen-space rotation follows the pawn yaw: 0 = up/north).
	if (Pawn)
	{
		const FVector2D P = ToLocal(FVector2D(Pawn->GetActorLocation()));
		const float Angle = FMath::DegreesToRadians(Pawn->GetActorRotation().Yaw);
		const float S = bFollowPlayer ? 1.f : 1.3f;
		auto Rot = [&](float X, float Y)
		{
			return FVector2f(P.X + (X * FMath::Cos(Angle) - Y * FMath::Sin(Angle)) * S, P.Y + (X * FMath::Sin(Angle) + Y * FMath::Cos(Angle)) * S);
		};
		TArray<FVector2f> Outline = { Rot(0.f, -10.f), Rot(7.f, 8.f), Rot(0.f, 4.f), Rot(-7.f, 8.f), Rot(0.f, -10.f) };
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), Outline, ESlateDrawEffect::None, FLinearColor::Black, true, 5.f);
		FSlateDrawElement::MakeLines(OutDrawElements, Layer + 1, AllottedGeometry.ToPaintGeometry(), Outline, ESlateDrawEffect::None, FLinearColor(1.f, 0.95f, 0.85f), true, 2.5f);
		Layer += 2;
	}

	// North indicator.
	DrawText(OutDrawElements, Layer, AllottedGeometry, TEXT("U"), GRStyle::Font(bFollowPlayer ? 10 : 14, true), FVector2D(Size.X * 0.5, 10.0), GRStyle::ColorAccent);
	return Layer + 2;
}
