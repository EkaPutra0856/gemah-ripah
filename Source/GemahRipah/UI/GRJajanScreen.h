#pragma once

#include "CoreMinimal.h"
#include "UI/GRScreenWidget.h"
#include "GRJajanScreen.generated.h"

class AGRWarga;
class UBorder;
class USizeBox;
class UTextBlock;
class UHorizontalBox;
class UGRMenuButton;

/**
 * Buying food from a cart vendor (from the talk menu, UGRWargaChatScreen). A side panel lists the cart's menu with a
 * stepper per dish, eat-here or take-away, the total and the energy it brings. Paying starts the sale in the world
 * (AGRWarga::StartServe: made, tossed over the cart, caught, eaten or bagged) while the panel follows along, then
 * shows the receipt. Mouse, keyboard (W/S or arrows pick, A/D change the amount, Tab, Enter) or gamepad d-pad.
 */
UCLASS()
class GEMAHRIPAH_API UGRJajanScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRJajanScreen(const FObjectInitializer& ObjectInitializer);

	void SetWarga(AGRWarga* InWarga);

	/** For scripted tests (GRJajan): amounts in menu order. */
	void SetOrder(const TArray<int32>& Amounts);
	void SetTakeAway(bool bInTakeAway);
	/** Pays and starts the sale; false (with a shake) when nothing is picked or the money is short. */
	bool Pay();

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	struct FRow
	{
		TWeakObjectPtr<UBorder> Card;
		TWeakObjectPtr<UWidget> Icon;
		TWeakObjectPtr<UTextBlock> Amount;
		TWeakObjectPtr<UBorder> Minus;
		TWeakObjectPtr<UBorder> Plus;
		float Highlight = 0.f;
	};

	UBorder* MakeRoundButton(const FText& Glyph);
	UBorder* MakePill(const FText& Label, TObjectPtr<UTextBlock>& OutText);
	void Select(int32 Row);
	void Change(int32 Row, int32 Delta);
	int32 Total() const;
	/** Energy the order gives now: everything when eaten here, only what cannot be bagged otherwise. */
	int32 EnergyGain() const;
	void Refresh();
	void Shake(const FText& Why);
	/** Energy and bagged items, once (sale over, or the panel closed before it ended). */
	void Deliver();
	void ShowReceipt();

	TWeakObjectPtr<AGRWarga> Warga;
	TArray<FRow> Rows;
	TArray<int32> Amounts;
	int32 Selected = 0;
	int32 Hovered = INDEX_NONE;
	bool bTakeAway = false;
	bool bPaid = false;
	bool bDelivered = false;
	bool bReceipt = false;
	int32 PaidTotal = 0;
	int32 PaidEnergy = 0;
	/** Energy before and after the food went in (it stops at 100). */
	int32 EnergyBefore = 0;
	int32 EnergyAfterMeal = 0;
	float ShownTotal = 0.f;
	float ShakeTime = 0.f;
	float Progress = 0.f;
	float ServeClock = 0.f;
	float CloseIn = -1.f;
	int32 LastStage = -1;

	UPROPERTY()
	TObjectPtr<USizeBox> PanelBox;
	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;
	UPROPERTY()
	TObjectPtr<UTextBlock> WalletText;
	UPROPERTY()
	TObjectPtr<UVerticalBox> OrderBox;
	UPROPERTY()
	TObjectPtr<UVerticalBox> ServeBox;
	UPROPERTY()
	TObjectPtr<UBorder> PillEat;
	UPROPERTY()
	TObjectPtr<UBorder> PillTake;
	UPROPERTY()
	TObjectPtr<UTextBlock> PillEatText;
	UPROPERTY()
	TObjectPtr<UTextBlock> PillTakeText;
	UPROPERTY()
	TObjectPtr<UTextBlock> TotalText;
	UPROPERTY()
	TObjectPtr<UTextBlock> LeftText;
	UPROPERTY()
	TObjectPtr<UTextBlock> EnergyText;
	UPROPERTY()
	TObjectPtr<USizeBox> EnergyNow;
	UPROPERTY()
	TObjectPtr<USizeBox> EnergyAfter;
	UPROPERTY()
	TObjectPtr<UTextBlock> WarnText;
	UPROPERTY()
	TObjectPtr<UGRMenuButton> PayButton;
	UPROPERTY()
	TObjectPtr<UTextBlock> StageText;
	UPROPERTY()
	TObjectPtr<UTextBlock> StageSub;
	UPROPERTY()
	TObjectPtr<USizeBox> ProgressFill;
	UPROPERTY()
	TObjectPtr<UHorizontalBox> Basket;
	UPROPERTY()
	TObjectPtr<UVerticalBox> ReceiptBox;
	UPROPERTY()
	TObjectPtr<UTextBlock> HintText;
	UPROPERTY()
	TObjectPtr<UGRMenuButton> DoneButton;
};
