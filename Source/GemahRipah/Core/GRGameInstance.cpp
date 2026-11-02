#include "Core/GRGameInstance.h"
#include "Data/GRDataSubsystem.h"
#include "UI/SGRLoadingScreen.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MoviePlayer.h"
#include "UnrealClient.h"
#include "GemahRipah.h"

void UGRGameInstance::Init()
{
	Super::Init();

	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UGRGameInstance::BeginLoadingScreen);

#if !UE_BUILD_SHIPPING
	LoadDevScript();
#endif
}

void UGRGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
	if (DevTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DevTickerHandle);
	}
	Super::Shutdown();
}

void UGRGameInstance::BeginLoadingScreen(const FString& MapName)
{
	// The movie player does not run inside the editor (PIE).
	if (GIsEditor || IsRunningDedicatedServer() || !IsMoviePlayerEnabled())
	{
		return;
	}

	const UGRDataSubsystem* Data = GetSubsystem<UGRDataSubsystem>();
	FLoadingScreenAttributes Attributes;
	Attributes.bAutoCompleteWhenLoadingCompletes = true;
	Attributes.MinimumLoadingScreenDisplayTime = 1.5f;
	Attributes.WidgetLoadingScreen = SNew(SGRLoadingScreen).Tip(FText::FromString(Data ? Data->GetRandomTip() : FString()));
	GetMoviePlayer()->SetupLoadingScreen(Attributes);
}

void UGRGameInstance::LoadDevScript()
{
	FString ScriptPath;
	if (!FParse::Value(FCommandLine::Get(), TEXT("-GRScript="), ScriptPath))
	{
		return;
	}

	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *ScriptPath))
	{
		UE_LOG(LogGemahRipah, Error, TEXT("Dev script tidak ditemukan: %s"), *ScriptPath);
		return;
	}

	for (const FString& RawLine : Lines)
	{
		const FString Line = RawLine.TrimStartAndEnd();
		FString TimeText, Command;
		if (Line.IsEmpty() || Line.StartsWith(TEXT("#")) || !Line.Split(TEXT(" "), &TimeText, &Command))
		{
			continue;
		}
		DevSteps.Add({ FCString::Atod(*TimeText), Command.TrimStartAndEnd() });
	}
	DevSteps.Sort([](const FDevStep& A, const FDevStep& B) { return A.Time < B.Time; });

	DevScriptStart = FPlatformTime::Seconds();
	DevTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UGRGameInstance::TickDevScript));
	UE_LOG(LogGemahRipah, Display, TEXT("Dev script: %d langkah dari %s"), DevSteps.Num(), *ScriptPath);
}

bool UGRGameInstance::TickDevScript(float DeltaTime)
{
	const double Elapsed = FPlatformTime::Seconds() - DevScriptStart;
	while (DevSteps.Num() > 0 && DevSteps[0].Time <= Elapsed)
	{
		const FString Command = DevSteps[0].Command;
		DevSteps.RemoveAt(0);
		UE_LOG(LogGemahRipah, Display, TEXT("Dev script [%.1fs]: %s"), Elapsed, *Command);

		if (Command.StartsWith(TEXT("shot ")))
		{
			const FString Name = Command.RightChop(5).TrimStartAndEnd();
			const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoShots") / (Name + TEXT(".png")));
			FScreenshotRequest::RequestScreenshot(Path, true, false);
		}
		else if (Command == TEXT("quit"))
		{
			FPlatformMisc::RequestExit(false);
			return false;
		}
		else if (APlayerController* PC = GetFirstLocalPlayerController())
		{
			PC->ConsoleCommand(Command);
		}
		else if (GEngine)
		{
			GEngine->Exec(GetWorld(), *Command);
		}
	}
	return DevSteps.Num() > 0;
}
