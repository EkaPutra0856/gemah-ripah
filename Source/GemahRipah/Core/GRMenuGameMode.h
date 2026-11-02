#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GRMenuGameMode.generated.h"

/** Main menu map: no pawn, no HUD, just a camera and the menu screen. */
UCLASS()
class GEMAHRIPAH_API AGRMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGRMenuGameMode();

	virtual void RestartPlayer(AController* NewPlayer) override {}
};
