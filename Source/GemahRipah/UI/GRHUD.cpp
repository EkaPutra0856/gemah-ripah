#include "UI/GRHUD.h"
#include "UI/GRHUDWidget.h"
#include "GameFramework/PlayerController.h"

void AGRHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (PC && PC->IsLocalController())
	{
		HUDWidget = CreateWidget<UGRHUDWidget>(PC, UGRHUDWidget::StaticClass());
		HUDWidget->AddToViewport(0);
	}
}

void AGRHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
