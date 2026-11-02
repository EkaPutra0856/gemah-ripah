#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GRStealthLevel.generated.h"

class AGRNpc;
class AGRStoryProp;
class AGRTopDownRoomVolume;
class UGRStorySubsystem;

/**
 * Stealth minigame played in the real map: builds the grid from bab1.json inside a top-down room
 * (shelves, partitions, evidence, SPN guards with flashlights), moves the real player in it, and
 * cleans everything up afterwards. Guard sight fills a suspicion meter; full = caught.
 *
 * Grid characters: '#' partition, 'C' shelf (hide behind it), 'B' evidence, 'G'/'H' guard patrolling
 * along its row/column, 'P' start, 'X' exit. The first row is the far wall (+X), columns run along +Y.
 */
UCLASS()
class GEMAHRIPAH_API AGRStealthLevel : public AActor
{
	GENERATED_BODY()

public:
	AGRStealthLevel();

	/** Starts the minigame in the room named by its "room" field. False when that room is not in this map. */
	static bool Start(UWorld* World, const FString& MinigameId);
	static AGRStealthLevel* GetActive(const UWorld* World);

	virtual void Tick(float DeltaSeconds) override;

	float GetSuspicion() const { return Suspicion; }
	int32 GetTaken() const;
	int32 GetTotal() const { return LootProps.Num(); }

	void Retry();
	void GiveUp();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool Build(AGRTopDownRoomVolume* Room);
	void ResetRun();
	void Caught();
	void Finish(bool bWin);
	void MovePlayer(const FVector& Location, float Yaw) const;
	FVector CellCenter(int32 Row, int32 Col) const;
	UGRStorySubsystem* GetStory() const;

	static TWeakObjectPtr<AGRStealthLevel> Active;

	FString MinigameId;
	FVector Top = FVector::ZeroVector;
	float Cell = 200.f;
	FVector2D HalfSize = FVector2D::ZeroVector;
	FVector StartPos = FVector::ZeroVector;
	FVector ExitPos = FVector::ZeroVector;
	FVector OutsidePos = FVector::ZeroVector;
	TArray<FString> LootItems;
	float Suspicion = 0.f;
	float ExitHintCooldown = 0.f;
	bool bOver = false;
	bool bWasInside = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> Built;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGRNpc>> Guards;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGRStoryProp>> LootProps;

	UPROPERTY(Transient)
	TObjectPtr<AGRTopDownRoomVolume> RoomVolume;

	FVector SavedCameraLocation = FVector::ZeroVector;
	FRotator SavedCameraRotation = FRotator::ZeroRotator;
	bool bSavedFollow = false;
};
