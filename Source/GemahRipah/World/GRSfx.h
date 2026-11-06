#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "GRSfx.generated.h"

class UAudioComponent;
class USceneComponent;
class USoundAttenuation;
class USoundBase;

/** Sound effects in /Game/GemahRipah/Audio/SFX (Tools/Python/prepare_sfx.py, import_sfx.py). */
enum class EGRSfx : uint8
{
	UIHover,
	UIKlik,
	UIBuka,
	UITutup,
	NotifInfo,
	NotifBaik,
	NotifBuruk,
	OrderMasuk,
	Koin,
	LangkahBeton,
	LangkahRumput,
	TabrakLogam,
	TabrakBadan,
	TabrakBarang,
	Klakson,
	Sirene,
	MesinMotor,
	MesinMobil,
	MesinAngkot,
	AmbJalan,
	AmbBurung,
	AmbJangkrik,
	AmbMassa,
	Count
};

/** Loads the sounds once and keeps them, with the shared falloff settings. Volumes follow the "Volume efek" setting. */
UCLASS()
class GEMAHRIPAH_API UGRSfxSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The sound, or one of its variants at random. Null when it was not imported. */
	USoundBase* Find(EGRSfx Id);
	/** Spherical falloff to silence at Reach cm, full volume inside a fifth of it. Shared per Reach. */
	USoundAttenuation* GetFalloff(float Reach);

private:
	UPROPERTY()
	TArray<TObjectPtr<USoundBase>> Sounds;

	UPROPERTY()
	TMap<int32, TObjectPtr<USoundAttenuation>> Falloffs;

	/** First entry of Sounds and number of variants, per EGRSfx. Empty until the first Find. */
	TArray<TPair<int32, int32>> Ranges;
};

namespace GRSfx
{
	/** The player's "Volume efek" setting, 0-1. */
	float Setting();
	/** Not placed in the world (UI). Keeps playing while the game is paused. */
	void Play(const UObject* Context, EGRSfx Id, float Volume = 1.f, float Pitch = 1.f);
	/** At a place in the world, gone at Reach cm. */
	void PlayAt(const UObject* Context, EGRSfx Id, const FVector& At, float Volume = 1.f, float Pitch = 1.f, float Reach = 3000.f);
	/** A looping sound following Parent, not started yet. */
	UAudioComponent* MakeLoop(USceneComponent* Parent, EGRSfx Id, float Reach);
}

/**
 * The sound of the town around Bayu: the road (louder near the two main roads, thinner at night), birds by day,
 * crickets by night. Everything drops low inside a room.
 */
UCLASS()
class GEMAHRIPAH_API UGRAmbienceSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGRAmbienceSubsystem, STATGROUP_Tickables); }

	/** For GRAudio: the three volumes now. */
	FString Describe() const;

private:
	UAudioComponent* Start(EGRSfx Id);
	bool IsPlayerIndoors() const;

	UPROPERTY()
	TObjectPtr<UAudioComponent> Road;

	UPROPERTY()
	TObjectPtr<UAudioComponent> Birds;

	UPROPERTY()
	TObjectPtr<UAudioComponent> Crickets;

	float RoadVolume = 0.f;
	float BirdVolume = 0.f;
	float CricketVolume = 0.f;
	float IndoorCheck = 0.f;
	bool bIndoors = false;
};
