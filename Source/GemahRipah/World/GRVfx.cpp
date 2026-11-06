#include "World/GRVfx.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Systems/GRClockSubsystem.h"

namespace
{
	constexpr int32 PoolSize = 192;
	/** No sprites beyond this distance from the camera (cm). */
	constexpr float MakeReach = 7000.f;
	/** The plane mesh is 100 cm across. */
	constexpr float PlaneSize = 100.f;
	/** Far under the map: where unused instances wait at scale zero. */
	const FVector Parked(0.f, 0.f, -100000.f);
	/** Daylight reaching a sprite at night, as a share of noon. */
	constexpr float NightLight = 0.25f;

	const TCHAR* const Materials[] = {
		TEXT("/Game/GemahRipah/VFX/MI_Vfx_Asap.MI_Vfx_Asap"),
		TEXT("/Game/GemahRipah/VFX/MI_Vfx_Kerikil.MI_Vfx_Kerikil"),
		TEXT("/Game/GemahRipah/VFX/MI_Vfx_Percikan.MI_Vfx_Percikan"),
		TEXT("/Game/GemahRipah/VFX/MI_Vfx_Kertas.MI_Vfx_Kertas"),
	};

	const FLinearColor ConfettiColors[] = {
		FLinearColor(0.9f, 0.1f, 0.1f), FLinearColor(0.95f, 0.8f, 0.1f), FLinearColor(0.15f, 0.55f, 0.9f),
		FLinearColor(0.2f, 0.75f, 0.3f), FLinearColor(0.95f, 0.95f, 0.95f),
	};

	FVector CameraSpot(const UWorld* World)
	{
		const APlayerController* PC = World->GetFirstPlayerController();
		return PC && PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
	}
}

UGRVfxSubsystem* UGRVfxSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UGRVfxSubsystem>() : nullptr;
}

bool UGRVfxSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UGRVfxSubsystem::EnsurePools()
{
	if (Holder)
	{
		return;
	}
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Holder = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	USceneComponent* Root = NewObject<USceneComponent>(Holder);
	Holder->SetRootComponent(Root);
	Root->RegisterComponent();
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	for (int32 Kind = 0; Kind < static_cast<int32>(ESprite::Count); ++Kind)
	{
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, Materials[Kind], nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Material || !Plane)
		{
			continue;
		}
		UInstancedStaticMeshComponent* Mesh = NewObject<UInstancedStaticMeshComponent>(Holder);
		Mesh->SetStaticMesh(Plane);
		Mesh->SetMaterial(0, Material);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetAbsolute(true, true, true);
		Mesh->NumCustomDataFloats = 4;
		Mesh->SetupAttachment(Root);
		Mesh->RegisterComponent();
		TArray<FTransform> Slots;
		Slots.Init(FTransform(FQuat::Identity, Parked, FVector::ZeroVector), PoolSize);
		Mesh->AddInstances(Slots, false, true);
		FPool& Pool = Pools[Kind];
		Pool.Mesh = Mesh;
		Pool.Alive.Reserve(PoolSize);
		for (int32 i = PoolSize - 1; i >= 0; --i)
		{
			Pool.FreeSlots.Add(i);
		}
	}
}

UGRVfxSubsystem::FSprite* UGRVfxSubsystem::Add(ESprite Kind, const FVector& At)
{
	if (FVector::DistSquared(At, CameraSpot(GetWorld())) > FMath::Square(MakeReach))
	{
		return nullptr;
	}
	EnsurePools();
	FPool& Pool = Pools[static_cast<int32>(Kind)];
	if (!Pool.Mesh || Pool.FreeSlots.IsEmpty())
	{
		return nullptr;
	}
	FSprite& Sprite = Pool.Alive.AddDefaulted_GetRef();
	Sprite.Slot = Pool.FreeSlots.Pop(EAllowShrinking::No);
	Sprite.Pos = At;
	Sprite.Spin = FMath::FRandRange(0.f, 360.f);
	return &Sprite;
}

void UGRVfxSubsystem::Exhaust(const FVector& At, const FVector& Back, float Strength)
{
	if (FSprite* S = Add(ESprite::Asap, At))
	{
		S->Vel = Back * FMath::FRandRange(40.f, 90.f) + FVector(0.f, 0.f, FMath::FRandRange(15.f, 35.f));
		S->Color = FLinearColor(0.3f, 0.3f, 0.32f, 0.45f * Strength);
		S->Life = FMath::FRandRange(0.7f, 1.2f);
		S->StartSize = 10.f;
		S->EndSize = FMath::FRandRange(45.f, 70.f);
		S->SpinRate = FMath::FRandRange(-40.f, 40.f);
		S->Drag = 1.5f;
	}
}

void UGRVfxSubsystem::Dust(const FVector& At, float Size)
{
	const int32 Clouds = FMath::Clamp(FMath::RoundToInt(Size / 12.f), 4, 12);
	for (int32 i = 0; i < Clouds; ++i)
	{
		const FVector Out = FMath::VRand().GetSafeNormal2D();
		if (FSprite* S = Add(ESprite::Asap, At + Out * Size * 0.25f))
		{
			S->Vel = Out * Size * FMath::FRandRange(2.f, 4.f) + FVector(0.f, 0.f, FMath::FRandRange(15.f, 50.f));
			S->Color = FLinearColor(0.38f, 0.32f, 0.24f, 0.42f);
			S->Life = FMath::FRandRange(0.6f, 1.2f);
			S->StartSize = Size * 0.4f;
			S->EndSize = Size * FMath::FRandRange(1.2f, 1.8f);
			S->SpinRate = FMath::FRandRange(-50.f, 50.f);
			S->Drag = 2.2f;
		}
	}
	for (int32 i = 0; i < Clouds / 2; ++i)
	{
		const FVector Out = FMath::VRand().GetSafeNormal2D();
		if (FSprite* S = Add(ESprite::Kerikil, At))
		{
			S->Vel = Out * FMath::FRandRange(80.f, 220.f) + FVector(0.f, 0.f, FMath::FRandRange(150.f, 320.f));
			S->Color = FLinearColor(0.32f, 0.27f, 0.21f, 0.9f);
			S->Life = FMath::FRandRange(0.4f, 0.7f);
			S->StartSize = S->EndSize = Size * 0.35f;
			S->SpinRate = FMath::FRandRange(-200.f, 200.f);
			S->Lift = -900.f;
		}
	}
}

void UGRVfxSubsystem::Sparks(const FVector& At, const FVector& Direction, int32 Count)
{
	for (int32 i = 0; i < Count; ++i)
	{
		if (FSprite* S = Add(ESprite::Percikan, At))
		{
			const FVector Throw = (Direction.GetSafeNormal() + FMath::VRand() * 0.7f + FVector(0.f, 0.f, 0.5f)).GetSafeNormal();
			S->Vel = Throw * FMath::FRandRange(250.f, 750.f);
			S->Color = FLinearColor(5.f, 2.6f, 0.7f, 1.f);
			S->Life = FMath::FRandRange(0.2f, 0.55f);
			S->StartSize = FMath::FRandRange(40.f, 70.f);
			S->EndSize = 12.f;
			S->Lift = -980.f;
			S->Drag = 0.6f;
			S->bGlow = true;
		}
	}
}

void UGRVfxSubsystem::Steam(const FVector& At)
{
	if (FSprite* S = Add(ESprite::Asap, At))
	{
		S->Vel = FVector(FMath::FRandRange(-8.f, 8.f), FMath::FRandRange(-8.f, 8.f), FMath::FRandRange(35.f, 55.f));
		S->Color = FLinearColor(0.7f, 0.7f, 0.7f, 0.3f);
		S->Life = FMath::FRandRange(1.3f, 2.f);
		S->StartSize = 12.f;
		S->EndSize = FMath::FRandRange(50.f, 75.f);
		S->SpinRate = FMath::FRandRange(-25.f, 25.f);
	}
}

void UGRVfxSubsystem::Confetti(const FVector& At, int32 Count)
{
	for (int32 i = 0; i < Count; ++i)
	{
		if (FSprite* S = Add(ESprite::Kertas, At))
		{
			const FVector Out = FMath::VRand().GetSafeNormal2D();
			S->Vel = Out * FMath::FRandRange(60.f, 260.f) + FVector(0.f, 0.f, FMath::FRandRange(180.f, 420.f));
			S->Color = ConfettiColors[FMath::RandRange(0, UE_ARRAY_COUNT(ConfettiColors) - 1)];
			S->Color.A = 1.f;
			S->Life = FMath::FRandRange(2.f, 3.5f);
			S->StartSize = S->EndSize = FMath::FRandRange(6.f, 10.f);
			S->SpinRate = FMath::FRandRange(200.f, 500.f);
			S->Lift = -260.f;
			S->Drag = 1.4f;
		}
	}
}

int32 UGRVfxSubsystem::CountAlive() const
{
	int32 Count = 0;
	for (const FPool& Pool : Pools)
	{
		Count += Pool.Alive.Num();
	}
	return Count;
}

void UGRVfxSubsystem::TickPool(FPool& Pool, ESprite Kind, const FVector& Camera, float Light, float DeltaTime)
{
	if (Pool.Alive.IsEmpty())
	{
		return;
	}
	const bool bPaper = Kind == ESprite::Kertas;
	for (int32 i = Pool.Alive.Num() - 1; i >= 0; --i)
	{
		FSprite& S = Pool.Alive[i];
		S.Age += DeltaTime;
		if (S.Age >= S.Life)
		{
			Pool.Mesh->UpdateInstanceTransform(S.Slot, FTransform(FQuat::Identity, Parked, FVector::ZeroVector), true, false, true);
			Pool.FreeSlots.Add(S.Slot);
			Pool.Alive.RemoveAtSwap(i, EAllowShrinking::No);
			continue;
		}
		S.Vel *= FMath::Max(0.f, 1.f - S.Drag * DeltaTime);
		S.Vel.Z += S.Lift * DeltaTime;
		S.Pos += S.Vel * DeltaTime;
		S.Spin += S.SpinRate * DeltaTime;

		const float T = S.Age / S.Life;
		FQuat Turn;
		if (bPaper)
		{
			Turn = FRotator(S.Spin, S.Spin * 0.7f, S.Spin * 1.3f).Quaternion();
		}
		else
		{
			const FVector ToCamera = (Camera - S.Pos).GetSafeNormal();
			const FQuat Facing = FRotationMatrix::MakeFromZX(ToCamera, FVector::UpVector + FVector(0.01f, 0.f, 0.f)).ToQuat();
			Turn = FQuat(ToCamera, FMath::DegreesToRadians(S.Spin)) * Facing;
		}
		const float Size = FMath::Lerp(S.StartSize, S.EndSize, 1.f - FMath::Square(1.f - T));
		Pool.Mesh->UpdateInstanceTransform(S.Slot, FTransform(Turn, S.Pos, FVector(Size / PlaneSize)), true, false, true);

		const float FadeIn = bPaper ? 1.f : FMath::Clamp(T / 0.12f, 0.f, 1.f);
		const float FadeOut = FMath::Clamp((1.f - T) / (bPaper ? 0.2f : 0.6f), 0.f, 1.f);
		const float Bright = S.bGlow ? 1.f : Light;
		Pool.Mesh->SetCustomDataValue(S.Slot, 0, S.Color.R * Bright, false);
		Pool.Mesh->SetCustomDataValue(S.Slot, 1, S.Color.G * Bright, false);
		Pool.Mesh->SetCustomDataValue(S.Slot, 2, S.Color.B * Bright, false);
		Pool.Mesh->SetCustomDataValue(S.Slot, 3, S.Color.A * FadeIn * FadeOut, false);
	}
	Pool.Mesh->MarkRenderStateDirty();
}

void UGRVfxSubsystem::Tick(float DeltaTime)
{
	if (!Holder)
	{
		return;
	}
	UWorld* World = GetWorld();
	const FVector Camera = CameraSpot(World);
	const UGRClockSubsystem* Clock = World->GetGameInstance()->GetSubsystem<UGRClockSubsystem>();
	const float Hour = Clock ? Clock->GetHours() : 12.f;
	const float Day = FMath::Clamp(Hour - 5.f, 0.f, 1.f) * FMath::Clamp((18.5f - Hour) / 1.5f, 0.f, 1.f);
	const float Light = FMath::Lerp(NightLight, 1.f, Day);
	for (int32 Kind = 0; Kind < static_cast<int32>(ESprite::Count); ++Kind)
	{
		TickPool(Pools[Kind], static_cast<ESprite>(Kind), Camera, Light, DeltaTime);
	}
}
