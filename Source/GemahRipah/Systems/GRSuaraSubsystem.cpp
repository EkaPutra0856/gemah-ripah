#include "Systems/GRSuaraSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Story/GRStorySubsystem.h"
#include "Systems/GRClockSubsystem.h"
#include "Systems/GRNotificationSubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "World/GRDistrictVolume.h"
#include "World/GRMapSubsystem.h"

namespace
{
	/** What the meter loses on a day Bayu does nothing for anyone; half once Radio Gang keeps his name on the air. */
	constexpr float FadePerDay = 2.f;
	/** A smear drains this much per game hour, down to BuzzerFloor. */
	constexpr float BuzzerPerHour = 0.4f;
	constexpr float BuzzerFloor = 40.f;
	constexpr float BuzzerStart = 70.f;
	constexpr int32 BuzzerDays = 3;
	/** Days after a smear before the next one. */
	constexpr int32 BuzzerRest = 3;
	constexpr int32 HoaxCount = 4;
	/** Each district's standing is kept in tenths, since story variables are whole numbers. */
	constexpr float AreaScale = 10.f;
	/** A deed counts for more in its own district than on the town's meter: a street notices what happens on it. */
	constexpr float AreaWeight = 2.f;
	constexpr float StrongArea = 15.f;
	const TCHAR* const AreaIds[] = { TEXT("KampungGang"), TEXT("Pasar"), TEXT("Sekolah"), TEXT("Pemerintahan"), TEXT("JalanRaya") };
}

UGRSuaraSubsystem* UGRSuaraSubsystem::Get(const UObject* Context)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UGRSuaraSubsystem>() : nullptr;
}

void UGRSuaraSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UGRStatusSubsystem>();
	Collection.InitializeDependency<UGRClockSubsystem>();
	Collection.InitializeDependency<UGRStorySubsystem>();
	Collection.InitializeDependency<UGRNotificationSubsystem>();
	GetStatus()->OnSuaraChanged.AddUObject(this, &UGRSuaraSubsystem::HandleSuaraChanged);
	UGRClockSubsystem* Clock = GetGameInstance()->GetSubsystem<UGRClockSubsystem>();
	Clock->OnNewDay.AddDynamic(this, &UGRSuaraSubsystem::HandleNewDay);
	Clock->OnHourChanged.AddDynamic(this, &UGRSuaraSubsystem::HandleHour);
}

void UGRSuaraSubsystem::Deinitialize()
{
	if (UGRStatusSubsystem* Status = GetStatus())
	{
		Status->OnSuaraChanged.RemoveAll(this);
	}
	if (UGRClockSubsystem* Clock = GetGameInstance()->GetSubsystem<UGRClockSubsystem>())
	{
		Clock->OnNewDay.RemoveDynamic(this, &UGRSuaraSubsystem::HandleNewDay);
		Clock->OnHourChanged.RemoveDynamic(this, &UGRSuaraSubsystem::HandleHour);
	}
	Super::Deinitialize();
}

UGRStorySubsystem* UGRSuaraSubsystem::GetStory() const { return GetGameInstance()->GetSubsystem<UGRStorySubsystem>(); }
UGRStatusSubsystem* UGRSuaraSubsystem::GetStatus() const { return GetGameInstance()->GetSubsystem<UGRStatusSubsystem>(); }
int32 UGRSuaraSubsystem::GetDay() const { return GetGameInstance()->GetSubsystem<UGRClockSubsystem>()->GetDay(); }

void UGRSuaraSubsystem::Say(const FString& Text, uint8 Type) const
{
	GetGameInstance()->GetSubsystem<UGRNotificationSubsystem>()->Notify(FText::FromString(Text), static_cast<EGRNotifyType>(Type));
}

EGRSuaraTier UGRSuaraSubsystem::TierOf(float Suara)
{
	return Suara >= 90.f ? EGRSuaraTier::Gerakan : Suara >= 80.f ? EGRSuaraTier::Dilindungi : Suara >= 60.f ? EGRSuaraTier::Dibela
		: Suara >= 40.f ? EGRSuaraTier::Dipercaya : Suara >= 20.f ? EGRSuaraTier::Dikenal : EGRSuaraTier::Sepi;
}

const TCHAR* UGRSuaraSubsystem::TierName(EGRSuaraTier Tier)
{
	switch (Tier)
	{
	case EGRSuaraTier::Dikenal: return TEXT("Dikenal");
	case EGRSuaraTier::Dipercaya: return TEXT("Dipercaya");
	case EGRSuaraTier::Dibela: return TEXT("Dibela");
	case EGRSuaraTier::Dilindungi: return TEXT("Dilindungi");
	case EGRSuaraTier::Gerakan: return TEXT("Gerakan");
	default: return TEXT("Sepi");
	}
}

float UGRSuaraSubsystem::TierStart(EGRSuaraTier Tier)
{
	switch (Tier)
	{
	case EGRSuaraTier::Dikenal: return 20.f;
	case EGRSuaraTier::Dipercaya: return 40.f;
	case EGRSuaraTier::Dibela: return 60.f;
	case EGRSuaraTier::Dilindungi: return 80.f;
	case EGRSuaraTier::Gerakan: return 90.f;
	default: return 0.f;
	}
}

const TCHAR* UGRSuaraSubsystem::TierPerk(EGRSuaraTier Tier)
{
	switch (Tier)
	{
	case EGRSuaraTier::Dikenal: return TEXT("Warga menyapamu di jalan.");
	case EGRSuaraTier::Dipercaya: return TEXT("Diskon pedagang 10%, tip penumpang lebih sering.");
	case EGRSuaraTier::Dibela: return TEXT("Saksi insiden lebih lunak, warga memberi tahu kalau SPN mendekat.");
	case EGRSuaraTier::Dilindungi: return TEXT("Warga menghadang SPN yang mengejarmu.");
	case EGRSuaraTier::Gerakan: return TEXT("Warga berjalan bersamamu, kerusuhan tidak terpicu.");
	default: return TEXT("Belum ada yang tahu siapa kamu.");
	}
}

EGRSuaraTier UGRSuaraSubsystem::GetTier() const
{
	return TierOf(GetStatus()->GetSuara());
}

float UGRSuaraSubsystem::GetAreaSuara(FName DistrictId) const
{
	return GetStory()->GetVar(TEXT("suara.kawasan.") + DistrictId.ToString()) / AreaScale;
}

int32 UGRSuaraSubsystem::GetStrongAreas() const
{
	int32 Count = 0;
	for (const TCHAR* Id : AreaIds)
	{
		Count += GetAreaSuara(Id) >= StrongArea ? 1 : 0;
	}
	return Count;
}

void UGRSuaraSubsystem::CreditArea(float Delta)
{
	const UWorld* World = GetGameInstance()->GetWorld();
	const UGRMapSubsystem* Map = World ? World->GetSubsystem<UGRMapSubsystem>() : nullptr;
	const AGRDistrictVolume* District = Map ? Map->GetCurrentDistrict() : nullptr;
	if (District)
	{
		const FString Key = TEXT("suara.kawasan.") + District->DistrictId.ToString();
		const int32 Tenths = GetStory()->GetVar(Key) + FMath::RoundToInt(Delta * AreaWeight * AreaScale);
		GetStory()->SetVar(Key, FMath::Clamp(Tenths, 0, FMath::RoundToInt(100.f * AreaScale)));
	}
}

void UGRSuaraSubsystem::HandleSuaraChanged(float Old, float New)
{
	CreditArea(New - Old);
	if (New > Old)
	{
		GetStory()->SetVar(TEXT("suara.aksi_hari"), GetDay());
	}
	const EGRSuaraTier Before = TierOf(Old);
	const EGRSuaraTier After = TierOf(New);
	if (After > Before)
	{
		Say(FString::Printf(TEXT("Tingkat Suara: %s. %s"), TierName(After), TierPerk(After)), 1);
	}
	else if (After < Before)
	{
		Say(FString::Printf(TEXT("Tingkat Suara turun: %s."), TierName(After)), 2);
	}
}

void UGRSuaraSubsystem::Earn(float Amount, const TCHAR* Deed, int32 DailyLimit)
{
	if (Deed && DailyLimit > 0)
	{
		// One number holds both the day and how many times today.
		const FString Key = FString(TEXT("suara.h.")) + Deed;
		const int32 Stored = GetStory()->GetVar(Key);
		const int32 Today = Stored / 100 == GetDay() ? Stored % 100 : 0;
		if (Today >= DailyLimit)
		{
			return;
		}
		GetStory()->SetVar(Key, GetDay() * 100 + Today + 1);
	}
	GetStatus()->AddSuara(Amount, true);
}

void UGRSuaraSubsystem::Lose(float Amount)
{
	GetStatus()->AddSuara(-Amount, true);
}

void UGRSuaraSubsystem::NoteCaught()
{
	if (HasTier(EGRSuaraTier::Dipercaya))
	{
		GetStatus()->AddSuara(5.f, true);
		Say(TEXT("Warga geram kamu ditangkap. Suara +5."), 1);
	}
	else
	{
		GetStatus()->AddSuara(-5.f, true);
		Say(TEXT("Warga makin takut berurusan denganmu. Suara -5."), 2);
	}
}

int32 UGRSuaraSubsystem::PriceFor(int32 Price) const
{
	return HasTier(EGRSuaraTier::Dipercaya) ? FMath::RoundToInt(Price * 0.009f) * 100 : Price;
}

float UGRSuaraSubsystem::TipChance(int32 Stars) const
{
	const bool bTrusted = HasTier(EGRSuaraTier::Dipercaya);
	return Stars == 5 ? (bTrusted ? 0.85f : 0.6f) : Stars == 4 && bTrusted ? 0.3f : 0.f;
}

float UGRSuaraSubsystem::IncidentScale() const
{
	return HasTier(EGRSuaraTier::Dibela) ? 0.7f : 1.f;
}

int32 UGRSuaraSubsystem::SpendCost(EGRSuaraSpend Spend)
{
	return Spend == EGRSuaraSpend::PanggilSaksi ? 15 : 10;
}

int32 UGRSuaraSubsystem::FundAmount() const
{
	return FMath::RoundToInt(GetStatus()->GetSuara()) * 500;
}

FString UGRSuaraSubsystem::WhyNot(EGRSuaraSpend Spend) const
{
	const float Suara = GetStatus()->GetSuara();
	const float Need = Spend == EGRSuaraSpend::RumahAman ? 20.f : Spend == EGRSuaraSpend::GalangDana ? 30.f : 40.f;
	if (Suara < Need)
	{
		return FString::Printf(TEXT("Butuh Suara %.0f"), Need);
	}
	if (Spend == EGRSuaraSpend::PanggilSaksi)
	{
		return HasWitness() ? TEXT("Saksi sudah bersiaga") : TEXT("");
	}
	if (Spend == EGRSuaraSpend::RumahAman && GetStatus()->GetDiawasi() < 1.f)
	{
		return TEXT("Belum diawasi");
	}
	const TCHAR* Key = Spend == EGRSuaraSpend::RumahAman ? TEXT("suara.beli.aman") : TEXT("suara.beli.dana");
	return GetStory()->GetVar(Key) == GetDay() ? TEXT("Sudah dipakai hari ini") : TEXT("");
}

bool UGRSuaraSubsystem::Buy(EGRSuaraSpend Spend)
{
	if (!WhyNot(Spend).IsEmpty())
	{
		return false;
	}
	UGRStorySubsystem* Story = GetStory();
	UGRStatusSubsystem* Status = GetStatus();
	switch (Spend)
	{
	case EGRSuaraSpend::RumahAman:
		Story->SetVar(TEXT("suara.beli.aman"), GetDay());
		Status->AddDiawasi(-20.f, true);
		Say(TEXT("Warga menyembunyikanmu semalam. Diawasi -20, Suara -10."), 1);
		break;
	case EGRSuaraSpend::PanggilSaksi:
		Story->SetVar(TEXT("suara.saksi"), 1);
		Say(TEXT("Warga bersiaga. Kalau SPN menangkapmu, mereka datang merekam. Suara -15."), 1);
		break;
	default:
	{
		const int32 Fund = FundAmount();
		Story->SetVar(TEXT("suara.beli.dana"), GetDay());
		Story->Run(FString::Printf(TEXT("uang %d Patungan warga"), Fund));
		Say(TEXT("Warga patungan untukmu. Suara -10."), 1);
		break;
	}
	}
	Status->AddSuara(-SpendCost(Spend), true);
	return true;
}

bool UGRSuaraSubsystem::HasWitness() const
{
	return GetStory()->GetVar(TEXT("suara.saksi")) > 0;
}

bool UGRSuaraSubsystem::UseWitness()
{
	if (!HasWitness())
	{
		return false;
	}
	GetStory()->SetVar(TEXT("suara.saksi"), 0);
	GetStatus()->AddDiawasi(-30.f, true);
	Say(TEXT("Warga datang merekam! SPN melepasmu. Diawasi -30."), 1);
	return true;
}

bool UGRSuaraSubsystem::IsBuzzerOn() const
{
	return GetStory()->GetVar(TEXT("buzzer.aktif")) > 0;
}

FString UGRSuaraSubsystem::GetBuzzerText() const
{
	const FString Id = FString::Printf(TEXT("hoaks%d"), GetStory()->GetVar(TEXT("buzzer.aktif")));
	const FGRPostDef* Post = GetStory()->GetData().KataWarga.FindByPredicate([&Id](const FGRPostDef& Entry) { return Entry.Id == Id; });
	return Post ? Post->Text : FString();
}

void UGRSuaraSubsystem::StartBuzzer()
{
	UGRStorySubsystem* Story = GetStory();
	const int32 Times = Story->GetVar(TEXT("buzzer.kali")) + 1;
	Story->SetVar(TEXT("buzzer.kali"), Times);
	Story->SetVar(TEXT("buzzer.aktif"), (Times - 1) % HoaxCount + 1);
	Story->SetVar(TEXT("buzzer.hari"), GetDay());
	Say(TEXT("Akun tanpa wajah menyebar isu tentangmu. Suara terus bocor. Bantah di Kata Warga."), 2);
	GetStatus()->OnStatusChanged.Broadcast();
}

void UGRSuaraSubsystem::EndBuzzer()
{
	GetStory()->SetVar(TEXT("buzzer.aktif"), 0);
	GetStory()->SetVar(TEXT("buzzer.selesai"), GetDay());
	GetStatus()->OnStatusChanged.Broadcast();
}

FString UGRSuaraSubsystem::WhyNotRefute() const
{
	const UGRStorySubsystem* Story = GetStory();
	if (GetKesaksian() > 0)
	{
		return FString();
	}
	for (const FGRItemDef& Item : Story->GetData().Items)
	{
		if (Item.Bukti && Story->GetItemCount(Item.Id) > 0)
		{
			return FString();
		}
	}
	return TEXT("Butuh bukti atau kesaksian warga");
}

bool UGRSuaraSubsystem::Refute()
{
	if (!IsBuzzerOn() || !WhyNotRefute().IsEmpty())
	{
		return false;
	}
	UGRStorySubsystem* Story = GetStory();
	// A testimony is spent; evidence stays, the story still needs it.
	if (GetKesaksian() > 0)
	{
		Story->SetVar(TEXT("suara.kesaksian"), GetKesaksian() - 1);
	}
	EndBuzzer();
	GetStatus()->AddSuara(3.f, true);
	GetStatus()->AddDiawasi(5.f, true);
	Say(TEXT("Isu terbantah. Warga berbalik menertawakan akun itu. Diawasi +5."), 1);
	return true;
}

int32 UGRSuaraSubsystem::GetKesaksian() const
{
	return GetStory()->GetVar(TEXT("suara.kesaksian"));
}

TArray<const FGRQuestDef*> UGRSuaraSubsystem::GetAduanList() const
{
	TArray<const FGRQuestDef*> List;
	for (const FGRQuestDef& Quest : GetStory()->GetData().Quests)
	{
		if (Quest.Id.StartsWith(TEXT("Aduan")))
		{
			List.Add(&Quest);
		}
	}
	return List;
}

const FGRQuestDef* UGRSuaraSubsystem::GetTodayAduan() const
{
	const TArray<const FGRQuestDef*> List = GetAduanList();
	const int32 Index = GetStory()->GetVar(TEXT("aduan.hari")) - 1;
	return List.IsValidIndex(Index) ? List[Index] : nullptr;
}

void UGRSuaraSubsystem::RollAduan()
{
	UGRStorySubsystem* Story = GetStory();
	const TArray<const FGRQuestDef*> List = GetAduanList();
	// One in hand stays until Bayu finishes it.
	if (const FGRQuestDef* Today = GetTodayAduan())
	{
		const int32 Step = Story->GetQuestStep(Today->Id);
		if (Step > 0 && Step != UGRStorySubsystem::QuestDone)
		{
			return;
		}
	}
	const int32 Count = List.Num();
	const int32 Start = (GetDay() + Story->GetValueOf(TEXT("bab"))) % FMath::Max(Count, 1);
	for (int32 i = 0; i < Count; ++i)
	{
		const int32 Index = (Start + i) % Count;
		if (Story->GetQuestStep(List[Index]->Id) == 0)
		{
			Story->SetVar(TEXT("aduan.hari"), Index + 1);
			return;
		}
	}
	Story->SetVar(TEXT("aduan.hari"), 0);
}

void UGRSuaraSubsystem::ForceAduan(int32 Number)
{
	GetStory()->SetVar(TEXT("aduan.hari"), Number);
}

void UGRSuaraSubsystem::OnChapterStart()
{
	UGRStorySubsystem* Story = GetStory();
	for (const FGRQuestDef* Quest : GetAduanList())
	{
		Story->SetVar(TEXT("q.") + Quest->Id, 0);
	}
	Story->SetVar(TEXT("aduan.hari"), 0);
	Story->SetVar(TEXT("buzzer.aktif"), 0);
	Story->SetVar(TEXT("buzzer.hari"), 0);
	Story->SetVar(TEXT("buzzer.selesai"), 0);
	Story->SetVar(TEXT("suara.aksi_hari"), GetDay());
	Story->SetVar(TEXT("suara.beli.aman"), 0);
	Story->SetVar(TEXT("suara.beli.dana"), 0);
	RollAduan();
}

void UGRSuaraSubsystem::HandleNewDay(int32 Day)
{
	UGRStorySubsystem* Story = GetStory();
	UGRStatusSubsystem* Status = GetStatus();
	const int32 LastDeed = Story->GetVar(TEXT("suara.aksi_hari"));
	if (LastDeed < Day - 1 && Status->GetSuara() > 0.f)
	{
		const float Fade = Story->GetVar(TEXT("radio_gang")) > 0 ? FadePerDay * 0.5f : FadePerDay;
		Status->AddSuara(-Fade, true);
		Say(FString::Printf(TEXT("Kemarin kamu tidak berbuat apa-apa untuk warga. Suara -%.0f."), Fade), 2);
	}
	if (IsBuzzerOn())
	{
		if (Day - Story->GetVar(TEXT("buzzer.hari")) >= BuzzerDays)
		{
			EndBuzzer();
			Say(TEXT("Isu tentangmu basi dengan sendirinya. Warga sudah pindah ke gosip lain."), 0);
		}
	}
	else if (Status->GetSuara() >= BuzzerStart && Day - Story->GetVar(TEXT("buzzer.selesai")) >= BuzzerRest)
	{
		StartBuzzer();
	}
	RollAduan();
}

void UGRSuaraSubsystem::HandleHour(int32 Hour)
{
	UGRStatusSubsystem* Status = GetStatus();
	if (IsBuzzerOn() && Status->GetSuara() > BuzzerFloor)
	{
		Status->AddSuara(-BuzzerPerHour, true);
	}
}

TArray<const FGRPostDef*> UGRSuaraSubsystem::GetFeed() const
{
	const UGRStorySubsystem* Story = GetStory();
	TArray<const FGRPostDef*> Matching;
	for (const FGRPostDef& Post : Story->GetData().KataWarga)
	{
		if (Story->Check(Post.If))
		{
			Matching.Add(&Post);
		}
	}
	// Smears stay on top; the rest take turns from day to day.
	TArray<const FGRPostDef*> Feed;
	for (const FGRPostDef* Post : Matching)
	{
		if (Post->Official)
		{
			Feed.Add(Post);
		}
	}
	const int32 Count = Matching.Num();
	for (int32 i = 0; i < Count && Feed.Num() < 8; ++i)
	{
		const FGRPostDef* Post = Matching[(i + GetDay() * 3) % Count];
		if (!Post->Official)
		{
			Feed.Add(Post);
		}
	}
	return Feed;
}
