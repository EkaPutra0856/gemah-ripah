#pragma once

#include "CoreMinimal.h"
#include "UI/GRScreenWidget.h"
#include "GRSettingsScreen.generated.h"

class UScrollBox;
class UGROptionRow;

/** Graphics, audio, controls and gameplay settings. Changes apply on "Terapkan"; "Kembali" discards. */
UCLASS()
class GEMAHRIPAH_API UGRSettingsScreen : public UGRScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual void HandleBack() override;

private:
	void RebuildRows();
	UGROptionRow* AddRow(const FText& Label, const TArray<FText>& Options, int32 Index, TFunction<void(int32)> OnChanged);
	void Apply();

	UPROPERTY()
	TObjectPtr<UScrollBox> RowList;

	TArray<FIntPoint> Resolutions;
};
