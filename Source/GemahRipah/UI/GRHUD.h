#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GRHUD.generated.h"

class UGRHUDWidget;

/** Owns the code-built UMG HUD (UGRHUDWidget) for the local player. */
UCLASS()
class GEMAHRIPAH_API AGRHUD : public AHUD
{
	GENERATED_BODY()

public:
	UGRHUDWidget* GetHUDWidget() const { return HUDWidget; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TObjectPtr<UGRHUDWidget> HUDWidget;
};
