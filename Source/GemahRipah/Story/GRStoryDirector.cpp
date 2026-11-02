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
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogGRDirector, Log, All);

namespace
{
	constexpr float OrderReach = 450.f;
	constexpr float TrafficHalfLength = 26000.f;
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
	SpawnTraffic();
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

void AGRStoryDirector::SpawnTraffic()
{
	UStaticMesh* Cube = GRStoryUtil::LoadShape(TEXT("cube"));
	const FLinearColor Colors[] = { FLinearColor(0.8f, 0.15f, 0.1f), FLinearColor(0.15f, 0.3f, 0.7f), FLinearColor(0.85f, 0.85f, 0.8f),
		FLinearColor(0.1f, 0.1f, 0.1f), FLinearColor(0.2f, 0.55f, 0.3f), FLinearColor(0.9f, 0.7f, 0.1f) };
	int32 Index = 0;
	for (int32 Road = 0; Road < 2; ++Road)
	{
		for (int32 Side = 0; Side < 2; ++Side)
		{
			for (int32 i = 0; i < 4; ++i)
			{
				FCar Car;
				Car.bAlongY = Road == 0;
				Car.Dir = Side == 0 ? 1.f : -1.f;
				// Left-hand traffic.
				Car.Lane = Car.bAlongY ? -650.f * Car.Dir : 650.f * Car.Dir;
				Car.Pos = -TrafficHalfLength + (i + Side * 0.5f) * (TrafficHalfLength * 0.5f);
				Car.Speed = FMath::FRandRange(900.f, 1300.f);
				Car.Body = NewObject<UStaticMeshComponent>(this);
				Car.Body->SetStaticMesh(Cube);
				Car.Body->SetMobility(EComponentMobility::Movable);
				Car.Body->SetupAttachment(RootComponent);
				Car.Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
				const FVector Size = (Index % 5 == 0) ? FVector(5.5f, 2.0f, 2.2f) : FVector(4.2f, 1.8f, 1.4f);
				Car.Body->SetWorldScale3D(Car.bAlongY ? FVector(Size.Y, Size.X, Size.Z) : Size);
				Car.Body->RegisterComponent();
				GRStoryUtil::ApplyColor(Car.Body, Colors[Index % UE_ARRAY_COUNT(Colors)]);
				Cars.Add(Car);
				++Index;
			}
		}
	}
}

void AGRStoryDirector::TickTraffic(float DeltaSeconds)
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector P = Player ? Player->GetActorLocation() : FVector(1e9);
	for (FCar& Car : Cars)
	{
		const float Along = Car.bAlongY ? P.Y : P.X;
		const float Across = Car.bAlongY ? P.X : P.Y;
		const float Ahead = (Along - Car.Pos) * Car.Dir;
		const bool bBlocked = FMath::Abs(Across - Car.Lane) < 400.f && Ahead > 0.f && Ahead < 900.f;
		if (!bBlocked)
		{
			Car.Pos += Car.Speed * Car.Dir * DeltaSeconds;
		}
		if (Car.Pos > TrafficHalfLength)
		{
			Car.Pos = -TrafficHalfLength;
		}
		else if (Car.Pos < -TrafficHalfLength)
		{
			Car.Pos = TrafficHalfLength;
		}
		const float Height = Car.Body->GetComponentScale().Z * 50.f;
		Car.Body->SetWorldLocation(Car.bAlongY ? FVector(Car.Lane, Car.Pos, Height) : FVector(Car.Pos, Car.Lane, Height));
	}
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

	TickTraffic(DeltaSeconds);
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
	// The exact spot may be blocked (lamp post, NPC): try a ring of nearby spots.
	const FVector Base = GetSpawnLocation(SpawnId) + FVector(0.f, 0.f, 110.f);
	for (int32 i = 0; i < 17; ++i)
	{
		const float Radius = i == 0 ? 0.f : (i <= 8 ? 200.f : 450.f);
		const float Angle = i * PI / 4.f;
		if (Player->TeleportTo(Base + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f), Player->GetActorRotation()))
		{
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
	if (APawn* Player = PC->GetPawn())
	{
		Player->TeleportTo(It->GetActorLocation() + FVector(250.f, 0.f, 100.f), Player->GetActorRotation());
	}
	if (AGRCharacter* Character = Cast<AGRCharacter>(PC->GetPawn()))
	{
		Character->SetRiding(false);
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
