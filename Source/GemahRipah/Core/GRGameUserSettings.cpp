#include "Core/GRGameUserSettings.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"

UGRGameUserSettings* UGRGameUserSettings::Get()
{
	return GEngine ? Cast<UGRGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void UGRGameUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();

	if (GEngine)
	{
		if (FAudioDeviceHandle Device = GEngine->GetMainAudioDevice())
		{
			Device->SetTransientPrimaryVolume(MasterVolume);
		}
	}
}

void UGRGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	MasterVolume = 1.f;
	MusicVolume = 0.8f;
	SfxVolume = 1.f;
	MouseSensitivity = 1.f;
	bInvertLookY = false;
	TextSpeed = 45.f;
	SubtitleSize = 1;
}
