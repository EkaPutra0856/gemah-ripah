#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameplayTagContainer.h"
#include "GRHUD.generated.h"

class UGRRegulationData;

/**
 * Prototype HUD drawn on Canvas: money, Suara/Diawasi bars, active rules,
 * interaction prompt and the "Pengumuman Pemerintah" banner.
 * Will be replaced by UMG once the 2D UI art (icons from Gemini) is ready.
 */
UCLASS()
class GEMAHRIPAH_API AGRHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleRegulationEnacted(UGRRegulationData* Regulation);

	UFUNCTION()
	void HandleRegulationViolated(UGRRegulationData* Regulation, FGameplayTag Action);

	void ShowBanner(const FString& Title, const FString& Body, const FLinearColor& Color, float Duration);
	void DrawMeter(const FString& Label, float Value, float Max, float X, float Y, const FLinearColor& Color);
	void DrawBanner();

	FString BannerTitle;
	FString BannerBody;
	FLinearColor BannerColor = FLinearColor::White;
	double BannerEndTime = 0.0;
};
