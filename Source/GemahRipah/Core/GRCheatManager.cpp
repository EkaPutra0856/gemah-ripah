#include "Core/GRCheatManager.h"
#include "Core/GRDevBot.h"
#include "UI/GRTrailerScreen.h"
#include "UI/GRUISubsystem.h"
#include "World/GRMusic.h"
#include "World/GRSfx.h"
#include "World/GRVfx.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Core/GRPlayerController.h"
#include "Player/GRCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Systems/GRStatusSubsystem.h"
#include "Systems/GRSuaraSubsystem.h"
#include "Systems/GRRegulationSubsystem.h"
#include "Systems/GRRegulationData.h"
#include "Systems/GRProgressSubsystem.h"
#include "Systems/GRSaveSubsystem.h"
#include "Systems/GRGameFlowSubsystem.h"
#include "Systems/GRNotificationSubsystem.h"
#include "Data/GRDataSubsystem.h"
#include "UI/GRMenuScreens.h"
#include "UI/GRSaveLoadScreen.h"
#include "UI/GRSettingsScreen.h"
#include "UI/GRMapScreen.h"
#include "UI/GRStoryScreens.h"
#include "UI/GRUISubsystem.h"
#include "UI/GRJajanScreen.h"
#include "Engine/LocalPlayer.h"
#include "Systems/GRClockSubsystem.h"
#include "World/GRBed.h"
#include "World/GRMapBounds.h"
#include "World/GRMapSubsystem.h"
#include "World/GRNpc.h"
#include "World/GRCrowd.h"
#include "World/GRTraffic.h"
#include "World/GRStoryProp.h"
#include "Story/GRStoryTypes.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Story/GRStorySubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GemahRipah.h"
#include "Framework/Application/SlateApplication.h"
#include "Containers/Ticker.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "LandscapeProxy.h"
#include "World/GROjol.h"

UGameInstance* UGRCheatManager::GetGI() const
{
	const APlayerController* PC = GetOuterAPlayerController();
	return PC ? PC->GetGameInstance() : nullptr;
}

void UGRCheatManager::Print(const FString& Message) const
{
	UE_LOG(LogGemahRipah, Display, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Cyan, Message);
	}
}

void UGRCheatManager::GRStatus()
{
	UGameInstance* GI = GetGI();
	if (!GI)
	{
		return;
	}

	const UGRStatusSubsystem* Status = GI->GetSubsystem<UGRStatusSubsystem>();
	const UGRProgressSubsystem* Progress = GI->GetSubsystem<UGRProgressSubsystem>();
	const UGRRegulationSubsystem* Regulations = GI->GetSubsystem<UGRRegulationSubsystem>();

	FString Rules;
	for (const UGRRegulationData* Regulation : Regulations->GetActiveRegulations())
	{
		Rules += Regulation->Title.ToString() + TEXT("; ");
	}

	Print(FString::Printf(TEXT("Uang=%d Suara=%.1f Diawasi=%.0f | Kota=%s | Aturan: %s | Flag: %s"),
		Status->GetUang(), Status->GetSuara(), Status->GetDiawasi(),
		*Progress->GetCurrentCityId().ToString(),
		Rules.IsEmpty() ? TEXT("-") : *Rules,
		*Progress->GetStoryFlags().ToStringSimple()));
}

void UGRCheatManager::GRUang(int32 Delta)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRStatusSubsystem>()->AddUang(Delta); }
}

void UGRCheatManager::GRSuara(float Delta)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRStatusSubsystem>()->AddSuara(Delta); }
}

void UGRCheatManager::GRBuzzer()
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRSuaraSubsystem>()->StartBuzzer(); }
}

void UGRCheatManager::GRBeliSuara(int32 Kind)
{
	if (UGameInstance* GI = GetGI())
	{
		UGRSuaraSubsystem* Suara = GI->GetSubsystem<UGRSuaraSubsystem>();
		const EGRSuaraSpend Spend = static_cast<EGRSuaraSpend>(FMath::Clamp(Kind, 0, 2));
		const FString Why = Suara->WhyNot(Spend);
		Print(Suara->Buy(Spend) ? TEXT("Terbeli.") : Why);
	}
}

void UGRCheatManager::GRBantah()
{
	if (UGameInstance* GI = GetGI())
	{
		UGRSuaraSubsystem* Suara = GI->GetSubsystem<UGRSuaraSubsystem>();
		const FString Why = Suara->WhyNotRefute();
		Print(Suara->Refute() ? TEXT("Terbantah.") : Why);
	}
}

void UGRCheatManager::GRAduan(int32 Number)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRSuaraSubsystem>()->ForceAduan(Number); }
}

void UGRCheatManager::GRDiawasi(float Delta)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRStatusSubsystem>()->AddDiawasi(Delta); }
}

void UGRCheatManager::GRUmumkanLarangan()
{
	GRUmumkan(TEXT("Aturan.LaranganBerkumpul"));
}

void UGRCheatManager::GRUmumkan(const FString& RegulationId)
{
	UGameInstance* GI = GetGI();
	if (GI && !GI->GetSubsystem<UGRRegulationSubsystem>()->EnactById(RegulationId))
	{
		Print(FString::Printf(TEXT("Aturan tidak ada di regulations.json: %s"), *RegulationId));
	}
}

void UGRCheatManager::GRCabutSemuaAturan()
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRRegulationSubsystem>()->RevokeAll(); }
}

void UGRCheatManager::GRAksi(const FString& ActionTag)
{
	UGameInstance* GI = GetGI();
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*ActionTag), false);
	if (!GI || !Tag.IsValid())
	{
		Print(FString::Printf(TEXT("Tag tidak dikenal: %s"), *ActionTag));
		return;
	}
	const bool bBroke = GI->GetSubsystem<UGRRegulationSubsystem>()->ReportAction(Tag);
	Print(bBroke ? TEXT("Aksi melanggar aturan.") : TEXT("Aksi aman."));
}

void UGRCheatManager::GRFlag(const FString& FlagTag)
{
	UGameInstance* GI = GetGI();
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*FlagTag), false);
	if (!GI || !Tag.IsValid())
	{
		Print(FString::Printf(TEXT("Tag tidak dikenal: %s"), *FlagTag));
		return;
	}
	GI->GetSubsystem<UGRProgressSubsystem>()->AddStoryFlag(Tag);
}

void UGRCheatManager::GRBukaKota(FName CityId)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRProgressSubsystem>()->UnlockCity(CityId); }
}

void UGRCheatManager::GRKota(FName CityId)
{
	if (UGameInstance* GI = GetGI())
	{
		if (!GI->GetSubsystem<UGRProgressSubsystem>()->TravelToCity(CityId))
		{
			Print(FString::Printf(TEXT("Tidak bisa pindah ke %s (tidak ada atau terkunci)."), *CityId.ToString()));
		}
	}
}

FString UGRCheatManager::SlotOrDefault(const FString& SlotName)
{
	return SlotName.IsEmpty() ? UGRSaveSubsystem::GetManualSlots()[0] : SlotName;
}

void UGRCheatManager::GRSimpan(const FString& SlotName)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRSaveSubsystem>()->SaveToSlot(SlotOrDefault(SlotName)); }
}

void UGRCheatManager::GRMuat(const FString& SlotName)
{
	if (UGameInstance* GI = GetGI())
	{
		const bool bOk = GI->GetSubsystem<UGRGameFlowSubsystem>()->LoadGame(SlotOrDefault(SlotName));
		Print(bOk ? TEXT("Save dimuat.") : TEXT("Save tidak ada."));
	}
}

void UGRCheatManager::GRUI(const FString& Screen)
{
	const APlayerController* PC = GetOuterAPlayerController();
	UGRUISubsystem* UI = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UGRUISubsystem>() : nullptr;
	if (!UI)
	{
		return;
	}
	const FString Name = Screen.ToLower();
	if (Name == TEXT("menu")) { UI->PushScreen<UGRMainMenuScreen>(); }
	else if (Name == TEXT("jeda")) { UI->PushScreen<UGRPauseScreen>(); }
	else if (Name == TEXT("simpan")) { UI->PushScreen<UGRSaveScreen>(); }
	else if (Name == TEXT("muat")) { UI->PushScreen<UGRLoadScreen>(); }
	else if (Name == TEXT("pengaturan")) { UI->PushScreen<UGRSettingsScreen>(); }
	else if (Name == TEXT("kredit")) { UI->PushScreen<UGRCreditsScreen>(); }
	else if (Name == TEXT("peta")) { UI->PushScreen<UGRMapScreen>(); }
	else if (Name == TEXT("tutup")) { UI->PopAll(); }
	else { Print(TEXT("Layar: menu, jeda, simpan, muat, pengaturan, kredit, tutup")); }
}

void UGRCheatManager::GRReloadData()
{
	if (UGameInstance* GI = GetGI())
	{
		GI->GetSubsystem<UGRDataSubsystem>()->ReloadAll();
		GI->GetSubsystem<UGRStorySubsystem>()->Reload();
		Print(TEXT("Data JSON dimuat ulang."));
	}
}

void UGRCheatManager::GRNewGame()
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRGameFlowSubsystem>()->StartNewGame(); }
}

void UGRCheatManager::GRMenu()
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRGameFlowSubsystem>()->ReturnToMainMenu(); }
}

void UGRCheatManager::GRBab(const FString& ChapterId)
{
	UGameInstance* GI = GetGI();
	if (!GI)
	{
		return;
	}
	UGRStorySubsystem* Story = GI->GetSubsystem<UGRStorySubsystem>();
	for (const FGRChapterDef& Chapter : Story->GetChapters())
	{
		if (Chapter.Id == ChapterId)
		{
			break;
		}
		Story->SetChapterDone(Chapter.Id);
	}
	if (!GI->GetSubsystem<UGRGameFlowSubsystem>()->StartChapter(ChapterId))
	{
		Print(FString::Printf(TEXT("Bab tidak bisa dimulai: %s"), *ChapterId));
	}
}

void UGRCheatManager::GRNotif(const FString& Message)
{
	if (UGameInstance* GI = GetGI())
	{
		GI->GetSubsystem<UGRNotificationSubsystem>()->Notify(FText::FromString(Message.IsEmpty() ? TEXT("Tes notifikasi") : Message));
	}
}

void UGRCheatManager::GRJam(float Hour)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRClockSubsystem>()->SetHour(Hour); }
}

void UGRCheatManager::GRAsap(float Ispu)
{
	if (UGameInstance* GI = GetGI()) { GI->GetSubsystem<UGRClockSubsystem>()->SetIspuOverride(Ispu); }
}

void UGRCheatManager::GRTidur()
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC)
	{
		return;
	}
	// A temporary bed does the fade, time jump and autosave.
	AGRBed* Bed = PC->GetWorld()->SpawnActor<AGRBed>(FVector(0.f, 0.f, -100000.f), FRotator::ZeroRotator);
	Bed->Sleep(PC);
	Bed->SetLifeSpan(5.f);
}

void UGRCheatManager::GRMotor()
{
	// Hands the scooter over wherever Bayu stands (the F key needs him next to it), or gets off it.
	APlayerController* PC = GetOuterAPlayerController();
	AGRCharacter* Character = PC ? Cast<AGRCharacter>(PC->GetPawn()) : nullptr;
	UGRStorySubsystem* Story = PC ? PC->GetGameInstance()->GetSubsystem<UGRStorySubsystem>() : nullptr;
	if (!Character || !Story)
	{
		return;
	}
	if (Story->GetVar(TEXT("punya_motor")) == 0)
	{
		Story->SetVar(TEXT("punya_motor"), 1);
	}
	Character->SetRiding(!Character->IsRiding());
}

void UGRCheatManager::GROjol()
{
	APlayerController* PC = GetOuterAPlayerController();
	UGRStorySubsystem* Story = PC ? PC->GetGameInstance()->GetSubsystem<UGRStorySubsystem>() : nullptr;
	UGROjolSubsystem* Ojol = UGROjolSubsystem::Get(PC);
	if (!Story || !Ojol)
	{
		return;
	}
	if (Story->GetVar(TEXT("punya_motor")) == 0)
	{
		Story->SetVar(TEXT("punya_motor"), 1);
	}
	Ojol->SetOnline(true);
	Ojol->ForceOffer();
}

void UGRCheatManager::GROjolLompat()
{
	if (UGROjolSubsystem* Ojol = UGROjolSubsystem::Get(GetOuterAPlayerController()))
	{
		Ojol->CheatJumpToTarget();
	}
}

void UGRCheatManager::GRTrailer(int32 Start)
{
	const APlayerController* PC = GetOuterAPlayerController();
	const ULocalPlayer* Local = PC ? PC->GetLocalPlayer() : nullptr;
	UGRTrailerScreen::Open(Local ? Local->GetSubsystem<UGRUISubsystem>() : nullptr, Start);
}

void UGRCheatManager::GRBot(const FString& What, int32 Count)
{
	UGRDevBot* Bot = UGRDevBot::Get(GetOuterAPlayerController());
	if (!Bot)
	{
		return;
	}
	if (What == TEXT("susup"))
	{
		Bot->StartStealth();
	}
	else if (What == TEXT("ojol"))
	{
		Bot->StartOjol(Count);
	}
	else if (What == TEXT("duduk"))
	{
		if (UGROjolSubsystem* Ojol = UGROjolSubsystem::Get(GetOuterAPlayerController()))
		{
			Ojol->CheatSeatPassenger(Count > 0);
		}
	}
	else
	{
		Bot->Stop();
	}
}

void UGRCheatManager::GRPos(float X, float Y, float Yaw)
{
	APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	// On the hills, stand on the landscape; in town (ground at 0) this is the old fixed height.
	float Z = 110.f;
	for (TActorIterator<ALandscapeProxy> It(Pawn->GetWorld()); It; ++It)
	{
		if (const TOptional<float> Ground = It->GetHeightAtLocation(FVector(X, Y, 0.f)))
		{
			Z = FMath::Max(Z, Ground.GetValue() + 110.f);
			break;
		}
	}
	UE_LOG(LogGemahRipah, Display, TEXT("GRPos %.0f %.0f z %.0f"), X, Y, Z);
	Pawn->TeleportTo(FVector(X, Y, Z), FRotator(0.f, Yaw, 0.f));
	PC->SetControlRotation(FRotator(-15.f, Yaw, 0.f));
}

void UGRCheatManager::GRKamera(float Yaw, float Pitch, float Distance)
{
	APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	PC->SetControlRotation(FRotator(Pitch, Pawn->GetActorRotation().Yaw + Yaw, 0.f));
	if (USpringArmComponent* Boom = Pawn->FindComponentByClass<USpringArmComponent>())
	{
		Boom->TargetArmLength = Distance;
	}
}

void UGRCheatManager::GRMobil(float Distance, float Yaw, float Hold)
{
	APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (AGRTraffic* Traffic = AGRTraffic::Get(PC); Traffic && Pawn)
	{
		Traffic->PlaceCar(Pawn->GetActorLocation() + Pawn->GetActorForwardVector().GetSafeNormal2D() * Distance,
			Pawn->GetActorRotation().Yaw + Yaw, Hold);
	}
}

void UGRCheatManager::GRLalin()
{
	if (const AGRTraffic* Traffic = AGRTraffic::Get(GetOuterAPlayerController()))
	{
		Traffic->LogCars();
	}
}

void UGRCheatManager::GRWarga()
{
	if (const AGRCrowd* Crowd = AGRCrowd::Get(GetOuterAPlayerController()))
	{
		Crowd->LogCrowd();
	}
}

void UGRCheatManager::GRWargaBaris()
{
	if (AGRCrowd* Crowd = AGRCrowd::Get(GetOuterAPlayerController()))
	{
		Crowd->Lineup();
	}
}

void UGRCheatManager::GRPolisi()
{
	if (AGRCrowd* Crowd = AGRCrowd::Get(GetOuterAPlayerController()))
	{
		Crowd->SpawnPoliceNear();
	}
}

namespace
{
	/** One gallery item: its world bounds and every component drawn for it (mesh or tile, decal, label). */
	struct FGaleriItem
	{
		FBox Box;
		TArray<TWeakObjectPtr<USceneComponent>> Parts;
	};
	TArray<FGaleriItem> GGaleriItems;
	TWeakObjectPtr<ACameraActor> GGaleriCam;
}

void UGRCheatManager::GRGaleri(const FString& ListFile)
{
	GGaleriItems.Reset();
	UWorld* World = GetWorld();
	APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!World || !Pawn)
	{
		return;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("GRGaleri")))
		{
			It->Destroy();
		}
	}
	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *ListFile);

	// 25 m above Bayu, over the roofs; the floor hides the town below.
	const FVector Origin = Pawn->GetActorLocation() + FVector(-1500.f, -1000.f, 2500.f);
	const float Gap = 120.f;
	// Rows about as wide as the gallery is deep, so one camera shot shows all of it.
	float Area = 0.f;
	for (const FString& Line : Lines)
	{
		const FString Path = Line.TrimStartAndEnd();
		if (UStaticMesh* Mesh = Path.IsEmpty() ? nullptr : LoadObject<UStaticMesh>(nullptr, *(Path + TEXT(".") + FPaths::GetBaseFilename(Path)), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			const FVector Size = Mesh->GetBoundingBox().GetSize();
			Area += (Size.X + Gap) * (Size.Y + Gap + 80.f);
		}
		else if (!Path.IsEmpty())
		{
			Area += 420.f * 500.f;
		}
	}
	const float RowWidth = FMath::Max(2500.f, FMath::Sqrt(Area) * 1.4f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));

	AActor* Gallery = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Origin), Params);
	Gallery->Tags.Add(TEXT("GRGaleri"));
	USceneComponent* Root = NewObject<USceneComponent>(Gallery);
	Gallery->SetRootComponent(Root);
	Root->RegisterComponent();
	Root->SetWorldLocation(Origin);
	auto AddMesh = [&](UStaticMesh* Mesh, const FVector& At, const FVector& Scale, UMaterialInterface* Material)
	{
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Gallery);
		Comp->SetStaticMesh(Mesh);
		Comp->SetupAttachment(Root);
		Comp->SetRelativeLocation(At);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Material)
		{
			Comp->SetMaterial(0, Material);
		}
		Comp->RegisterComponent();
		if (GGaleriItems.Num() > 0)
		{
			GGaleriItems.Last().Parts.Add(Comp);
		}
		return Comp;
	};

	float X = 0.f;
	float Y = 0.f;
	float RowDepth = 0.f;
	float MaxX = 0.f;
	int32 Count = 0;
	for (FString Line : Lines)
	{
		Line.TrimStartAndEndInline();
		if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
		{
			continue;
		}
		const FString Name = FPaths::GetBaseFilename(Line);
		UObject* Asset = LoadObject<UObject>(nullptr, *(Line + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
		UStaticMesh* Mesh = Cast<UStaticMesh>(Asset);
		UMaterialInterface* Material = Cast<UMaterialInterface>(Asset);
		if (!Mesh && !Material)
		{
			UE_LOG(LogTemp, Warning, TEXT("GRGaleri tidak ketemu: %s"), *Line);
			continue;
		}
		FVector Size(300.f, 300.f, 10.f);
		FBox Bounds(ForceInit);
		if (Mesh)
		{
			Bounds = Mesh->GetBoundingBox();
			Size = Bounds.GetSize();
		}
		if (X > 0.f && X + Size.X > RowWidth)
		{
			Y += RowDepth + Gap + 80.f;
			X = 0.f;
			RowDepth = 0.f;
		}
		const FVector Center(X + Size.X * 0.5f, Y + Size.Y * 0.5f, 0.f);
		FGaleriItem& Item = GGaleriItems.AddDefaulted_GetRef();
		Item.Box = FBox::BuildAABB(Origin + Center + FVector(0.f, 0.f, Size.Z * 0.5f), FVector(Size.X, Size.Y, FMath::Max(Size.Z, 10.f)) * 0.5f);
		if (Mesh)
		{
			AddMesh(Mesh, Center - FVector(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z), FVector::OneVector, nullptr);
		}
		else if (Material->GetMaterial() && Material->GetMaterial()->MaterialDomain == MD_DeferredDecal)
		{
			AddMesh(Cube, Center + FVector(0.f, 0.f, 2.f), FVector(3.f, 3.f, 0.04f), nullptr)->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
			UDecalComponent* Decal = NewObject<UDecalComponent>(Gallery);
			Decal->SetupAttachment(Root);
			Decal->SetDecalMaterial(Material);
			Decal->DecalSize = FVector(60.f, 150.f, 150.f);
			Decal->SetRelativeLocation(Center + FVector(0.f, 0.f, 5.f));
			Decal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
			Decal->RegisterComponent();
			Item.Parts.Add(Decal);
		}
		else
		{
			AddMesh(Plane, Center + FVector(0.f, 0.f, 3.f), FVector(3.f, 3.f, 1.f), Material);
		}
		UTextRenderComponent* Label = NewObject<UTextRenderComponent>(Gallery);
		Label->SetupAttachment(Root);
		Label->SetText(FText::FromString(FString::Printf(TEXT("%d %s"), Count + 1, *Name.Replace(TEXT("SM_"), TEXT("")).Replace(TEXT("MI_"), TEXT("")).Left(28))));
		Label->SetWorldSize(FMath::Clamp(Size.X * 0.09f, 14.f, 60.f));
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetTextRenderColor(FColor::Yellow);
		Label->SetRelativeLocation(FVector(Center.X, Y - 40.f, 4.f));
		Label->SetRelativeRotation(FRotator(90.f, -90.f, 0.f));
		Label->RegisterComponent();
		Item.Parts.Add(Label);
		UE_LOG(LogTemp, Display, TEXT("GRGaleri %d %s size %.0f %.0f %.0f"), Count + 1, *Name, Size.X, Size.Y, Size.Z);

		X += Size.X + Gap;
		MaxX = FMath::Max(MaxX, X);
		RowDepth = FMath::Max(RowDepth, Size.Y);
		++Count;
	}
	const float Depth = Y + RowDepth;
	// The floor goes into a scratch item that is dropped again, so focusing never hides it.
	GGaleriItems.AddDefaulted();
	AddMesh(Plane, FVector(MaxX * 0.5f, Depth * 0.5f, 0.f), FVector((MaxX + 2000.f) / 100.f, (Depth + 3000.f) / 100.f, 1.f),
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
	UStaticMeshComponent* Floor = AddMesh(Cube, FVector(MaxX * 0.5f, Depth * 0.5f, -50.f), FVector((MaxX + 2000.f) / 100.f, (Depth + 3000.f) / 100.f, 1.f), nullptr);
	Floor->SetVisibility(false);
	Floor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GGaleriItems.Pop();
	// Bayu stays at home (moving him up here trips the fall safety net); a gallery camera looks down at the middle.
	const float Span = FMath::Max(900.f, FMath::Max(MaxX * 0.62f, Depth * 1.05f));
	const FRotator View(-62.f, 90.f, 0.f);
	const FVector Middle = Origin + FVector(MaxX * 0.5f, Depth * 0.5f, 0.f);
	ACameraActor* Cam = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform(View, Middle - View.Vector() * Span), Params);
	Cam->Tags.Add(TEXT("GRGaleri"));
	Cam->GetCameraComponent()->bConstrainAspectRatio = false;
	PC->SetViewTarget(Cam);
	GGaleriCam = Cam;
	UE_LOG(LogTemp, Display, TEXT("GRGaleri %d aset, lebar %.0f dalam %.0f"), Count, MaxX, Depth);
}

void UGRCheatManager::GRGaleriFokus(int32 Index)
{
	ACameraActor* Cam = GGaleriCam.Get();
	if (!Cam)
	{
		return;
	}
	for (int32 i = 0; i < GGaleriItems.Num(); ++i)
	{
		for (const TWeakObjectPtr<USceneComponent>& Part : GGaleriItems[i].Parts)
		{
			if (USceneComponent* Comp = Part.Get())
			{
				// Labels stay hidden in close-ups; the contact sheet prints the names instead.
				Comp->SetVisibility((Index <= 0 || i == Index - 1) && !Comp->IsA<UTextRenderComponent>());
			}
		}
	}
	if (!GGaleriItems.IsValidIndex(Index - 1))
	{
		return;
	}
	const FBox& Box = GGaleriItems[Index - 1].Box;
	const FRotator View(-35.f, 60.f, 0.f);
	const float Radius = FMath::Max(Box.GetExtent().Size(), 60.f);
	Cam->SetActorLocationAndRotation(Box.GetCenter() - View.Vector() * Radius * 2.4f, View);
}

void UGRCheatManager::GRBarang(const FString& Models)
{
	APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	TArray<FString> Names;
	(Models.IsEmpty() ? FString(TEXT("kursi_plastik pot kotak_suara karpet sembako meja_kecil")) : Models).ParseIntoArrayWS(Names);
	const FVector Ahead = Pawn->GetActorForwardVector().GetSafeNormal2D();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < Names.Num(); ++i)
	{
		const FVector Where = Pawn->GetActorLocation() + Ahead * (500.f + i * 500.f);
		FGRSpawnDef Def;
		Def.Id = FString::Printf(TEXT("tes_barang_%d"), i);
		Def.Model = Names[i];
		Def.Pos = { static_cast<float>(Where.X), static_cast<float>(Where.Y) };
		Def.Yaw = FMath::FRandRange(0.f, 360.f);
		AGRStoryProp* Prop = GetWorld()->SpawnActor<AGRStoryProp>(AGRStoryProp::StaticClass(), FVector(Where.X, Where.Y, 0.f), FRotator(0.f, Def.Yaw, 0.f), Params);
		if (Prop)
		{
			Prop->Setup(Def);
			const FVector Size = Prop->GetComponentsBoundingBox(false).GetSize();
			UE_LOG(LogTemp, Display, TEXT("GRBarang %s at %.0f %.0f size %.0f %.0f %.0f"), *Def.Model, Where.X, Where.Y, Size.X, Size.Y, Size.Z);
		}
	}
}

void UGRCheatManager::GRAudio()
{
	const UGRAmbienceSubsystem* Ambience = GetWorld()->GetSubsystem<UGRAmbienceSubsystem>();
	const UGRVfxSubsystem* Vfx = UGRVfxSubsystem::Get(this);
	UE_LOG(LogGemahRipah, Display, TEXT("GRAudio: %s, sprite %d, efek %.2f"), Ambience ? *Ambience->Describe() : TEXT("-"), Vfx ? Vfx->CountAlive() : -1, GRSfx::Setting());
	for (TActorIterator<AGRSpeaker> It(GetWorld()); It; ++It)
	{
		UE_LOG(LogGemahRipah, Display, TEXT("GRAudio: speaker %s %s %s"), *It->GetSpeakerId(), *It->GetActorLocation().ToCompactString(), It->IsHidden() ? TEXT("tersembunyi") : TEXT("tampil"));
	}
}

void UGRCheatManager::GRVfx(const FString& Kind)
{
	const APawn* Pawn = GetPlayerController() ? GetPlayerController()->GetPawn() : nullptr;
	UGRVfxSubsystem* Vfx = UGRVfxSubsystem::Get(this);
	if (!Pawn || !Vfx)
	{
		return;
	}
	const FVector At = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 300.f + Pawn->GetActorRightVector() * 200.f;
	if (Kind == TEXT("asap"))
	{
		for (int32 i = 0; i < 12; ++i)
		{
			Vfx->Exhaust(At + FVector(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-20.f, 20.f), 0.f), Pawn->GetActorForwardVector(), 1.f);
		}
	}
	else if (Kind == TEXT("debu"))
	{
		Vfx->Dust(At - FVector(0.f, 0.f, 85.f), 60.f);
	}
	else if (Kind == TEXT("percikan"))
	{
		Vfx->Sparks(At, Pawn->GetActorForwardVector(), 24);
	}
	else if (Kind == TEXT("uap"))
	{
		for (int32 i = 0; i < 10; ++i)
		{
			Vfx->Steam(At + FVector(FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(-25.f, 25.f), 0.f));
		}
	}
	else if (Kind == TEXT("kertas"))
	{
		Vfx->Confetti(At + FVector(0.f, 0.f, 100.f), 30);
	}
}

void UGRCheatManager::GRRekam(const FString& What)
{
	if (What == TEXT("mulai"))
	{
		UAudioMixerBlueprintLibrary::StartRecordingOutput(GetWorld(), 0.f);
	}
	else
	{
		UAudioMixerBlueprintLibrary::StopRecordingOutput(GetWorld(), EAudioRecordingExportType::WavFile, What, FString());
	}
}

void UGRCheatManager::GRBunyi(int32 Id)
{
	if (Id >= 100 && GetPlayerController() && GetPlayerController()->GetPawn())
	{
		GRSfx::PlayAt(GetWorld(), static_cast<EGRSfx>(Id - 100), GetPlayerController()->GetPawn()->GetActorLocation());
		return;
	}
	if (Id >= 0 && Id < static_cast<int32>(EGRSfx::Count))
	{
		GRSfx::Play(GetWorld(), static_cast<EGRSfx>(Id));
	}
}

void UGRCheatManager::GRGerobak()
{
	APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	const AGRWarga* Best = nullptr;
	for (TActorIterator<AGRWarga> It(GetWorld()); It; ++It)
	{
		if (It->IsVendor() && !It->IsHidden() && (!Best || FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation()) < FVector::DistSquared(Best->GetActorLocation(), Pawn->GetActorLocation())))
		{
			Best = *It;
		}
	}
	if (!Best)
	{
		UE_LOG(LogGemahRipah, Display, TEXT("GRGerobak: tidak ada pedagang"));
		return;
	}
	const FVector Spot = Best->GetActorLocation() + Best->GetActorRightVector() * 250.f;
	Pawn->SetActorLocation(Spot, false, nullptr, ETeleportType::ResetPhysics);
	const FRotator Face(0.f, (Best->GetActorLocation() - Spot).Rotation().Yaw, 0.f);
	Pawn->SetActorRotation(Face);
	PC->SetControlRotation(Face);
	UE_LOG(LogGemahRipah, Display, TEXT("GRGerobak: %s yaw=%.0f %s"), *Best->GetName(), Best->GetActorRotation().Yaw, *Best->CartDebug());
}

void UGRCheatManager::GRJajan(const FString& Order)
{
	APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UGRUISubsystem* UI = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UGRUISubsystem>() : nullptr;
	if (!Pawn || !UI)
	{
		return;
	}
	UGRJajanScreen* Screen = Cast<UGRJajanScreen>(UI->GetTopScreen());
	if (!Screen)
	{
		AGRWarga* Best = nullptr;
		for (TActorIterator<AGRWarga> It(GetWorld()); It; ++It)
		{
			if (It->IsVendor() && !It->IsHidden() && (!Best || FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation()) < FVector::DistSquared(Best->GetActorLocation(), Pawn->GetActorLocation())))
			{
				Best = *It;
			}
		}
		if (!Best)
		{
			UE_LOG(LogGemahRipah, Display, TEXT("GRJajan: tidak ada pedagang"));
			return;
		}
		// In front of the cart, facing the vendor across it.
		const FVector Spot = Best->GetActorLocation() + Best->GetActorForwardVector() * 230.f;
		Pawn->SetActorLocation(Spot, false, nullptr, ETeleportType::ResetPhysics);
		const FRotator Face(0.f, (Best->GetActorLocation() - Spot).Rotation().Yaw, 0.f);
		Pawn->SetActorRotation(Face);
		PC->SetControlRotation(Face);
		Screen = UI->PushScreen<UGRJajanScreen>();
		if (!Screen)
		{
			return;
		}
		Screen->SetWarga(Best);
		UE_LOG(LogGemahRipah, Display, TEXT("GRJajan: buka %s"), *Best->GetName());
	}
	TArray<FString> Words;
	Order.ParseIntoArrayWS(Words);
	TArray<int32> Amounts;
	for (const FString& Word : Words)
	{
		if (Word.IsNumeric())
		{
			Amounts.Add(FCString::Atoi(*Word));
		}
	}
	if (!Amounts.IsEmpty())
	{
		Screen->SetOrder(Amounts);
	}
	for (const FString& Word : Words)
	{
		if (Word == TEXT("bungkus") || Word == TEXT("sini"))
		{
			Screen->SetTakeAway(Word == TEXT("bungkus"));
		}
		else if (Word == TEXT("bayar"))
		{
			UE_LOG(LogGemahRipah, Display, TEXT("GRJajan: bayar %s"), Screen->Pay() ? TEXT("ok") : TEXT("gagal"));
		}
	}
}

void UGRCheatManager::GRTubuh()
{
	const APlayerController* PC = GetOuterAPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	int32 Count = 0;
	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
	{
		ACharacter* Who = *It;
		if (Who->IsHidden() || FVector::Dist(Who->GetActorLocation(), Pawn->GetActorLocation()) > 2500.f || ++Count > 8)
		{
			continue;
		}
		TArray<USkeletalMeshComponent*> Meshes;
		Who->GetComponents(Meshes);
		for (const USkeletalMeshComponent* Mesh : Meshes)
		{
			const bool bTripo = Mesh != Who->GetMesh();
			const FName Head = bTripo ? FName(TEXT("Head")) : FName(TEXT("head"));
			const FName Foot = bTripo ? FName(TEXT("LeftFoot")) : FName(TEXT("foot_l"));
			const float Z = Who->GetActorLocation().Z;
			UE_LOG(LogGemahRipah, Display, TEXT("GRTubuh %s %s mesh=%s visible=%d anim=%s head=%.0f foot=%.0f bounds=%.0f scale=%.2f"),
				*Who->GetName(), bTripo ? TEXT("tripo") : TEXT("manekin"), *GetNameSafe(Mesh->GetSkeletalMeshAsset()), Mesh->IsVisible() ? 1 : 0,
				*GetNameSafe(Mesh->GetAnimInstance()), Mesh->GetBoneLocation(Head).Z - Z, Mesh->GetBoneLocation(Foot).Z - Z,
				Mesh->Bounds.SphereRadius, Mesh->GetComponentScale().Z);
			if (bTripo && Mesh->GetSkeletalMeshAsset())
			{
				UMaterialInterface* Mat = Mesh->GetMaterial(0);
				UTexture* Base = nullptr;
				if (Mat)
				{
					Mat->GetTextureParameterValue(FHashedMaterialParameterInfo(TEXT("Base")), Base);
				}
				UE_LOG(LogGemahRipah, Display, TEXT("GRTubuh   material %s base=%s uv=%d"), *GetPathNameSafe(Mat), *GetNameSafe(Base),
					Mesh->GetSkeletalMeshAsset()->GetResourceForRendering() ? Mesh->GetSkeletalMeshAsset()->GetResourceForRendering()->LODRenderData[0].GetNumTexCoords() : -1);
				const FReferenceSkeleton& Ref = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
				for (int32 i = 0; i < FMath::Min(3, Ref.GetNum()); ++i)
				{
					UE_LOG(LogGemahRipah, Display, TEXT("GRTubuh   ref %s %s"), *Ref.GetBoneName(i).ToString(), *Ref.GetRefBonePose()[i].ToString());
				}
			}
		}
	}
}

void UGRCheatManager::GRPosisi()
{
	const APlayerController* PC = GetOuterAPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	int32 Seated = 0;
	int32 Moved = 0;
	for (TActorIterator<AGRNpc> It(GetWorld()); It; ++It)
	{
		if (It->IsHidden() || !It->IsSeated())
		{
			continue;
		}
		++Seated;
		const FVector Loc = It->GetActorLocation();
		const float Drift = FVector::Dist2D(Loc, It->GetHome());
		const float Turn = FMath::Abs(FRotator::NormalizeAxis(It->GetActorRotation().Yaw - It->GetHomeYaw()));
		if (Drift > 5.f || Turn > 2.f || Loc.Z > 120.f)
		{
			++Moved;
			UE_LOG(LogGemahRipah, Warning, TEXT("GRPosisi %s bergeser %.0f cm, berputar %.0f derajat, z %.0f"), *It->SpawnId, Drift, Turn, Loc.Z);
		}
	}
	const FVector P = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	FString Ride = TEXT("jalan kaki");
	if (const AGRCharacter* Character = Cast<AGRCharacter>(Pawn); Character && Character->IsRiding())
	{
		Ride = FString::Printf(TEXT("motor %.0f km/j, miring %.0f"), Character->GetRideSpeed() * 0.036f, Character->GetRideLean());
	}
	else if (const AGRCharacter* Walker = Cast<AGRCharacter>(Pawn); Walker && Walker->GetMotorDistance() >= 0.f)
	{
		Ride += FString::Printf(TEXT(", motor parkir %.0f m"), Walker->GetMotorDistance());
	}
	UE_LOG(LogGemahRipah, Warning, TEXT("GRPosisi pemain (%.0f, %.0f, %.0f) yaw %.0f, %s | NPC duduk %d, bergeser %d"), P.X, P.Y, P.Z,
		Pawn ? Pawn->GetActorRotation().Yaw : 0.f, *Ride, Seated, Moved);
}

void UGRCheatManager::GRTekan(const FString& KeyName, float Seconds)
{
	const FKey Key(*KeyName);
	if (!Key.IsValid() || !FSlateApplication::IsInitialized())
	{
		UE_LOG(LogTemp, Warning, TEXT("GRTekan: tombol %s tidak dikenal"), *KeyName);
		return;
	}
	FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
	// Core ticker, not the world timer: menus and minigames pause the world.
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Key](float)
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
		}
		return false;
	}), FMath::Max(Seconds, 0.05f));
}

void UGRCheatManager::GRPetaUlang()
{
	const APlayerController* PC = GetOuterAPlayerController();
	const UGRMapSubsystem* Map = PC ? PC->GetWorld()->GetSubsystem<UGRMapSubsystem>() : nullptr;
	if (UGRMapSubsystem* Mutable = const_cast<UGRMapSubsystem*>(Map))
	{
		Mutable->InvalidateFootprints();
	}
}

void UGRCheatManager::GRCerita(const FString& Actions)
{
	if (UGRStorySubsystem* Story = GetWorld()->GetGameInstance()->GetSubsystem<UGRStorySubsystem>())
	{
		Story->Run(Actions);
	}
}

void UGRCheatManager::GRCek(const FString& Condition)
{
	if (const UGRStorySubsystem* Story = GetWorld()->GetGameInstance()->GetSubsystem<UGRStorySubsystem>())
	{
		UE_LOG(LogTemp, Warning, TEXT("GRCek %s = %s | objektif: %s | energi %.0f"), *Condition, Story->Check(Condition) ? TEXT("benar") : TEXT("salah"),
			*Story->GetObjectiveText(), Story->GetEnergi());
	}
}

void UGRCheatManager::GRHp(const FString& What, float Speed)
{
	const APlayerController* PC = GetOuterAPlayerController();
	UGRUISubsystem* UI = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UGRUISubsystem>() : nullptr;
	if (!UI)
	{
		return;
	}
	const FString Name = What.ToLower();
	if (Name == TEXT("tutup"))
	{
		UI->PopAll();
		return;
	}
	// The map opened from the phone sits on top of it: going to another app closes the map first.
	if (Cast<UGRMapScreen>(UI->GetTopScreen()))
	{
		UI->PopScreen(UI->GetTopScreen());
	}
	UGRPhoneScreen* Phone = Cast<UGRPhoneScreen>(UI->GetTopScreen());
	if (!Phone)
	{
		Phone = UI->PushScreen<UGRPhoneScreen>();
	}
	if (!Phone)
	{
		return;
	}
	if (Name == TEXT("gulir"))
	{
		Phone->DevScroll(Speed);
	}
	else
	{
		Phone->DevShowApp(Name);
	}
}

void UGRCheatManager::GRJalan(float X, float Y, float Speed, float CameraYaw)
{
	if (UGRDevBot* Bot = UGRDevBot::Get(GetOuterAPlayerController()))
	{
		Bot->StartPath(FVector2D(X, Y), Speed, CameraYaw);
	}
}

void UGRCheatManager::GRPindah(float X, float Y, float Yaw)
{
	if (UGRDevBot* Bot = UGRDevBot::Get(GetOuterAPlayerController()))
	{
		Bot->Stop();
	}
	GRPos(X, Y, Yaw);
	if (APlayerController* PC = GetOuterAPlayerController())
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.6f, FLinearColor::Black, false, false);
	}
}
