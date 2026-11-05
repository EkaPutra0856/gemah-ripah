#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_RetargetPoseFromMesh.h"
#include "GRBody.generated.h"

class ACharacter;
class USkeletalMesh;
class USkeletalMeshComponent;
class UIKRetargeter;

/** Runs a single "retarget pose from mesh" node: no anim blueprint needed. */
USTRUCT()
struct FGRBodyAnimProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FGRBodyAnimProxy() = default;
	FGRBodyAnimProxy(UAnimInstance* Instance, FAnimNode_RetargetPoseFromMesh* InNode)
		: FAnimInstanceProxy(Instance), Node(InNode)
	{
	}

	virtual FAnimNode_Base* GetCustomRootNode() override { return Node; }
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override { OutNodes.Add(Node); }
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

	FAnimNode_RetargetPoseFromMesh* Node = nullptr;
	/** Copied from UGRBodyAnimInstance::SetGrip on the game thread, actor space. */
	bool bGrip = false;
	FVector Grip[2] = { FVector::ZeroVector, FVector::ZeroVector };
};

/**
 * Anim instance of a Tripo character's body: copies, every frame, the pose of the hidden UE mannequin it is
 * attached to (which still plays ABP_Unarmed and every MM_GR_* montage) through RTG_GR_MannyKeTripo.
 */
UCLASS(Transient, NotBlueprintable)
class UGRBodyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	void Follow(UIKRetargeter* Retargeter, USkeletalMeshComponent* Source);
	/**
	 * Puts the wrists (LeftHand, RightHand) on Left and Right, owner actor space, bending the arms with two-bone IK
	 * after the retargeted pose; for a cart's handlebar. bOn false lets the arms do what the animation does.
	 */
	void SetGrip(bool bOn, const FVector& Left = FVector::ZeroVector, const FVector& Right = FVector::ZeroVector);

	bool bGrip = false;
	FVector Grip[2] = { FVector::ZeroVector, FVector::ZeroVector };

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

	UPROPERTY(Transient)
	FAnimNode_RetargetPoseFromMesh Node;
};

/** The game's people: Tripo models (Art/_skrip_rig, Tools/Python/import_tripo_chars.py) over the mannequin rig. */
namespace GRBody
{
	/** /Game/GemahRipah/Characters/<Model>/SK_<Model>, or null when that model is not imported. */
	GEMAHRIPAH_API USkeletalMesh* Find(const FString& Model);

	/**
	 * Dresses a character in a Tripo model: the mannequin (GetMesh) keeps animating but is no longer drawn, and a
	 * second mesh on top of it copies its pose. Model null puts the mannequin back. Body is the owner's slot for
	 * that second mesh (created on first use). Returns true when the Tripo body is shown.
	 */
	GEMAHRIPAH_API bool Apply(ACharacter* Owner, TObjectPtr<USkeletalMeshComponent>& Body, USkeletalMesh* Model);

	/** Model for a story NPC, from its name in the chapter data (and "spn" guards). Empty when none fits. */
	GEMAHRIPAH_API FString ForStoryNpc(const FString& Name, const FString& Kind, const FString& Id);

	/** Model for a townsperson: Role 0 stroller, 1 jogger, 2 vendor, 3 SPN; Pick spreads them. */
	GEMAHRIPAH_API FString ForTownsperson(int32 Role, int32 Pick);
}
