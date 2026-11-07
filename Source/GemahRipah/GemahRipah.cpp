#include "GemahRipah.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GameDelegates.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#endif

/** Adds the assets the game loads by path to a cook and drops what it cannot reach. Tools/make_cook_list.py writes the list. */
class FGemahRipahModule : public FDefaultGameModuleImpl
{
#if WITH_EDITOR
public:
	virtual void StartupModule() override
	{
		CookHandle = FGameDelegates::Get().GetModifyCookDelegate().AddLambda(
			[](TConstArrayView<const ITargetPlatform*>, TArray<FName>& PackagesToCook, TArray<FName>& PackagesToNeverCook)
			{
				TArray<FString> Lines;
				if (!FFileHelper::LoadFileToStringArray(Lines, *(FPaths::ProjectConfigDir() / TEXT("CookExtra.txt"))))
				{
					UE_LOG(LogGemahRipah, Error, TEXT("Config/CookExtra.txt is missing, the package would lack assets loaded by path."));
					return;
				}
				for (const FString& Line : Lines)
				{
					// A leading minus keeps a package out, for what only a soft reference would drag in.
					if (Line.StartsWith(TEXT("-")))
					{
						PackagesToNeverCook.AddUnique(FName(*Line.RightChop(1)));
					}
					else if (!Line.IsEmpty())
					{
						PackagesToCook.AddUnique(FName(*Line));
					}
				}
				UE_LOG(LogGemahRipah, Display, TEXT("Cook: added %d assets loaded by path."), Lines.Num());
			});
	}

	virtual void ShutdownModule() override
	{
		FGameDelegates::Get().GetModifyCookDelegate().Remove(CookHandle);
	}

private:
	FDelegateHandle CookHandle;
#endif
};

IMPLEMENT_PRIMARY_GAME_MODULE(FGemahRipahModule, GemahRipah, "GemahRipah");

DEFINE_LOG_CATEGORY(LogGemahRipah);
