#include "World/GRDayNightController.h"
#include "Systems/GRClockSubsystem.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/GameInstance.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"

AGRDayNightController::AGRDayNightController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->bUnbound = true;
	PostProcess->Priority = -1.f;
	FPostProcessSettings& PP = PostProcess->Settings;
	PP.bOverride_AutoExposureMinBrightness = true;
	PP.AutoExposureMinBrightness = -3.f;
	PP.bOverride_AutoExposureMaxBrightness = true;
	PP.AutoExposureMaxBrightness = 12.f;
	PP.bOverride_AutoExposureBias = true;
	RootComponent = PostProcess;
}

void AGRDayNightController::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	FindLights();
	Apply(PreviewHour, FMath::Clamp(PreviewIspu / 300.f, 0.f, 1.f));
}

void AGRDayNightController::BeginPlay()
{
	Super::BeginPlay();
	FindLights();

	StreetLamps.Reset();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("LampuJalan")))
		{
			TInlineComponentArray<ULightComponent*> Lights(*It);
			StreetLamps.Append(Lights);
		}
	}
	bLampsInitialized = false;

	// Only the sun lights fog and translucency; the moon is fill light.
	if (Sun)
	{
		Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetForwardShadingPriority(1);
	}
	if (Moon)
	{
		Cast<UDirectionalLightComponent>(Moon->GetLightComponent())->SetForwardShadingPriority(0);
	}
}

void AGRDayNightController::FindLights()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("Bulan")))
		{
			if (!Moon) { Moon = *It; }
		}
		else if (!Sun)
		{
			Sun = *It;
		}
	}
	if (!SkyLight)
	{
		TActorIterator<ASkyLight> It(World);
		SkyLight = It ? *It : nullptr;
	}
	if (!Fog)
	{
		TActorIterator<AExponentialHeightFog> It(World);
		Fog = It ? *It : nullptr;
	}
}

void AGRDayNightController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UGRClockSubsystem* Clock = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGRClockSubsystem>() : nullptr;
	if (Clock)
	{
		Apply(Clock->GetHours(), Clock->GetSmoke01());
	}
}

void AGRDayNightController::Apply(float Hours, float Smoke01)
{
	// The sun circles once per day: rises at 06:00 in the east (+Y), sets at 18:00 in the west.
	const float Phase = (Hours - 6.f) / 24.f * 2.f * PI;
	const float Sine = FMath::Sin(Phase);
	const float Elevation = Sine >= 0.f ? Sine * MaxSunElevation : Sine * 35.f;
	const float Yaw = -90.f + (Hours - 6.f) / 12.f * 180.f;
	const float DayFactor = FMath::SmoothStep(-6.f, 10.f, Elevation);
	const float SunUp = FMath::SmoothStep(-2.f, 8.f, Elevation);

	if (Sun)
	{
		Sun->SetActorRotation(FRotator(-Elevation, Yaw, 0.f));
		ULightComponent* Light = Sun->GetLightComponent();
		const FLinearColor Low(1.f, 0.45f, 0.2f);
		const FLinearColor High(1.f, 0.93f, 0.82f);
		FLinearColor Color = FMath::Lerp(Low, High, FMath::Clamp(Elevation / 35.f, 0.f, 1.f));
		Color = FMath::Lerp(Color, FLinearColor(1.f, 0.55f, 0.3f), Smoke01 * 0.6f);
		Light->SetLightColor(Color);
		Light->SetIntensity(SunIntensity * SunUp * FMath::Lerp(1.f, 0.4f, Smoke01));
	}
	if (Moon)
	{
		Moon->SetActorRotation(FRotator(-45.f, Yaw + 180.f, 0.f));
		ULightComponent* Light = Moon->GetLightComponent();
		Light->SetLightColor(FLinearColor(0.55f, 0.65f, 1.f));
		Light->SetIntensity(MoonIntensity * (1.f - DayFactor));
	}
	if (SkyLight)
	{
		SkyLight->GetLightComponent()->SetIntensity(FMath::Lerp(0.6f, 1.f, DayFactor));
	}
	if (Fog)
	{
		UExponentialHeightFogComponent* FogComp = Fog->GetComponent();
		FogComp->SetFogDensity(FMath::Lerp(MinFogDensity, MaxFogDensity, Smoke01));
		const FLinearColor Clear(0.45f, 0.55f, 0.65f);
		const FLinearColor Smoky(0.85f, 0.45f, 0.16f);
		FLinearColor FogColor = FMath::Lerp(Clear, Smoky, Smoke01);
		// At night the smoke only glows faintly from the city lights.
		FogColor *= FMath::Lerp(0.22f, 1.f, DayFactor);
		FogComp->SetFogInscatteringColor(FogColor);
	}
	if (PostProcess)
	{
		PostProcess->Settings.AutoExposureBias = FMath::Lerp(-0.5f, 0.f, DayFactor);
	}

	const bool bWantLamps = DayFactor < 0.35f;
	if (!bLampsInitialized || bWantLamps != bLampsOn)
	{
		bLampsInitialized = true;
		bLampsOn = bWantLamps;
		for (ULightComponent* Lamp : StreetLamps)
		{
			if (Lamp)
			{
				Lamp->SetVisibility(bLampsOn);
			}
		}
	}
}
