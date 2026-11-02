#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GRDayNightController.generated.h"

class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class ULightComponent;
class UPostProcessComponent;

/**
 * Drives sun, moon, sky light, fog and street lamps from the game clock and smoke index.
 * Place one per city map. Lights are found automatically when not assigned:
 * sun = DirectionalLight tagged "Matahari" (or the first one), moon = tagged "Bulan",
 * street lamps = any actor tagged "LampuJalan".
 */
UCLASS()
class GEMAHRIPAH_API AGRDayNightController : public AActor
{
	GENERATED_BODY()

public:
	AGRDayNightController();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Cahaya")
	TObjectPtr<ADirectionalLight> Sun;

	UPROPERTY(EditAnywhere, Category = "Cahaya")
	TObjectPtr<ADirectionalLight> Moon;

	UPROPERTY(EditAnywhere, Category = "Cahaya")
	TObjectPtr<ASkyLight> SkyLight;

	UPROPERTY(EditAnywhere, Category = "Cahaya")
	TObjectPtr<AExponentialHeightFog> Fog;

	UPROPERTY(EditAnywhere, Category = "Cahaya")
	float SunIntensity = 8.f;

	UPROPERTY(EditAnywhere, Category = "Cahaya")
	float MoonIntensity = 1.5f;

	/** Highest sun elevation at noon, in degrees. */
	UPROPERTY(EditAnywhere, Category = "Cahaya")
	float MaxSunElevation = 65.f;

	UPROPERTY(EditAnywhere, Category = "Asap")
	float MinFogDensity = 0.008f;

	UPROPERTY(EditAnywhere, Category = "Asap")
	float MaxFogDensity = 0.07f;

	/** Preview hour when no clock is running (editor). */
	UPROPERTY(EditAnywhere, Category = "Waktu", meta = (ClampMin = 0, ClampMax = 24))
	float PreviewHour = 9.f;

	UPROPERTY(EditAnywhere, Category = "Waktu")
	float PreviewIspu = 160.f;

	UPROPERTY(VisibleAnywhere, Category = "Komponen")
	TObjectPtr<UPostProcessComponent> PostProcess;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void FindLights();
	void Apply(float Hours, float Smoke01);

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULightComponent>> StreetLamps;

	bool bLampsOn = false;
	bool bLampsInitialized = false;
};
