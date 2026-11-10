#include "Systems/GRStatusSubsystem.h"
#include "World/GRSfx.h"
#include "Systems/GRNotificationSubsystem.h"
#include "Systems/GRSaveGame.h"

void UGRStatusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UGRNotificationSubsystem>();
}

FString UGRStatusSubsystem::FormatRupiah(int32 Amount)
{
	FString Digits = FString::FromInt(FMath::Abs(Amount));
	for (int32 Index = Digits.Len() - 3; Index > 0; Index -= 3)
	{
		Digits.InsertAt(Index, TEXT('.'));
	}
	return (Amount < 0 ? TEXT("-Rp ") : TEXT("Rp ")) + Digits;
}

void UGRStatusSubsystem::NotifyChange(const FString& Message, bool bGood) const
{
	if (UGRNotificationSubsystem* Notifications = GetGameInstance()->GetSubsystem<UGRNotificationSubsystem>())
	{
		Notifications->Notify(FText::FromString(Message), bGood ? EGRNotifyType::Good : EGRNotifyType::Bad);
	}
}

void UGRStatusSubsystem::AddUang(int32 Delta)
{
	const int32 Old = Uang;
	Uang = FMath::Max(0, Uang + Delta);
	if (Uang != Old)
	{
		const int32 Change = Uang - Old;
		GRSfx::Play(this, EGRSfx::Koin);
		NotifyChange(FString::Printf(TEXT("Uang %s%s"), Change > 0 ? TEXT("+") : TEXT(""), *FormatRupiah(Change)), Change > 0);
	}
	OnStatusChanged.Broadcast();
}

bool UGRStatusSubsystem::SpendUang(int32 Amount)
{
	if (Amount < 0 || Uang < Amount)
	{
		return false;
	}
	AddUang(-Amount);
	return true;
}

void UGRStatusSubsystem::AddSuara(float Delta, bool bQuiet)
{
	const float Old = Suara;
	const float Gain = Delta > 0.f ? Delta * (1.f - Suara / SuaraGainEnd) : Delta;
	Suara = FMath::Clamp(Suara + Gain, 0.f, MeterMax);
	if (!FMath::IsNearlyEqual(Old, Suara))
	{
		if (!bQuiet && FMath::Abs(Suara - Old) >= 0.5f)
		{
			NotifyChange(FString::Printf(TEXT("Suara %+.0f"), Suara - Old), Suara > Old);
		}
		OnSuaraChanged.Broadcast(Old, Suara);
	}
	OnStatusChanged.Broadcast();
}

void UGRStatusSubsystem::AddDiawasi(float Delta, bool bQuiet)
{
	const float Old = Diawasi;
	Diawasi = FMath::Clamp(Diawasi + Delta, 0.f, MeterMax);
	if (!bQuiet && !FMath::IsNearlyEqual(Old, Diawasi))
	{
		NotifyChange(FString::Printf(TEXT("Diawasi %+.0f"), Diawasi - Old), Diawasi < Old);
	}
	OnStatusChanged.Broadcast();

	if (Old < MeterMax && Diawasi >= MeterMax)
	{
		OnDiawasiMaxed.Broadcast();
	}
}

void UGRStatusSubsystem::ResetState()
{
	Uang = StartingUang;
	Suara = 0.f;
	Diawasi = 0.f;
	OnStatusChanged.Broadcast();
}

void UGRStatusSubsystem::WriteToSave(UGRSaveGame& Save) const
{
	Save.Uang = Uang;
	Save.Suara = Suara;
	Save.Diawasi = Diawasi;
}

void UGRStatusSubsystem::ReadFromSave(const UGRSaveGame& Save)
{
	Uang = Save.Uang;
	Suara = Save.Suara;
	Diawasi = Save.Diawasi;
	OnStatusChanged.Broadcast();
}
