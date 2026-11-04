#include "Story/GRStoryDirector.h"
#include "World/GRStealthLevel.h"
#include "Story/GRStorySubsystem.h"
#include "Story/GRStoryUtil.h"
#include "Player/GRCharacter.h"
#include "Interaction/GRInteractableActor.h"
#include "Systems/GRSaveSubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "UI/GRStoryScreens.h"
#include "UI/GRUISubsystem.h"
#include "World/GRBed.h"
#include "World/GRMapMarkerComponent.h"
#include "World/GRNpc.h"
#include "World/GRStoryProp.h"
#include "World/GRTraffic.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogGRDirector, Log, All);

namespace
{
	constexpr float OrderReach = 450.f;
	const FVector HiddenSpot(0.f, 0.f, -100000.f);
}

AGRStoryDirector::AGRStoryDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

AGRStoryDirector* AGRStoryDirector::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<AGRStoryDirector> It(World);
	return It ? *It : nullptr;
}

UGRStorySubsystem* AGRStoryDirector::GetStory() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UGRStorySubsystem>() : nullptr;
}

void AGRStoryDirector::BeginPlay()
{
	Super::BeginPlay();
	UGRStorySubsystem* Story = GetStory();
	if (!Story)
	{
		return;
	}

	// The prototype interactables from Phase A are replaced by story props.
	for (TActorIterator<AGRInteractableActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass() == AGRInteractableActor::StaticClass())
		{
			It->Destroy();
		}
	}

	SpawnAll();
	GetWorld()->SpawnActor<AGRTraffic>(AGRTraffic::StaticClass(), FTransform::Identity);
	RequestHandle = Story->OnRequest.AddUObject(this, &AGRStoryDirector::HandleRequest);
	GetGameInstance()->GetSubsystem<UGRStatusSubsystem>()->OnDiawasiMaxed.AddDynamic(this, &AGRStoryDirector::HandleDiawasiMaxed);
	RefreshSpawns();
}

void AGRStoryDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGRStorySubsystem* Story = GetStory())
	{
		Story->OnRequest.Remove(RequestHandle);
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		GI->GetSubsystem<UGRStatusSubsystem>()->OnDiawasiMaxed.RemoveDynamic(this, &AGRStoryDirector::HandleDiawasiMaxed);
	}
	Super::EndPlay(EndPlayReason);
}

void AGRStoryDirector::SpawnAll()
{
	const UGRStorySubsystem* Story = GetStory();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const FGRSpawnDef& Def : Story->GetData().Spawns)
	{
		const bool bNpc = Def.Kind == TEXT("npc") || Def.Kind == TEXT("spn");
		const FVector Location = GRStoryUtil::ToLocation(Def.Pos, bNpc ? 100.f : 0.f);
		const FRotator Rotation(0.f, Def.Yaw, 0.f);
		AActor* Actor = nullptr;
		if (bNpc)
		{
			AGRNpc* Npc = GetWorld()->SpawnActor<AGRNpc>(AGRNpc::StaticClass(), Location, Rotation, Params);
			if (Npc)
			{
				Npc->Setup(Def);
				Actor = Npc;
			}
		}
		else
		{
			AGRStoryProp* Prop = GetWorld()->SpawnActor<AGRStoryProp>(AGRStoryProp::StaticClass(), Location, Rotation, Params);
			if (Prop)
			{
				Prop->Setup(Def);
				Actor = Prop;
			}
		}
		if (!Actor)
		{
			continue;
		}
		if (!Def.Marker.IsEmpty())
		{
			UGRMapMarkerComponent* Marker = NewObject<UGRMapMarkerComponent>(Actor);
			Marker->Label = FText::FromString(Def.Name);
			Marker->Glyph = Def.Marker;
			Marker->Color = GRStoryUtil::ParseColor(Def.MarkerColor);
			Marker->SetupAttachment(Actor->GetRootComponent());
			Marker->RegisterComponent();
		}
		Spawned.Add(Def.Id, Actor);
	}

	FActorSpawnParameters MarkerParams;
	TargetMarker = GetWorld()->SpawnActor<AGRMapMarker>(AGRMapMarker::StaticClass(), HiddenSpot, FRotator::ZeroRotator, MarkerParams);
	if (TargetMarker)
	{
		TargetMarker->Marker->Glyph = TEXT("!");
		TargetMarker->Marker->Color = FLinearColor(1.f, 0.78f, 0.15f);
		TargetMarker->Marker->Label = FText::FromString(TEXT("Tujuan"));
		TargetMarker->Marker->bInLegend = false;
		TargetMarker->Marker->bShowOnMinimap = false;
		TargetMarker->Marker->bTracked = false;
	}
	UE_LOG(LogGRDirector, Log, TEXT("Director: %d objek cerita"), Spawned.Num());
}

void AGRStoryDirector::RefreshSpawns()
{
	const UGRStorySubsystem* Story = GetStory();
	for (const FGRSpawnDef& Def : Story->GetData().Spawns)
	{
		AActor* Actor = FindSpawnedActor(Def.Id);
		if (!Actor)
		{
			continue;
		}
		const bool bActive = Story->Check(Def.If);
		if (Actor->IsHidden() != !bActive)
		{
			if (AGRNpc* Npc = Cast<AGRNpc>(Actor))
			{
				Npc->SetActive(bActive);
			}
			else if (AGRStoryProp* Prop = Cast<AGRStoryProp>(Actor))
			{
				Prop->SetActive(bActive);
			}
		}
	}
}

void AGRStoryDirector::CheckZones()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	UGRStorySubsystem* Story = GetStory();
	if (!Player || IsUIBusy())
	{
		return;
	}
	for (const TPair<FString, TObjectPtr<AActor>>& Pair : Spawned)
	{
		AGRStoryProp* Prop = Cast<AGRStoryProp>(Pair.Value);
		if (!Prop || !Prop->IsZone() || Prop->IsHidden())
		{
			continue;
		}
		const bool bInside = FVector::Dist2D(Player->GetActorLocation(), Prop->GetActorLocation()) < Prop->GetRadius();
		if (bInside && !Prop->bPlayerInside)
		{
			Prop->bPlayerInside = true;
			Story->Run(Prop->GetActions());
		}
		else if (!bInside)
		{
			Prop->bPlayerInside = false;
		}
	}
}

void AGRStoryDirector::CheckOrder()
{
	UGRStorySubsystem* Story = GetStory();
	const FGROrderDef* Order = Story->GetActiveOrder();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Order || !Player)
	{
		return;
	}
	const FString& TargetId = Story->GetOrderStage() == 1 ? Order->From : Order->To;
	if (FVector::Dist2D(Player->GetActorLocation(), GetSpawnLocation(TargetId)) < OrderReach)
	{
		if (Story->GetOrderStage() == 1)
		{
			Story->PickUpOrder();
		}
		else
		{
			Story->CompleteOrder();
		}
	}
}

void AGRStoryDirector::UpdateTargetMarker()
{
	if (!TargetMarker)
	{
		return;
	}
	const FString TargetId = GetStory()->GetTargetSpawnId();
	const bool bHasTarget = !TargetId.IsEmpty() && FindSpawnedActor(TargetId) != nullptr;
	TargetMarker->Marker->bTracked = bHasTarget;
	TargetMarker->Marker->bShowOnMinimap = bHasTarget;
	TargetMarker->SetActorLocation(bHasTarget ? GetSpawnLocation(TargetId) : HiddenSpot);
	if (bHasTarget)
	{
		if (const FGRSpawnDef* Def = GetStory()->FindSpawn(TargetId))
		{
			TargetMarker->Marker->Label = FText::FromString(Def->Name);
		}
	}
}

bool AGRStoryDirector::GetTrackedTarget(FVector& OutLocation, FText& OutLabel) const
{
	if (!TargetMarker || !TargetMarker->Marker->bTracked)
	{
		return false;
	}
	OutLocation = TargetMarker->GetActorLocation();
	OutLabel = TargetMarker->Marker->Label;
	return true;
}

AActor* AGRStoryDirector::FindSpawnedActor(const FString& SpawnId) const
{
	const TObjectPtr<AActor>* Found = Spawned.Find(SpawnId);
	return Found ? Found->Get() : nullptr;
}

FVector AGRStoryDirector::GetSpawnLocation(const FString& SpawnId) const
{
	// A hidden actor may not be where its spawn is; use the data position then.
	if (const AActor* Actor = FindSpawnedActor(SpawnId); Actor && !Actor->IsHidden())
	{
		return Actor->GetActorLocation();
	}
	const FGRSpawnDef* Def = GetStory()->FindSpawn(SpawnId);
	return Def ? GRStoryUtil::ToLocation(Def->Pos) : FVector::ZeroVector;
}

bool AGRStoryDirector::IsUIBusy() const
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	const UGRUISubsystem* UI = LP ? LP->GetSubsystem<UGRUISubsystem>() : nullptr;
	return UI && UI->HasScreens();
}

void AGRStoryDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UGRStorySubsystem* Story = GetStory();
	if (!Story)
	{
		return;
	}

	if (!bStarted)
	{
		bStarted = true;
		if (Story->GetVar(TEXT("bab_dimulai")) == 0)
		{
			Story->SetVar(TEXT("bab_dimulai"), 1);
			Story->StartChapter();
		}
	}

	// Safety net: anyone who falls out of the world wakes up at home.
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0); Player && Player->GetActorLocation().Z < -3000.f)
	{
		Teleport(TEXT("rumah"));
		Story->Run(TEXT("notifburuk Kamu terjatuh. Bayu kembali ke rumah."));
	}
	const bool bBusy = IsUIBusy();
	if (!bBusy)
	{
		const AGRCharacter* Character = Cast<AGRCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		const bool bExerting = Character && !Character->IsRiding() && Character->GetVelocity().Size2D() > 520.f;
		Story->TickEnergy(DeltaSeconds, bExerting);
		Story->TickOrder(DeltaSeconds);
	}

	SlowTimer -= DeltaSeconds;
	if (SlowTimer <= 0.f)
	{
		SlowTimer = 0.25f;
		if (!bBusy)
		{
			Story->TickEvents();
		}
		RefreshSpawns();
		CheckZones();
		CheckOrder();
		UpdateTargetMarker();
	}
	ProcessRequests();
}

void AGRStoryDirector::HandleRequest(FName Kind, const FString& Arg)
{
	if (Kind == TEXT("simpan"))
	{
		GetGameInstance()->GetSubsystem<UGRSaveSubsystem>()->Autosave();
		return;
	}
	Queue.Add({ Kind, Arg });
}

void AGRStoryDirector::ProcessRequests()
{
	while (Queue.Num() > 0 && !IsUIBusy())
	{
		const FRequest Request = Queue[0];
		Queue.RemoveAt(0);

		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		UGRUISubsystem* UI = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UGRUISubsystem>() : nullptr;
		if (!UI)
		{
			return;
		}
		UE_LOG(LogGRDirector, Log, TEXT("Permintaan cerita: %s %s"), *Request.Kind.ToString(), *Request.Arg);
		const FString& Arg = Request.Arg;
		if (Request.Kind == TEXT("dialog"))
		{
			UGRDialogScreen::Open(UI, Arg);
		}
		else if (Request.Kind == TEXT("cutscene"))
		{
			UGRCutsceneScreen::Open(UI, Arg);
		}
		else if (Request.Kind == TEXT("minigame"))
		{
			const FGRMinigameDef* Def = GetStory()->FindMinigame(Arg);
			if (Def && Def->Grid.Num() > 0)
			{
				// Played in the real room when the map has it; the board screen is the fallback.
				if (!AGRStealthLevel::Start(GetWorld(), Arg))
				{
					UGRStealthScreen::Open(UI, Arg);
				}
			}
			else
			{
				UGRWarnetScreen::Open(UI, Arg);
			}
		}
		else if (Request.Kind == TEXT("toko"))
		{
			UGRShopScreen::Open(UI, Arg);
		}
		else if (Request.Kind == TEXT("ponsel"))
		{
			UI->PushScreen<UGRPhoneScreen>();
		}
		else if (Request.Kind == TEXT("selesai"))
		{
			GetStory()->CompleteChapter();
			GetGameInstance()->GetSubsystem<UGRSaveSubsystem>()->Autosave();
			UI->PushScreen<UGRChapterEndScreen>();
		}
		else if (Request.Kind == TEXT("teleport"))
		{
			Teleport(Arg);
		}
		else if (Request.Kind == TEXT("tidur"))
		{
			ForceSleep();
		}
	}
}

void AGRStoryDirector::Teleport(const FString& SpawnId)
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	if (AGRCharacter* Character = Cast<AGRCharacter>(Player))
	{
		Character->EndKnockdown();
	}
	// The exact spot may be blocked (lamp post, NPC): try a ring of nearby spots.
	const FVector Base = GetSpawnLocation(SpawnId) + FVector(0.f, 0.f, 110.f);
	for (int32 i = 0; i < 17; ++i)
	{
		const float Radius = i == 0 ? 0.f : (i <= 8 ? 200.f : 450.f);
		const float Angle = i * PI / 4.f;
		const FVector Spot = Base + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		// Off the exact spot, face it: E then reaches the person or thing, not whatever was ahead before.
		const FRotator Facing = i == 0 ? Player->GetActorRotation() : FRotator(0.f, (Base - Spot).Rotation().Yaw, 0.f);
		if (Player->TeleportTo(Spot, Facing))
		{
			if (AController* Controller = Player->GetController(); Controller && i > 0)
			{
				Controller->SetControlRotation(FRotator(Controller->GetControlRotation().Pitch, Facing.Yaw, 0.f));
			}
			return;
		}
	}
}

void AGRStoryDirector::ForceSleep()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	TActorIterator<AGRBed> It(GetWorld());
	if (!PC || !It)
	{
		return;
	}
	if (AGRCharacter* Character = Cast<AGRCharacter>(PC->GetPawn()))
	{
		Character->EndKnockdown();
		Character->SetRiding(false);
	}
	if (APawn* Player = PC->GetPawn())
	{
		Player->TeleportTo(It->GetActorLocation() + FVector(250.f, 0.f, 100.f), Player->GetActorRotation());
	}
	It->Sleep(PC);
}

void AGRStoryDirector::HandleDiawasiMaxed()
{
	if (UGRStorySubsystem* Story = GetStory())
	{
		Story->CancelOrder();
		Story->Run(Story->GetData().OnCaught);
	}
}
