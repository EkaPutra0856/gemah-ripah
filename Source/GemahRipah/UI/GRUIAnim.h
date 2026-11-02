#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtrTemplates.h"

class UWidget;
class UPanelWidget;

/**
 * Small code-driven tweens for the C++ UI. Only render opacity and render transform change, so layout
 * never jumps. Uses real (unpaused) delta time from the widget tick.
 */
namespace GRAnim
{
	enum class EEase : uint8
	{
		OutCubic,
		OutBack,
		InCubic,
		InOutSine,
	};

	float Ease(EEase Type, float T);

	struct FTween
	{
		TWeakObjectPtr<UWidget> Widget;
		float Delay = 0.f;
		float Duration = 0.25f;
		float Time = 0.f;
		float FromOpacity = 0.f;
		float ToOpacity = 1.f;
		FVector2D FromOffset = FVector2D::ZeroVector;
		FVector2D ToOffset = FVector2D::ZeroVector;
		float FromScale = 1.f;
		float ToScale = 1.f;
		EEase Ease = EEase::OutCubic;
		/** Remove the widget from its parent when done (fade-outs). */
		bool bRemoveWhenDone = false;
	};

	/** Fade and slide in from Offset. */
	FTween In(UWidget* Widget, float Delay = 0.f, const FVector2D& Offset = FVector2D(0.f, 16.f), float Duration = 0.25f);
	/** Scale up with a little overshoot. */
	FTween Pop(UWidget* Widget, float Delay = 0.f, float FromScale = 0.85f, const FVector2D& Offset = FVector2D::ZeroVector, float Duration = 0.3f);
	/** Fade and slide out to Offset, then remove the widget. */
	FTween Out(UWidget* Widget, const FVector2D& Offset = FVector2D(30.f, 0.f), float Duration = 0.2f);

	/** Plays tweens; one tween per widget (a new one replaces the old). */
	class FPlayer
	{
	public:
		/** Applies the start state right away so nothing flashes before the first tick. */
		void Add(const FTween& Tween);
		/** Staggered In() for every child of Panel. */
		void Stagger(UPanelWidget* Panel, float Delay = 0.f, float Step = 0.04f, const FVector2D& Offset = FVector2D(0.f, 14.f), float Duration = 0.24f);
		void Tick(float DeltaSeconds);
		/** Jump every tween to its end state. */
		void Finish();
		bool IsPlaying(const UWidget* Widget) const;
		bool IsEmpty() const { return Tweens.IsEmpty(); }

	private:
		static void Apply(FTween& Tween, float Alpha);
		TArray<FTween> Tweens;
	};
}
