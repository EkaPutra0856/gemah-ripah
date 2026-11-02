#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "GRClockSubsystem.generated.h"

class UGRSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGROnHourChanged, int32, Hour);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGROnNewDay, int32, Day);

/**
 * Game clock (1 game day = DayLengthMinutes real minutes, from world.json) and the daily smoke index.
 * Runs only while a city map is being played and the game is not paused.
 */
UCLASS()
class GEMAHRIPAH_API UGRClockSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override { return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Conditional; }
	virtual bool IsTickable() const override { return bRunning; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGRClockSubsystem, STATGROUP_Tickables); }

	void SetRunning(bool bInRunning) { bRunning = bInRunning; }

	UFUNCTION(BlueprintPure, Category = "Waktu")
	int32 GetDay() const { return Day; }

	/** 0..24 */
	UFUNCTION(BlueprintPure, Category = "Waktu")
	float GetHours() const { return Minutes / 60.f; }

	UFUNCTION(BlueprintPure, Category = "Waktu")
	int32 GetHour() const { return FMath::FloorToInt(Minutes / 60.f); }

	UFUNCTION(BlueprintPure, Category = "Waktu")
	bool IsNight() const;

	/** "07:30" */
	FText GetClockText() const;

	/** Sets the time of day. If the hour is earlier than now and bNextDay, moves to the next day. */
	UFUNCTION(BlueprintCallable, Category = "Waktu")
	void SetHour(float Hour, bool bNextDay = false);

	UFUNCTION(BlueprintPure, Category = "Waktu")
	bool CanSleepNow() const;

	/** Jumps to the next morning. */
	UFUNCTION(BlueprintCallable, Category = "Waktu")
	void SleepUntilMorning();

	UFUNCTION(BlueprintPure, Category = "Asap")
	float GetIspu() const;

	/** 0 = clean air, 1 = ISPU 300 or worse. */
	UFUNCTION(BlueprintPure, Category = "Asap")
	float GetSmoke01() const { return FMath::Clamp(GetIspu() / 300.f, 0.f, 1.f); }

	FText GetSmokeCategory() const;
	FLinearColor GetSmokeColor() const;
	FString GetOfficialClaim() const;

	/** Debug override; negative clears it. */
	void SetIspuOverride(float Ispu) { IspuOverride = Ispu; }

	void ResetState();
	void WriteToSave(UGRSaveGame& Save) const;
	void ReadFromSave(const UGRSaveGame& Save);

	UPROPERTY(BlueprintAssignable, Category = "Waktu")
	FGROnHourChanged OnHourChanged;

	UPROPERTY(BlueprintAssignable, Category = "Waktu")
	FGROnNewDay OnNewDay;

private:
	float GetIspuForDay(int32 InDay) const;
	void SetMinutes(int32 NewDay, float NewMinutes);

	int32 Day = 1;
	float Minutes = 0.f;
	float IspuOverride = -1.f;
	bool bRunning = false;
};
