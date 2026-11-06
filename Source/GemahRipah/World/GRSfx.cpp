#include "World/GRSfx.h"
#include "Camera/GRTopDownRoomVolume.h"
#include "Components/AudioComponent.h"
#include "Core/GRGameUserSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "Systems/GRClockSubsystem.h"

namespace
{
	struct FSfxFile
	{
		const TCHAR* Name;
		/** 0 = one file S_Name; n = files S_Name_0 to S_Name_(n-1). */
		int32 Variants;
	};

	/** In EGRSfx order. */
	constexpr FSfxFile Files[] = {
		{ TEXT("S_UI_Hover"), 0 },
		{ TEXT("S_UI_Klik"), 0 },
		{ TEXT("S_UI_Buka"), 0 },
		{ TEXT("S_UI_Tutup"), 0 },
		{ TEXT("S_Notif_Info"), 0 },
		{ TEXT("S_Notif_Baik"), 0 },
		{ TEXT("S_Notif_Buruk"), 0 },
		{ TEXT("S_Order_Masuk"), 0 },
		{ TEXT("S_Koin"), 2 },
		{ TEXT("S_Langkah_Beton"), 5 },
		{ TEXT("S_Langkah_Rumput"), 5 },
		{ TEXT("S_Tabrak_Logam"), 5 },
		{ TEXT("S_Tabrak_Badan"), 5 },
		{ TEXT("S_Tabrak_Barang"), 5 },
		{ TEXT("S_Klakson"), 0 },
		{ TEXT("S_Sirene"), 0 },
		{ TEXT("S_Mesin_Motor"), 0 },
		{ TEXT("S_Mesin_Mobil"), 0 },
		{ TEXT("S_Mesin_Angkot"), 0 },
		{ TEXT("S_Amb_Jalan"), 0 },
		{ TEXT("S_Amb_Burung"), 0 },
		{ TEXT("S_Amb_Jangkrik"), 0 },
		{ TEXT("S_Amb_Massa"), 0 },
	};
	static_assert(UE_ARRAY_COUNT(Files) == static_cast<int32>(EGRSfx::Count), "One file entry per EGRSfx");

	USoundBase* LoadSfx(const FString& Name)
	{
		return LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/GemahRipah/Audio/SFX/%s.%s"), *Name, *Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	UGRSfxSubsystem* GetSfx(const UObject* Context)
	{
		const UGameInstance* Game = UGameplayStatics::GetGameInstance(Context);
		return Game ? Game->GetSubsystem<UGRSfxSubsystem>() : nullptr;
	}

	/** Road half width plus pavement: from here out the road gets quieter. */
	constexpr float RoadEdge = 1200.f;
	/** Past this distance from the main roads only the far hum of the town is left. */
	constexpr float RoadFade = 5000.f;
	constexpr float FarHum = 0.25f;
	/** Everything is this much of itself inside a room. */
	constexpr float IndoorLevel = 0.2f;
}

USoundBase* UGRSfxSubsystem::Find(EGRSfx Id)
{
	if (Ranges.IsEmpty())
	{
		for (const FSfxFile& File : Files)
		{
			const int32 First = Sounds.Num();
			if (File.Variants == 0)
			{
				if (USoundBase* Sound = LoadSfx(File.Name))
				{
					Sounds.Add(Sound);
				}
			}
			for (int32 i = 0; i < File.Variants; ++i)
			{
				if (USoundBase* Sound = LoadSfx(FString::Printf(TEXT("%s_%d"), File.Name, i)))
				{
					Sounds.Add(Sound);
				}
			}
			Ranges.Emplace(First, Sounds.Num() - First);
		}
	}
	const TPair<int32, int32>& Range = Ranges[static_cast<int32>(Id)];
	return Range.Value > 0 ? Sounds[Range.Key + FMath::RandRange(0, Range.Value - 1)].Get() : nullptr;
}

USoundAttenuation* UGRSfxSubsystem::GetFalloff(float Reach)
{
	const int32 Key = FMath::RoundToInt(Reach);
	if (const TObjectPtr<USoundAttenuation>* Found = Falloffs.Find(Key))
	{
		return *Found;
	}
	USoundAttenuation* Falloff = NewObject<USoundAttenuation>(this);
	Falloff->Attenuation.bAttenuate = true;
	Falloff->Attenuation.bSpatialize = true;
	Falloff->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
	Falloff->Attenuation.AttenuationShapeExtents = FVector(Reach * 0.2f);
	Falloff->Attenuation.FalloffDistance = Reach * 0.8f;
	Falloffs.Add(Key, Falloff);
	return Falloff;
}

float GRSfx::Setting()
{
	const UGRGameUserSettings* Settings = UGRGameUserSettings::Get();
	return Settings ? Settings->SfxVolume : 1.f;
}

void GRSfx::Play(const UObject* Context, EGRSfx Id, float Volume, float Pitch)
{
	UGRSfxSubsystem* Sfx = GetSfx(Context);
	if (USoundBase* Sound = Sfx ? Sfx->Find(Id) : nullptr)
	{
		UGameplayStatics::PlaySound2D(Context, Sound, Volume * Setting(), Pitch);
	}
}

void GRSfx::PlayAt(const UObject* Context, EGRSfx Id, const FVector& At, float Volume, float Pitch, float Reach)
{
	UGRSfxSubsystem* Sfx = GetSfx(Context);
	if (USoundBase* Sound = Sfx ? Sfx->Find(Id) : nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(Context, Sound, At, FRotator::ZeroRotator, Volume * Setting(), Pitch, 0.f, Sfx->GetFalloff(Reach));
	}
}

UAudioComponent* GRSfx::MakeLoop(USceneComponent* Parent, EGRSfx Id, float Reach)
{
	UGRSfxSubsystem* Sfx = Parent ? GetSfx(Parent) : nullptr;
	USoundBase* Sound = Sfx ? Sfx->Find(Id) : nullptr;
	if (!Sound)
	{
		return nullptr;
	}
	UAudioComponent* Audio = NewObject<UAudioComponent>(Parent->GetOwner());
	Audio->bAutoActivate = false;
	Audio->bAllowSpatialization = true;
	Audio->AttenuationSettings = Sfx->GetFalloff(Reach);
	Audio->SetSound(Sound);
	Audio->SetupAttachment(Parent);
	Audio->RegisterComponent();
	return Audio;
}

// ---------------------------------------------------------------- Ambience

bool UGRAmbienceSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

UAudioComponent* UGRAmbienceSubsystem::Start(EGRSfx Id)
{
	UGRSfxSubsystem* Sfx = GetSfx(this);
	USoundBase* Sound = Sfx ? Sfx->Find(Id) : nullptr;
	UAudioComponent* Audio = Sound ? UGameplayStatics::CreateSound2D(this, Sound, 0.01f, 1.f, 0.f, nullptr, false, false) : nullptr;
	if (Audio)
	{
		Audio->Play(FMath::FRandRange(0.f, 5.f));
	}
	return Audio;
}

bool UGRAmbienceSubsystem::IsPlayerIndoors() const
{
	for (TActorIterator<AGRTopDownRoomVolume> It(GetWorld()); It; ++It)
	{
		if (It->HasPlayerInside())
		{
			return true;
		}
	}
	return false;
}

void UGRAmbienceSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player || !World->GetMapName().Contains(TEXT("KotaSentosa")))
	{
		return;
	}
	if (!Road)
	{
		Road = Start(EGRSfx::AmbJalan);
		Birds = Start(EGRSfx::AmbBurung);
		Crickets = Start(EGRSfx::AmbJangkrik);
	}
	IndoorCheck -= DeltaTime;
	if (IndoorCheck <= 0.f)
	{
		IndoorCheck = 0.5f;
		bIndoors = IsPlayerIndoors();
	}

	const UGRClockSubsystem* Clock = World->GetGameInstance()->GetSubsystem<UGRClockSubsystem>();
	const float Hour = Clock ? Clock->GetHours() : 12.f;
	// 1 from 6:00 to 17:00, easing out to night by 18:30 and back in from 5:00.
	const float Day = FMath::Clamp(Hour - 5.f, 0.f, 1.f) * FMath::Clamp((18.5f - Hour) / 1.5f, 0.f, 1.f);
	const float Morning = FMath::Clamp((11.f - Hour) / 3.f, 0.f, 1.f);

	const FVector Spot = Player->GetActorLocation();
	const float FromRoad = FMath::Min(FMath::Abs(Spot.X), FMath::Abs(Spot.Y)) - RoadEdge;
	const float NearRoad = 1.f - FMath::Clamp(FromRoad / RoadFade, 0.f, 1.f);

	const float Inside = bIndoors ? IndoorLevel : 1.f;
	const float Level = GRSfx::Setting() * Inside;
	const float WantRoad = (FarHum + (1.f - FarHum) * NearRoad) * (0.3f + 0.7f * Day) * Level;
	const float WantBirds = Day * (0.5f + 0.5f * Morning) * (1.f - 0.6f * NearRoad) * Level;
	const float WantCrickets = (1.f - Day) * (1.f - 0.5f * NearRoad) * Level;

	RoadVolume = FMath::FInterpTo(RoadVolume, WantRoad, DeltaTime, 1.5f);
	BirdVolume = FMath::FInterpTo(BirdVolume, WantBirds, DeltaTime, 1.5f);
	CricketVolume = FMath::FInterpTo(CricketVolume, WantCrickets, DeltaTime, 1.5f);
	for (const TPair<UAudioComponent*, float>& Layer : { TPair<UAudioComponent*, float>(Road, RoadVolume), TPair<UAudioComponent*, float>(Birds, BirdVolume), TPair<UAudioComponent*, float>(Crickets, CricketVolume) })
	{
		if (Layer.Key)
		{
			// Volume 0 would stop a looping sound for good.
			Layer.Key->SetVolumeMultiplier(FMath::Max(Layer.Value, 0.01f));
		}
	}
}

FString UGRAmbienceSubsystem::Describe() const
{
	return FString::Printf(TEXT("jalan %.2f burung %.2f jangkrik %.2f%s"), RoadVolume, BirdVolume, CricketVolume, bIndoors ? TEXT(" (dalam ruangan)") : TEXT(""));
}
