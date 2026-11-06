#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "Interaction/GRInteractable.h"
#include "UI/GRScreenWidget.h"
#include "GRMusic.generated.h"

class UAudioComponent;
class UBoxComponent;
class USoundBase;
class UStaticMeshComponent;
class UTextBlock;
class UGRMenuButton;

/**
 * A speaker in the world (campaign toa, office radio). Plays its song in 3D and loops it; E turns it off or on.
 * The off state is kept by UGRMusicSubsystem per speaker id, so it stays off after a reload of the map.
 */
UCLASS()
class GEMAHRIPAH_API AGRSpeaker : public AActor, public IGRInteractable
{
	GENERATED_BODY()

public:
	AGRSpeaker();

	/** bToa = campaign loudspeaker on a pole; otherwise a small radio on a desk. */
	void Setup(const FString& InId, USoundBase* Song, bool bToa, float Volume, float Reach);
	void SetSpeakerOn(bool bOn);
	/** The crowd in front of the stage, heard from this speaker. */
	void SetCrowdOn(bool bOn);
	bool IsSpeakerOn() const { return bOn; }
	const FString& GetSpeakerId() const { return SpeakerId; }

	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<UBoxComponent> Box;

	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<UAudioComponent> Audio;

private:
	UStaticMeshComponent* AddPart(const TCHAR* Shape, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color);

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Crowd;

	FString SpeakerId;
	bool bOn = true;
	bool bToaModel = false;
};

/**
 * Music in the world. Bayu's motor radio: eight slots, two unlocked at first, plays while he rides with the radio on
 * (R opens UGRRadioScreen). Speakers: the campaign toa at every active "panggung" story prop and the radio in the
 * kelurahan office. Radio state and switched-off speakers live for the whole session.
 */
UCLASS()
class GEMAHRIPAH_API UGRMusicSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 SlotCount = 8;

	struct FSlot
	{
		FText Title;
		FText Info;
		const TCHAR* Path = nullptr;
	};

	static UGRMusicSubsystem* Get(const UObject* WorldContext);
	static const FSlot& GetSlot(int32 Index);

	bool IsUnlocked(int32 Index) const;
	void Unlock(int32 Index);
	/** Plays a slot on the radio (turns the radio on). */
	void PlaySlot(int32 Index);
	void Next();
	void SetRadioOn(bool bOn);
	bool IsRadioOn() const;
	int32 GetCurrent() const;
	float GetVolume() const;
	void SetVolume(float Volume);
	/** Seconds into the song and its length, for the radio screen. */
	void GetProgress(float& Seconds, float& Duration) const;
	bool CanUseRadio() const;

	bool IsSpeakerOff(const FString& Id) const;
	void SetSpeakerOff(const FString& Id, bool bOff);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGRMusicSubsystem, STATGROUP_Tickables); }

private:
	void UpdateRadio(float DeltaTime);
	void UpdateSpeakers();
	AGRSpeaker* SpawnSpeaker(const FString& Id, const TCHAR* SongPath, const FVector& Ground, float Yaw, bool bToa, float Volume, float Reach);
	void StartRadioSong();

	UPROPERTY()
	TObjectPtr<UAudioComponent> RadioAudio;

	UPROPERTY()
	TMap<FString, TObjectPtr<AGRSpeaker>> Speakers;

	float SpeakerCheck = 0.f;
	float SongSeconds = 0.f;
	float SongDuration = 0.f;
	bool bRadioPlaying = false;
};

/** The motor radio: eight song slots (locked ones greyed), on/off, next and volume. Keys 1-8, 0, - and +. */
UCLASS()
class GEMAHRIPAH_API UGRRadioScreen : public UGRScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void Refresh();

	UPROPERTY()
	TArray<TObjectPtr<UGRMenuButton>> SlotButtons;

	UPROPERTY()
	TObjectPtr<UGRMenuButton> PowerButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> NowText;

	UPROPERTY()
	TObjectPtr<UTextBlock> VolumeText;
};
