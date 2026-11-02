#include "Core/GRPlayerController.h"
#include "Core/GRCheatManager.h"
#include "Camera/GRTopDownRoomVolume.h"
#include "Player/GRCharacter.h"
#include "Player/GRInteractionComponent.h"
#include "UI/GRMenuScreens.h"
#include "UI/GRUISubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "GemahRipah.h"

namespace
{
	UInputAction* MakeAction(UObject* Outer, const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = Type;
		return Action;
	}

	// WASD -> 2D axis: W/S need X and Y swapped, S and A are negated.
	void MapDirectionalKey(UInputMappingContext* Context, const UInputAction* Action, const FKey& Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		}
	}

	void MapLookKey(UInputMappingContext* Context, const UInputAction* Action, const FKey& Key)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		UInputModifierNegate* InvertY = NewObject<UInputModifierNegate>(Context);
		InvertY->bX = false;
		InvertY->bY = true;
		InvertY->bZ = false;
		Mapping.Modifiers.Add(InvertY);
	}
}

AGRPlayerController::AGRPlayerController()
{
	CheatClass = UGRCheatManager::StaticClass();
}

void AGRPlayerController::BuildInputAssets()
{
	if (DefaultContext)
	{
		return;
	}

	MoveAction = MakeAction(this, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(this, TEXT("IA_Look"), EInputActionValueType::Axis2D);
	ZoomAction = MakeAction(this, TEXT("IA_Zoom"), EInputActionValueType::Axis1D);
	JumpAction = MakeAction(this, TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(this, TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	InteractAction = MakeAction(this, TEXT("IA_Interact"), EInputActionValueType::Boolean);
	PauseAction = MakeAction(this, TEXT("IA_Pause"), EInputActionValueType::Boolean);

	DefaultContext = NewObject<UInputMappingContext>(this, TEXT("IMC_GemahRipah"));

	MapDirectionalKey(DefaultContext, MoveAction, EKeys::W, true, false);
	MapDirectionalKey(DefaultContext, MoveAction, EKeys::S, true, true);
	MapDirectionalKey(DefaultContext, MoveAction, EKeys::D, false, false);
	MapDirectionalKey(DefaultContext, MoveAction, EKeys::A, false, true);
	DefaultContext->MapKey(MoveAction, EKeys::Gamepad_Left2D);

	MapLookKey(DefaultContext, LookAction, EKeys::Mouse2D);
	MapLookKey(DefaultContext, LookAction, EKeys::Gamepad_Right2D);

	DefaultContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);

	DefaultContext->MapKey(JumpAction, EKeys::SpaceBar);
	DefaultContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);

	DefaultContext->MapKey(SprintAction, EKeys::LeftShift);
	DefaultContext->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);

	DefaultContext->MapKey(InteractAction, EKeys::E);
	DefaultContext->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);

	// P as well, because Escape ends Play-In-Editor.
	DefaultContext->MapKey(PauseAction, EKeys::Escape);
	DefaultContext->MapKey(PauseAction, EKeys::P);
	DefaultContext->MapKey(PauseAction, EKeys::Gamepad_Special_Right);
}

void AGRPlayerController::AddMappingContext()
{
	if (const ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(DefaultContext, 0);
		}
	}
}

void AGRPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	BuildInputAssets();
	AddMappingContext();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);
	if (!Input)
	{
		UE_LOG(LogGemahRipah, Error, TEXT("Enhanced Input component tidak ada. Cek DefaultInput.ini."));
		return;
	}

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGRPlayerController::HandleMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGRPlayerController::HandleLook);
	Input->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AGRPlayerController::HandleZoom);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AGRPlayerController::HandleJumpStarted);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AGRPlayerController::HandleJumpCompleted);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AGRPlayerController::HandleSprintStarted);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AGRPlayerController::HandleSprintCompleted);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AGRPlayerController::HandleInteract);
	Input->BindAction(PauseAction, ETriggerEvent::Started, this, &AGRPlayerController::HandlePause);
}

void AGRPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Safety net in case the local player was not ready during SetupInputComponent.
	if (IsLocalController())
	{
		BuildInputAssets();
		AddMappingContext();
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

void AGRPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CurrentRoom.Reset();
}

AGRCharacter* AGRPlayerController::GetGRCharacter() const
{
	return Cast<AGRCharacter>(GetPawn());
}

void AGRPlayerController::EnterTopDownRoom(AGRTopDownRoomVolume* Room)
{
	if (!Room)
	{
		return;
	}
	CurrentRoom = Room;
	SetViewTargetWithBlend(Room, Room->BlendTime, VTBlend_Cubic);
	if (AGRCharacter* GRCharacter = GetGRCharacter())
	{
		GRCharacter->SetTopDownMode(true);
	}
}

void AGRPlayerController::ExitTopDownRoom(AGRTopDownRoomVolume* Room)
{
	if (CurrentRoom.Get() != Room)
	{
		return;
	}
	CurrentRoom.Reset();
	SetViewTargetWithBlend(GetPawn(), Room ? Room->BlendTime : 0.5f, VTBlend_Cubic);
	if (AGRCharacter* GRCharacter = GetGRCharacter())
	{
		GRCharacter->SetTopDownMode(false);
	}
}

void AGRPlayerController::HandleMove(const FInputActionValue& Value)
{
	if (AGRCharacter* GRCharacter = GetGRCharacter())
	{
		GRCharacter->MoveFromInput(Value.Get<FVector2D>());
	}
}

void AGRPlayerController::HandleLook(const FInputActionValue& Value)
{
	if (AGRCharacter* GRCharacter = GetGRCharacter())
	{
		GRCharacter->LookFromInput(Value.Get<FVector2D>());
	}
}

void AGRPlayerController::HandleZoom(const FInputActionValue& Value)
{
	if (AGRCharacter* GRCharacter = GetGRCharacter())
	{
		GRCharacter->ZoomFromInput(Value.Get<float>());
	}
}

void AGRPlayerController::HandleJumpStarted()
{
	if (AGRCharacter* GRCharacter = GetGRCharacter())
	{
		GRCharacter->Jump();
	}
}

void AGRPlayerController::HandleJumpCompleted()
{
	if (AGRCharacter* GRCharacter = GetGRCharacter())
	{
		GRCharacter->StopJumping();
	}
}

void AGRPlayerController::HandleSprintStarted()
{
	if (AGRCharacter* GRCharacter = GetGRCharacter())
	{
		GRCharacter->SetSprinting(true);
	}
}

void AGRPlayerController::HandleSprintCompleted()
{
	if (AGRCharacter* GRCharacter = GetGRCharacter())
	{
		GRCharacter->SetSprinting(false);
	}
}

void AGRPlayerController::HandleInteract()
{
	if (AGRCharacter* GRCharacter = GetGRCharacter())
	{
		GRCharacter->GetInteraction()->TryInteract();
	}
}

void AGRPlayerController::HandlePause()
{
	UGRUISubsystem* UI = GetLocalPlayer() ? GetLocalPlayer()->GetSubsystem<UGRUISubsystem>() : nullptr;
	if (UI && !UI->HasScreens())
	{
		UI->PushScreen<UGRPauseScreen>();
	}
}
