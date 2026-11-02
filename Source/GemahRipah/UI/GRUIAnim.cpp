#include "UI/GRUIAnim.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"

namespace GRAnim
{
	float Ease(EEase Type, float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		switch (Type)
		{
		case EEase::OutBack:
		{
			constexpr float C1 = 1.70158f;
			constexpr float C3 = C1 + 1.f;
			return 1.f + C3 * FMath::Pow(T - 1.f, 3.f) + C1 * FMath::Pow(T - 1.f, 2.f);
		}
		case EEase::InCubic:
			return T * T * T;
		case EEase::InOutSine:
			return -(FMath::Cos(PI * T) - 1.f) * 0.5f;
		case EEase::OutCubic:
		default:
			return 1.f - FMath::Pow(1.f - T, 3.f);
		}
	}

	FTween In(UWidget* Widget, float Delay, const FVector2D& Offset, float Duration)
	{
		FTween Tween;
		Tween.Widget = Widget;
		Tween.Delay = Delay;
		Tween.Duration = Duration;
		Tween.FromOffset = Offset;
		return Tween;
	}

	FTween Pop(UWidget* Widget, float Delay, float FromScale, const FVector2D& Offset, float Duration)
	{
		FTween Tween = In(Widget, Delay, Offset, Duration);
		Tween.FromScale = FromScale;
		Tween.Ease = EEase::OutBack;
		return Tween;
	}

	FTween Out(UWidget* Widget, const FVector2D& Offset, float Duration)
	{
		FTween Tween;
		Tween.Widget = Widget;
		Tween.Duration = Duration;
		Tween.FromOpacity = Widget ? Widget->GetRenderOpacity() : 1.f;
		Tween.ToOpacity = 0.f;
		Tween.ToOffset = Offset;
		Tween.Ease = EEase::InCubic;
		Tween.bRemoveWhenDone = true;
		return Tween;
	}

	void FPlayer::Apply(FTween& Tween, float Alpha)
	{
		UWidget* Widget = Tween.Widget.Get();
		if (!Widget)
		{
			return;
		}
		const float E = Ease(Tween.Ease, Alpha);
		// Overshoot is fine for movement and scale, not for opacity.
		Widget->SetRenderOpacity(FMath::Clamp(FMath::Lerp(Tween.FromOpacity, Tween.ToOpacity, FMath::Clamp(E, 0.f, 1.f)), 0.f, 1.f));
		const float Scale = FMath::Lerp(Tween.FromScale, Tween.ToScale, E);
		FWidgetTransform Transform;
		Transform.Translation = FMath::Lerp(Tween.FromOffset, Tween.ToOffset, E);
		Transform.Scale = FVector2D(Scale, Scale);
		Widget->SetRenderTransform(Transform);
	}

	void FPlayer::Add(const FTween& Tween)
	{
		if (!Tween.Widget.IsValid())
		{
			return;
		}
		Tweens.RemoveAll([&Tween](const FTween& Other) { return Other.Widget == Tween.Widget; });
		FTween& Added = Tweens.Add_GetRef(Tween);
		Apply(Added, 0.f);
	}

	void FPlayer::Stagger(UPanelWidget* Panel, float Delay, float Step, const FVector2D& Offset, float Duration)
	{
		if (!Panel)
		{
			return;
		}
		for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
		{
			Add(In(Panel->GetChildAt(i), Delay + Step * i, Offset, Duration));
		}
	}

	void FPlayer::Tick(float DeltaSeconds)
	{
		// Long hitches (loading, alt-tab) should not skip whole animations in one frame.
		DeltaSeconds = FMath::Min(DeltaSeconds, 1.f / 20.f);
		for (int32 i = Tweens.Num() - 1; i >= 0; --i)
		{
			FTween& Tween = Tweens[i];
			if (!Tween.Widget.IsValid())
			{
				Tweens.RemoveAtSwap(i);
				continue;
			}
			Tween.Time += DeltaSeconds;
			const float Alpha = Tween.Duration > 0.f ? (Tween.Time - Tween.Delay) / Tween.Duration : 1.f;
			if (Alpha <= 0.f)
			{
				continue;
			}
			Apply(Tween, Alpha);
			if (Alpha >= 1.f)
			{
				if (Tween.bRemoveWhenDone)
				{
					Tween.Widget->RemoveFromParent();
				}
				Tweens.RemoveAtSwap(i);
			}
		}
	}

	void FPlayer::Finish()
	{
		for (FTween& Tween : Tweens)
		{
			Apply(Tween, 1.f);
			if (Tween.bRemoveWhenDone && Tween.Widget.IsValid())
			{
				Tween.Widget->RemoveFromParent();
			}
		}
		Tweens.Reset();
	}

	bool FPlayer::IsPlaying(const UWidget* Widget) const
	{
		return Tweens.ContainsByPredicate([Widget](const FTween& Tween) { return Tween.Widget.Get() == Widget; });
	}
}
