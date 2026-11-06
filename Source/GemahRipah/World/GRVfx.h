#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GRVfx.generated.h"

class AActor;
class UInstancedStaticMeshComponent;

/**
 * Small effects drawn as sprites: exhaust, dust, sparks, steam, confetti. One pool of camera-facing planes per sprite
 * (Kenney textures in /Game/GemahRipah/VFX, Tools/Python/import_sfx.py); each sprite's tint and opacity go through the
 * instance custom data, so all of a pool's sprites are one draw. Sprites far from the camera are not made at all.
 */
UCLASS()
class GEMAHRIPAH_API UGRVfxSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UGRVfxSubsystem* Get(const UObject* WorldContext);

	/** One puff out of a tailpipe. Back = the way the pipe points. Strength 0-1 is how thick. */
	void Exhaust(const FVector& At, const FVector& Back, float Strength);
	/** A cloud of dust and grit kicked up at ground level. Size = cloud radius in cm. */
	void Dust(const FVector& At, float Size);
	/** Sparks off metal, thrown along Direction and up. */
	void Sparks(const FVector& At, const FVector& Direction, int32 Count);
	/** One wisp rising from something hot. */
	void Steam(const FVector& At);
	/** Paper thrown in the air. */
	void Confetti(const FVector& At, int32 Count);

	/** Sprites alive, for GRAudio. */
	int32 CountAlive() const;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGRVfxSubsystem, STATGROUP_Tickables); }

private:
	enum class ESprite : uint8 { Asap, Kerikil, Percikan, Kertas, Count };

	struct FSprite
	{
		FVector Pos = FVector::ZeroVector;
		FVector Vel = FVector::ZeroVector;
		FLinearColor Color = FLinearColor::White;
		float Age = 0.f;
		float Life = 1.f;
		float StartSize = 10.f;
		float EndSize = 10.f;
		/** Turn about the view axis, degrees, and its rate. Paper tumbles about all three axes instead. */
		float Spin = 0.f;
		float SpinRate = 0.f;
		/** Share of the speed lost per second. */
		float Drag = 0.f;
		/** cm/s2, negative pulls down. */
		float Lift = 0.f;
		/** Light of its own: not dimmed at night. */
		bool bGlow = false;
		int32 Slot = 0;
	};

	struct FPool
	{
		TObjectPtr<UInstancedStaticMeshComponent> Mesh;
		TArray<FSprite> Alive;
		TArray<int32> FreeSlots;
	};

	/** Null when the camera is too far from At. */
	FSprite* Add(ESprite Kind, const FVector& At);
	void EnsurePools();
	void TickPool(FPool& Pool, ESprite Kind, const FVector& Camera, float Light, float DeltaTime);

	UPROPERTY()
	TObjectPtr<AActor> Holder;

	FPool Pools[static_cast<int32>(ESprite::Count)];
};
