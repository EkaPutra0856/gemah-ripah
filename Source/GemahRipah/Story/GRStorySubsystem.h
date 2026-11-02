#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Story/GRStoryTypes.h"
#include "GRStorySubsystem.generated.h"

class UGRSaveGame;

/** UI or world work an action asks for. The story director in the city map carries it out. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FGROnStoryRequest, FName /*Kind*/, const FString& /*Arg*/);

/**
 * Chapter state and the tiny story language used by the JSON content.
 *
 * Conditions ("If"): terms joined by "&&" or "||", each "var", "!var" or "var<op>number"
 * (op: >= <= == != > <). Built-in vars: day, hour, uang, suara, diawasi, energi, malam, order.
 * Items are vars "item.<id>", quest steps "q.<id>" (0 = not started, 1.. = step, 100 = done).
 *
 * Actions ("Do"): commands separated by ";", for example
 * "uang -20000 Pungli; quest Pagi; step Pagi 2; item beras 1; flag bukti; dialog ibu_pagi".
 */
UCLASS()
class GEMAHRIPAH_API UGRStorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 QuestDone = 100;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void Reload();
	const FGRStoryFile& GetData() const { return Data; }

	int32 GetVar(const FString& Name) const;
	void SetVar(const FString& Name, int32 Value);
	bool Check(const FString& Condition) const;
	void Run(const FString& Actions);

	const FGRDialogDef* FindDialog(const FString& Id) const;
	const FGRSpeakerDef* FindSpeaker(const FString& Id) const;
	const FGRQuestDef* FindQuest(const FString& Id) const;
	const FGRItemDef* FindItem(const FString& Id) const;
	const FGRSpawnDef* FindSpawn(const FString& Id) const;
	const FGRCutsceneDef* FindCutscene(const FString& Id) const;
	const FGROrderDef* FindOrder(const FString& Id) const;
	const FGRMinigameDef* FindMinigame(const FString& Id) const;

	int32 GetQuestStep(const FString& QuestId) const { return GetVar(TEXT("q.") + QuestId); }
	/** Active quests, most recently started first. */
	TArray<const FGRQuestDef*> GetActiveQuests() const;
	TArray<const FGRQuestDef*> GetDoneQuests() const;
	FString GetObjectiveText() const;
	/** Spawn id the tracked marker should point at (active order first, then the tracked quest). */
	FString GetTargetSpawnId() const;
	int32 GetItemCount(const FString& ItemId) const { return GetVar(TEXT("item.") + ItemId); }
	int32 GetShopPrice(const FGRShopEntry& Entry) const;

	// Energy (0..100). Drains faster in thick smoke; empty = Bayu faints.
	float GetEnergi() const { return Energi; }
	void AddEnergi(float Delta);
	void TickEnergy(float DeltaSeconds, bool bExerting);

	// OjekKu orders.
	const FGROrderDef* GetActiveOrder() const { return FindOrder(ActiveOrder); }
	int32 GetOrderStage() const { return OrderStage; }
	float GetOrderTimeLeft() const { return OrderTimeLeft; }
	bool IsOrderAvailable(const FGROrderDef& Order) const;
	void StartOrder(const FString& OrderId);
	void PickUpOrder();
	void CompleteOrder();
	void CancelOrder();
	void TickOrder(float DeltaSeconds);

	// Phone.
	const TArray<FString>& GetInbox() const { return Inbox; }
	const TArray<FString>& GetLedger() const { return Ledger; }
	void AddLedger(int32 Amount, const FString& Label);

	/** Runs every event whose condition holds (once each). */
	void TickEvents();
	/** Morning routine after sleeping: OnMorning actions and family needs. */
	void RunMorning();

	void ResetState();
	void StartChapter();
	void WriteToSave(UGRSaveGame& Save) const;
	void ReadFromSave(const UGRSaveGame& Save);

	FGROnStoryRequest OnRequest;
	FSimpleMulticastDelegate OnChanged;

private:
	void RunCommand(const FString& Command, const FString& Arg);
	bool CheckTerm(const FString& Term) const;
	float GetValue(const FString& Name) const;
	void Notify(const FString& Text, uint8 Type) const;

	FGRStoryFile Data;
	TMap<FString, int32> Vars;
	TArray<FString> QuestOrder;
	TArray<FString> Inbox;
	TArray<FString> Ledger;
	float Energi = 100.f;
	FString ActiveOrder;
	int32 OrderStage = 0;
	float OrderTimeLeft = 0.f;
	bool bFainting = false;
};
