#pragma once

#include "CoreMinimal.h"
#include "UI/GRScreenWidget.h"
#include "GRWargaChat.generated.h"

class AGRWarga;
class UGRMenuButton;
class UTextBlock;

/**
 * Small talk menu beside a townsperson Bayu greeted with E: say hello, ask how things are,
 * ask the way to the current goal, buy a snack from a cart vendor. The game keeps running;
 * the answers come as speech bubbles. Walking away, Escape or "Pamit" closes it.
 */
UCLASS()
class GEMAHRIPAH_API UGRWargaChatScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRWargaChatScreen(const FObjectInitializer& ObjectInitializer);

	void SetWarga(AGRWarga* InWarga);

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void Pick(int32 Topic);

	TWeakObjectPtr<AGRWarga> Warga;

	UPROPERTY()
	TObjectPtr<UVerticalBox> Options;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> SubtitleText;

	/** Topic of each option row, in key order (1, 2, ...). */
	TArray<int32> Topics;

	UPROPERTY()
	TArray<TObjectPtr<UGRMenuButton>> Buttons;
};
