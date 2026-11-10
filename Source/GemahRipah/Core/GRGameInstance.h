#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Containers/Ticker.h"
#include "GRGameInstance.generated.h"

/**
 * Shows the loading screen between maps and runs dev scripts.
 *
 * Dev script (non-shipping): -GRScript=<file> with lines "<seconds> <command>".
 * Commands: "shot <name>" (screenshot to Saved/AutoShots), "quit", or any console command.
 * Time counts from game start, across map loads.
 */
UCLASS()
class GEMAHRIPAH_API UGRGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

private:
	void BeginLoadingScreen(const FString& MapName);

	struct FDevStep
	{
		double Time = 0.0;
		FString Command;
	};

	void LoadDevScript();
	bool TickDevScript(float DeltaTime);

	TArray<FDevStep> DevSteps;
	double DevScriptStart = 0.0;
	/** "tunggu <seconds>": the timeline stands still until no screen is up, for at most this long. */
	double DevHoldUntil = 0.0;
	double DevHoldClear = 0.0;
	double DevLastTick = 0.0;
	double DevLastPress = 0.0;
	/** The hold ends when the dev bot is done, instead of when the screens are gone. */
	bool bDevHoldBot = false;
	FTSTicker::FDelegateHandle DevTickerHandle;
	FDelegateHandle PreLoadMapHandle;
};
