#pragma once

#include "CoreMinimal.h"
#include "UI/GRScreenWidget.h"
#include "GRTrailerScreen.generated.h"

class ACineCameraActor;
class UBorder;
class UTextBlock;
class FJsonObject;

/**
 * Plays the trailer from Content/Data/trailer.json for recording: shot by shot it sets the hour and the smog, spawns
 * the people (story models, animations, walks), props and rigged vehicles, moves a cinema camera (fixed, panning, or
 * following someone, focus on whoever it aims at) and shows title cards and captions. Bayu can ride for real in a
 * shot. Everything it spawns goes when the shot or the trailer ends; story NPCs are hidden meanwhile. Cheat GRTrailer.
 */
UCLASS()
class GEMAHRIPAH_API UGRTrailerScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRTrailerScreen(const FObjectInitializer& ObjectInitializer);
	/** From shot Start (0 = the beginning), to test a part. */
	static bool Open(class UGRUISubsystem* UI, int32 Start = 0);

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	virtual void HandleBack() override;

private:
	struct FCast
	{
		FString Id;
		TWeakObjectPtr<AActor> Actor;
		FVector WalkTo = FVector::ZeroVector;
		bool bWalk = false;
		FVector MoveFrom = FVector::ZeroVector;
		FVector MoveTo = FVector::ZeroVector;
		bool bSlide = false;
	};

	void StartShot(int32 Index);
	void Finish(bool bClose = true);
	void ClearGroup(TArray<FCast>& Group);
	void SpawnCast(const FJsonObject& Shot);
	void SpawnProps(const FJsonObject& Shot);
	void SpawnVehicles(const FJsonObject& Shot);
	void SetupRide(const FJsonObject* Ride);
	AActor* FindTarget(const FString& Id) const;
	void TickCamera(float Alpha, float DeltaTime);
	void TickCast(float DeltaTime, float Alpha);
	void ShowPlayer(bool bShow);
	/** Hides the HUD, phone overlays and story people that appeared since the last shot. */
	void HideWorld();

	static int32 PendingStart;
	TArray<TSharedPtr<FJsonObject>> Shots;
	int32 ShotIndex = INDEX_NONE;
	float ShotTime = 0.f;
	float ShotLength = 1.f;
	float FadeIn = 0.f;
	float FadeOut = 0.f;
	bool bDone = false;

	TArray<FCast> People;
	TArray<FCast> Props;
	TArray<FCast> Vehicles;
	TArray<TWeakObjectPtr<AActor>> HiddenStory;
	TMap<TWeakObjectPtr<UUserWidget>, ESlateVisibility> HiddenWidgets;

	FVector CamFrom = FVector::ZeroVector;
	FVector CamTo = FVector::ZeroVector;
	FRotator RotFrom = FRotator::ZeroRotator;
	FRotator RotTo = FRotator::ZeroRotator;
	float FocalFrom = 35.f;
	float FocalTo = 35.f;
	FString FollowId;
	FVector FollowOffset = FVector(-420.f, 0.f, 160.f);
	FVector FollowOffsetTo = FVector(-420.f, 0.f, 160.f);
	FString AimId;
	float AimHeight = 150.f;
	bool bHasCamera = false;

	bool bRiding = false;
	float RideThrottle = 0.f;
	float RideSteer = 0.f;

	UPROPERTY() TObjectPtr<ACineCameraActor> Camera;
	UPROPERTY() TObjectPtr<UBorder> CardPanel;
	UPROPERTY() TObjectPtr<UTextBlock> TitleText;
	UPROPERTY() TObjectPtr<UTextBlock> SubText;
	UPROPERTY() TObjectPtr<UTextBlock> CaptionText;
	UPROPERTY() TObjectPtr<UBorder> CaptionPanel;
};
