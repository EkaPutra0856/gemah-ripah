#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GRStoryDirector.generated.h"

class AGRMapMarker;
class AGRNpc;
class AGRStoryProp;
class UStaticMeshComponent;
class UGRStorySubsystem;

/**
 * Runs the chapter inside the city map: spawns NPCs, props and zones from bab1.json, shows/hides them
 * by their conditions, fires story events, tracks OjekKu orders, drives simple road traffic, and opens
 * the dialog/cutscene/minigame screens that story actions ask for (one at a time).
 * The game mode spawns one automatically.
 */
UCLASS()
class GEMAHRIPAH_API AGRStoryDirector : public AActor
{
	GENERATED_BODY()

public:
	AGRStoryDirector();

	virtual void Tick(float DeltaSeconds) override;

	AActor* FindSpawnedActor(const FString& SpawnId) const;
	FVector GetSpawnLocation(const FString& SpawnId) const;

	static AGRStoryDirector* Get(const UObject* WorldContext);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FCar
	{
		TObjectPtr<UStaticMeshComponent> Body;
		bool bAlongY = true;
		float Lane = 0.f;
		float Dir = 1.f;
		float Pos = 0.f;
		float Speed = 1000.f;
	};

	struct FRequest
	{
		FName Kind;
		FString Arg;
	};

	void SpawnAll();
	void SpawnTraffic();
	void RefreshSpawns();
	void CheckZones();
	void CheckOrder();
	void UpdateTargetMarker();
	void TickTraffic(float DeltaSeconds);
	void HandleRequest(FName Kind, const FString& Arg);
	void ProcessRequests();
	bool IsUIBusy() const;
	void Teleport(const FString& SpawnId);
	void ForceSleep();

	UFUNCTION()
	void HandleDiawasiMaxed();

	UGRStorySubsystem* GetStory() const;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<AActor>> Spawned;

	UPROPERTY(Transient)
	TObjectPtr<AGRMapMarker> TargetMarker;

	TArray<FCar> Cars;
	TArray<FRequest> Queue;
	FDelegateHandle RequestHandle;
	float SlowTimer = 0.f;
	bool bStarted = false;
};
