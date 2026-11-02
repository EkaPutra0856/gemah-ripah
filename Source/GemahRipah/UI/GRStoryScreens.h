#pragma once

#include "CoreMinimal.h"
#include "UI/GRScreenWidget.h"
#include "GRStoryScreens.generated.h"

class UBorder;
class UProgressBar;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class ACameraActor;
class UGRStorySubsystem;
struct FGRDialogDef;
struct FGRCutsceneDef;
struct FGRMinigameDef;

/** Conversation from bab1.json in one of three looks: "vn" (portraits), "3d" (subtitles) or "chat" (phone). */
UCLASS()
class GEMAHRIPAH_API UGRDialogScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRDialogScreen(const FObjectInitializer& ObjectInitializer);
	static bool Open(class UGRUISubsystem* UI, const FString& DialogId);

protected:
	virtual void NativeOnInitialized() override;
	virtual void BuildScreen(UOverlay* Root) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void HandleBack() override {}

private:
	void ShowNode(int32 Index);
	void GoTo(const FString& NodeId, int32 Fallback);
	void Advance();
	void Finish();
	void RefreshMeter();
	void AddBubble(class UWidget* Bubble, bool bMine);
	UGRStorySubsystem* GetStory() const;

	static FString PendingId;
	FString DialogId;
	FString Style;
	int32 NodeIndex = INDEX_NONE;
	FString FullText;
	FString LastSpeaker;
	float Reveal = 0.f;
	/** Short stop after punctuation while typing. */
	float RevealPause = 0.f;
	float HintTime = 0.f;
	bool bHasChoices = false;
	bool bFinished = false;

	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UTextBlock> BodyText;
	UPROPERTY() TObjectPtr<UTextBlock> HintText;
	UPROPERTY() TObjectPtr<UVerticalBox> ChoiceBox;
	UPROPERTY() TObjectPtr<UBorder> Portrait;
	UPROPERTY() TObjectPtr<UTextBlock> PortraitGlyph;
	UPROPERTY() TObjectPtr<UTextBlock> PortraitName;
	UPROPERTY() TObjectPtr<UProgressBar> MeterBar;
	UPROPERTY() TObjectPtr<UTextBlock> MeterText;
	UPROPERTY() TObjectPtr<UScrollBox> ChatList;
};

/** Data-driven cutscene: camera moves, captions and fades. Esc skips (actions still run). */
UCLASS()
class GEMAHRIPAH_API UGRCutsceneScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRCutsceneScreen(const FObjectInitializer& ObjectInitializer);
	static bool Open(class UGRUISubsystem* UI, const FString& CutsceneId);

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	virtual void HandleBack() override;

private:
	void StartStep(int32 Index);
	void Finish();

	static FString PendingId;
	FString CutsceneId;
	int32 StepIndex = INDEX_NONE;
	float StepTime = 0.f;
	float FadeFrom = 0.f;
	float FadeNow = 0.f;
	FVector CamFrom = FVector::ZeroVector;
	FVector CamTo = FVector::ZeroVector;
	FRotator RotFrom = FRotator::ZeroRotator;
	FRotator RotTo = FRotator::ZeroRotator;
	bool bDone = false;

	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	UPROPERTY() TObjectPtr<UBorder> FadePanel;
	UPROPERTY() TObjectPtr<UTextBlock> TitleText;
	UPROPERTY() TObjectPtr<UTextBlock> CaptionText;
	UPROPERTY() TObjectPtr<UTextBlock> SpeakerText;
};

/** Market stall or warung: prices rise every day. */
UCLASS()
class GEMAHRIPAH_API UGRShopScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	static bool Open(class UGRUISubsystem* UI, const FString& Place);

protected:
	virtual void BuildScreen(UOverlay* Root) override;

private:
	void Rebuild();

	static FString PendingPlace;
	FString Place;
	UPROPERTY() TObjectPtr<UVerticalBox> List;
	UPROPERTY() TObjectPtr<UTextBlock> MoneyText;
};

/** Journal (J): quests, items, Bayu and family. */
UCLASS()
class GEMAHRIPAH_API UGRJournalScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRJournalScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void ShowTab(int32 Tab);

	UPROPERTY() TObjectPtr<UVerticalBox> Content;
	UPROPERTY() TObjectPtr<UVerticalBox> Buttons;
};

/** Bayu's phone (Tab): OjekKu, Kicau, Pesan, Peta, Galeri, Berita, Dompet. */
UCLASS()
class GEMAHRIPAH_API UGRPhoneScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRPhoneScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual void OnShown() override;
	virtual void PlayIntro() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void HandleBack() override;

private:
	void ShowApp(const FString& App);
	void FillApp(const FString& App);
	void AddLine(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = false);
	void AddCard(const FString& Header, const FString& Body, const FLinearColor& HeaderColor);

	FString CurrentApp;
	UPROPERTY() TObjectPtr<UVerticalBox> Content;
	UPROPERTY() TObjectPtr<UTextBlock> AppTitle;
	UPROPERTY() TObjectPtr<UTextBlock> StatusBar;
};

/** Warnet minigame: build a post from a headline, evidence and tone. Reach vs. risk. */
UCLASS()
class GEMAHRIPAH_API UGRWarnetScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRWarnetScreen(const FObjectInitializer& ObjectInitializer);
	static bool Open(class UGRUISubsystem* UI, const FString& MinigameId);

protected:
	virtual void BuildScreen(UOverlay* Root) override;

private:
	void Rebuild();
	void Submit();

	static FString PendingId;
	FString MinigameId;
	int32 Headline = INDEX_NONE;
	int32 Tone = INDEX_NONE;
	TSet<FString> Evidence;
	UPROPERTY() TObjectPtr<UVerticalBox> Body;
};

/** Top-down stealth on a grid (dapur PPR): collect evidence, avoid guard sight, reach the exit. */
UCLASS()
class GEMAHRIPAH_API UGRStealthScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRStealthScreen(const FObjectInitializer& ObjectInitializer);
	static bool Open(class UGRUISubsystem* UI, const FString& MinigameId);

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual void HandleBack() override;

private:
	struct FGuard
	{
		FIntPoint Pos;
		FIntPoint Dir;
		bool bVertical = false;
	};

	void Reset();
	bool IsWall(const FIntPoint& P) const;
	bool IsCover(const FIntPoint& P) const;
	bool GuardSees(const FGuard& Guard, const FIntPoint& P) const;
	void TryMove(const FIntPoint& Delta);
	void StepGuards();
	void End(bool bWin);
	void UpdateStatus();

	static FString PendingId;
	FString MinigameId;
	TArray<FString> Grid;
	FIntPoint Player;
	FIntPoint Start;
	TArray<FGuard> Guards;
	TArray<FIntPoint> Loot;
	TArray<FIntPoint> Taken;
	float GuardTimer = 0.f;
	float Suspicion = 0.f;
	bool bOver = false;
	UPROPERTY() TObjectPtr<UTextBlock> StatusText;
	UPROPERTY() TObjectPtr<UVerticalBox> EndBox;
};

/** Caught in the real-map stealth level: try again or slip away without evidence. */
UCLASS()
class GEMAHRIPAH_API UGRStealthCaughtScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRStealthCaughtScreen(const FObjectInitializer& ObjectInitializer);
	static bool Open(class UGRUISubsystem* UI, class AGRStealthLevel* Level);

protected:
	virtual void BuildScreen(UOverlay* Root) override;
	virtual void HandleBack() override {}

private:
	static TWeakObjectPtr<AGRStealthLevel> PendingLevel;
	TWeakObjectPtr<AGRStealthLevel> Level;
};

/** End of chapter summary. */
UCLASS()
class GEMAHRIPAH_API UGRChapterEndScreen : public UGRScreenWidget
{
	GENERATED_BODY()

public:
	UGRChapterEndScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BuildScreen(UOverlay* Root) override;
};
