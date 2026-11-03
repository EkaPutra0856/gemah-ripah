#include "Story/GRStorySubsystem.h"
#include "Data/GRDataSubsystem.h"
#include "Systems/GRClockSubsystem.h"
#include "Systems/GRNotificationSubsystem.h"
#include "Systems/GRRegulationSubsystem.h"
#include "Systems/GRSaveGame.h"
#include "Systems/GRStatusSubsystem.h"
#include "GameplayTagContainer.h"

DEFINE_LOG_CATEGORY_STATIC(LogGRStory, Log, All);

namespace
{
	template <typename T>
	const T* FindById(const TArray<T>& List, const FString& Id)
	{
		return List.FindByPredicate([&Id](const T& Entry) { return Entry.Id == Id; });
	}
}

void UGRStorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UGRStatusSubsystem>();
	Collection.InitializeDependency<UGRClockSubsystem>();
	Collection.InitializeDependency<UGRRegulationSubsystem>();
	Collection.InitializeDependency<UGRNotificationSubsystem>();
	Reload();
}

void UGRStorySubsystem::Reload()
{
	Chapters = FGRChapterList();
	UGRDataSubsystem::LoadJsonFile(TEXT("chapters.json"), Chapters);
	if (!FindChapter(ChapterId))
	{
		ChapterId = Chapters.Chapters.Num() > 0 ? Chapters.Chapters[0].Id : FString();
	}
	const FGRChapterDef* Chapter = FindChapter(ChapterId);
	const FString File = Chapter && !Chapter->File.IsEmpty() ? Chapter->File : FString(TEXT("bab1.json"));
	Data = FGRStoryFile();
	UGRDataSubsystem::LoadJsonFile(File, Data);
	// Furniture shared by every chapter (tables and chairs in the rooms where people talk).
	FGRStoryFile Furniture;
	if (UGRDataSubsystem::LoadJsonFile(TEXT("furniture.json"), Furniture))
	{
		Data.Spawns.Append(Furniture.Spawns);
	}
	UE_LOG(LogGRStory, Log, TEXT("%s: %d dialog, %d quest, %d spawn, %d event"), *File, Data.Dialogs.Num(), Data.Quests.Num(), Data.Spawns.Num(), Data.Events.Num());
}

const FGRChapterDef* UGRStorySubsystem::FindChapter(const FString& Id) const
{
	return Id.IsEmpty() ? nullptr : FindById(Chapters.Chapters, Id);
}

const FGRChapterDef* UGRStorySubsystem::GetNextChapter() const
{
	const int32 Index = Chapters.Chapters.IndexOfByPredicate([this](const FGRChapterDef& C) { return C.Id == ChapterId; });
	return Chapters.Chapters.IsValidIndex(Index + 1) ? &Chapters.Chapters[Index + 1] : nullptr;
}

bool UGRStorySubsystem::IsChapterUnlocked(const FString& Id) const
{
	const int32 Index = Chapters.Chapters.IndexOfByPredicate([&Id](const FGRChapterDef& C) { return C.Id == Id; });
	if (Index == INDEX_NONE || Chapters.Chapters[Index].File.IsEmpty())
	{
		return false;
	}
	return Index == 0 || IsChapterDone(Chapters.Chapters[Index - 1].Id) || IsChapterDone(Id);
}

void UGRStorySubsystem::SetChapterDone(const FString& Id)
{
	CompletedChapters.AddUnique(Id);
	OnChanged.Broadcast();
}

void UGRStorySubsystem::BeginChapter(const FString& Id)
{
	ChapterId = Id;
	Reload();
	// Choices from earlier chapters stay in Vars (Bab 2 can ask "guru_dilindungi"); the rest is per chapter.
	Vars.Remove(TEXT("bab_dimulai"));
	QuestOrder.Reset();
	Inbox.Reset();
	Energi = 100.f;
	ActiveOrder.Reset();
	OrderStage = 0;
	bFainting = false;
	OnChanged.Broadcast();
}

const FGRDialogDef* UGRStorySubsystem::FindDialog(const FString& Id) const { return FindById(Data.Dialogs, Id); }
const FGRSpeakerDef* UGRStorySubsystem::FindSpeaker(const FString& Id) const { return FindById(Data.Speakers, Id); }
const FGRQuestDef* UGRStorySubsystem::FindQuest(const FString& Id) const { return FindById(Data.Quests, Id); }
const FGRItemDef* UGRStorySubsystem::FindItem(const FString& Id) const { return FindById(Data.Items, Id); }
const FGRSpawnDef* UGRStorySubsystem::FindSpawn(const FString& Id) const { return FindById(Data.Spawns, Id); }
const FGRCutsceneDef* UGRStorySubsystem::FindCutscene(const FString& Id) const { return FindById(Data.Cutscenes, Id); }
const FGROrderDef* UGRStorySubsystem::FindOrder(const FString& Id) const { return Id.IsEmpty() ? nullptr : FindById(Data.Orders, Id); }
const FGRMinigameDef* UGRStorySubsystem::FindMinigame(const FString& Id) const { return FindById(Data.Minigames, Id); }

int32 UGRStorySubsystem::GetVar(const FString& Name) const
{
	const int32* Value = Vars.Find(Name);
	return Value ? *Value : 0;
}

void UGRStorySubsystem::SetVar(const FString& Name, int32 Value)
{
	if (Value == 0)
	{
		Vars.Remove(Name);
	}
	else
	{
		Vars.Add(Name, Value);
	}
	OnChanged.Broadcast();
}

float UGRStorySubsystem::GetValue(const FString& Name) const
{
	const UGameInstance* GI = GetGameInstance();
	const UGRClockSubsystem* Clock = GI->GetSubsystem<UGRClockSubsystem>();
	const UGRStatusSubsystem* Status = GI->GetSubsystem<UGRStatusSubsystem>();
	if (Name == TEXT("day")) { return Clock->GetDay(); }
	if (Name == TEXT("hour")) { return Clock->GetHours(); }
	if (Name == TEXT("malam")) { return Clock->IsNight() ? 1.f : 0.f; }
	if (Name == TEXT("uang")) { return Status->GetUang(); }
	if (Name == TEXT("suara")) { return Status->GetSuara(); }
	if (Name == TEXT("diawasi")) { return Status->GetDiawasi(); }
	if (Name == TEXT("energi")) { return Energi; }
	if (Name == TEXT("order")) { return ActiveOrder.IsEmpty() ? 0.f : 1.f; }
	if (Name == TEXT("bab"))
	{
		// Chapter number, for spawns shared by every chapter (furniture.json).
		return Chapters.Chapters.IndexOfByPredicate([this](const FGRChapterDef& C) { return C.Id == ChapterId; }) + 1.f;
	}
	return GetVar(Name);
}

bool UGRStorySubsystem::CheckTerm(const FString& InTerm) const
{
	FString Term = InTerm.TrimStartAndEnd();
	if (Term.IsEmpty())
	{
		return true;
	}
	if (Term.StartsWith(TEXT("!")))
	{
		return !CheckTerm(Term.RightChop(1));
	}
	static const TCHAR* Ops[] = { TEXT(">="), TEXT("<="), TEXT("=="), TEXT("!="), TEXT(">"), TEXT("<") };
	for (const TCHAR* Op : Ops)
	{
		FString Left, Right;
		if (Term.Split(Op, &Left, &Right))
		{
			const float A = GetValue(Left.TrimStartAndEnd());
			const float B = FCString::Atof(*Right.TrimStartAndEnd());
			const FString O(Op);
			if (O == TEXT(">=")) { return A >= B; }
			if (O == TEXT("<=")) { return A <= B; }
			if (O == TEXT("==")) { return FMath::IsNearlyEqual(A, B); }
			if (O == TEXT("!=")) { return !FMath::IsNearlyEqual(A, B); }
			if (O == TEXT(">")) { return A > B; }
			return A < B;
		}
	}
	return GetValue(Term) != 0.f;
}

bool UGRStorySubsystem::Check(const FString& Condition) const
{
	if (Condition.TrimStartAndEnd().IsEmpty())
	{
		return true;
	}
	TArray<FString> Ors;
	Condition.ParseIntoArray(Ors, TEXT("||"));
	for (const FString& Or : Ors)
	{
		TArray<FString> Ands;
		Or.ParseIntoArray(Ands, TEXT("&&"));
		bool bAll = true;
		for (const FString& And : Ands)
		{
			if (!CheckTerm(And))
			{
				bAll = false;
				break;
			}
		}
		if (bAll)
		{
			return true;
		}
	}
	return false;
}

void UGRStorySubsystem::Run(const FString& Actions)
{
	TArray<FString> Commands;
	Actions.ParseIntoArray(Commands, TEXT(";"));
	for (FString& Command : Commands)
	{
		Command.TrimStartAndEndInline();
		if (Command.IsEmpty())
		{
			continue;
		}
		FString Name, Arg;
		if (!Command.Split(TEXT(" "), &Name, &Arg))
		{
			Name = Command;
		}
		RunCommand(Name.ToLower(), Arg.TrimStartAndEnd());
	}
	OnChanged.Broadcast();
}

void UGRStorySubsystem::Notify(const FString& Text, uint8 Type) const
{
	GetGameInstance()->GetSubsystem<UGRNotificationSubsystem>()->Notify(FText::FromString(Text), static_cast<EGRNotifyType>(Type));
}

void UGRStorySubsystem::RunCommand(const FString& Command, const FString& Arg)
{
	UGameInstance* GI = GetGameInstance();
	UGRStatusSubsystem* Status = GI->GetSubsystem<UGRStatusSubsystem>();
	FString First, Rest;
	if (!Arg.Split(TEXT(" "), &First, &Rest))
	{
		First = Arg;
	}
	const int32 Number = FCString::Atoi(*Rest);

	if (Command == TEXT("set")) { SetVar(First, Number); }
	else if (Command == TEXT("add")) { SetVar(First, GetVar(First) + (Rest.IsEmpty() ? 1 : Number)); }
	else if (Command == TEXT("flag")) { SetVar(First, 1); }
	else if (Command == TEXT("unflag")) { SetVar(First, 0); }
	else if (Command == TEXT("uang"))
	{
		const int32 Amount = FCString::Atoi(*First);
		Status->AddUang(Amount);
		AddLedger(Amount, Rest.IsEmpty() ? TEXT("Lain-lain") : Rest);
	}
	else if (Command == TEXT("suara")) { Status->AddSuara(FCString::Atof(*First)); }
	else if (Command == TEXT("diawasi")) { Status->AddDiawasi(FCString::Atof(*First)); }
	else if (Command == TEXT("energi")) { AddEnergi(FCString::Atof(*First)); }
	else if (Command == TEXT("item"))
	{
		const int32 Delta = Rest.IsEmpty() ? 1 : Number;
		SetVar(TEXT("item.") + First, FMath::Max(0, GetItemCount(First) + Delta));
		if (const FGRItemDef* Item = FindItem(First))
		{
			Notify(FString::Printf(TEXT("%s %s"), Delta > 0 ? TEXT("Dapat:") : TEXT("Hilang:"), *Item->Name), Delta > 0 ? 1 : 3);
		}
	}
	else if (Command == TEXT("quest"))
	{
		if (GetQuestStep(First) == 0)
		{
			SetVar(TEXT("q.") + First, 1);
			QuestOrder.Remove(First);
			QuestOrder.Insert(First, 0);
			if (const FGRQuestDef* Quest = FindQuest(First))
			{
				Notify(FString::Printf(TEXT("Tugas baru: %s"), *Quest->Title), 0);
			}
		}
	}
	else if (Command == TEXT("step"))
	{
		SetVar(TEXT("q.") + First, FMath::Max(1, Number));
		QuestOrder.Remove(First);
		QuestOrder.Insert(First, 0);
		const FGRQuestDef* Quest = FindQuest(First);
		if (Quest && Quest->Steps.IsValidIndex(Number - 1))
		{
			Notify(FormatText(Quest->Steps[Number - 1]), 0);
		}
	}
	else if (Command == TEXT("done"))
	{
		if (GetQuestStep(First) != QuestDone)
		{
			SetVar(TEXT("q.") + First, QuestDone);
			if (const FGRQuestDef* Quest = FindQuest(First))
			{
				Notify(FString::Printf(TEXT("Tugas selesai: %s"), *Quest->Title), 1);
			}
		}
	}
	else if (Command == TEXT("jika"))
	{
		// "jika <condition> ? <one action>"
		FString Condition, Action;
		if (Arg.Split(TEXT("?"), &Condition, &Action) && Check(Condition))
		{
			Action.TrimStartAndEndInline();
			FString Name, Rest2;
			if (!Action.Split(TEXT(" "), &Name, &Rest2))
			{
				Name = Action;
			}
			RunCommand(Name.ToLower(), Rest2.TrimStartAndEnd());
		}
	}
	else if (Command == TEXT("notif")) { Notify(Arg, 0); }
	else if (Command == TEXT("notifbaik")) { Notify(Arg, 1); }
	else if (Command == TEXT("notifburuk")) { Notify(Arg, 2); }
	else if (Command == TEXT("pesan"))
	{
		Inbox.Remove(First);
		Inbox.Add(First);
		SetVar(TEXT("baca.") + First, 0);
		const FGRDialogDef* Dialog = FindDialog(First);
		const FGRSpeakerDef* Speaker = Dialog && Dialog->Nodes.Num() > 0 ? FindSpeaker(Dialog->Nodes[0].Speaker) : nullptr;
		Notify(FString::Printf(TEXT("Pesan baru dari %s (Tab)"), Speaker ? *Speaker->Name : TEXT("nomor tak dikenal")), 3);
	}
	else if (Command == TEXT("umumkan")) { GI->GetSubsystem<UGRRegulationSubsystem>()->EnactById(First); }
	else if (Command == TEXT("aksi")) { GI->GetSubsystem<UGRRegulationSubsystem>()->ReportAction(FGameplayTag::RequestGameplayTag(FName(*First), false)); }
	else if (Command == TEXT("jam")) { GI->GetSubsystem<UGRClockSubsystem>()->SetHour(FCString::Atof(*First), false); }
	else if (Command == TEXT("order")) { StartOrder(First); }
	else if (Command == TEXT("batalorder")) { CancelOrder(); }
	else if (Command == TEXT("dialog") || Command == TEXT("cutscene") || Command == TEXT("minigame") || Command == TEXT("toko")
		|| Command == TEXT("teleport") || Command == TEXT("tidur") || Command == TEXT("selesai") || Command == TEXT("simpan") || Command == TEXT("ponsel"))
	{
		OnRequest.Broadcast(FName(*Command), Arg);
	}
	else
	{
		UE_LOG(LogGRStory, Warning, TEXT("Perintah cerita tidak dikenal: %s %s"), *Command, *Arg);
	}
}

TArray<const FGRQuestDef*> UGRStorySubsystem::GetActiveQuests() const
{
	TArray<const FGRQuestDef*> Result;
	for (const FString& Id : QuestOrder)
	{
		const int32 Step = GetQuestStep(Id);
		if (Step > 0 && Step != QuestDone)
		{
			if (const FGRQuestDef* Quest = FindQuest(Id))
			{
				Result.Add(Quest);
			}
		}
	}
	return Result;
}

TArray<const FGRQuestDef*> UGRStorySubsystem::GetDoneQuests() const
{
	TArray<const FGRQuestDef*> Result;
	for (const FGRQuestDef& Quest : Data.Quests)
	{
		if (GetQuestStep(Quest.Id) == QuestDone)
		{
			Result.Add(&Quest);
		}
	}
	return Result;
}

FString UGRStorySubsystem::GetObjectiveText() const
{
	if (const FGROrderDef* Order = GetActiveOrder())
	{
		const FGRSpawnDef* Target = FindSpawn(OrderStage == 1 ? Order->From : Order->To);
		return FString::Printf(TEXT("OjekKu: %s %s"), OrderStage == 1 ? TEXT("Ambil pesanan di") : TEXT("Antar ke"), Target ? *Target->Name : TEXT("?"));
	}
	const TArray<const FGRQuestDef*> Active = GetActiveQuests();
	if (Active.Num() > 0)
	{
		const int32 Step = GetQuestStep(Active[0]->Id);
		if (Active[0]->Steps.IsValidIndex(Step - 1))
		{
			return FormatText(Active[0]->Steps[Step - 1]);
		}
	}
	return FString();
}

FString UGRStorySubsystem::FormatText(const FString& Text) const
{
	FString Out;
	int32 From = 0;
	while (true)
	{
		const int32 Open = Text.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, From);
		const int32 Close = Open == INDEX_NONE ? INDEX_NONE : Text.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open);
		if (Close == INDEX_NONE)
		{
			Out += Text.Mid(From);
			return Out;
		}
		Out += Text.Mid(From, Open - From);
		Out += FString::FromInt(FMath::RoundToInt(GetValue(Text.Mid(Open + 1, Close - Open - 1))));
		From = Close + 1;
	}
}

FString UGRStorySubsystem::GetTargetSpawnId() const
{
	if (const FGROrderDef* Order = GetActiveOrder())
	{
		return OrderStage == 1 ? Order->From : Order->To;
	}
	const TArray<const FGRQuestDef*> Active = GetActiveQuests();
	if (Active.Num() > 0)
	{
		const int32 Step = GetQuestStep(Active[0]->Id);
		if (Active[0]->Targets.IsValidIndex(Step - 1))
		{
			return Active[0]->Targets[Step - 1];
		}
	}
	return FString();
}

int32 UGRStorySubsystem::GetShopPrice(const FGRShopEntry& Entry) const
{
	const int32 Day = GetGameInstance()->GetSubsystem<UGRClockSubsystem>()->GetDay();
	return Entry.Price + Entry.PriceStepPerDay * FMath::Max(0, Day - 1);
}

void UGRStorySubsystem::AddEnergi(float Delta)
{
	Energi = FMath::Clamp(Energi + Delta, 0.f, 100.f);
	if (Energi > 5.f)
	{
		bFainting = false;
	}
	OnChanged.Broadcast();
}

void UGRStorySubsystem::TickEnergy(float DeltaSeconds, bool bExerting)
{
	const float Smoke = GetGameInstance()->GetSubsystem<UGRClockSubsystem>()->GetSmoke01();
	const float Mask = GetItemCount(TEXT("masker")) > 0 ? 0.35f : 1.f;
	const float Rate = (0.035f + 0.07f * Smoke * Mask) * (bExerting ? 1.8f : 1.f);
	Energi = FMath::Max(0.f, Energi - Rate * DeltaSeconds);
	if (Energi <= 0.f && !bFainting)
	{
		bFainting = true;
		Run(Data.OnFaint);
	}
}

bool UGRStorySubsystem::IsOrderAvailable(const FGROrderDef& Order) const
{
	const int32 Day = GetGameInstance()->GetSubsystem<UGRClockSubsystem>()->GetDay();
	return ActiveOrder.IsEmpty() && GetVar(TEXT("order.") + Order.Id) != Day && Check(Order.If);
}

void UGRStorySubsystem::StartOrder(const FString& OrderId)
{
	if (const FGROrderDef* Order = FindOrder(OrderId))
	{
		ActiveOrder = OrderId;
		OrderStage = 1;
		OrderTimeLeft = Order->Time;
		const FGRSpawnDef* From = FindSpawn(Order->From);
		Notify(FString::Printf(TEXT("Order diterima: %s. Jemput di %s."), *Order->Title, From ? *From->Name : TEXT("?")), 0);
		OnChanged.Broadcast();
	}
}

void UGRStorySubsystem::PickUpOrder()
{
	if (const FGROrderDef* Order = GetActiveOrder())
	{
		OrderStage = 2;
		OrderTimeLeft = Order->Time;
		const FGRSpawnDef* To = FindSpawn(Order->To);
		Notify(FString::Printf(TEXT("Pesanan diambil. Antar ke %s dalam %d detik."), To ? *To->Name : TEXT("?"), FMath::RoundToInt(Order->Time)), 0);
		OnChanged.Broadcast();
	}
}

void UGRStorySubsystem::CompleteOrder()
{
	const FGROrderDef* Order = GetActiveOrder();
	if (!Order)
	{
		return;
	}
	const bool bLate = OrderTimeLeft <= 0.f;
	const int32 Gross = bLate ? Order->Fare / 2 : Order->Fare;
	const int32 Net = FMath::RoundToInt(Gross * 0.8f);
	const FString Actions = Order->Do;
	SetVar(TEXT("order.") + Order->Id, GetGameInstance()->GetSubsystem<UGRClockSubsystem>()->GetDay());
	SetVar(TEXT("orders_done"), GetVar(TEXT("orders_done")) + 1);
	ActiveOrder.Reset();
	OrderStage = 0;
	GetGameInstance()->GetSubsystem<UGRStatusSubsystem>()->AddUang(Net);
	AddLedger(Net, FString::Printf(TEXT("OjekKu (potongan aplikasi 20%%)%s"), bLate ? TEXT(", telat") : TEXT("")));
	Notify(bLate ? TEXT("Telat! Pelanggan memberi bintang 1. Tarif dipotong setengah.") : TEXT("Order selesai. Bintang 5!"), bLate ? 2 : 1);
	AddEnergi(-4.f);
	Run(Actions);
}

void UGRStorySubsystem::CancelOrder()
{
	if (!ActiveOrder.IsEmpty())
	{
		ActiveOrder.Reset();
		OrderStage = 0;
		Notify(TEXT("Order dibatalkan. Performa akunmu turun."), 2);
		OnChanged.Broadcast();
	}
}

void UGRStorySubsystem::TickOrder(float DeltaSeconds)
{
	if (OrderStage == 2)
	{
		OrderTimeLeft -= DeltaSeconds;
	}
}

void UGRStorySubsystem::AddLedger(int32 Amount, const FString& Label)
{
	const UGRClockSubsystem* Clock = GetGameInstance()->GetSubsystem<UGRClockSubsystem>();
	Ledger.Add(FString::Printf(TEXT("H%d %s|%s|%d"), Clock->GetDay(), *Clock->GetClockText().ToString(), *Label, Amount));
	if (Ledger.Num() > 60)
	{
		Ledger.RemoveAt(0);
	}
}

void UGRStorySubsystem::TickEvents()
{
	for (const FGREventDef& Event : Data.Events)
	{
		const FString Key = TEXT("ev.") + Event.Id;
		if (GetVar(Key) == 0 && Check(Event.If))
		{
			Vars.Add(Key, 1);
			UE_LOG(LogGRStory, Log, TEXT("Event %s"), *Event.Id);
			Run(Event.Do);
		}
	}
}

void UGRStorySubsystem::RunMorning()
{
	Energi = 100.f;
	bFainting = false;
	Run(Data.OnMorning);
	for (const FGRNeedDef& Need : Data.Needs)
	{
		if (!Check(Need.If))
		{
			continue;
		}
		if (GetItemCount(Need.Item) > 0)
		{
			SetVar(TEXT("item.") + Need.Item, GetItemCount(Need.Item) - 1);
			Run(Need.Ok);
		}
		else
		{
			Run(Need.Fail);
		}
	}
}

void UGRStorySubsystem::ResetState()
{
	if (Chapters.Chapters.Num() > 0 && ChapterId != Chapters.Chapters[0].Id)
	{
		ChapterId = Chapters.Chapters[0].Id;
		Reload();
	}
	CompletedChapters.Reset();
	Vars.Reset();
	QuestOrder.Reset();
	Inbox.Reset();
	Ledger.Reset();
	Energi = 100.f;
	ActiveOrder.Reset();
	OrderStage = 0;
	bFainting = false;
}

void UGRStorySubsystem::StartChapter()
{
	Run(Data.Start);
}

void UGRStorySubsystem::WriteToSave(UGRSaveGame& Save) const
{
	Save.StoryVars.Reset();
	for (const TPair<FString, int32>& Pair : Vars)
	{
		Save.StoryVars.Add(Pair.Key, Pair.Value);
	}
	Save.QuestOrder = QuestOrder;
	Save.Inbox = Inbox;
	Save.Ledger = Ledger;
	Save.Energi = Energi;
	Save.ChapterId = ChapterId;
	Save.CompletedChapters = CompletedChapters;
}

void UGRStorySubsystem::ReadFromSave(const UGRSaveGame& Save)
{
	ResetState();
	for (const TPair<FString, int32>& Pair : Save.StoryVars)
	{
		Vars.Add(Pair.Key, Pair.Value);
	}
	QuestOrder = Save.QuestOrder;
	Inbox = Save.Inbox;
	Ledger = Save.Ledger;
	Energi = Save.Energi;
	// Saves from before chapters existed are Bab 1.
	const FString SavedChapter = Save.ChapterId.IsEmpty() ? FString(TEXT("bab1")) : Save.ChapterId;
	if (SavedChapter != ChapterId)
	{
		ChapterId = SavedChapter;
		Reload();
	}
	CompletedChapters = Save.CompletedChapters;
	OnChanged.Broadcast();
}
