#include "World/GRBed.h"
#include "Systems/GRClockSubsystem.h"
#include "Systems/GRNotificationSubsystem.h"
#include "Systems/GRSaveSubsystem.h"
#include "Data/GRDataSubsystem.h"
#include "Story/GRStorySubsystem.h"
#include "UI/GRMenuScreens.h"
#include "UI/GRUISubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

AGRBed::AGRBed()
{
	Prompt = LOCTEXT("BedPrompt", "Tidur");
}

void AGRBed::Interact_Implementation(AActor* Interactor)
{
	const APawn* Pawn = Cast<APawn>(Interactor);
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	UGameInstance* GI = GetGameInstance();
	if (!PC || !GI)
	{
		return;
	}

	if (!GI->GetSubsystem<UGRClockSubsystem>()->CanSleepNow())
	{
		const float From = GI->GetSubsystem<UGRDataSubsystem>()->GetWorldConfig().SleepFromHour;
		GI->GetSubsystem<UGRNotificationSubsystem>()->Notify(
			FText::Format(LOCTEXT("NotSleepy", "Belum waktunya tidur. Bisa tidur mulai pukul {0}.00."), FText::AsNumber(FMath::FloorToInt(From))),
			EGRNotifyType::Warning);
		return;
	}

	UGRUISubsystem* UI = PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UGRUISubsystem>() : nullptr;
	TWeakObjectPtr<AGRBed> WeakThis(this);
	TWeakObjectPtr<APlayerController> WeakPC(PC);
	UGRConfirmScreen::Ask(UI, LOCTEXT("SleepAsk", "Tidur sampai pagi?\nGame akan disimpan otomatis."), [WeakThis, WeakPC]()
	{
		if (WeakThis.IsValid() && WeakPC.IsValid())
		{
			WeakThis->Sleep(WeakPC.Get());
		}
	});
}

void AGRBed::Sleep(APlayerController* PC)
{
	PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.6f, FLinearColor::Black, false, true);
	PC->SetIgnoreMoveInput(true);
	TWeakObjectPtr<APlayerController> WeakPC(PC);
	GetWorldTimerManager().SetTimer(SleepTimer, FTimerDelegate::CreateWeakLambda(this, [this, WeakPC]()
	{
		if (WeakPC.IsValid())
		{
			WakeUp(WeakPC.Get());
		}
	}), 1.2f, false);
}

void AGRBed::WakeUp(APlayerController* PC)
{
	UGameInstance* GI = GetGameInstance();
	UGRClockSubsystem* Clock = GI->GetSubsystem<UGRClockSubsystem>();
	Clock->SleepUntilMorning();
	UGRStorySubsystem* Story = GI->GetSubsystem<UGRStorySubsystem>();
	Story->CancelOrder();
	// Whoever parked the scooter where, it is back in the yard in the morning.
	Story->SetVar(TEXT("motor_parkir"), 0);
	Story->RunMorning();
	GI->GetSubsystem<UGRSaveSubsystem>()->Autosave();
	GI->GetSubsystem<UGRNotificationSubsystem>()->Notify(
		FText::Format(LOCTEXT("NewDay", "Hari {0} dimulai. Udara: {1}"), FText::AsNumber(Clock->GetDay()), Clock->GetSmokeCategory()), EGRNotifyType::Info);
	PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 1.2f, FLinearColor::Black, false, false);
	PC->ResetIgnoreMoveInput();
}

#undef LOCTEXT_NAMESPACE
