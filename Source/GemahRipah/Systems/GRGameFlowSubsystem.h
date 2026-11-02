#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GRGameFlowSubsystem.generated.h"

/** Top-level flow used by the menus: new game, continue, load, back to menu, quit. */
UCLASS()
class GEMAHRIPAH_API UGRGameFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Alur")
	void StartNewGame();

	UFUNCTION(BlueprintCallable, Category = "Alur")
	bool CanContinue() const;

	UFUNCTION(BlueprintCallable, Category = "Alur")
	bool ContinueGame();

	UFUNCTION(BlueprintCallable, Category = "Alur")
	bool LoadGame(const FString& SlotName);

	UFUNCTION(BlueprintCallable, Category = "Alur")
	void ReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Alur")
	void QuitGame();
};
