#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GRUISubsystem.generated.h"

class UGRScreenWidget;

/**
 * Stack of menu screens for one local player. Handles input mode, mouse cursor and pause:
 * empty stack = game input; any screen = UI input; pause while any screen asks for it.
 */
UCLASS()
class GEMAHRIPAH_API UGRUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UGRScreenWidget* PushScreen(TSubclassOf<UGRScreenWidget> ScreenClass);

	template <typename T>
	T* PushScreen() { return Cast<T>(PushScreen(T::StaticClass())); }

	void PopScreen(UGRScreenWidget* Screen);
	void PopAll();

	bool HasScreens() const { return Stack.Num() > 0; }
	UGRScreenWidget* GetTopScreen() const { return Stack.Num() > 0 ? Stack.Last().Get() : nullptr; }

private:
	void Refresh();
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	UPROPERTY()
	TArray<TObjectPtr<UGRScreenWidget>> Stack;

	bool bPausedByUI = false;
	FDelegateHandle WorldCleanupHandle;
};
