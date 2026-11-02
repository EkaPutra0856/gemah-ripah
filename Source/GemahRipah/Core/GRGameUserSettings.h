#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "GRGameUserSettings.generated.h"

/** Player settings saved in Saved/Config/<Platform>/GameUserSettings.ini. */
UCLASS(Config = GameUserSettings)
class GEMAHRIPAH_API UGRGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static UGRGameUserSettings* Get();

	virtual void ApplyNonResolutionSettings() override;
	virtual void SetToDefaults() override;

	/** 0..1 */
	UPROPERTY(Config)
	float MasterVolume = 1.f;

	/** 0..1. Applied once music sound classes exist (audio phase). */
	UPROPERTY(Config)
	float MusicVolume = 0.8f;

	/** 0..1. Applied once SFX sound classes exist (audio phase). */
	UPROPERTY(Config)
	float SfxVolume = 1.f;

	UPROPERTY(Config)
	float MouseSensitivity = 1.f;

	UPROPERTY(Config)
	bool bInvertLookY = false;

	/** Characters per second for dialog typewriter; 0 = instant. */
	UPROPERTY(Config)
	float TextSpeed = 45.f;

	/** 0 small, 1 medium, 2 large */
	UPROPERTY(Config)
	int32 SubtitleSize = 1;
};
