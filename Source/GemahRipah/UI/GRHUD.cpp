#include "UI/GRHUD.h"
#include "Player/GRCharacter.h"
#include "Player/GRInteractionComponent.h"
#include "Systems/GRStatusSubsystem.h"
#include "Systems/GRRegulationSubsystem.h"
#include "Systems/GRRegulationData.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"

namespace
{
	// 1250000 -> "1.250.000" (Indonesian thousands separator).
	FString FormatRupiah(int32 Amount)
	{
		FString Digits = FString::FromInt(FMath::Abs(Amount));
		for (int32 Index = Digits.Len() - 3; Index > 0; Index -= 3)
		{
			Digits.InsertAt(Index, TEXT('.'));
		}
		return (Amount < 0 ? TEXT("-Rp ") : TEXT("Rp ")) + Digits;
	}
}

void AGRHUD::BeginPlay()
{
	Super::BeginPlay();

	if (UGRRegulationSubsystem* Regulations = GetGameInstance()->GetSubsystem<UGRRegulationSubsystem>())
	{
		Regulations->OnRegulationEnacted.AddDynamic(this, &AGRHUD::HandleRegulationEnacted);
		Regulations->OnRegulationViolated.AddDynamic(this, &AGRHUD::HandleRegulationViolated);
	}
}

void AGRHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The subsystem outlives this HUD across map travel.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGRRegulationSubsystem* Regulations = GI->GetSubsystem<UGRRegulationSubsystem>())
		{
			Regulations->OnRegulationEnacted.RemoveAll(this);
			Regulations->OnRegulationViolated.RemoveAll(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AGRHUD::HandleRegulationEnacted(UGRRegulationData* Regulation)
{
	if (Regulation)
	{
		ShowBanner(TEXT("PENGUMUMAN PEMERINTAH: ") + Regulation->Title.ToString(), Regulation->Announcement.ToString(), FLinearColor(1.f, 0.85f, 0.2f), 7.f);
	}
}

void AGRHUD::HandleRegulationViolated(UGRRegulationData* Regulation, FGameplayTag Action)
{
	if (Regulation)
	{
		ShowBanner(TEXT("MELANGGAR ATURAN: ") + Regulation->Title.ToString(),
			FString::Printf(TEXT("Diawasi +%.0f"), Regulation->DiawasiPenalty), FLinearColor(1.f, 0.3f, 0.25f), 4.f);
	}
}

void AGRHUD::ShowBanner(const FString& Title, const FString& Body, const FLinearColor& Color, float Duration)
{
	BannerTitle = Title;
	BannerBody = Body;
	BannerColor = Color;
	BannerEndTime = GetWorld()->GetRealTimeSeconds() + Duration;
}

void AGRHUD::DrawMeter(const FString& Label, float Value, float Max, float X, float Y, const FLinearColor& Color)
{
	const float Width = 220.f;
	const float Height = 14.f;
	DrawText(FString::Printf(TEXT("%s  %.0f"), *Label, Value), FLinearColor::White, X, Y, GEngine->GetSmallFont());
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), X, Y + 18.f, Width, Height);
	DrawRect(Color, X, Y + 18.f, Width * FMath::Clamp(Value / Max, 0.f, 1.f), Height);
}

void AGRHUD::DrawBanner()
{
	if (GetWorld()->GetRealTimeSeconds() > BannerEndTime || BannerTitle.IsEmpty())
	{
		return;
	}

	UFont* Font = GEngine->GetMediumFont();
	float TitleW = 0.f, TitleH = 0.f, BodyW = 0.f, BodyH = 0.f;
	GetTextSize(BannerTitle, TitleW, TitleH, Font);
	GetTextSize(BannerBody, BodyW, BodyH, Font);

	const float Width = FMath::Max(TitleW, BodyW) + 40.f;
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = Canvas->ClipY * 0.12f;

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.7f), X, Y, Width, TitleH + BodyH + 30.f);
	DrawText(BannerTitle, BannerColor, X + 20.f, Y + 10.f, Font);
	DrawText(BannerBody, FLinearColor::White, X + 20.f, Y + 18.f + TitleH, Font);
}

void AGRHUD::DrawHUD()
{
	Super::DrawHUD();

	UGameInstance* GI = GetGameInstance();
	if (!Canvas || !GI)
	{
		return;
	}

	if (const UGRStatusSubsystem* Status = GI->GetSubsystem<UGRStatusSubsystem>())
	{
		DrawText(FormatRupiah(Status->GetUang()), FLinearColor(0.95f, 0.95f, 0.8f), 30.f, 24.f, GEngine->GetMediumFont());
		DrawMeter(TEXT("Suara"), Status->GetSuara(), UGRStatusSubsystem::MeterMax, 30.f, 60.f, FLinearColor(0.2f, 0.75f, 1.f));
		DrawMeter(TEXT("Diawasi"), Status->GetDiawasi(), UGRStatusSubsystem::MeterMax, 30.f, 100.f, FLinearColor(1.f, 0.3f, 0.25f));
	}

	if (const UGRRegulationSubsystem* Regulations = GI->GetSubsystem<UGRRegulationSubsystem>())
	{
		float Y = 150.f;
		for (const UGRRegulationData* Regulation : Regulations->GetActiveRegulations())
		{
			DrawText(TEXT("! ") + Regulation->Title.ToString(), FLinearColor(1.f, 0.85f, 0.2f), 30.f, Y, GEngine->GetSmallFont());
			Y += 18.f;
		}
	}

	if (const AGRCharacter* GRCharacter = Cast<AGRCharacter>(GetOwningPawn()))
	{
		const FText Prompt = GRCharacter->GetInteraction()->GetFocusedPrompt();
		if (!Prompt.IsEmpty())
		{
			const FString Line = TEXT("[E]  ") + Prompt.ToString();
			float W = 0.f, H = 0.f;
			GetTextSize(Line, W, H, GEngine->GetMediumFont());
			const float X = (Canvas->ClipX - W) * 0.5f;
			const float Y = Canvas->ClipY * 0.78f;
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), X - 14.f, Y - 6.f, W + 28.f, H + 12.f);
			DrawText(Line, FLinearColor::White, X, Y, GEngine->GetMediumFont());
		}
	}

	DrawBanner();
}
