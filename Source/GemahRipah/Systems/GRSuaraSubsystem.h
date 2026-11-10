#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GRSuaraSubsystem.generated.h"

struct FGRPostDef;
struct FGRQuestDef;

/** What the town does for Bayu at each level of Suara. */
enum class EGRSuaraTier : uint8
{
	Sepi,		// under 20: nobody knows him
	Dikenal,	// 20: people greet him
	Dipercaya,	// 40: vendors give a discount, passengers tip more
	Dibela,		// 60: witnesses go easy on him, people warn him about SPN
	Dilindungi,	// 80: people get in the way of officers chasing him
	Gerakan,	// 90: people walk with him and no riot turns on him
};

/** What Bayu can spend Suara on from the Kata Warga app. */
enum class EGRSuaraSpend : uint8 { RumahAman, PanggilSaksi, GalangDana };

/**
 * Suara as a living meter, the counterweight to Diawasi: Diawasi is the state watching Bayu, Suara is the town covering
 * for him. It fades on days he does nothing for anyone, rises and falls with how he behaves, changes how the town treats
 * him (EGRSuaraTier), can be spent, and draws a smear campaign once it gets too high.
 *
 * The meter itself stays in UGRStatusSubsystem. Everything here lives in story variables, so it is saved with them:
 * suara.aksi_hari, suara.h.<deed>, suara.saksi, suara.beli.<spend>, suara.kesaksian, aduan.hari, buzzer.*.
 */
UCLASS()
class GEMAHRIPAH_API UGRSuaraSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UGRSuaraSubsystem* Get(const UObject* Context);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static EGRSuaraTier TierOf(float Suara);
	static const TCHAR* TierName(EGRSuaraTier Tier);
	/** The Suara that starts a level. */
	static float TierStart(EGRSuaraTier Tier);
	/** One line on what a level gives. */
	static const TCHAR* TierPerk(EGRSuaraTier Tier);
	EGRSuaraTier GetTier() const;
	bool HasTier(EGRSuaraTier Tier) const { return GetTier() >= Tier; }

	/** A good deed in the city: a small silent gain. With a Deed name it counts at most DailyLimit times a day. */
	void Earn(float Amount, const TCHAR* Deed = nullptr, int32 DailyLimit = 0);
	/** A bad one: a small silent loss. */
	void Lose(float Amount);
	/** Bayu was taken in: people who trust him are outraged, the others keep their distance. */
	void NoteCaught();

	/** Vendor and shop price at the current level. */
	int32 PriceFor(int32 Price) const;
	/** Chance of a tip for a trip with this many stars. */
	float TipChance(int32 Stars) const;
	/** What an incident adds to Diawasi once witnesses have had their say, as a multiplier. */
	float IncidentScale() const;

	/** Empty when Bayu can spend on it now, else why not. */
	FString WhyNot(EGRSuaraSpend Spend) const;
	static int32 SpendCost(EGRSuaraSpend Spend);
	/** Money the next Galang Dana would bring. */
	int32 FundAmount() const;
	bool Buy(EGRSuaraSpend Spend);
	/** A witness is on call. */
	bool HasWitness() const;
	/** Uses the witness on call to stop an arrest. */
	bool UseWitness();

	bool IsBuzzerOn() const;
	/** The smear going round, empty when there is none. */
	FString GetBuzzerText() const;
	/** Empty when Bayu has something to answer the smear with, else why not. */
	FString WhyNotRefute() const;
	bool Refute();

	/** Today's complaint, null when there is none left this chapter. */
	const FGRQuestDef* GetTodayAduan() const;
	/** Testimonies from solved complaints, each one answers a smear. */
	int32 GetKesaksian() const;

	/** Bayu's standing in one district (0 to 100), from what he did there. */
	float GetAreaSuara(FName DistrictId) const;
	/** Districts where his name is strong enough to bring out voters. */
	int32 GetStrongAreas() const;

	/** What the town is saying now, newest first. */
	TArray<const FGRPostDef*> GetFeed() const;

	/** A chapter begins: the clock starts over, so nothing tied to a day may carry across. */
	void OnChapterStart();

	/** Cheats: starts a smear, or picks a complaint by its place in the list (1 = first). */
	void StartBuzzer();
	void ForceAduan(int32 Number);

private:
	UFUNCTION()
	void HandleNewDay(int32 Day);

	UFUNCTION()
	void HandleHour(int32 Hour);

	void HandleSuaraChanged(float Old, float New);
	void CreditArea(float Delta);
	void RollAduan();
	void EndBuzzer();
	void Say(const FString& Text, uint8 Type) const;
	class UGRStorySubsystem* GetStory() const;
	class UGRStatusSubsystem* GetStatus() const;
	int32 GetDay() const;
	/** Every complaint of this chapter, in order. */
	TArray<const FGRQuestDef*> GetAduanList() const;
};
