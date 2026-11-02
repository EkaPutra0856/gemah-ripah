#pragma once

#include "CoreMinimal.h"
#include "GRStoryTypes.generated.h"

// Content of Content/Data/bab1.json. "If" fields are conditions and "Do" fields are action lists,
// both evaluated by UGRStorySubsystem (see README, "Bahasa cerita").

USTRUCT()
struct FGRItemDef
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() FString Name;
	UPROPERTY() FString Desc;
	/** Actions when the player uses the item from the journal (empty = cannot be used). */
	UPROPERTY() FString Use;
	/** Evidence items count for the warnet post. */
	UPROPERTY() bool Bukti = false;
	/** Icon name in Content/Data/Icons (default item_<Id>). */
	UPROPERTY() FString Icon;
};

USTRUCT()
struct FGRShopEntry
{
	GENERATED_BODY()
	UPROPERTY() FString Item;
	UPROPERTY() FString Place;
	UPROPERTY() int32 Price = 0;
	/** Price rise per story day. */
	UPROPERTY() int32 PriceStepPerDay = 0;
};

USTRUCT()
struct FGRQuestDef
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() FString Title;
	UPROPERTY() FString Desc;
	UPROPERTY() TArray<FString> Steps;
	/** Spawn id the map marker points at, per step (may be shorter than Steps). */
	UPROPERTY() TArray<FString> Targets;
};

USTRUCT()
struct FGRDialogChoice
{
	GENERATED_BODY()
	UPROPERTY() FString Text;
	UPROPERTY() FString If;
	UPROPERTY() FString Do;
	UPROPERTY() FString Next;
};

USTRUCT()
struct FGRDialogNode
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() FString Speaker;
	UPROPERTY() FString Text;
	UPROPERTY() FString Do;
	UPROPERTY() FString Next;
	/** Node is skipped (go to Next) when this is false. */
	UPROPERTY() FString If;
	UPROPERTY() TArray<FGRDialogChoice> Choices;
};

USTRUCT()
struct FGRDialogDef
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	/** "vn" (portraits), "3d" (subtitle box over the world) or "chat" (phone messages). */
	UPROPERTY() FString Style = TEXT("3d");
	/** Optional story variable shown as a meter (0..100), e.g. interrogation pressure. */
	UPROPERTY() FString Meter;
	UPROPERTY() FString MeterLabel;
	UPROPERTY() TArray<FGRDialogNode> Nodes;
};

USTRUCT()
struct FGRSpeakerDef
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() FString Name;
	UPROPERTY() FString Role;
	UPROPERTY() FString Color = TEXT("#E8A33A");
};

USTRUCT()
struct FGRSpawnDef
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	/** "npc", "spn" (patrol with vision), "prop" (interactable) or "zone" (invisible trigger). */
	UPROPERTY() FString Kind = TEXT("prop");
	UPROPERTY() FString Name;
	/** x, y and optionally z (cm). */
	UPROPERTY() TArray<float> Pos;
	UPROPERTY() float Yaw = 0.f;
	/** Present in the world only while this holds. */
	UPROPERTY() FString If;
	UPROPERTY() FString Prompt;
	/** Actions on interact (prop/npc) or on enter (zone). */
	UPROPERTY() FString Do;
	/** Dialog started on interact when Do is empty. */
	UPROPERTY() FString Dialog;
	UPROPERTY() FString Color = TEXT("#B0A090");
	/** "cube", "cylinder", "sphere" or "none". */
	UPROPERTY() FString Shape = TEXT("cube");
	/** Size in cm (x, y, z). */
	UPROPERTY() TArray<float> Size;
	/** Zone radius in cm. */
	UPROPERTY() float Radius = 300.f;
	UPROPERTY() TArray<FString> Barks;
	/** NPC wander radius in cm (0 = stand still). */
	UPROPERTY() float Wander = 0.f;
	/** Patrol points as flat x, y pairs. */
	UPROPERTY() TArray<float> Patrol;
	UPROPERTY() FString Marker;
	UPROPERTY() FString MarkerColor = TEXT("#E8A33A");
};

USTRUCT()
struct FGREventDef
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() FString If;
	UPROPERTY() FString Do;
};

USTRUCT()
struct FGRCutStep
{
	GENERATED_BODY()
	/** Seconds. */
	UPROPERTY() float T = 3.f;
	/** Camera at the start of the step: x, y, z, pitch, yaw. Empty = keep. */
	UPROPERTY() TArray<float> Cam;
	/** Camera at the end of the step (empty = same as Cam). */
	UPROPERTY() TArray<float> Cam2;
	UPROPERTY() FString Title;
	UPROPERTY() FString Text;
	UPROPERTY() FString Speaker;
	/** Screen fade at the end of the step: 0 = clear, 1 = black, -1 = unchanged. */
	UPROPERTY() float Fade = -1.f;
	UPROPERTY() FString Do;
};

USTRUCT()
struct FGRCutsceneDef
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() TArray<FGRCutStep> Steps;
	UPROPERTY() FString End;
};

USTRUCT()
struct FGROrderDef
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() FString Title;
	UPROPERTY() FString Customer;
	/** Spawn ids. */
	UPROPERTY() FString From;
	UPROPERTY() FString To;
	UPROPERTY() int32 Fare = 15000;
	/** Delivery time limit in seconds (after pickup). */
	UPROPERTY() float Time = 90.f;
	UPROPERTY() FString If;
	UPROPERTY() FString Do;
};

USTRUCT()
struct FGRPostDef
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() FString If;
	UPROPERTY() FString Author;
	UPROPERTY() FString Handle;
	UPROPERTY() FString Text;
	UPROPERTY() int32 Likes = 0;
	UPROPERTY() bool Official = false;
};

USTRUCT()
struct FGRNewsDef
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() FString If;
	UPROPERTY() FString Source;
	UPROPERTY() FString Title;
	UPROPERTY() FString Body;
};

USTRUCT()
struct FGRNeedDef
{
	GENERATED_BODY()
	UPROPERTY() FString Item;
	UPROPERTY() FString If;
	UPROPERTY() FString Ok;
	UPROPERTY() FString Fail;
};

USTRUCT()
struct FGRBarkDef
{
	GENERATED_BODY()
	UPROPERTY() FString If;
	UPROPERTY() FString Text;
};

USTRUCT()
struct FGRPostOption
{
	GENERATED_BODY()
	/** "judul" or "nada". */
	UPROPERTY() FString Group;
	UPROPERTY() FString Text;
	UPROPERTY() int32 Viral = 0;
	UPROPERTY() int32 Risk = 0;
};

USTRUCT()
struct FGRMinigameDef
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() FString Title;
	UPROPERTY() FString Help;
	UPROPERTY() FString Win;
	UPROPERTY() FString Lose;
	/** Stealth layout: # wall, . floor, P player, G guard (patrols along its row), H guard (column), B evidence, X exit, C cover. */
	/** Stealth: top-down room in the map to build the grid in (MinigameId tag of the room volume, or the id of a spawn inside it). */
	UPROPERTY() FString Room;
	/** Stealth: name shown on the guards. */
	UPROPERTY() FString Guard;
	/** Post builder: label of the publish button. */
	UPROPERTY() FString Submit;
	UPROPERTY() TArray<FString> Grid;
	UPROPERTY() TArray<FString> Loot;
	UPROPERTY() TArray<FGRPostOption> Options;
};

/** One line of the chapter end screen: Text when If holds, else Else (either may be empty). */
USTRUCT()
struct FGREndingLine
{
	GENERATED_BODY()
	UPROPERTY() FString If;
	UPROPERTY() FString Text;
	UPROPERTY() FString Else;
	UPROPERTY() int32 Size = 16;
	UPROPERTY() FString Color;
};

/** Entry of chapters.json: one chapter on the chapter map. */
USTRUCT()
struct FGRChapterDef
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() int32 Number = 1;
	/** Story file in Content/Data. Empty = not written yet ("segera"). */
	UPROPERTY() FString File;
	UPROPERTY() FString Title;
	UPROPERTY() FString Subtitle;
	UPROPERTY() FString Summary;
	/** Icon name in Content/Data/Icons. */
	UPROPERTY() FString Icon;
	UPROPERTY() FString Color = TEXT("#E8A33A");
	/** Place on the chapter map, 0..1 from the top-left. */
	UPROPERTY() TArray<float> Pos;
	UPROPERTY() FString Place;
};

USTRUCT()
struct FGRChapterList
{
	GENERATED_BODY()
	UPROPERTY() TArray<FGRChapterDef> Chapters;
};

USTRUCT()
struct FGRStoryFile
{
	GENERATED_BODY()
	UPROPERTY() FString Title;
	/** Actions on a new game. */
	UPROPERTY() FString Start;
	/** Actions when Diawasi reaches the maximum. */
	UPROPERTY() FString OnCaught;
	/** Actions when energy runs out. */
	UPROPERTY() FString OnFaint;
	/** Actions every morning after sleeping (before the needs check). */
	UPROPERTY() FString OnMorning;
	/** Condition that makes SPN patrols react to the player. */
	UPROPERTY() FString Suspicious;
	UPROPERTY() TArray<FGRItemDef> Items;
	UPROPERTY() TArray<FGRShopEntry> Shop;
	UPROPERTY() TArray<FGRQuestDef> Quests;
	UPROPERTY() TArray<FGRSpeakerDef> Speakers;
	UPROPERTY() TArray<FGRDialogDef> Dialogs;
	UPROPERTY() TArray<FGRSpawnDef> Spawns;
	UPROPERTY() TArray<FGREventDef> Events;
	UPROPERTY() TArray<FGRCutsceneDef> Cutscenes;
	UPROPERTY() TArray<FGROrderDef> Orders;
	UPROPERTY() TArray<FGRPostDef> Kicau;
	UPROPERTY() TArray<FGRNewsDef> Berita;
	UPROPERTY() TArray<FGRNeedDef> Needs;
	UPROPERTY() TArray<FGRBarkDef> Barks;
	UPROPERTY() TArray<FGRMinigameDef> Minigames;
	/** Chapter end screen lines (after "selesai"). */
	UPROPERTY() TArray<FGREndingLine> Ending;
};
