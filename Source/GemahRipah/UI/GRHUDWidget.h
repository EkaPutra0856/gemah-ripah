#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Systems/GRNotificationSubsystem.h"
#include "GRHUDWidget.generated.h"

class UBorder;
class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UGRRegulationData;
class UGRMapView;
class AGRDistrictVolume;

/**
 * In-game HUD built in code: money, Suara/Diawasi meters, active rules, city name,
 * interaction prompt, "Pengumuman Pemerintah" banner and toast notifications.
 */
UCLASS()
class GEMAHRIPAH_API UGRHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void HandleStatusChanged();

	UFUNCTION()
	void HandleRegulationEnacted(UGRRegulationData* Regulation);

	UFUNCTION()
	void HandleRegulationRevoked(FGameplayTag RegulationId);

	UFUNCTION()
	void HandleRegulationViolated(UGRRegulationData* Regulation, FGameplayTag Action);

	void HandleNotify(const FText& Message, EGRNotifyType Type);
	void HandleDistrictChanged(AGRDistrictVolume* District, bool bEntered);
	void RefreshClock();
	void RefreshLocation();

	void RefreshStatus();
	void RefreshRules();
	void ShowBanner(const FText& Title, const FText& Body, const FLinearColor& Color, float Duration);
	UProgressBar* MakeMeter(UVerticalBox* Parent, const FText& Label, const FLinearColor& Color, UTextBlock*& OutValue);

	UPROPERTY() TObjectPtr<UTextBlock> UangText;
	UPROPERTY() TObjectPtr<UProgressBar> SuaraBar;
	UPROPERTY() TObjectPtr<UTextBlock> SuaraValue;
	UPROPERTY() TObjectPtr<UProgressBar> DiawasiBar;
	UPROPERTY() TObjectPtr<UTextBlock> DiawasiValue;
	UPROPERTY() TObjectPtr<UVerticalBox> RuleList;
	UPROPERTY() TObjectPtr<UTextBlock> CityText;
	UPROPERTY() TObjectPtr<UTextBlock> ClockText;
	UPROPERTY() TObjectPtr<UTextBlock> SmokeText;
	UPROPERTY() TObjectPtr<UGRMapView> Minimap;
	UPROPERTY() TObjectPtr<UVerticalBox> DistrictPopup;
	UPROPERTY() TObjectPtr<UTextBlock> DistrictTitle;
	UPROPERTY() TObjectPtr<UTextBlock> DistrictSubtitle;
	UPROPERTY() TObjectPtr<UBorder> PromptPanel;
	UPROPERTY() TObjectPtr<UTextBlock> PromptText;
	UPROPERTY() TObjectPtr<UBorder> BannerPanel;
	UPROPERTY() TObjectPtr<UTextBlock> BannerTitle;
	UPROPERTY() TObjectPtr<UTextBlock> BannerBody;
	UPROPERTY() TObjectPtr<UVerticalBox> ToastList;

	struct FToast
	{
		TWeakObjectPtr<UWidget> Widget;
		double EndTime = 0.0;
	};
	TArray<FToast> Toasts;

	double BannerEndTime = 0.0;
	double DistrictPopupStart = -100.0;
	double NextClockRefresh = 0.0;
	FDelegateHandle DistrictHandle;
	FDelegateHandle NotifyHandle;
};
