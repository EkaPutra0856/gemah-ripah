#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GRGameMode.generated.h"

UCLASS()
class GEMAHRIPAH_API AGRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGRGameMode();

	/** Spawns as usual, then moves the pawn to the loaded save position if there is one. */
	virtual void RestartPlayer(AController* NewPlayer) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
