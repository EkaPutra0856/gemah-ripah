#include "World/GRMusic.h"
#include "Player/GRCharacter.h"
#include "Story/GRStoryUtil.h"
#include "Systems/GRNotificationSubsystem.h"
#include "UI/GRMenuButton.h"
#include "UI/GRUIStyle.h"
#include "UI/GRUISubsystem.h"
#include "World/GRStoryProp.h"
#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

namespace
{
	const TCHAR* KampanyeSong = TEXT("/Game/GemahRipah/Audio/Musik/MS_Kampanye_OkeGas.MS_Kampanye_OkeGas");

	/** Radio state for the whole session (survives map reloads). */
	struct FRadioState
	{
		uint8 Unlocked = 0b11;
		int32 Current = 0;
		bool bOn = true;
		float Volume = 0.7f;
		TSet<FString> SpeakersOff;
	};
	FRadioState Radio;

	TArray<UGRMusicSubsystem::FSlot> MakeSlots()
	{
		TArray<UGRMusicSubsystem::FSlot> Slots;
		Slots.SetNum(UGRMusicSubsystem::SlotCount);
		Slots[0] = { LOCTEXT("Song0", "Seventeen (cover)"), LOCTEXT("Song0Info", "Lagu ngojek"), TEXT("/Game/GemahRipah/Audio/Musik/MS_Radio_Seventeen.MS_Radio_Seventeen") };
		Slots[1] = { LOCTEXT("Song1", "Pak Vramroro"), LOCTEXT("Song1Info", "Lagu ngojek"), TEXT("/Game/GemahRipah/Audio/Musik/MS_Radio_PakVramroro.MS_Radio_PakVramroro") };
		for (int32 i = 2; i < Slots.Num(); ++i)
		{
			Slots[i] = { LOCTEXT("SongLocked", "Terkunci"), LOCTEXT("SongLockedInfo", "Belum dibuka"), nullptr };
		}
		return Slots;
	}

	USoundBase* LoadSong(const TCHAR* Path)
	{
		return Path ? LoadObject<USoundBase>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
	}

	FVector FindGround(UWorld* World, const FVector& At, float From, const AActor* Ignore = nullptr)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GRSpeakerGround), false, Ignore);
		const FVector Start(At.X, At.Y, At.Z + From);
		if (World->LineTraceSingleByChannel(Hit, Start, Start - FVector(0.f, 0.f, From + 600.f), ECC_Visibility, Params))
		{
			return Hit.ImpactPoint;
		}
		return FVector(At.X, At.Y, 0.f);
	}
}

// ---------------------------------------------------------------- Speaker

AGRSpeaker::AGRSpeaker()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(30.f, 30.f, 40.f));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionObjectType(ECC_WorldDynamic);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = Box;

	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(Box);
	Audio->bAutoActivate = false;
	Audio->bAllowSpatialization = true;
}

UStaticMeshComponent* AGRSpeaker::AddPart(const TCHAR* Shape, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color)
{
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Shape);
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
	Part->SetStaticMesh(Mesh);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetupAttachment(Box);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(Scale);
	Part->SetMaterial(0, GRStoryUtil::MakeColorMaterial(this, Color, 0.9f));
	Part->RegisterComponent();
	return Part;
}

void AGRSpeaker::Setup(const FString& InId, USoundBase* Song, bool bToa, float Volume, float Reach)
{
	SpeakerId = InId;
	bToaModel = bToa;
	const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* Cylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* Cone = TEXT("/Engine/BasicShapes/Cone.Cone");
	if (bToa)
	{
		// Pole with two horn loudspeakers facing both ways along the road, and an amplifier box at its foot.
		Box->SetBoxExtent(FVector(40.f, 40.f, 60.f));
		Box->SetRelativeLocation(FVector::ZeroVector);
		AddPart(Cylinder, FVector(0.f, 0.f, 160.f), FRotator::ZeroRotator, FVector(0.08f, 0.08f, 3.2f), FLinearColor(FColor(90, 90, 92)));
		AddPart(Cone, FVector(28.f, 0.f, 300.f), FRotator(-90.f, 0.f, 0.f), FVector(0.45f, 0.45f, 0.55f), FLinearColor(FColor(225, 225, 215)));
		AddPart(Cone, FVector(-28.f, 0.f, 300.f), FRotator(90.f, 0.f, 0.f), FVector(0.45f, 0.45f, 0.55f), FLinearColor(FColor(225, 225, 215)));
		AddPart(Cube, FVector(0.f, 0.f, 25.f), FRotator::ZeroRotator, FVector(0.6f, 0.45f, 0.5f), FLinearColor(FColor(30, 30, 32)));
		AddPart(Cube, FVector(30.5f, 0.f, 30.f), FRotator::ZeroRotator, FVector(0.02f, 0.3f, 0.3f), FLinearColor(FColor(70, 70, 70)));
	}
	else
	{
		// Small transistor radio: body, speaker grille, dial and antenna.
		Box->SetBoxExtent(FVector(25.f, 25.f, 25.f));
		AddPart(Cube, FVector(0.f, 0.f, 11.f), FRotator::ZeroRotator, FVector(0.38f, 0.14f, 0.22f), FLinearColor(FColor(120, 40, 30)));
		AddPart(Cylinder, FVector(-8.f, -7.5f, 11.f), FRotator(0.f, 0.f, 90.f), FVector(0.14f, 0.14f, 0.02f), FLinearColor(FColor(40, 36, 32)));
		AddPart(Cube, FVector(11.f, -7.3f, 14.f), FRotator::ZeroRotator, FVector(0.1f, 0.02f, 0.06f), FLinearColor(FColor(230, 210, 150)));
		AddPart(Cylinder, FVector(14.f, 0.f, 40.f), FRotator(0.f, 0.f, 15.f), FVector(0.01f, 0.01f, 0.4f), FLinearColor(FColor(190, 190, 190)));
	}

	USoundAttenuation* Attenuation = NewObject<USoundAttenuation>(this);
	Attenuation->Attenuation.bAttenuate = true;
	Attenuation->Attenuation.bSpatialize = true;
	Attenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
	Attenuation->Attenuation.AttenuationShapeExtents = FVector(Reach * 0.2f);
	Attenuation->Attenuation.FalloffDistance = Reach;
	Audio->AttenuationSettings = Attenuation;
	Audio->SetSound(Song);
	Audio->SetVolumeMultiplier(Volume);
	if (USoundWave* Wave = Cast<USoundWave>(Song))
	{
		Wave->bLooping = true;
	}
	SetSpeakerOn(bOn);
}

void AGRSpeaker::SetSpeakerOn(bool bInOn)
{
	bOn = bInOn;
	if (!Audio->Sound)
	{
		return;
	}
	if (bOn && !Audio->IsPlaying())
	{
		// Start somewhere in the song, as if it had been playing for a while.
		const USoundWave* Wave = Cast<USoundWave>(Audio->Sound);
		const float Duration = Wave ? Wave->Duration : 0.f;
		Audio->Play(Duration > 10.f && Duration < 10000.f ? FMath::FRandRange(0.f, Duration * 0.8f) : 0.f);
	}
	else if (!bOn)
	{
		Audio->FadeOut(0.4f, 0.f);
	}
}

FText AGRSpeaker::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	if (bToaModel)
	{
		return bOn ? LOCTEXT("ToaOff", "Matikan speaker") : LOCTEXT("ToaOn", "Nyalakan speaker");
	}
	return bOn ? LOCTEXT("RadioOff", "Matikan radio") : LOCTEXT("RadioOn", "Nyalakan radio");
}

bool AGRSpeaker::CanInteract_Implementation(AActor* Interactor) const
{
	return !IsHidden();
}

void AGRSpeaker::Interact_Implementation(AActor* Interactor)
{
	SetSpeakerOn(!bOn);
	if (UGRMusicSubsystem* Music = UGRMusicSubsystem::Get(this))
	{
		Music->SetSpeakerOff(SpeakerId, !bOn);
	}
}

// ---------------------------------------------------------------- Subsystem

UGRMusicSubsystem* UGRMusicSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UGRMusicSubsystem>() : nullptr;
}

const UGRMusicSubsystem::FSlot& UGRMusicSubsystem::GetSlot(int32 Index)
{
	static const TArray<FSlot> Slots = MakeSlots();
	return Slots[FMath::Clamp(Index, 0, SlotCount - 1)];
}

bool UGRMusicSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

bool UGRMusicSubsystem::IsUnlocked(int32 Index) const
{
	return Index >= 0 && Index < SlotCount && (Radio.Unlocked & (1 << Index)) && GetSlot(Index).Path;
}

void UGRMusicSubsystem::Unlock(int32 Index)
{
	if (Index >= 0 && Index < SlotCount)
	{
		Radio.Unlocked |= (1 << Index);
	}
}

bool UGRMusicSubsystem::IsRadioOn() const { return Radio.bOn; }
int32 UGRMusicSubsystem::GetCurrent() const { return Radio.Current; }
float UGRMusicSubsystem::GetVolume() const { return Radio.Volume; }

void UGRMusicSubsystem::GetProgress(float& Seconds, float& Duration) const
{
	Seconds = bRadioPlaying ? SongSeconds : 0.f;
	Duration = SongDuration;
}

bool UGRMusicSubsystem::CanUseRadio() const
{
	const AGRCharacter* Bayu = Cast<AGRCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	return Bayu && Bayu->IsRiding();
}

void UGRMusicSubsystem::PlaySlot(int32 Index)
{
	if (!IsUnlocked(Index))
	{
		return;
	}
	Radio.Current = Index;
	Radio.bOn = true;
	StartRadioSong();
}

void UGRMusicSubsystem::Next()
{
	for (int32 Step = 1; Step <= SlotCount; ++Step)
	{
		const int32 Index = (Radio.Current + Step) % SlotCount;
		if (IsUnlocked(Index))
		{
			PlaySlot(Index);
			return;
		}
	}
}

void UGRMusicSubsystem::SetRadioOn(bool bOn)
{
	Radio.bOn = bOn;
	if (!bOn && RadioAudio)
	{
		RadioAudio->FadeOut(0.5f, 0.f);
		bRadioPlaying = false;
	}
}

void UGRMusicSubsystem::SetVolume(float Volume)
{
	Radio.Volume = FMath::Clamp(Volume, 0.f, 1.f);
	if (RadioAudio)
	{
		RadioAudio->SetVolumeMultiplier(FMath::Max(Radio.Volume, 0.001f));
	}
}

bool UGRMusicSubsystem::IsSpeakerOff(const FString& Id) const { return Radio.SpeakersOff.Contains(Id); }

void UGRMusicSubsystem::SetSpeakerOff(const FString& Id, bool bOff)
{
	if (bOff)
	{
		Radio.SpeakersOff.Add(Id);
	}
	else
	{
		Radio.SpeakersOff.Remove(Id);
	}
}

void UGRMusicSubsystem::StartRadioSong()
{
	USoundBase* Song = LoadSong(GetSlot(Radio.Current).Path);
	if (!Song)
	{
		return;
	}
	if (!RadioAudio)
	{
		RadioAudio = UGameplayStatics::CreateSound2D(GetWorld(), Song, 1.f, 1.f, 0.f, nullptr, false, false);
		if (!RadioAudio)
		{
			return;
		}
		// Keeps playing under the radio screen and the pause menu.
		RadioAudio->bIsUISound = true;
	}
	RadioAudio->Stop();
	RadioAudio->SetSound(Song);
	RadioAudio->SetVolumeMultiplier(FMath::Max(Radio.Volume, 0.001f));
	RadioAudio->FadeIn(0.6f, 1.f, 0.f);
	SongSeconds = 0.f;
	SongDuration = Song->GetDuration();
	bRadioPlaying = true;
	if (UGRNotificationSubsystem* Notify = GetWorld()->GetGameInstance()->GetSubsystem<UGRNotificationSubsystem>())
	{
		Notify->Notify(FText::Format(LOCTEXT("RadioNow", "Radio: {0}"), GetSlot(Radio.Current).Title));
	}
}

void UGRMusicSubsystem::UpdateRadio(float DeltaTime)
{
	const bool bShould = Radio.bOn && CanUseRadio();
	if (!bShould)
	{
		if (bRadioPlaying && RadioAudio)
		{
			// Got off the motor: stop; the next ride starts the next song.
			RadioAudio->FadeOut(0.8f, 0.f);
			bRadioPlaying = false;
			Radio.Current = (Radio.Current + 1) % SlotCount;
			if (!IsUnlocked(Radio.Current))
			{
				Radio.Current = 0;
			}
		}
		return;
	}
	if (!bRadioPlaying)
	{
		StartRadioSong();
		return;
	}
	SongSeconds += DeltaTime;
	if (SongSeconds > SongDuration + 0.5f || (RadioAudio && !RadioAudio->IsPlaying() && SongSeconds > 1.f))
	{
		Next();
	}
}

AGRSpeaker* UGRMusicSubsystem::SpawnSpeaker(const FString& Id, const TCHAR* SongPath, const FVector& Ground, float Yaw, bool bToa, float Volume, float Reach)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGRSpeaker* Speaker = GetWorld()->SpawnActor<AGRSpeaker>(AGRSpeaker::StaticClass(), FTransform(FRotator(0.f, Yaw, 0.f), Ground), Params);
	if (Speaker)
	{
		Speaker->SetSpeakerOn(!IsSpeakerOff(Id));
		Speaker->Setup(Id, LoadSong(SongPath), bToa, Volume, Reach);
		Speakers.Add(Id, Speaker);
		UE_LOG(LogTemp, Display, TEXT("GRMusic speaker %s at %s"), *Id, *Ground.ToString());
	}
	return Speaker;
}

void UGRMusicSubsystem::UpdateSpeakers()
{
	UWorld* World = GetWorld();
	if (!World->GetMapName().Contains(TEXT("KotaSentosa")))
	{
		return;
	}

	// The radio in the kelurahan office, on the floor beside Pak Darmo's desk.
	if (!Speakers.Contains(TEXT("radio_kelurahan")))
	{
		const FVector Ground = FindGround(World, FVector(8850.f, -8940.f, 0.f), 170.f);
		SpawnSpeaker(TEXT("radio_kelurahan"), KampanyeSong, Ground, 200.f, false, 0.45f, 1400.f);
	}

	// A toa at both ends of every campaign stage, shown only while the stage is.
	for (TActorIterator<AGRStoryProp> It(World); It; ++It)
	{
		AGRStoryProp* Stage = *It;
		if (!Stage->SpawnId.StartsWith(TEXT("panggung")))
		{
			continue;
		}
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const FString Id = FString::Printf(TEXT("toa_%s_%d"), *Stage->SpawnId, Side);
			AGRSpeaker* Speaker = Speakers.FindRef(Id);
			if (!Speaker)
			{
				const FVector At = Stage->GetActorTransform().TransformPosition(FVector(0.f, Side == 0 ? 540.f : -540.f, 0.f));
				Speaker = SpawnSpeaker(Id, KampanyeSong, FindGround(World, At, 300.f, Stage), Stage->GetActorRotation().Yaw, true, 1.f, 4500.f);
			}
			if (Speaker)
			{
				const bool bShown = !Stage->IsHidden();
				if (Speaker->IsHidden() == bShown)
				{
					Speaker->SetActorHiddenInGame(!bShown);
					Speaker->SetActorEnableCollision(bShown);
					Speaker->SetSpeakerOn(bShown && !IsSpeakerOff(Id));
				}
			}
		}
	}
}

void UGRMusicSubsystem::Tick(float DeltaTime)
{
	UpdateRadio(DeltaTime);
	SpeakerCheck -= DeltaTime;
	if (SpeakerCheck <= 0.f)
	{
		SpeakerCheck = 2.f;
		UpdateSpeakers();
	}
}

// ---------------------------------------------------------------- Radio screen

void UGRRadioScreen::BuildScreen(UOverlay* Root)
{
	UVerticalBox* Box = MakeCenteredPanel(Root, LOCTEXT("RadioTitle", "Radio Motor"), 520.f);

	NowText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 17, GRStyle::ColorAccent, true);
	Box->AddChildToVerticalBox(NowText)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

	for (int32 i = 0; i < UGRMusicSubsystem::SlotCount; ++i)
	{
		UGRMenuButton* Button = AddButton(Box, FText::GetEmpty(), [this, i]()
		{
			if (UGRMusicSubsystem* Music = UGRMusicSubsystem::Get(this))
			{
				Music->PlaySlot(i);
				Refresh();
			}
		});
		SlotButtons.Add(Button);
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Box->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	PowerButton = AddButton(Row, FText::GetEmpty(), [this]()
	{
		if (UGRMusicSubsystem* Music = UGRMusicSubsystem::Get(this))
		{
			Music->SetRadioOn(!Music->IsRadioOn());
			Refresh();
		}
	});
	AddButton(Row, LOCTEXT("VolDown", "Vol -"), [this]()
	{
		if (UGRMusicSubsystem* Music = UGRMusicSubsystem::Get(this))
		{
			Music->SetVolume(Music->GetVolume() - 0.1f);
			Refresh();
		}
	});
	AddButton(Row, LOCTEXT("VolUp", "Vol +"), [this]()
	{
		if (UGRMusicSubsystem* Music = UGRMusicSubsystem::Get(this))
		{
			Music->SetVolume(Music->GetVolume() + 0.1f);
			Refresh();
		}
	});
	VolumeText = GRStyle::MakeText(WidgetTree, FText::GetEmpty(), 15, GRStyle::ColorTextDim);
	Box->AddChildToVerticalBox(VolumeText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

	UTextBlock* Hint = GRStyle::MakeText(WidgetTree, LOCTEXT("RadioHint", "1-8 pilih lagu  |  0 nyala/mati  |  - + volume  |  R/Esc tutup"), 13, GRStyle::ColorTextDim);
	Box->AddChildToVerticalBox(Hint)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

	AddButton(Box, LOCTEXT("RadioClose", "Tutup"), [this]() { Close(); });
	Refresh();
}

void UGRRadioScreen::Refresh()
{
	const UGRMusicSubsystem* Music = UGRMusicSubsystem::Get(this);
	if (!Music)
	{
		return;
	}
	for (int32 i = 0; i < SlotButtons.Num(); ++i)
	{
		const UGRMusicSubsystem::FSlot& SongSlot = UGRMusicSubsystem::GetSlot(i);
		const bool bOpen = Music->IsUnlocked(i);
		const bool bPlaying = bOpen && Music->IsRadioOn() && Music->GetCurrent() == i;
		SlotButtons[i]->SetLabel(FText::Format(LOCTEXT("SlotLabel", "{0}.  {1}{2}"), FText::AsNumber(i + 1),
			bOpen ? SongSlot.Title : LOCTEXT("SlotLocked", "[Terkunci]"), bPlaying ? LOCTEXT("SlotPlaying", "   (diputar)") : FText::GetEmpty()));
		SlotButtons[i]->SetIsEnabled(bOpen);
	}
	PowerButton->SetLabel(Music->IsRadioOn() ? LOCTEXT("RadioPowerOff", "Matikan") : LOCTEXT("RadioPowerOn", "Nyalakan"));
	PowerButton->SetDanger(Music->IsRadioOn());
	VolumeText->SetText(FText::Format(LOCTEXT("RadioVolume", "Volume {0}%"), FText::AsNumber(FMath::RoundToInt(Music->GetVolume() * 100.f))));
}

void UGRRadioScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const UGRMusicSubsystem* Music = UGRMusicSubsystem::Get(this);
	if (!Music || !NowText)
	{
		return;
	}
	if (!Music->IsRadioOn())
	{
		NowText->SetText(LOCTEXT("RadioOffNow", "Radio mati"));
		return;
	}
	float Seconds = 0.f;
	float Duration = 0.f;
	Music->GetProgress(Seconds, Duration);
	auto Clock = [](float S)
	{
		const int32 T = FMath::Max(0, FMath::FloorToInt(S));
		return FText::FromString(FString::Printf(TEXT("%d:%02d"), T / 60, T % 60));
	};
	NowText->SetText(FText::Format(LOCTEXT("RadioNowPlaying", "Diputar: {0}   {1} / {2}"),
		UGRMusicSubsystem::GetSlot(Music->GetCurrent()).Title, Clock(Seconds), Clock(Duration)));
}

FReply UGRRadioScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	UGRMusicSubsystem* Music = UGRMusicSubsystem::Get(this);
	const FKey Key = InKeyEvent.GetKey();
	static const FKey Numbers[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight };
	for (int32 i = 0; i < UE_ARRAY_COUNT(Numbers); ++i)
	{
		if (Key == Numbers[i] && Music)
		{
			Music->PlaySlot(i);
			Refresh();
			return FReply::Handled();
		}
	}
	if (Music && (Key == EKeys::Zero))
	{
		Music->SetRadioOn(!Music->IsRadioOn());
		Refresh();
		return FReply::Handled();
	}
	if (Music && (Key == EKeys::Hyphen || Key == EKeys::Subtract))
	{
		Music->SetVolume(Music->GetVolume() - 0.1f);
		Refresh();
		return FReply::Handled();
	}
	if (Music && (Key == EKeys::Equals || Key == EKeys::Add))
	{
		Music->SetVolume(Music->GetVolume() + 0.1f);
		Refresh();
		return FReply::Handled();
	}
	if (Key == EKeys::R)
	{
		Close();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
