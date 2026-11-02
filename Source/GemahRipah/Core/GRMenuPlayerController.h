#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GRMenuPlayerController.generated.h"

/** Looks through the first camera in the menu map and shows the main menu. */
UCLASS()
class GEMAHRIPAH_API AGRMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGRMenuPlayerController();

protected:
	virtual void BeginPlay() override;
};
