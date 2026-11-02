#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "GRRegulationSubsystem.generated.h"

class UGRRegulationData;
class UGRSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGROnRegulationEnacted, UGRRegulationData*, Regulation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGROnRegulationRevoked, FGameplayTag, RegulationId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGROnRegulationViolated, UGRRegulationData*, Regulation, FGameplayTag, Action);

/**
 * Rules that change suddenly ("aturan diubah seenaknya").
 * Gameplay reports actions here; if an active rule forbids it, the player is penalized.
 */
UCLASS()
class GEMAHRIPAH_API UGRRegulationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Aturan")
	void EnactRegulation(UGRRegulationData* Regulation);

	UFUNCTION(BlueprintCallable, Category = "Aturan")
	void RevokeRegulation(FGameplayTag RegulationId);

	UFUNCTION(BlueprintCallable, Category = "Aturan")
	void RevokeAll();

	/** Returns the first active rule that forbids Action, or null. */
	UFUNCTION(BlueprintPure, Category = "Aturan")
	UGRRegulationData* FindViolatedRegulation(FGameplayTag Action) const;

	/** Call when the player does Action. Applies the penalty and returns true if it broke a rule. */
	UFUNCTION(BlueprintCallable, Category = "Aturan")
	bool ReportAction(FGameplayTag Action);

	UFUNCTION(BlueprintPure, Category = "Aturan")
	TArray<UGRRegulationData*> GetActiveRegulations() const;

	void WriteToSave(UGRSaveGame& Save) const;
	void ReadFromSave(const UGRSaveGame& Save);

	UPROPERTY(BlueprintAssignable, Category = "Aturan")
	FGROnRegulationEnacted OnRegulationEnacted;

	UPROPERTY(BlueprintAssignable, Category = "Aturan")
	FGROnRegulationRevoked OnRegulationRevoked;

	UPROPERTY(BlueprintAssignable, Category = "Aturan")
	FGROnRegulationViolated OnRegulationViolated;

private:
	UPROPERTY()
	TArray<TObjectPtr<UGRRegulationData>> ActiveRegulations;
};
