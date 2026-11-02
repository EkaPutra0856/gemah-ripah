#pragma once

#include "CoreMinimal.h"
#include "UI/GRScreenWidget.h"
#include "GRSaveLoadScreen.generated.h"

class UVerticalBox;

/** Slot list shared by the save and load screens. Rebuilt every time it is shown. */
UCLASS(Abstract)
class GEMAHRIPAH_API UGRSlotScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	virtual void OnShown() override;

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual bool IsSaveMode() const { return false; }

private:
	void RebuildSlots();
	void HandleSlot(const FString& SlotName, bool bExists);
	bool IsInGame() const;

	UPROPERTY()
	TObjectPtr<UVerticalBox> SlotList;
};

UCLASS()
class GEMAHRIPAH_API UGRSaveScreen : public UGRSlotScreen
{
	GENERATED_BODY()

protected:
	virtual bool IsSaveMode() const override { return true; }
};

UCLASS()
class GEMAHRIPAH_API UGRLoadScreen : public UGRSlotScreen
{
	GENERATED_BODY()
};
