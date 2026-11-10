#include "UI/GRMapView.h"
#include "UI/GRUIStyle.h"
#include "Systems/GRSuaraSubsystem.h"
#include "World/GRDistrictVolume.h"
#include "World/GRMapMarkerComponent.h"
#include "World/GRMapSubsystem.h"
#include "World/GROjol.h"
#include "World/GRTerrain.h"
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

		// The river and the kali, under the roads and buildings so the bridges show.
		for (const GRTerrain::FWaterway* Way : { &GRTerrain::River(), &GRTerrain::Creek() })
		{
			TArray<FVector2f> Line;
			Line.Reserve(Way->Points.Num() / 2 + 1);
			for (int32 i = 0; i < Way->Points.Num(); i += 2)
			{
				Line.Add(FVector2f(ToLocal(Way->Points[i])));
			}
			Line.Add(FVector2f(ToLocal(Way->Points.Last())));
			const float Thickness = FMath::Max(float(Way->HalfWidth * 2.0 * Scale), 1.5f);
			FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None,
				FLinearColor(0.20f, 0.36f, 0.42f, 1.f), true, Thickness);
		}
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
		// The more a district stands behind Bayu, the more strongly it shows on the map.
		const UGameInstance* GI = GetGameInstance();
		const UGRSuaraSubsystem* Town = GI ? GI->GetSubsystem<UGRSuaraSubsystem>() : nullptr;
		Tint.A = 0.1f + 0.3f * FMath::Clamp((Town ? Town->GetAreaSuara(District->DistrictId) : 0.f) / 50.f, 0.f, 1.f);
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

	// OjekKu: the route to follow and a pin on the pickup or drop-off.
	if (const UGROjolSubsystem* Ojol = UGROjolSubsystem::Get(this))
	{
		const FLinearColor RouteColor(FColor(0, 200, 105));
		const TArray<FVector2D>& Route = Ojol->GetRoute();
		if (Route.Num() >= 2)
		{
			TArray<FVector2f> Line;
			for (const FVector2D& Point : Route)
			{
				Line.Add(FVector2f(ToLocal(Point)));
			}
			FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.75f), true, bFollowPlayer ? 7.f : 9.f);
			FSlateDrawElement::MakeLines(OutDrawElements, Layer + 1, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, RouteColor, true, bFollowPlayer ? 4.f : 5.f);
		}
		FVector Target;
		FText TargetLabel;
		if (Ojol->GetTarget(Target, TargetLabel))
		{
			const float Pin = bFollowPlayer ? 18.f : 24.f;
			FVector2D P = ToLocal(FVector2D(Target));
			P.X = FMath::Clamp(P.X, Pin * 0.5, Size.X - Pin * 0.5);
			P.Y = FMath::Clamp(P.Y, Pin * 0.5, Size.Y - Pin * 0.5);
			const FSlateRoundedBoxBrush Circle(FSlateColor(RouteColor), Pin * 0.5f, FSlateColor(FLinearColor::White), 2.5f);
			FSlateDrawElement::MakeBox(OutDrawElements, Layer + 2, AllottedGeometry.ToPaintGeometry(FVector2f(Pin, Pin), FSlateLayoutTransform(FVector2f(P - FVector2D(Pin * 0.5)))), &Circle, ESlateDrawEffect::None, RouteColor);
			const bool bPickup = Ojol->GetStage() != EGROjolStage::Riding;
			DrawText(OutDrawElements, Layer + 3, AllottedGeometry, bPickup ? TEXT("J") : TEXT("T"), GRStyle::Font(bFollowPlayer ? 9 : 11, true), P, FLinearColor::White);
			if (!bFollowPlayer || Size.X > 400.0)
			{
				DrawText(OutDrawElements, Layer + 3, AllottedGeometry, TargetLabel.ToString(), GRStyle::Font(11, true), P + FVector2D(0.0, Pin * 0.5 + 10.0), RouteColor);
			}
		}
		Layer += 5;
	}

	// Markers.
	const float MarkerSize = bFollowPlayer ? 16.f : 22.f;
	const FSlateFontInfo GlyphFont = GRStyle::Font(bFollowPlayer ? 8 : 10, true);
	const FSlateFontInfo LabelFont = GRStyle::Font(10);
	for (const TWeakObjectPtr<UGRMapMarkerComponent>& Weak : Map->GetMarkers())
	{
		const UGRMapMarkerComponent* Marker = Weak.Get();
		if (!Marker || Marker->bOffMap || (bFollowPlayer && !Marker->bShowOnMinimap))
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
