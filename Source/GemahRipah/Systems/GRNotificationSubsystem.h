#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GRNotificationSubsystem.generated.h"

UENUM(BlueprintType)
enum class EGRNotifyType : uint8
{
	Info,
	Good,
	Bad,
	Warning
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FGROnNotify, const FText& /*Message*/, EGRNotifyType /*Type*/);

/** Small toast messages ("Uang +Rp15.000", "Game disimpan"). The HUD listens and shows them. */
UCLASS()
class GEMAHRIPAH_API UGRNotificationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Notifikasi")
	void Notify(const FText& Message, EGRNotifyType Type = EGRNotifyType::Info)
	{
		OnNotify.Broadcast(Message, Type);
	}

	FGROnNotify OnNotify;
};
