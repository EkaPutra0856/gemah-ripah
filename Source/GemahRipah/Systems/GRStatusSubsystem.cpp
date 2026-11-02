#include "Systems/GRStatusSubsystem.h"
#include "Systems/GRSaveGame.h"

void UGRStatusSubsystem::AddUang(int32 Delta)
{
	Uang = FMath::Max(0, Uang + Delta);
	OnStatusChanged.Broadcast();
}

bool UGRStatusSubsystem::SpendUang(int32 Amount)
{
	if (Amount < 0 || Uang < Amount)
	{
		return false;
	}
	Uang -= Amount;
	OnStatusChanged.Broadcast();
	return true;
}

void UGRStatusSubsystem::AddSuara(float Delta)
{
	Suara = FMath::Clamp(Suara + Delta, 0.f, MeterMax);
	OnStatusChanged.Broadcast();
}

void UGRStatusSubsystem::AddDiawasi(float Delta)
{
	const bool bWasMaxed = Diawasi >= MeterMax;
	Diawasi = FMath::Clamp(Diawasi + Delta, 0.f, MeterMax);
	OnStatusChanged.Broadcast();

	if (!bWasMaxed && Diawasi >= MeterMax)
	{
		OnDiawasiMaxed.Broadcast();
	}
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
