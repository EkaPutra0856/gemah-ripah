#include "UI/GRTrailerScreen.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/SkeletalMeshActor.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/TextBlock.h"
#include "Components/TextRenderComponent.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/GRPlayerController.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Player/GRCharacter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Story/GRStorySubsystem.h"
#include "Story/GRStoryUtil.h"
#include "Systems/GRClockSubsystem.h"
#include "UI/GRUIStyle.h"
#include "UI/GRUISubsystem.h"
#include "World/GRMotor.h"
#include "World/GRBody.h"
#include "World/GRNpc.h"
#include "World/GRRoomShells.h"
#include "World/GRStoryProp.h"

#define LOCTEXT_NAMESPACE "GRTrailer"

int32 UGRTrailerScreen::PendingStart = 0;

namespace
{
	float Num(const FJsonObject& Obj, const TCHAR* Field, float Default)
	{
		double Value = Default;
		return Obj.TryGetNumberField(Field, Value) ? static_cast<float>(Value) : Default;
	}

	FString Str(const FJsonObject& Obj, const TCHAR* Field)
	{
		FString Value;
		Obj.TryGetStringField(Field, Value);
		return Value;
	}

	TArray<float> Nums(const FJsonObject& Obj, const TCHAR* Field)
	{
		TArray<float> Out;
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (Obj.TryGetArrayField(Field, Values))
		{
			for (const TSharedPtr<FJsonValue>& Value : *Values)
			{
				Out.Add(static_cast<float>(Value->AsNumber()));
			}
		}
		return Out;
	}

	FVector Vec(const TArray<float>& A, const FVector& Default = FVector::ZeroVector)
	{
		return FVector(A.IsValidIndex(0) ? A[0] : Default.X, A.IsValidIndex(1) ? A[1] : Default.Y, A.IsValidIndex(2) ? A[2] : Default.Z);
	}

	/** Ground height under (X, Y): the first thing below a point a few metres up that is not a person or a car. */
	float GroundZ(UWorld* World, float X, float Y, float From = 600.f)
	{
		FHitResult Hit;
		FCollisionObjectQueryParams Objects;
		Objects.AddObjectTypesToQuery(ECC_WorldStatic);
		if (World->LineTraceSingleByObjectType(Hit, FVector(X, Y, From), FVector(X, Y, From - 3000.f), Objects))
		{
			return Hit.ImpactPoint.Z;
		}
		return 0.f;
	}

	UAnimSequenceBase* LoadAnim(const FString& Path)
	{
		return Path.IsEmpty() ? nullptr : LoadObject<UAnimSequenceBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
}

UGRTrailerScreen::UGRTrailerScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bDimBackground = false;
	bCanClose = true;
}

bool UGRTrailerScreen::Open(UGRUISubsystem* UI, int32 Start)
{
	PendingStart = FMath::Max(Start, 0);
	return UI && UI->PushScreen<UGRTrailerScreen>() != nullptr;
}

void UGRTrailerScreen::BuildScreen(UOverlay* Root)
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("Data/trailer.json");
	FString Text;
	TSharedPtr<FJsonObject> Data;
	if (FFileHelper::LoadFileToString(Text, *Path))
	{
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Data);
	}
	const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
	if (Data.IsValid() && Data->TryGetArrayField(TEXT("shots"), List))
	{
		for (const TSharedPtr<FJsonValue>& Value : *List)
		{
			if (Value->AsObject().IsValid())
			{
				Shots.Add(Value->AsObject());
			}
		}
	}
	UE_LOG(LogTemp, Display, TEXT("GRTRAILER mulai, %d shot dari %s"), Shots.Num(), *Path);

	// Widescreen bars, the cinema look.
	for (int32 i = 0; i < 2; ++i)
	{
		USizeBox* Bar = WidgetTree->ConstructWidget<USizeBox>();
		Bar->SetHeightOverride(118.f);
		UBorder* Black = WidgetTree->ConstructWidget<UBorder>();
		Black->SetBrushColor(FLinearColor::Black);
		Bar->SetContent(Black);
		UOverlaySlot* BarSlot = Root->AddChildToOverlay(Bar);
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetVerticalAlignment(i == 0 ? VAlign_Top : VAlign_Bottom);
		if (i == 1)
		{
			CaptionPanel = Black;
			CaptionText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 24, GRStyle::ColorText);
			CaptionText->SetJustification(ETextJustify::Center);
			CaptionText->SetAutoWrapText(true);
			Black->SetContent(CaptionText);
			Black->SetHorizontalAlignment(HAlign_Center);
			Black->SetVerticalAlignment(VAlign_Center);
			Black->SetPadding(FMargin(160.f, 8.f));
		}
	}

	// Title cards: black, or over the picture.
	CardPanel = WidgetTree->ConstructWidget<UBorder>();
	CardPanel->SetBrushColor(FLinearColor::Black);
	UOverlaySlot* CardSlot = Root->AddChildToOverlay(CardPanel);
	CardSlot->SetHorizontalAlignment(HAlign_Fill);
	CardSlot->SetVerticalAlignment(VAlign_Fill);
	CardPanel->SetVisibility(ESlateVisibility::Collapsed);

	UVerticalBox* TitleBox = WidgetTree->ConstructWidget<UVerticalBox>();
	TitleText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 64, GRStyle::ColorAccent, true);
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetShadowOffset(FVector2D(3.f, 3.f));
	TitleText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	TitleBox->AddChildToVerticalBox(TitleText)->SetHorizontalAlignment(HAlign_Center);
	SubText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 28, GRStyle::ColorText);
	SubText->SetJustification(ETextJustify::Center);
	SubText->SetShadowOffset(FVector2D(2.f, 2.f));
	SubText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	UVerticalBoxSlot* SubSlot = TitleBox->AddChildToVerticalBox(SubText);
	SubSlot->SetHorizontalAlignment(HAlign_Center);
	SubSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	UOverlaySlot* TitleSlot = Root->AddChildToOverlay(TitleBox);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetVerticalAlignment(VAlign_Center);

	if (APlayerController* PC = GetOwningPlayer())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Camera = GetWorld()->SpawnActor<ACineCameraActor>(ACineCameraActor::StaticClass(), PC->PlayerCameraManager->GetCameraLocation(), PC->PlayerCameraManager->GetCameraRotation(), Params);
		if (Camera)
		{
			UCineCameraComponent* Cine = Camera->GetCineCameraComponent();
			Cine->Filmback.SensorWidth = 36.f;
			Cine->Filmback.SensorHeight = 20.25f;
			Cine->bConstrainAspectRatio = false;
			PC->SetViewTargetWithBlend(Camera, 0.f);
		}
	}

	if (UGRClockSubsystem* Clock = GetGameInstance()->GetSubsystem<UGRClockSubsystem>())
	{
		Clock->SetRunning(false);
	}
	if (UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>())
	{
		Story->SetVar(TEXT("punya_motor"), 1);
	}
	ShowPlayer(false);
	StartShot(PendingStart);
}

void UGRTrailerScreen::HideWorld()
{
	// The story's own people would stand in the way of the cast; some spawn only as the chapter moves on.
	for (TActorIterator<AGRNpc> It(GetWorld()); It; ++It)
	{
		const bool bOurs = People.ContainsByPredicate([&](const FCast& Entry) { return Entry.Actor.Get() == *It; });
		if (!bOurs && !It->IsHidden())
		{
			It->SetActorHiddenInGame(true);
			HiddenStory.Add(*It);
		}
	}
	// The HUD, minimap, area banners and toasts are all top level widgets; only the trailer stays.
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UUserWidget::StaticClass(), true);
	for (UUserWidget* Widget : Widgets)
	{
		if (Widget != this && Widget->IsVisible())
		{
			HiddenWidgets.FindOrAdd(Widget, Widget->GetVisibility());
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UGRTrailerScreen::ShowPlayer(bool bShow)
{
	AGRCharacter* Bayu = Cast<AGRCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Bayu)
	{
		return;
	}
	Bayu->SetActorHiddenInGame(!bShow);
	if (AGRMotor* Motor = Bayu->GetMotor())
	{
		Motor->SetActorHiddenInGame(!bShow);
	}
}

void UGRTrailerScreen::ClearGroup(TArray<FCast>& Group)
{
	for (FCast& Entry : Group)
	{
		if (AActor* Actor = Entry.Actor.Get())
		{
			Actor->Destroy();
		}
	}
	Group.Reset();
}

void UGRTrailerScreen::SpawnCast(const FJsonObject& Shot)
{
	const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
	if (!Shot.TryGetArrayField(TEXT("cast"), List))
	{
		return;
	}
	ClearGroup(People);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const TSharedPtr<FJsonValue>& Value : *List)
	{
		const TSharedPtr<FJsonObject> Obj = Value->AsObject();
		if (!Obj.IsValid())
		{
			continue;
		}
		FGRSpawnDef Def;
		Def.Id = Str(*Obj, TEXT("id"));
		Def.Name = Str(*Obj, TEXT("name"));
		Def.Kind = Obj->HasField(TEXT("kind")) ? Str(*Obj, TEXT("kind")) : FString(TEXT("npc"));
		Def.Pos = Nums(*Obj, TEXT("pos"));
		Def.Yaw = Num(*Obj, TEXT("yaw"), 0.f);
		Def.Pose = Str(*Obj, TEXT("pose"));
		const FVector Location = GRStoryUtil::ToLocation(Def.Pos, 100.f);
		AGRNpc* Npc = GetWorld()->SpawnActor<AGRNpc>(AGRNpc::StaticClass(), Location, FRotator(0.f, Def.Yaw, 0.f), Params);
		if (!Npc)
		{
			continue;
		}
		Npc->Setup(Def);
		// An extra can wear any model ("N35_Petugas_kebersihan"), not just the one its name gives.
		if (const FString Model = Str(*Obj, TEXT("model")); !Model.IsEmpty())
		{
			GRBody::Apply(Npc, Npc->TripoBody, GRBody::Find(Model));
		}
		// Acting only: no wandering, barks, patrols or name labels.
		Npc->SetActorTickEnabled(false);
		Npc->NameText->SetVisibility(false);
		const bool bLight = Obj->HasField(TEXT("light"));
		Npc->Flashlight->SetVisibility(bLight);
		if (bLight)
		{
			const TArray<float> Light = Nums(*Obj, TEXT("light"));
			Npc->SetStealthGuard(Light.IsValidIndex(0) ? Light[0] : 700.f, Light.IsValidIndex(1) ? Light[1] : 28.f);
		}
		FCast Entry;
		Entry.Id = Def.Id;
		Entry.Actor = Npc;
		const TArray<float> Walk = Nums(*Obj, TEXT("walk"));
		if (Walk.Num() >= 2)
		{
			Entry.bWalk = true;
			Entry.WalkTo = FVector(Walk[0], Walk[1], 0.f);
			Npc->GetCharacterMovement()->MaxWalkSpeed = Num(*Obj, TEXT("speed"), 150.f);
		}
		const FString AnimName = Str(*Obj, TEXT("anim"));
		if (!AnimName.IsEmpty() && !Entry.bWalk)
		{
			const FString AnimPath = FString::Printf(TEXT("/Game/GemahRipah/Anims/MM_GR_%s.MM_GR_%s"), *AnimName, *AnimName);
			if (UAnimSequenceBase* Seq = LoadAnim(AnimPath))
			{
				USkeletalMeshComponent* Mesh = Npc->GetMesh();
				Mesh->PlayAnimation(Seq, Obj->HasField(TEXT("loop")) ? Obj->GetBoolField(TEXT("loop")) : true);
				Mesh->SetPlayRate(Num(*Obj, TEXT("rate"), 1.f));
				Mesh->SetPosition(Num(*Obj, TEXT("at"), 0.f), false);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("GRTRAILER: animasi %s tidak ada"), *AnimPath);
			}
		}
		const FString Say = Str(*Obj, TEXT("say"));
		if (!Say.IsEmpty())
		{
			Npc->Say(Say, Num(*Obj, TEXT("saytime"), 3.f));
		}
		People.Add(Entry);
	}
}

void UGRTrailerScreen::SpawnProps(const FJsonObject& Shot)
{
	const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
	if (!Shot.TryGetArrayField(TEXT("props"), List))
	{
		return;
	}
	ClearGroup(Props);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const TSharedPtr<FJsonValue>& Value : *List)
	{
		const TSharedPtr<FJsonObject> Obj = Value->AsObject();
		if (!Obj.IsValid())
		{
			continue;
		}
		FGRSpawnDef Def;
		Def.Kind = TEXT("prop");
		Def.Pos = Nums(*Obj, TEXT("pos"));
		if (Def.Pos.Num() == 2)
		{
			Def.Pos.Add(GroundZ(GetWorld(), Def.Pos[0], Def.Pos[1]));
		}
		Def.Yaw = Num(*Obj, TEXT("yaw"), 0.f);
		Def.Size = Nums(*Obj, TEXT("size"));
		Def.Model = Str(*Obj, TEXT("model"));
		Def.Name = Str(*Obj, TEXT("name"));
		if (Obj->HasField(TEXT("color")))
		{
			Def.Color = Str(*Obj, TEXT("color"));
		}
		if (Obj->HasField(TEXT("shape")))
		{
			Def.Shape = Str(*Obj, TEXT("shape"));
		}
		const FVector Location = GRStoryUtil::ToLocation(Def.Pos, 0.f);
		if (AGRStoryProp* Prop = GetWorld()->SpawnActor<AGRStoryProp>(AGRStoryProp::StaticClass(), Location, FRotator(0.f, Def.Yaw, 0.f), Params))
		{
			Prop->Setup(Def);
			FCast Entry;
			Entry.Actor = Prop;
			Props.Add(Entry);
		}
	}
}

void UGRTrailerScreen::SpawnVehicles(const FJsonObject& Shot)
{
	const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
	if (!Shot.TryGetArrayField(TEXT("veh"), List))
	{
		return;
	}
	ClearGroup(Vehicles);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const TSharedPtr<FJsonValue>& Value : *List)
	{
		const TSharedPtr<FJsonObject> Obj = Value->AsObject();
		if (!Obj.IsValid())
		{
			continue;
		}
		const FString Name = Str(*Obj, TEXT("mesh"));
		const FString Base = FString::Printf(TEXT("/Game/GemahRipah/Props/Kendaraan/Rig/%s/"), *Name);
		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%sSK_%s.SK_%s"), *Base, *Name, *Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Mesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("GRTRAILER: kendaraan %s tidak ada"), *Name);
			continue;
		}
		const TArray<float> Pos = Nums(*Obj, TEXT("pos"));
		const TArray<float> To = Nums(*Obj, TEXT("to"));
		auto OnGround = [&](const TArray<float>& P)
		{
			return FVector(P[0], P[1], P.IsValidIndex(2) ? P[2] : GroundZ(GetWorld(), P[0], P[1]));
		};
		if (Pos.Num() < 2)
		{
			continue;
		}
		const FVector From = OnGround(Pos);
		ASkeletalMeshActor* Actor = GetWorld()->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), From, FRotator(0.f, Num(*Obj, TEXT("yaw"), 0.f), 0.f), Params);
		if (!Actor)
		{
			continue;
		}
		USkeletalMeshComponent* Comp = Actor->GetSkeletalMeshComponent();
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetSkeletalMesh(Mesh);
		Actor->SetActorScale3D(FVector(Num(*Obj, TEXT("scale"), 1.f)));
		const FString AnimName = Str(*Obj, TEXT("anim"));
		if (!AnimName.IsEmpty())
		{
			if (UAnimSequenceBase* Seq = LoadAnim(FString::Printf(TEXT("%sA_%s_%s.A_%s_%s"), *Base, *Name, *AnimName, *Name, *AnimName)))
			{
				Comp->PlayAnimation(Seq, true);
				Comp->SetPlayRate(Num(*Obj, TEXT("rate"), 1.f));
			}
		}
		FCast Entry;
		Entry.Id = Str(*Obj, TEXT("id"));
		Entry.Actor = Actor;
		if (To.Num() >= 2)
		{
			Entry.bSlide = true;
			Entry.MoveFrom = From;
			Entry.MoveTo = OnGround(To);
		}
		Vehicles.Add(Entry);
	}
}

void UGRTrailerScreen::SetupRide(const FJsonObject* Ride)
{
	AGRCharacter* Bayu = Cast<AGRCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	bRiding = Ride != nullptr && Bayu != nullptr;
	ShowPlayer(bRiding);
	if (!bRiding)
	{
		return;
	}
	const TArray<float> Pos = Nums(*Ride, TEXT("pos"));
	const float Yaw = Num(*Ride, TEXT("yaw"), 0.f);
	RideThrottle = Num(*Ride, TEXT("throttle"), 1.f);
	RideSteer = Num(*Ride, TEXT("steer"), 0.f);
	if (Bayu->IsRiding())
	{
		Bayu->SetRiding(false);
	}
	if (AGRMotor* Motor = Bayu->GetMotor(); Motor && Pos.Num() >= 2)
	{
		// Bayu walks up to the scooter first, so mounting seats him where it stands, not the other way round.
		const FVector Ground(Pos[0], Pos[1], GroundZ(GetWorld(), Pos[0], Pos[1]));
		Motor->Park(Ground, Yaw);
		Bayu->TeleportTo(Ground + FRotator(0.f, Yaw, 0.f).RotateVector(FVector(0.f, -90.f, 100.f)), FRotator(0.f, Yaw, 0.f));
	}
	Bayu->SetRiding(true);
}

AActor* UGRTrailerScreen::FindTarget(const FString& Id) const
{
	if (Id.IsEmpty())
	{
		return nullptr;
	}
	if (Id == TEXT("bayu_motor"))
	{
		return UGameplayStatics::GetPlayerPawn(this, 0);
	}
	for (const TArray<FCast>* Group : { &People, &Vehicles })
	{
		for (const FCast& Entry : *Group)
		{
			if (Entry.Id == Id)
			{
				return Entry.Actor.Get();
			}
		}
	}
	return nullptr;
}

void UGRTrailerScreen::StartShot(int32 Index)
{
	if (!Shots.IsValidIndex(Index))
	{
		Finish();
		return;
	}
	ShotIndex = Index;
	ShotTime = 0.f;
	const FJsonObject& Shot = *Shots[Index];
	ShotLength = FMath::Max(Num(Shot, TEXT("t"), 3.f), 0.1f);
	FadeIn = Num(Shot, TEXT("fadein"), 0.f);
	FadeOut = Num(Shot, TEXT("fadeout"), 0.f);
	UE_LOG(LogTemp, Display, TEXT("GRTRAILER shot %d %s"), Index, *Str(Shot, TEXT("name")));

	UGRClockSubsystem* Clock = GetGameInstance()->GetSubsystem<UGRClockSubsystem>();
	if (Clock && Shot.HasField(TEXT("hour")))
	{
		Clock->SetHour(Num(Shot, TEXT("hour"), 9.f));
	}
	if (Clock && Shot.HasField(TEXT("ispu")))
	{
		Clock->SetIspuOverride(Num(Shot, TEXT("ispu"), -1.f));
	}

	// Roofs off for a look inside the rooms; on for every other shot.
	const bool bRoofs = !Shot.HasField(TEXT("roofs")) || Shot.GetBoolField(TEXT("roofs"));
	for (TActorIterator<AGRRoomShells> It(GetWorld()); It; ++It)
	{
		It->SetActorHiddenInGame(!bRoofs);
	}
	SpawnProps(Shot);
	SpawnVehicles(Shot);
	SpawnCast(Shot);
	const TSharedPtr<FJsonObject>* Ride = nullptr;
	if (Shot.TryGetObjectField(TEXT("ride"), Ride))
	{
		SetupRide(Ride->Get());
	}
	else if (!Shot.HasField(TEXT("keepride")))
	{
		SetupRide(nullptr);
	}

	// Camera: fixed or moving between two spots, aimed at someone, or following someone.
	const TArray<float> Cam = Nums(Shot, TEXT("cam"));
	const TArray<float> Cam2 = Nums(Shot, TEXT("cam2"));
	bHasCamera = Cam.Num() >= 3 || Shot.HasField(TEXT("follow"));
	CamFrom = Vec(Cam);
	RotFrom = FRotator(Cam.IsValidIndex(3) ? Cam[3] : 0.f, Cam.IsValidIndex(4) ? Cam[4] : 0.f, 0.f);
	CamTo = Cam2.Num() >= 3 ? Vec(Cam2) : CamFrom;
	RotTo = Cam2.Num() >= 5 ? FRotator(Cam2[3], Cam2[4], 0.f) : RotFrom;
	FocalFrom = Num(Shot, TEXT("focal"), 35.f);
	FocalTo = Num(Shot, TEXT("focal2"), FocalFrom);
	FollowId = Str(Shot, TEXT("follow"));
	FollowOffset = Vec(Nums(Shot, TEXT("offset")), FVector(-420.f, 0.f, 160.f));
	FollowOffsetTo = Vec(Nums(Shot, TEXT("offset2")), FollowOffset);
	AimId = Str(Shot, TEXT("aim"));
	AimHeight = Num(Shot, TEXT("aimz"), 140.f);
	if (Camera)
	{
		UCineCameraComponent* Cine = Camera->GetCineCameraComponent();
		AActor* Focus = FindTarget(!AimId.IsEmpty() ? AimId : FollowId);
		const float Aperture = Num(Shot, TEXT("f"), 2.8f);
		if (Focus && Aperture > 0.f)
		{
			Cine->FocusSettings.FocusMethod = ECameraFocusMethod::Tracking;
			Cine->FocusSettings.TrackingFocusSettings.ActorToTrack = Focus;
			Cine->FocusSettings.TrackingFocusSettings.RelativeOffset = FVector(0.f, 0.f, AimHeight - 90.f);
			Cine->CurrentAperture = Aperture;
		}
		else
		{
			Cine->FocusSettings.FocusMethod = ECameraFocusMethod::DoNotOverride;
		}
	}
	TickCamera(0.f, 1.f);

	// Text: a card on black, a title over the picture, a caption in the lower bar.
	const bool bCard = Shot.HasField(TEXT("card")) && Shot.GetBoolField(TEXT("card"));
	CardPanel->SetVisibility(bCard ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	TitleText->SetText(FText::FromString(Str(Shot, TEXT("title"))));
	SubText->SetText(FText::FromString(Str(Shot, TEXT("sub"))));
	CaptionText->SetText(FText::FromString(Str(Shot, TEXT("text"))));
	const float TitleSize = Num(Shot, TEXT("size"), 64.f);
	FSlateFontInfo Font = TitleText->GetFont();
	Font.Size = TitleSize;
	TitleText->SetFont(Font);
	TitleText->SetColorAndOpacity(Shot.HasField(TEXT("titlecolor")) ? FSlateColor(GRStoryUtil::ParseColor(Str(Shot, TEXT("titlecolor")))) : FSlateColor(GRStyle::ColorAccent));
}

void UGRTrailerScreen::TickCamera(float Alpha, float DeltaTime)
{
	if (!Camera || !bHasCamera)
	{
		return;
	}
	const float Smooth = FMath::SmoothStep(0.f, 1.f, Alpha);
	FVector Location = FMath::Lerp(CamFrom, CamTo, Smooth);
	FRotator Rotation = FMath::Lerp(RotFrom, RotTo, Smooth);
	if (const AActor* Follow = FindTarget(FollowId))
	{
		const FVector Offset = FMath::Lerp(FollowOffset, FollowOffsetTo, Smooth);
		const FRotator Yaw(0.f, Follow->GetActorRotation().Yaw, 0.f);
		const FVector Wanted = Follow->GetActorLocation() + Yaw.RotateVector(Offset);
		Location = DeltaTime >= 1.f ? Wanted : FMath::VInterpTo(Camera->GetActorLocation(), Wanted, DeltaTime, 6.f);
		Rotation = (Follow->GetActorLocation() + FVector(0.f, 0.f, AimHeight - 90.f) - Location).Rotation();
	}
	else if (const AActor* Aim = FindTarget(AimId))
	{
		const FRotator Wanted = (Aim->GetActorLocation() + FVector(0.f, 0.f, AimHeight - 90.f) - Location).Rotation();
		Rotation = DeltaTime >= 1.f ? Wanted : FMath::RInterpTo(Camera->GetActorRotation(), Wanted, DeltaTime, 5.f);
	}
	// A wall or tree between the lens and whoever the shot is about: move the lens in front of it.
	const AActor* Subject = FindTarget(FollowId);
	Subject = Subject ? Subject : FindTarget(AimId);
	if (Subject)
	{
		const FVector Eye = Subject->GetActorLocation() + FVector(0.f, 0.f, AimHeight - 90.f);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GRTrailerLens), false);
		for (const TArray<FCast>* Group : { &People, &Props, &Vehicles })
		{
			for (const FCast& Entry : *Group)
			{
				Params.AddIgnoredActor(Entry.Actor.Get());
			}
		}
		if (APawn* Bayu = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			Params.AddIgnoredActor(Bayu);
			if (AGRCharacter* Rider = Cast<AGRCharacter>(Bayu))
			{
				Params.AddIgnoredActor(Rider->GetMotor());
			}
		}
		FHitResult Hit;
		if (GetWorld()->SweepSingleByChannel(Hit, Eye, Location, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12.f), Params) && Hit.Distance > 60.f)
		{
			const FVector Back = Location - Eye;
			Location = Eye + Back.GetSafeNormal() * FMath::Max(Hit.Distance - 25.f, 60.f);
			const FRotator Looking = (Eye - Location).Rotation();
			Rotation = FollowId.IsEmpty() && AimId.IsEmpty() ? Rotation : Looking;
		}
	}
	Camera->SetActorLocationAndRotation(Location, Rotation);
	Camera->GetCineCameraComponent()->SetCurrentFocalLength(FMath::Lerp(FocalFrom, FocalTo, Smooth));
}

void UGRTrailerScreen::TickCast(float DeltaTime, float Alpha)
{
	for (FCast& Entry : People)
	{
		ACharacter* Person = Cast<ACharacter>(Entry.Actor.Get());
		if (!Person || !Entry.bWalk)
		{
			continue;
		}
		const FVector To = FVector(Entry.WalkTo.X, Entry.WalkTo.Y, Person->GetActorLocation().Z) - Person->GetActorLocation();
		if (To.Size2D() > 40.f)
		{
			Person->AddMovementInput(To.GetSafeNormal2D(), 1.f);
		}
	}
	for (FCast& Entry : Vehicles)
	{
		if (AActor* Actor = Entry.Actor.Get(); Actor && Entry.bSlide)
		{
			Actor->SetActorLocation(FMath::Lerp(Entry.MoveFrom, Entry.MoveTo, Alpha));
		}
	}
	if (bRiding)
	{
		if (AGRCharacter* Bayu = Cast<AGRCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			Bayu->MoveFromInput(FVector2D(RideSteer, RideThrottle));
		}
	}
}

void UGRTrailerScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bDone)
	{
		return;
	}
	HideWorld();
	if (!Shots.IsValidIndex(ShotIndex))
	{
		Finish();
		return;
	}
	ShotTime += InDeltaTime;
	const float Alpha = FMath::Clamp(ShotTime / ShotLength, 0.f, 1.f);
	TickCast(InDeltaTime, Alpha);
	TickCamera(Alpha, InDeltaTime);

	float Fade = 0.f;
	if (FadeIn > 0.f && ShotTime < FadeIn)
	{
		Fade = 1.f - ShotTime / FadeIn;
	}
	if (FadeOut > 0.f && ShotTime > ShotLength - FadeOut)
	{
		Fade = FMath::Max(Fade, (ShotTime - (ShotLength - FadeOut)) / FadeOut);
	}
	Fade = FMath::Clamp(Fade, 0.f, 1.f);
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->PlayerCameraManager->SetManualCameraFade(Fade, FLinearColor::Black, false);
	}
	TitleText->SetRenderOpacity(1.f - Fade);
	SubText->SetRenderOpacity(1.f - Fade);
	CaptionText->SetRenderOpacity(1.f - Fade);

	if (ShotTime >= ShotLength)
	{
		StartShot(ShotIndex + 1);
	}
}

void UGRTrailerScreen::HandleBack()
{
	Finish();
}

void UGRTrailerScreen::Finish(bool bClose)
{
	if (bDone)
	{
		return;
	}
	bDone = true;
	UE_LOG(LogTemp, Display, TEXT("GRTRAILER selesai"));
	ClearGroup(People);
	ClearGroup(Props);
	ClearGroup(Vehicles);
	for (const TWeakObjectPtr<AActor>& Weak : HiddenStory)
	{
		if (AActor* Actor = Weak.Get())
		{
			Actor->SetActorHiddenInGame(false);
		}
	}
	HiddenStory.Reset();
	for (const TPair<TWeakObjectPtr<UUserWidget>, ESlateVisibility>& Pair : HiddenWidgets)
	{
		if (UUserWidget* Widget = Pair.Key.Get())
		{
			Widget->SetVisibility(Pair.Value);
		}
	}
	HiddenWidgets.Reset();
	for (TActorIterator<AGRRoomShells> It(GetWorld()); It; ++It)
	{
		It->SetActorHiddenInGame(false);
	}
	bRiding = false;
	ShowPlayer(true);
	if (UGRClockSubsystem* Clock = GetGameInstance()->GetSubsystem<UGRClockSubsystem>())
	{
		Clock->SetRunning(true);
		Clock->SetIspuOverride(-1.f);
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->PlayerCameraManager->SetManualCameraFade(0.f, FLinearColor::Black, false);
		if (AGRPlayerController* GRPC = Cast<AGRPlayerController>(PC))
		{
			GRPC->ReturnToPlayView();
		}
	}
	if (Camera)
	{
		Camera->Destroy();
		Camera = nullptr;
	}
	if (bClose)
	{
		Close();
	}
}

void UGRTrailerScreen::NativeDestruct()
{
	Finish(false);
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
