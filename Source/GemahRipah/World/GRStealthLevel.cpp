#include "World/GRStealthLevel.h"
#include "Camera/GRTopDownRoomVolume.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/GRCharacter.h"
#include "Story/GRStorySubsystem.h"
#include "UI/GRStoryScreens.h"
#include "UI/GRUISubsystem.h"
#include "World/GRNpc.h"
#include "World/GRStoryProp.h"

TWeakObjectPtr<AGRStealthLevel> AGRStealthLevel::Active;

namespace
{
	constexpr float SuspicionRise = 120.f;
	constexpr float SuspicionFall = 15.f;
}

AGRStealthLevel::AGRStealthLevel()
{
	PrimaryActorTick.bCanEverTick = true;
}

bool AGRStealthLevel::Start(UWorld* World, const FString& MinigameId)
{
	const UGRStorySubsystem* Story = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UGRStorySubsystem>() : nullptr;
	const FGRMinigameDef* Def = Story ? Story->FindMinigame(MinigameId) : nullptr;
	if (!Def || Def->Room.IsEmpty() || Def->Grid.Num() == 0)
	{
		return false;
	}
	if (AGRStealthLevel* Old = Active.Get())
	{
		Old->Destroy();
	}
	AGRTopDownRoomVolume* Room = nullptr;
	for (TActorIterator<AGRTopDownRoomVolume> It(World); It; ++It)
	{
		if (It->MinigameId.ToString() == Def->Room)
		{
			Room = *It;
			break;
		}
	}
	if (!Room)
	{
		return false;
	}
	AGRStealthLevel* Level = World->SpawnActor<AGRStealthLevel>(AGRStealthLevel::StaticClass(), Room->GetActorTransform());
	if (!Level)
	{
		return false;
	}
	Level->MinigameId = MinigameId;
	if (!Level->Build(Room))
	{
		Level->Destroy();
		return false;
	}
	Active = Level;
	Level->ResetRun();
	UE_LOG(LogTemp, Log, TEXT("Stealth level %s started in %s, cell %.0f"), *MinigameId, *Room->GetName(), Level->Cell);
	if (UGRStorySubsystem* MutableStory = World->GetGameInstance()->GetSubsystem<UGRStorySubsystem>())
	{
		// Actions are split on ';', so keep the help text in one notification.
		MutableStory->Run(TEXT("notif ") + Def->Help.Replace(TEXT(";"), TEXT(",")));
	}
	return true;
}

AGRStealthLevel* AGRStealthLevel::GetActive(const UWorld* World)
{
	AGRStealthLevel* Level = Active.Get();
	return Level && Level->GetWorld() == World ? Level : nullptr;
}

UGRStorySubsystem* AGRStealthLevel::GetStory() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UGRStorySubsystem>() : nullptr;
}

FVector AGRStealthLevel::CellCenter(int32 Row, int32 Col) const
{
	return FVector(Top.X - Cell * (Row + 0.5f), Top.Y + Cell * (Col + 0.5f), Top.Z);
}

bool AGRStealthLevel::Build(AGRTopDownRoomVolume* Room)
{
	UGRStorySubsystem* Story = GetStory();
	const FGRMinigameDef* Def = Story->FindMinigame(MinigameId);
	const TArray<FString>& Grid = Def->Grid;
	int32 Cols = 0;
	for (const FString& Row : Grid)
	{
		Cols = FMath::Max(Cols, Row.Len());
	}
	const int32 Rows = Grid.Num();

	// The room builder shrinks the trigger by 40 cm on each side.
	const FVector Center = Room->GetActorLocation();
	const FVector Extent = Room->Trigger->GetScaledBoxExtent();
	Cell = FMath::Min((Extent.X + 40.f) * 2.f / Rows, (Extent.Y + 40.f) * 2.f / Cols);
	float FloorZ = Center.Z - Extent.Z;
	FHitResult Hit;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(GRStealthFloor), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Center + FVector(0.f, 0.f, 300.f), Center - FVector(0.f, 0.f, 1000.f), ECC_Visibility, TraceParams))
	{
		FloorZ = Hit.ImpactPoint.Z;
	}
	Top = FVector(Center.X + Rows * Cell * 0.5f, Center.Y - Cols * Cell * 0.5f, FloorZ);
	HalfSize = FVector2D(Rows * Cell * 0.5f, Cols * Cell * 0.5f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto SpawnProp = [&](const FVector& At, const FVector& Size, const TCHAR* Color, const FString& Name, const FString& Prompt, const FString& Do) -> AGRStoryProp*
	{
		FGRSpawnDef PropDef;
		PropDef.Kind = TEXT("prop");
		PropDef.Name = Name;
		PropDef.Pos = { static_cast<float>(At.X), static_cast<float>(At.Y) };
		PropDef.Size = { static_cast<float>(Size.X), static_cast<float>(Size.Y), static_cast<float>(Size.Z) };
		PropDef.Color = Color;
		PropDef.Prompt = Prompt;
		PropDef.Do = Do;
		AGRStoryProp* Prop = GetWorld()->SpawnActor<AGRStoryProp>(AGRStoryProp::StaticClass(), At, FRotator::ZeroRotator, Params);
		if (Prop)
		{
			Prop->Setup(PropDef);
			Built.Add(Prop);
		}
		return Prop;
	};

	auto IsBlocked = [&](int32 R, int32 C)
	{
		if (!Grid.IsValidIndex(R) || C < 0 || C >= Grid[R].Len())
		{
			return true;
		}
		return Grid[R][C] == '#' || Grid[R][C] == 'C';
	};

	for (int32 R = 0; R < Rows; ++R)
	{
		for (int32 C = 0; C < Grid[R].Len(); ++C)
		{
			const TCHAR Ch = Grid[R][C];
			const FVector At = CellCenter(R, C);
			if (Ch == '#')
			{
				SpawnProp(At, FVector(Cell, Cell, 300.f), TEXT("#8A8F98"), FString(), FString(), FString());
			}
			else if (Ch == 'C')
			{
				// Shelves stand taller than a guard's eyes, so they hide whoever is behind them.
				SpawnProp(At, FVector(Cell * 0.8f, Cell * 0.8f, 200.f), TEXT("#6B4F2A"), FString(), FString(), FString());
			}
			else if (Ch == 'B')
			{
				const int32 Index = LootProps.Num();
				const FString ItemId = Def->Loot.IsValidIndex(Index) ? Def->Loot[Index] : FString();
				const FGRItemDef* Item = Story->FindItem(ItemId);
				const FString Name = Item ? Item->Name : TEXT("Bukti");
				AGRStoryProp* Loot = SpawnProp(At, FVector(80.f, 80.f, 45.f), TEXT("#F2C94C"), Name, TEXT("Ambil ") + Name,
					FString::Printf(TEXT("set sl.%d 1; notifbaik Dapat: %s"), Index, *Name));
				LootProps.Add(Loot);
				LootItems.Add(ItemId);
			}
			else if (Ch == 'P')
			{
				StartPos = At;
			}
			else if (Ch == 'X')
			{
				ExitPos = At;
				SpawnProp(At, FVector(Cell * 0.8f, Cell * 0.8f, 4.f), TEXT("#4CAF6E"), TEXT("Pintu keluar"), FString(), FString());
			}
			else if (Ch == 'G' || Ch == 'H')
			{
				// G walks along its row, H along its column, turning at walls and shelves.
				const FIntPoint Dir = Ch == 'G' ? FIntPoint(0, 1) : FIntPoint(1, 0);
				FIntPoint A(R, C);
				FIntPoint B(R, C);
				while (!IsBlocked(A.X - Dir.X, A.Y - Dir.Y)) { A -= Dir; }
				while (!IsBlocked(B.X + Dir.X, B.Y + Dir.Y)) { B += Dir; }
				const FVector PA = CellCenter(A.X, A.Y);
				const FVector PB = CellCenter(B.X, B.Y);

				FGRSpawnDef GuardDef;
				GuardDef.Id = FString::Printf(TEXT("stealth_guard_%d"), Guards.Num());
				GuardDef.Kind = TEXT("spn");
				GuardDef.Name = TEXT("Satpam Dapur");
				GuardDef.Color = TEXT("#22305A");
				GuardDef.Yaw = Ch == 'G' ? 90.f : 180.f;
				GuardDef.Patrol = { static_cast<float>(PB.X), static_cast<float>(PB.Y), static_cast<float>(PA.X), static_cast<float>(PA.Y) };
				AGRNpc* Guard = GetWorld()->SpawnActor<AGRNpc>(AGRNpc::StaticClass(), At + FVector(0.f, 0.f, 100.f), FRotator(0.f, GuardDef.Yaw, 0.f), Params);
				if (Guard)
				{
					Guard->Setup(GuardDef);
					Guard->SetStealthGuard(Cell * 3.5f, 28.f);
					Guards.Add(Guard);
					Built.Add(Guard);
				}
			}
		}
	}

	// Dim kitchen lights so the room is readable at night; the flashlights do the rest.
	for (int32 i = -1; i <= 1; i += 2)
	{
		APointLight* Lamp = GetWorld()->SpawnActor<APointLight>(APointLight::StaticClass(), FVector(Center.X + i * Rows * Cell * 0.25f, Center.Y, FloorZ + 330.f), FRotator::ZeroRotator, Params);
		if (Lamp)
		{
			UPointLightComponent* Light = Cast<UPointLightComponent>(Lamp->GetLightComponent());
			Lamp->GetLightComponent()->SetMobility(EComponentMobility::Movable);
			Light->SetIntensityUnits(ELightUnits::Candelas);
			Light->SetIntensity(35.f);
			Light->SetAttenuationRadius(Rows * Cell * 0.7f);
			Light->SetLightColor(FLinearColor(1.f, 0.78f, 0.5f));
			Light->SetCastShadows(false);
			Built.Add(Lamp);
		}
	}

	if (ExitPos.IsZero())
	{
		ExitPos = StartPos;
	}
	OutsidePos = ExitPos + (ExitPos - Center).GetSafeNormal2D() * Cell * 2.f;

	// A closer camera that follows Bayu, high enough to see the guards' flashlight cones.
	RoomVolume = Room;
	Room->GetView(SavedCameraLocation, SavedCameraRotation, bSavedFollow);
	Room->SetView(FVector(-700.f, 0.f, 1900.f), FRotator(-70.f, 0.f, 0.f), true);
	return !StartPos.IsZero() && LootProps.Num() > 0;
}

int32 AGRStealthLevel::GetTaken() const
{
	int32 Taken = 0;
	for (const AGRStoryProp* Loot : LootProps)
	{
		Taken += Loot && Loot->IsHidden() ? 1 : 0;
	}
	return Taken;
}

void AGRStealthLevel::MovePlayer(const FVector& Location, float Yaw) const
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	if (AGRCharacter* Character = Cast<AGRCharacter>(Player))
	{
		Character->SetRiding(false);
		Character->GetCharacterMovement()->StopMovementImmediately();
	}
	// TeleportTo refuses spots it thinks are blocked; the grid already keeps these cells clear.
	Player->SetActorLocationAndRotation(Location + FVector(0.f, 0.f, 100.f), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* Controller = Player->GetController())
	{
		Controller->SetControlRotation(FRotator(0.f, Yaw, 0.f));
	}
}

void AGRStealthLevel::ResetRun()
{
	UGRStorySubsystem* Story = GetStory();
	Suspicion = 0.f;
	bOver = false;
	bWasInside = false;
	for (int32 i = 0; i < LootProps.Num(); ++i)
	{
		Story->SetVar(FString::Printf(TEXT("sl.%d"), i), 0);
		if (LootProps[i])
		{
			LootProps[i]->SetActive(true);
		}
	}
	for (AGRNpc* Guard : Guards)
	{
		if (Guard)
		{
			Guard->SetActorTickEnabled(true);
			Guard->ReturnHome();
		}
	}
	MovePlayer(StartPos, 0.f);
}

void AGRStealthLevel::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UGRStorySubsystem* Story = GetStory();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (bOver || !Story || !Player)
	{
		return;
	}

	for (int32 i = 0; i < LootProps.Num(); ++i)
	{
		if (LootProps[i] && !LootProps[i]->IsHidden() && Story->GetVar(FString::Printf(TEXT("sl.%d"), i)) > 0)
		{
			LootProps[i]->SetActive(false);
		}
	}

	// While a dialog or the pause menu is up, nobody is watching.
	const ULocalPlayer* LP = GetWorld()->GetFirstLocalPlayerFromController();
	const UGRUISubsystem* UI = LP ? LP->GetSubsystem<UGRUISubsystem>() : nullptr;
	if (UI && UI->HasScreens())
	{
		return;
	}

	bool bSeen = false;
	for (const AGRNpc* Guard : Guards)
	{
		if (Guard && Guard->SeesPlayer())
		{
			bSeen = true;
			if (FVector::Dist2D(Guard->GetActorLocation(), Player->GetActorLocation()) < 130.f)
			{
				Suspicion = 100.f;
			}
		}
	}
	Suspicion = FMath::Clamp(Suspicion + (bSeen ? SuspicionRise : -SuspicionFall) * DeltaSeconds, 0.f, 100.f);
	if (Suspicion >= 100.f)
	{
		Caught();
		return;
	}

	ExitHintCooldown -= DeltaSeconds;
	const float ToExit = FVector::Dist2D(Player->GetActorLocation(), ExitPos);
	if (ToExit < Cell * 0.45f)
	{
		if (GetTaken() > 0)
		{
			Finish(true);
			return;
		}
		if (ExitHintCooldown <= 0.f)
		{
			ExitHintCooldown = 4.f;
			Story->Run(TEXT("notif Belum ada bukti yang kamu bawa. Cari kotak kuning."));
		}
	}
	// Walked out some other way without anything: just call it off.
	const FVector FromCenter = Player->GetActorLocation() - GetActorLocation();
	const bool bInside = FMath::Abs(FromCenter.X) <= HalfSize.X + 120.f && FMath::Abs(FromCenter.Y) <= HalfSize.Y + 120.f;
	bWasInside |= bInside;
	if (!bInside && bWasInside)
	{
		UE_LOG(LogTemp, Log, TEXT("Stealth level %s: player left the room"), *MinigameId);
		Destroy();
	}
}

void AGRStealthLevel::Caught()
{
	bOver = true;
	for (AGRNpc* Guard : Guards)
	{
		if (Guard)
		{
			Guard->Say(TEXT("Hei! Berhenti di situ!"), 3.f);
			Guard->GetCharacterMovement()->StopMovementImmediately();
			Guard->SetActorTickEnabled(false);
		}
	}
	const ULocalPlayer* LP = GetWorld()->GetFirstLocalPlayerFromController();
	if (UGRUISubsystem* UI = LP ? LP->GetSubsystem<UGRUISubsystem>() : nullptr)
	{
		UGRStealthCaughtScreen::Open(UI, this);
	}
}

void AGRStealthLevel::Retry()
{
	ResetRun();
}

void AGRStealthLevel::GiveUp()
{
	Finish(false);
}

void AGRStealthLevel::Finish(bool bWin)
{
	UGRStorySubsystem* Story = GetStory();
	const FGRMinigameDef* Def = Story ? Story->FindMinigame(MinigameId) : nullptr;
	FString Actions;
	if (Def && bWin)
	{
		for (int32 i = 0; i < LootProps.Num(); ++i)
		{
			if (LootProps[i] && LootProps[i]->IsHidden() && !LootItems[i].IsEmpty())
			{
				Actions += FString::Printf(TEXT("item %s 1; "), *LootItems[i]);
			}
		}
		Actions += Def->Win;
	}
	else if (Def)
	{
		Actions = Def->Lose;
	}
	bOver = true;
	MovePlayer(OutsidePos, 180.f);
	Destroy();
	if (Story)
	{
		Story->Run(Actions);
	}
}

void AGRStealthLevel::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AActor* Actor : Built)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	Built.Reset();
	if (RoomVolume)
	{
		RoomVolume->SetView(SavedCameraLocation, SavedCameraRotation, bSavedFollow);
	}
	if (UGRStorySubsystem* Story = GetStory())
	{
		for (int32 i = 0; i < LootProps.Num(); ++i)
		{
			Story->SetVar(FString::Printf(TEXT("sl.%d"), i), 0);
		}
	}
	if (Active.Get() == this)
	{
		Active.Reset();
	}
	Super::EndPlay(EndPlayReason);
}
