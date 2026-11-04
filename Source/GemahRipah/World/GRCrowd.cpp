#include "World/GRCrowd.h"
#include "Camera/GRTopDownRoomVolume.h"
#include "Player/GRCharacter.h"
#include "Story/GRStorySubsystem.h"
#include "Story/GRStoryTypes.h"
#include "Story/GRStoryUtil.h"
#include "Systems/GRClockSubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "World/GRNpc.h"
#include "World/GRPropModels.h"
#include "World/GRStealthLevel.h"
#include "World/GRStoryProp.h"
#include "World/GRTraffic.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "GemahRipah"

DEFINE_LOG_CATEGORY_STATIC(LogGRCrowd, Log, All);

namespace
{
	// Pavement strips (400 wide, top z 12) run either side of Jl. Raya (x) and Jl. Merdeka (y).
	constexpr float PavementLine = 1000.f;
	constexpr float PavementHalf = 200.f;
	/** Benches stand at the back of the pavement, facing the road. */
	constexpr float BenchLine = 1160.f;
	constexpr float RoadEdge = 815.f;
	/** Pavement walks stay clear of the roundabout asphalt (r 2500) and the ring roads at 27000. */
	constexpr float AlongNear = 2800.f;
	constexpr float AlongFar = 25000.f;
	constexpr float RingRoadLine = 27000.f;
	constexpr float MapLimit = 26400.f;
	constexpr float CapsuleHalf = 96.f;
	constexpr float RecycleRange = 6500.f;
	constexpr int32 PoolSize = 44;
	constexpr int32 MaxVendors = 2;
	/** Walking pace of MM_GR_GerobakJalan at rate 1, cm/s (make_talk_anims.py logs it as speed=). */
	constexpr float CartWalkSpeed = 68.f;

	const TCHAR* const ShirtColors[] = {
		TEXT("#F2F2EE"), TEXT("#8E2C2C"), TEXT("#2B4C7E"), TEXT("#6E8FB5"), TEXT("#5E6B3A"), TEXT("#C99A2E"),
		TEXT("#D88CA0"), TEXT("#7B5A3C"), TEXT("#3E7F6B"), TEXT("#B5462E"), TEXT("#9A9A9A"), TEXT("#E5D3A8") };
	const TCHAR* const PantsColors[] = {
		TEXT("#2E2E33"), TEXT("#23324F"), TEXT("#6B5B45"), TEXT("#1A1A1A"), TEXT("#4A4F57"), TEXT("#7D6E57") };
	const TCHAR* const SportColors[] = { TEXT("#F07F2A"), TEXT("#9ACD32"), TEXT("#2BB3C9"), TEXT("#E84A6F"), TEXT("#F2F2EE") };
	const TCHAR* const ScarfColors[] = {
		TEXT("#1F2A44"), TEXT("#C94F6D"), TEXT("#E8D9C0"), TEXT("#3E6E5E"), TEXT("#7A3B5E"), TEXT("#B9895A"), TEXT("#F2F2EE"), TEXT("#5B6FA8") };
	const TCHAR* const SkinColors[] = { TEXT("#C68E63"), TEXT("#A8714A"), TEXT("#8D5B3C"), TEXT("#D2A07A"), TEXT("#B98058") };
	const TCHAR* const CapColors[] = { TEXT("#B03A2E"), TEXT("#2B4C7E"), TEXT("#1A1A1A"), TEXT("#E8E2D2"), TEXT("#3E7F6B") };

	struct FVendorKind
	{
		const TCHAR* Cry;
		const TCHAR* Color;
	};
	const FVendorKind DayVendors[] = {
		{ TEXT("Baksooo! Bakso!"), TEXT("#2F7FB8") },
		{ TEXT("Sateee, sate ayam!"), TEXT("#C0392B") },
		{ TEXT("Es dawet, seger!"), TEXT("#3E9E5A") },
		{ TEXT("Gorengan anget, gorengan!"), TEXT("#D9A21B") },
		{ TEXT("Siomay, siomay!"), TEXT("#8E44AD") } };
	const FVendorKind NightVendor = { TEXT("Nasi goreng! Tek, tek, tek!"), TEXT("#B5462E") };

	const TCHAR* const OpenerLines[] = {
		TEXT("Harga cabai naik lagi, Bu."),
		TEXT("Katanya jalan ini mau dilebarkan. Rumahku kena nggak, ya?"),
		TEXT("Anakku daftar sekolah, kok ada uang gedung lagi."),
		TEXT("Semalam listrik padam lagi."),
		TEXT("Baliho makin banyak, lampu jalan makin sedikit."),
		TEXT("Gaji UMR, cicilannya bukan UMR."),
		TEXT("Hati-hati, ada razia di perempatan."),
		TEXT("Bensin naik, ojek sepi."),
		TEXT("Bantuan sosialnya cuma sampai RT sebelah."),
		TEXT("Jalan baru diaspal, sudah bolong lagi."),
		TEXT("Got depan rumah mampet. Laporannya belum dibalas."),
		TEXT("Ngopi dulu. Mikir negara belakangan."),
		TEXT("Kalau mau pemilu, sembako tiba-tiba murah."),
		TEXT("Katanya ekonomi tumbuh. Tumbuh di mana, ya?"),
		TEXT("Kemarin ada yang dipanggil polisi gara-gara status."),
		TEXT("Pasar mau direnovasi. Yang jualan disuruh ke mana?") };
	const TCHAR* const ReplyLines[] = {
		TEXT("Iya, betul itu."),
		TEXT("Wah, parah."),
		TEXT("Masa, sih?"),
		TEXT("Ya gimana lagi."),
		TEXT("Sabar, sabar."),
		TEXT("Sstt, jangan keras-keras."),
		TEXT("Hehe, biasa itu."),
		TEXT("Kita mah cuma bisa ngelus dada."),
		TEXT("Nanti juga lupa sendiri."),
		TEXT("Aku juga dengar begitu.") };
	const TCHAR* const GreetLines[] = {
		TEXT("Eh, Mas Bayu. Sehat?"),
		TEXT("Hati-hati, Mas. Lubang jalannya lebih banyak dari janjinya."),
		TEXT("Mampir, Mas. Kopinya masih anget."),
		TEXT("Mas Bayu yang suka nulis itu, ya? Hati-hati, lho."),
		TEXT("Maaf, Mas, buru-buru. Antre minyak goreng."),
		TEXT("Jangan tanya kabar, Mas. Tanya harga saja."),
		TEXT("Monggo, Mas.") };
	const TCHAR* const DodgeLines[] = {
		TEXT("Eh! Pelan-pelan, Mas!"),
		TEXT("Astaga! Ini trotoar, bukan sirkuit!"),
		TEXT("Woi, motornya!") };
	const TCHAR* const VictimLines[] = {
		TEXT("Woi! Matamu di mana?!"),
		TEXT("Heh! Nggak lihat ada orang?!"),
		TEXT("Aduh, sakit! Tanggung jawab, kamu!"),
		TEXT("Naik motor kok kayak dikejar setan!"),
		TEXT("Trotoar buat jalan kaki, bukan buat motor!"),
		TEXT("Awas kamu! Kuingat mukamu!") };
	const TCHAR* const ShoveLines[] = {
		TEXT("Eh! Jangan main seruduk!"),
		TEXT("Pelan-pelan, Mas! Nggak sopan!"),
		TEXT("Heh, buru-buru amat! Dikejar utang?") };
	const TCHAR* const DriverLines[] = {
		TEXT("Woi! Mobilku penyok! Ganti rugi!"),
		TEXT("Lihat ini, lecet semua! Siapa yang bayar?!"),
		TEXT("Mana SIM-mu?! Punya nggak?!"),
		TEXT("Cicilan belum lunas, sudah ditabrak!"),
		TEXT("Kurang lebar jalannya, hah?!") };
	const TCHAR* const BystanderLines[] = {
		TEXT("Ada yang ketabrak! Ada yang ketabrak!"),
		TEXT("Rekam, rekam! Biar viral!"),
		TEXT("Wah, ribut! Panggil polisi!"),
		TEXT("Itu Mas Bayu, kan? Wah, wah."),
		TEXT("Kepung! Eh, jangan main hakim sendiri."),
		TEXT("Ya ampun, kasihan.") };
	const TCHAR* const CalmLines[] = {
		TEXT("Huh. Dasar."),
		TEXT("Zaman sekarang, ya..."),
		TEXT("Sudahlah. Sabar, sabar.") };
	const FColor AngryColor(255, 110, 90);

	template <typename T, int32 N>
	const T& PickOne(FRandomStream& Rng, const T (&Items)[N])
	{
		return Items[Rng.RandRange(0, N - 1)];
	}

	int32 HourNow(const UObject* Context)
	{
		const UGameInstance* GI = Context->GetWorld() ? Context->GetWorld()->GetGameInstance() : nullptr;
		const UGRClockSubsystem* Clock = GI ? GI->GetSubsystem<UGRClockSubsystem>() : nullptr;
		return Clock ? Clock->GetHour() % 24 : 9;
	}

	/** What blocks a walking person: walls, lamps, furniture. Not trigger volumes (districts, rooms) and not people. */
	FCollisionResponseParams BodyResponse()
	{
		FCollisionResponseParams Response;
		Response.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);
		Response.CollisionResponse.SetResponse(ECC_Vehicle, ECR_Ignore);
		return Response;
	}

	bool GroundAt(const UWorld* World, const FVector2D& P, float& OutZ)
	{
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, FVector(P, 400.f), FVector(P, -200.f), ECC_Visibility, FCollisionQueryParams(SCENE_QUERY_STAT(GRCrowdGround), false)))
		{
			return false;
		}
		OutZ = Hit.ImpactPoint.Z;
		return true;
	}

	bool BodyBlocked(const UWorld* World, const FVector& Center, const FQuat& Rot, const FCollisionShape& Shape)
	{
		return World->OverlapBlockingTestByChannel(Center, Rot, ECC_Pawn, Shape, FCollisionQueryParams(SCENE_QUERY_STAT(GRCrowdRoom), false), BodyResponse());
	}

	void PutPiece(UStaticMeshComponent* Piece, const TCHAR* Shape, const FVector& Loc, const FVector& Scale, const FLinearColor& Color)
	{
		Piece->SetStaticMesh(GRStoryUtil::LoadShape(Shape));
		Piece->SetRelativeLocation(Loc);
		Piece->SetRelativeScale3D(Scale);
		GRStoryUtil::ApplyColor(Piece, Color);
		Piece->SetVisibility(true);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// AGRWarga

AGRWarga::AGRWarga()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(38.f, CapsuleHalf);
	// The player's camera boom passes through people instead of zooming into them.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	bUseControllerRotationYaw = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 300.f, 0.f);
	Move->MaxWalkSpeed = Speed;
	// Same limit as Bayu: nobody climbs onto chairs and tables.
	Move->MaxStepHeight = 34.f;
	Move->bRunPhysicsWithNoController = true;
	AutoPossessAI = EAutoPossessAI::Disabled;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Quinn(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> MannyAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	QuinnMesh = Quinn.Object;
	MannyMesh = Manny.Object;
	if (QuinnMesh)
	{
		GetMesh()->SetSkeletalMeshAsset(QuinnMesh);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -CapsuleHalf), FRotator(0.f, -90.f, 0.f));
	}
	if (MannyAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(MannyAnim.Class);
	}
	// Made from MM_Idle by Tools/Python/make_talk_anims.py (shared with AGRNpc).
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> TalkSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Talk.MM_GR_Talk"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> SitSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Sit.MM_GR_Sit"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> SitTalkSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_SitTalk.MM_GR_SitTalk"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> AngrySeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Marah.MM_GR_Marah"));
	TalkAnim = TalkSeq.Object;
	SitAnim = SitSeq.Object;
	SitTalkAnim = SitTalkSeq.Object;
	AngryAnim = AngrySeq.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> CartSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Gerobak.MM_GR_Gerobak"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> CartWalkSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_GerobakJalan.MM_GR_GerobakJalan"));
	CartAnim = CartSeq.Object;
	CartWalkAnim = CartWalkSeq.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ClothesMat(TEXT("/Game/GemahRipah/Materials/M_GR_Warga.M_GR_Warga"));
	ClothesBase = ClothesMat.Object;

	Headwear = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Headwear"));
	HeadExtra = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadExtra"));
	for (UStaticMeshComponent* Piece : { Headwear.Get(), HeadExtra.Get() })
	{
		Piece->SetupAttachment(RootComponent);
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetCastShadow(false);
		Piece->SetVisibility(false);
	}

	BarkText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BarkText"));
	BarkText->SetupAttachment(RootComponent);
	BarkText->SetRelativeLocation(FVector(0.f, 0.f, 135.f));
	BarkText->SetHorizontalAlignment(EHTA_Center);
	BarkText->SetWorldSize(16.f);
	BarkText->SetTextRenderColor(FColor::White);
	BarkText->SetHiddenInGame(true);
}

void AGRWarga::Dress(int32 Seed, ERole InRole, bool bNight)
{
	Rng.Initialize(Seed);
	Role = InRole;
	const bool bFemale = Rng.FRand() < 0.5f;
	USkeletalMeshComponent* Body = GetMesh();
	USkeletalMesh* Want = bFemale ? QuinnMesh.Get() : MannyMesh.Get();
	if (Want && Body->GetSkeletalMeshAsset() != Want)
	{
		Body->SetSkeletalMeshAsset(Want);
	}
	// A new mesh restarts the anim instance, and with it any pose montage.
	CurrentPose = nullptr;
	const float Scale = Rng.FRandRange(0.93f, 1.04f) * (bFemale ? 0.97f : 1.f);
	Body->SetRelativeScale3D(FVector(Scale));

	// Clothes: one material over the whole mannequin, colored by height (shoes, trousers, shirt, skin).
	const FLinearColor Shirt = GRStoryUtil::ParseColor(Role == ERole::Jogger ? PickOne(Rng, SportColors) : PickOne(Rng, ShirtColors));
	const FLinearColor Pants = GRStoryUtil::ParseColor(PickOne(Rng, PantsColors));
	// Heights in the mesh's own space, so the body scale does not move them.
	HipZ = bFemale ? 92.f : 97.f;
	NeckZ = bFemale ? 146.f : 151.f;
	if (!Clothes && ClothesBase)
	{
		Clothes = UMaterialInstanceDynamic::Create(ClothesBase, this);
	}
	if (Clothes)
	{
		Clothes->SetVectorParameterValue(TEXT("Shirt"), Shirt);
		Clothes->SetVectorParameterValue(TEXT("Pants"), Pants);
		Clothes->SetVectorParameterValue(TEXT("Skin"), GRStoryUtil::ParseColor(PickOne(Rng, SkinColors)));
		Clothes->SetScalarParameterValue(TEXT("ShoeZ"), 9.f);
		for (int32 i = 0; i < Body->GetNumMaterials(); ++i)
		{
			Body->SetMaterial(i, Clothes);
		}
	}

	// Headwear: peci, cap, hijab, a vendor's straw hat, or nothing. Laid out on the reference pose (the top of
	// the mesh bounds is the top of the head), then pinned to the head bone to follow it when they sit or talk.
	const float Top = (Want ? Want->GetImportedBounds().GetBox().Max.Z : 178.f) * Scale - CapsuleHalf;
	for (UStaticMeshComponent* Piece : { Headwear.Get(), HeadExtra.Get() })
	{
		Piece->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		Piece->SetVisibility(false);
	}
	const float Roll = Rng.FRand();
	if (Role == ERole::Vendor && !bFemale && Roll < 0.6f)
	{
		const FLinearColor Straw = GRStoryUtil::ParseColor(TEXT("#C9A867"));
		PutPiece(Headwear, TEXT("cylinder"), FVector(0.f, 0.f, Top - 1.f), FVector(0.56f, 0.56f, 0.03f), Straw);
		PutPiece(HeadExtra, TEXT("cylinder"), FVector(0.f, 0.f, Top + 3.f), FVector(0.24f, 0.24f, 0.09f), Straw);
	}
	else if (bFemale && Roll < 0.5f)
	{
		const FLinearColor Scarf = GRStoryUtil::ParseColor(PickOne(Rng, ScarfColors));
		// Over the hair and ears, the face left open, and draped over the shoulders.
		PutPiece(Headwear, TEXT("sphere"), FVector(-3.f, 0.f, Top - 12.f), FVector(0.24f, 0.24f, 0.27f), Scarf);
		PutPiece(HeadExtra, TEXT("sphere"), FVector(-3.f, 0.f, Top - 30.f), FVector(0.34f, 0.4f, 0.14f), Scarf);
	}
	else if (!bFemale && Role != ERole::Jogger && Roll < 0.3f)
	{
		PutPiece(Headwear, TEXT("cylinder"), FVector(-1.f, 0.f, Top - 1.f), FVector(0.22f, 0.2f, 0.09f), GRStoryUtil::ParseColor(TEXT("#1A1A1A")));
	}
	else if (Roll < 0.5f || (Role == ERole::Jogger && Roll < 0.7f))
	{
		const FLinearColor Cap = GRStoryUtil::ParseColor(PickOne(Rng, CapColors));
		PutPiece(Headwear, TEXT("cylinder"), FVector(-1.f, 0.f, Top - 3.f), FVector(0.22f, 0.21f, 0.08f), Cap);
		PutPiece(HeadExtra, TEXT("cube"), FVector(12.f, 0.f, Top - 6.f), FVector(0.14f, 0.18f, 0.015f), Cap);
	}
	PinToHead(Headwear);
	PinToHead(HeadExtra);
	BarkZ = Top + 40.f;
	BarkText->SetRelativeLocation(FVector(0.f, 0.f, BarkZ));

	// Vendors push a gerobak: body, glass case, roof on four posts, two wheels, a front leg and the handles.
	const bool bVendor = Role == ERole::Vendor;
	if (bVendor && CartParts.IsEmpty())
	{
		for (int32 i = 0; i < 13; ++i)
		{
			UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
			Part->SetupAttachment(RootComponent);
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Part->SetCastShadow(false);
			Part->RegisterComponent();
			CartParts.Add(Part);
		}
	}
	if (bVendor)
	{
		const FVendorKind& Kind = bNight ? NightVendor : PickOne(Rng, DayVendors);
		Cry = Kind.Cry;
		const FLinearColor Paint = GRStoryUtil::ParseColor(Kind.Color);
		const FLinearColor Dark = GRStoryUtil::ParseColor(TEXT("#2A2A2A"));
		const float F = -CapsuleHalf;
		int32 n = 0;
		PutPiece(CartParts[n++], TEXT("cube"), FVector(118.f, 0.f, F + 78.f), FVector(1.f, 0.66f, 0.56f), Paint);
		PutPiece(CartParts[n++], TEXT("cube"), FVector(118.f, 0.f, F + 125.f), FVector(0.92f, 0.6f, 0.38f), GRStoryUtil::ParseColor(TEXT("#DCEBEE")));
		PutPiece(CartParts[n++], TEXT("cube"), FVector(118.f, 0.f, F + 188.f), FVector(1.2f, 0.82f, 0.04f), Paint);
		for (int32 PX = -1; PX <= 1; PX += 2)
		{
			for (int32 PY = -1; PY <= 1; PY += 2)
			{
				PutPiece(CartParts[n++], TEXT("cube"), FVector(118.f + PX * 46.f, PY * 30.f, F + 166.f), FVector(0.03f, 0.03f, 0.44f), Dark);
			}
		}
		for (int32 PY = -1; PY <= 1; PY += 2)
		{
			PutPiece(CartParts[n], TEXT("cylinder"), FVector(128.f, PY * 38.f, F + 22.f), FVector(0.44f, 0.44f, 0.06f), Dark);
			CartParts[n++]->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
		}
		PutPiece(CartParts[n++], TEXT("cube"), FVector(160.f, 0.f, F + 25.f), FVector(0.04f, 0.04f, 0.5f), Dark);
		// The handlebar sits where MM_GR_Gerobak puts the hands (PUSH_GRIP in make_talk_anims.py, mesh space), so it
		// scales with the person; two side bars join it to the cart.
		const float BarX = 32.f * Scale;
		const float BarZ = F + 103.f * Scale;
		PutPiece(CartParts[n++], TEXT("cube"), FVector(BarX, 0.f, BarZ), FVector(0.035f, 0.62f, 0.035f), Dark);
		for (int32 PY = -1; PY <= 1; PY += 2)
		{
			PutPiece(CartParts[n++], TEXT("cube"), FVector((BarX + 70.f) * 0.5f, PY * 29.f, BarZ), FVector((70.f - BarX) / 100.f, 0.035f, 0.035f), Dark);
		}
	}
	else
	{
		Cry.Reset();
		for (UStaticMeshComponent* Part : CartParts)
		{
			Part->SetVisibility(false);
		}
	}

	Speed = Role == ERole::Jogger ? Rng.FRandRange(320.f, 380.f) : bVendor ? Rng.FRandRange(70.f, 85.f) : Rng.FRandRange(105.f, 150.f);
	StripDir = Rng.FRand() < 0.5f ? 1.f : -1.f;
}

void AGRWarga::Activate(const FVector& Feet, float Yaw)
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	SetActorLocationAndRotation(Feet + FVector(0.f, 0.f, CapsuleHalf + 2.f), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::ResetPhysics);
	SetActorTickEnabled(true);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->SetComponentTickEnabled(true);
	Move->SetMovementMode(MOVE_Walking);
	Move->Velocity = FVector::ZeroVector;
	Move->MaxWalkSpeed = Speed;
	bActive = true;
	Mode = EMode::Pause;
	Wait = Rng.FRandRange(0.2f, 1.5f);
	Group = -1;
	Bench = -1;
	bCrossing = false;
	bReply = false;
	TalkPause = 0.f;
	Dodge = 0.f;
	DodgeCooldown = 0.f;
	FurnitureIgnore = 0.f;
	BarkHide = 0.f;
	WalkTime = 0.f;
	Stuck = 0.f;
	CryTimer = Rng.FRandRange(1.f, 6.f);
	AngryTime = 0.f;
	HitCooldown = 0.f;
	ShoveCooldown = 0.f;
	AngryAt = nullptr;
	DriverCar = nullptr;
	BarkText->SetHiddenInGame(true);
	SetSeatedLook(false);
}

void AGRWarga::Deactivate()
{
	StopRagdoll();
	if (AGRTrafficCar* Car = DriverCar.Get())
	{
		// Recycled while out of the car: it drives on without them.
		Car->HoldForDriver(0.f);
	}
	DriverCar = nullptr;
	AngryAt = nullptr;
	if (AGRCrowd* C = Crowd.Get(); C && C->Benches.IsValidIndex(Bench) && C->Benches[Bench].Sitters[Seat] == this)
	{
		C->Benches[Bench].Sitters[Seat] = nullptr;
	}
	Bench = -1;
	Group = -1;
	PlayPose(nullptr);
	bActive = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->StopMovementImmediately();
	Move->DisableMovement();
	Move->SetComponentTickEnabled(false);
	// Far below the town: traffic and the story only look at people near street level.
	SetActorLocation(FVector(0.f, 0.f, -30000.f), false, nullptr, ETeleportType::ResetPhysics);
	BarkText->SetHiddenInGame(true);
}

void AGRWarga::Say(const FString& Text, float Duration, const FColor& Color)
{
	BarkText->SetText(FText::FromString(Text));
	BarkText->SetTextRenderColor(Color);
	BarkText->SetHiddenInGame(false);
	BarkHide = Duration;
}

bool AGRWarga::KnockedBy(AActor* Culprit, float Closing, const FVector& Direction, float Diawasi, const TCHAR* What)
{
	if (!bActive || Mode == EMode::Down || HitCooldown > 0.f)
	{
		return false;
	}
	HitCooldown = 1.f;
	if (Mode == EMode::Sit)
	{
		StandUp();
	}
	Group = -1;
	bCrossing = false;
	Dodge = 0.f;
	TalkPause = 0.f;
	const FVector Dir = Direction.GetSafeNormal2D();
	const AGRCharacter* Bayu = Cast<AGRCharacter>(Culprit);
	const bool bShove = Bayu && !Bayu->IsRiding();
	// A vendor's cart takes the hit: they stagger instead of going down.
	const bool bFall = Closing >= 450.f && Role != ERole::Vendor;
	UE_LOG(LogGRCrowd, Display, TEXT("GRWARGA %s ditabrak %s, %.0f cm/s%s"), *GetName(), Culprit ? *Culprit->GetName() : TEXT("-"), Closing,
		bFall ? TEXT(", jatuh") : TEXT(""));
	if (bFall)
	{
		AngryAt = Culprit;
		Say(TEXT("Aduh!!"), 1.6f, AngryColor);
		StartFall(Dir * Closing * 0.7f + FVector(0.f, 0.f, 220.f));
	}
	else
	{
		LaunchCharacter(Dir * FMath::Clamp(Closing * 0.9f, 250.f, 650.f) + FVector(0.f, 0.f, 140.f), true, true);
		GetAngry(Culprit, FMath::Clamp(Closing / 90.f, 3.5f, 6.f), bShove ? EAnger::Shoved : EAnger::Victim);
	}
	if (AGRCrowd* C = Crowd.Get(); C && Diawasi > 0.f)
	{
		C->ReportIncident(Diawasi, GetActorLocation(), What ? What
			: bFall ? TEXT("Bayu menabrak warga sampai jatuh!") : bShove ? TEXT("Bayu menyeruduk warga.") : TEXT("Bayu menyerempet warga."));
	}
	return true;
}

void AGRWarga::GetAngry(AActor* At, float Duration, EAnger Kind)
{
	if (Mode == EMode::Sit)
	{
		if (Kind == EAnger::Bystander)
		{
			// Onlookers on a bench shout from their seat.
			Say(PickOne(Rng, BystanderLines), 2.5f, AngryColor);
			return;
		}
		StandUp();
	}
	Mode = EMode::Angry;
	Anger = Kind;
	AngryAt = At;
	AngryTime = Duration;
	ShoutTimer = Kind == EAnger::Bystander ? Rng.FRandRange(0.f, 0.8f) : 0.f;
	bApproach = Kind != EAnger::Bystander;
	Group = -1;
	bCrossing = false;
	Dodge = 0.f;
	TalkPause = 0.f;
}

void AGRWarga::TickAngry(float DeltaSeconds)
{
	AngryTime -= DeltaSeconds;
	const AActor* At = AngryAt.Get();
	const FVector Loc = GetActorLocation();
	const float Dist = At ? FVector::Dist2D(At->GetActorLocation(), Loc) : 1e6f;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (At)
	{
		const FRotator Face(0.f, (At->GetActorLocation() - Loc).Rotation().Yaw, 0.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 7.f));
		// Step up to have it out with him, but not into his face.
		if (bApproach && Dist > 260.f && Dist < 1500.f && Move->IsMovingOnGround())
		{
			Move->MaxWalkSpeed = 240.f;
			AddMovementInput((At->GetActorLocation() - Loc).GetSafeNormal2D(), 1.f);
		}
	}
	if (Dist > 2500.f)
	{
		// He is long gone.
		AngryTime = FMath::Min(AngryTime, 0.5f);
	}
	ShoutTimer -= DeltaSeconds;
	if (ShoutTimer <= 0.f && AngryTime > 0.8f)
	{
		ShoutTimer = Rng.FRandRange(2.f, 2.8f);
		const TCHAR* Line = Anger == EAnger::Driver ? PickOne(Rng, DriverLines)
			: Anger == EAnger::Shoved ? PickOne(Rng, ShoveLines)
			: Anger == EAnger::Bystander ? PickOne(Rng, BystanderLines) : PickOne(Rng, VictimLines);
		Say(Line, 1.9f, AngryColor);
	}
	if (AngryTime > 0.f)
	{
		return;
	}
	Move->MaxWalkSpeed = Speed;
	if (const AGRTrafficCar* Car = DriverCar.Get())
	{
		// Back to the car, still muttering.
		Target = Car->GetActorTransform().TransformPosition(DriverDoor);
		Mode = EMode::Walk;
		WalkTime = 0.f;
		Stuck = 0.f;
		Say(TEXT("Huh! Awas kalau ketemu lagi!"), 2.f, AngryColor);
		return;
	}
	if (Anger != EAnger::Bystander)
	{
		Say(PickOne(Rng, CalmLines), 2.2f);
	}
	NextTarget(false);
}

void AGRWarga::StartFall(const FVector& Velocity)
{
	Mode = EMode::Down;
	Anger = EAnger::Victim;
	DownTime = 0.f;
	bGettingUp = false;
	bRagdoll = true;
	PlayPose(nullptr);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->StopMovementImmediately();
	Move->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	USkeletalMeshComponent* Body = GetMesh();
	MeshProfile = Body->GetCollisionProfileName();
	Body->SetCollisionProfileName(TEXT("Ragdoll"));
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	// The scooter rolls on through; the ground, walls and cars still stop the body.
	Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	// The mesh stays on the capsule while the bodies tumble in the world, so getting up can blend from the heap
	// back into the standing pose instead of popping.
	Body->PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::ComponentTransformIsKinematic;
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->SetAllBodiesBelowSimulatePhysics(TEXT("pelvis"), true, true);
	Body->SetAllBodiesBelowPhysicsBlendWeight(TEXT("pelvis"), 1.f);
	Body->WakeAllRigidBodies();
	Body->SetAllPhysicsLinearVelocity(Velocity);
}

void AGRWarga::TickDown(float DeltaSeconds)
{
	DownTime += DeltaSeconds;
	USkeletalMeshComponent* Body = GetMesh();
	if (bGettingUp)
	{
		const float Weight = 1.f - DownTime / 0.7f;
		if (Weight > 0.f)
		{
			Body->SetAllBodiesBelowPhysicsBlendWeight(TEXT("pelvis"), Weight);
			return;
		}
		StopRagdoll();
		GetAngry(AngryAt.Get(), 6.f, EAnger::Victim);
		return;
	}
	const FBodyInstance* Pelvis = Body->GetBodyInstance(TEXT("pelvis"));
	const FVector Hip = Pelvis ? Pelvis->GetUnrealWorldTransform().GetLocation() : GetActorLocation();
	// Lying there: the capsule keeps to the body so traffic stops for them and the crowd knows where they are.
	SetActorLocation(Hip);
	const bool bSettled = !Pelvis || Pelvis->GetUnrealWorldVelocity().SizeSquared() < FMath::Square(30.f);
	if (DownTime < 2.6f || (!bSettled && DownTime < 5.f))
	{
		return;
	}
	// Stand where they landed, facing whoever hit them.
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GRWargaGetUp), false, this);
	const float Ground = GetWorld()->LineTraceSingleByChannel(Hit, Hip + FVector(0.f, 0.f, 80.f), Hip - FVector(0.f, 0.f, 300.f), ECC_Visibility, Params)
		? Hit.ImpactPoint.Z : Hip.Z - 15.f;
	const AActor* At = AngryAt.Get();
	const float Yaw = At ? (At->GetActorLocation() - Hip).Rotation().Yaw : GetActorRotation().Yaw;
	SetActorLocationAndRotation(FVector(Hip.X, Hip.Y, Ground + CapsuleHalf + 2.f), FRotator(0.f, Yaw, 0.f));
	bGettingUp = true;
	DownTime = 0.f;
	Say(TEXT("Aduh, aduh..."), 1.2f, AngryColor);
}

void AGRWarga::StopRagdoll()
{
	if (!bRagdoll)
	{
		return;
	}
	bRagdoll = false;
	bGettingUp = false;
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetAllBodiesBelowSimulatePhysics(TEXT("pelvis"), false, true);
	Body->SetCollisionProfileName(MeshProfile);
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Body->PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::SimulationUpatesComponentTransform;
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Body->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -CapsuleHalf), FRotator(0.f, -90.f, 0.f));
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void AGRWarga::PlayPose(UAnimSequenceBase* Anim, float Rate)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}
	if (Anim == CurrentPose)
	{
		if (Anim && CurrentMontage)
		{
			AnimInstance->Montage_SetPlayRate(CurrentMontage, Rate);
		}
		return;
	}
	CurrentPose = Anim;
	CurrentMontage = nullptr;
	if (!Anim)
	{
		AnimInstance->StopSlotAnimation(0.3f, TEXT("DefaultSlot"));
		return;
	}
	// The pushing walk comes from a walk cycle, which can carry root motion; the crowd code moves people itself.
	AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	CurrentMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(Anim, TEXT("DefaultSlot"), 0.3f, 0.3f, Rate, 10000);
}

void AGRWarga::PinToHead(UStaticMeshComponent* Piece)
{
	USkeletalMeshComponent* Body = GetMesh();
	const USkeletalMesh* Asset = Body->GetSkeletalMeshAsset();
	const int32 Head = Asset ? Asset->GetRefSkeleton().FindBoneIndex(TEXT("head")) : INDEX_NONE;
	if (Head == INDEX_NONE)
	{
		return;
	}
	// The head bone in component space, reference pose (the live pose may still be a frozen seated one).
	FTransform HeadRef = FTransform::Identity;
	for (int32 Bone = Head; Bone != INDEX_NONE; Bone = Asset->GetRefSkeleton().GetParentIndex(Bone))
	{
		HeadRef = HeadRef * Asset->GetRefSkeleton().GetRefBonePose()[Bone];
	}
	const FTransform InMesh = Piece->GetRelativeTransform().GetRelativeTransform(Body->GetRelativeTransform());
	Piece->AttachToComponent(Body, FAttachmentTransformRules::KeepRelativeTransform, TEXT("head"));
	Piece->SetRelativeTransform(InMesh.GetRelativeTransform(HeadRef));
}

void AGRWarga::SetSeatedLook(bool bSeated)
{
	// Headwear rides the head bone; the speech line and the clothes' waist and neck drop with the pelvis.
	BarkText->SetRelativeLocation(FVector(0.f, 0.f, bSeated ? BarkZ - 42.f : BarkZ));
	if (Clothes)
	{
		// Seated, the waist sits just above the thighs and the neck drops with the pelvis.
		Clothes->SetScalarParameterValue(TEXT("HipZ"), bSeated ? 58.f : HipZ);
		Clothes->SetScalarParameterValue(TEXT("NeckZ"), bSeated ? NeckZ - 42.f : NeckZ);
	}
}

FText AGRWarga::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return Role == ERole::Vendor ? LOCTEXT("GreetVendor", "Sapa pedagang") : LOCTEXT("GreetWarga", "Sapa warga");
}

bool AGRWarga::CanInteract_Implementation(AActor* Interactor) const
{
	if (!bActive || IsHidden() || Dodge > 0.f || Mode == EMode::Down || Mode == EMode::Angry)
	{
		return false;
	}
	// Story people and things near Bayu come first: their E prompt is what the scene needs.
	if (Interactor && GetWorld())
	{
		FCollisionObjectQueryParams Objects;
		Objects.AddObjectTypesToQuery(ECC_WorldStatic);
		Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
		Objects.AddObjectTypesToQuery(ECC_Pawn);
		TArray<FOverlapResult> Overlaps;
		GetWorld()->OverlapMultiByObjectType(Overlaps, Interactor->GetActorLocation(), FQuat::Identity, Objects,
			FCollisionShape::MakeSphere(400.f), FCollisionQueryParams(SCENE_QUERY_STAT(GRWargaPrompt), false, Interactor));
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Other = Overlap.GetActor();
			if (Other && Other != this && !Other->IsA<AGRWarga>() && Other->Implements<UGRInteractable>()
				&& IGRInteractable::Execute_CanInteract(Other, Interactor))
			{
				return false;
			}
		}
	}
	return true;
}

void AGRWarga::Interact_Implementation(AActor* Interactor)
{
	TalkPause = 4.5f;
	if (Mode == EMode::Walk)
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
	TArray<FString> Lines;
	const int32 Hour = HourNow(this);
	Lines.Add(Hour >= 4 && Hour < 11 ? TEXT("Pagi, Mas Bayu!") : Hour < 15 ? TEXT("Siang, Mas. Panas, ya.")
		: Hour < 18 ? TEXT("Sore, Mas Bayu.") : TEXT("Malam, Mas. Jangan kemalaman, banyak razia."));
	if (Role == ERole::Vendor)
	{
		Lines.Add(TEXT("Beli, Mas? Masih anget."));
		Lines.Add(TEXT("Murah, Mas. Belum kena pajak."));
	}
	else if (Role == ERole::Jogger)
	{
		Lines.Add(TEXT("Ayo lari, Mas! Biar kuat ngadepin harga."));
	}
	else
	{
		for (const TCHAR* Line : GreetLines)
		{
			Lines.Add(Line);
		}
		// What the town is talking about this chapter.
		const UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>();
		for (const FGRBarkDef& Bark : Story ? Story->GetData().Barks : TArray<FGRBarkDef>())
		{
			if (Story->Check(Bark.If))
			{
				Lines.Add(Bark.Text);
			}
		}
	}
	Say(Lines[Rng.RandRange(0, Lines.Num() - 1)], 4.f);
}

void AGRWarga::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive)
	{
		return;
	}
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (BarkHide > 0.f)
	{
		BarkHide -= DeltaSeconds;
		if (BarkHide <= 0.f)
		{
			BarkText->SetHiddenInGame(true);
		}
	}
	DodgeCooldown -= DeltaSeconds;
	HitCooldown -= DeltaSeconds;
	ShoveCooldown -= DeltaSeconds;
	// Bayu sprinting into someone shoves them (walking pace only brushes past).
	if (const AGRCharacter* Bayu = Cast<AGRCharacter>(Player); Bayu && !Bayu->IsRiding() && ShoveCooldown <= 0.f && Mode != EMode::Down)
	{
		const FVector2D Rel(GetActorLocation() - Bayu->GetActorLocation());
		if (Rel.SizeSquared() < FMath::Square(120.f) && FVector2D::DotProduct(FVector2D(Bayu->GetVelocity()), Rel.GetSafeNormal()) > 560.f)
		{
			ShoveCooldown = 4.f;
			KnockedBy(Player, 260.f, FVector(Rel.GetSafeNormal(), 0.f), 1.f);
		}
	}
	if (FurnitureIgnore > 0.f)
	{
		FurnitureIgnore -= DeltaSeconds;
		if (FurnitureIgnore <= 0.f)
		{
			GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		}
	}

	if (TalkPause > 0.f)
	{
		TalkPause -= DeltaSeconds;
		if (Player && Mode != EMode::Sit)
		{
			const FRotator Face(0.f, (Player->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0.f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 6.f));
		}
	}
	else
	{
		TickDodge(Player);
		switch (Mode)
		{
		case EMode::Walk:
			TickWalk(DeltaSeconds);
			break;
		case EMode::Pause:
			Wait -= DeltaSeconds;
			if (Wait <= 0.f)
			{
				NextTarget(false);
			}
			break;
		case EMode::Chat:
			TickChat(DeltaSeconds);
			break;
		case EMode::Sit:
			TickSit(DeltaSeconds);
			break;
		case EMode::Down:
			TickDown(DeltaSeconds);
			break;
		case EMode::Angry:
			TickAngry(DeltaSeconds);
			break;
		}
	}

	if (Role == ERole::Vendor && Mode != EMode::Down && Mode != EMode::Angry)
	{
		CryTimer -= DeltaSeconds;
		if (CryTimer <= 0.f)
		{
			CryTimer = Rng.FRandRange(7.f, 13.f);
			if (TalkPause <= 0.f)
			{
				Say(Cry, 2.5f);
			}
		}
	}
	UpdatePose();

	if (!BarkText->bHiddenInGame)
	{
		if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			BarkText->SetWorldRotation(FRotator(0.f, (Camera->GetCameraLocation() - GetActorLocation()).Rotation().Yaw, 0.f));
		}
	}
}

void AGRWarga::UpdatePose()
{
	const bool bTalking = BarkHide > 0.f || TalkPause > 0.f;
	UAnimSequenceBase* Want = nullptr;
	if (Mode == EMode::Down)
	{
		return;
	}
	float Rate = 1.f;
	if (Mode == EMode::Angry)
	{
		// Shaking a fist while standing; walking up to him (or staggering) uses the walk.
		Want = GetVelocity().SizeSquared2D() < FMath::Square(60.f) && GetCharacterMovement()->IsMovingOnGround() ? AngryAnim.Get() : nullptr;
	}
	else if (Role == ERole::Vendor && CartAnim)
	{
		// Both hands stay on the cart's handlebar; the steps keep pace with the cart.
		const float Moving = GetVelocity().Size2D();
		Want = Moving > 20.f && CartWalkAnim ? CartWalkAnim.Get() : CartAnim.Get();
		Rate = Want == CartWalkAnim ? FMath::Clamp(Moving / CartWalkSpeed, 0.3f, 2.f) : 1.f;
	}
	else if (Mode == EMode::Sit)
	{
		Want = bTalking ? SitTalkAnim.Get() : SitAnim.Get();
	}
	else if (bTalking && Mode != EMode::Walk)
	{
		Want = TalkAnim.Get();
	}
	else if (TalkPause > 0.f)
	{
		Want = TalkAnim.Get();
	}
	PlayPose(Want, Rate);
}

void AGRWarga::TickWalk(float DeltaSeconds)
{
	WalkTime += DeltaSeconds;
	Dodge -= DeltaSeconds;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = Speed * (Dodge > 0.f ? 2.6f : bCrossing ? 1.25f : 1.f);

	const FVector Loc = GetActorLocation();
	const FVector2D To(Target - Loc);
	const float Dist = To.Size();
	if (AGRTrafficCar* Car = DriverCar.Get(); Car && (Dist < 90.f || Stuck > 1.f || WalkTime > 10.f))
	{
		// Back behind the wheel: the car drives on.
		Car->HoldForDriver(0.8f);
		DriverCar = nullptr;
		Deactivate();
		return;
	}
	if (Dist < (Role == ERole::Jogger ? 120.f : 60.f))
	{
		bCrossing = false;
		Dodge = 0.f;
		Mode = EMode::Pause;
		Wait = Role == ERole::Jogger ? Rng.FRandRange(0.2f, 1.f)
			: Role == ERole::Vendor ? Rng.FRandRange(6.f, 14.f)
			: Rng.FRand() < 0.25f ? Rng.FRandRange(6.f, 12.f) : Rng.FRandRange(1.5f, 5.f);
		return;
	}
	if (WalkTime > 50.f)
	{
		NextTarget(false);
		return;
	}

	// Make room for whoever comes the other way: keep left of them.
	const FVector2D Dir = To / Dist;
	float Side = 0.f;
	auto Avoid = [&](const FVector& Other, float Width)
	{
		const FVector2D Rel(Other - Loc);
		const float Along = FVector2D::DotProduct(Rel, Dir);
		if (Along > 0.f && Along < 230.f)
		{
			const float Lat = FVector2D::CrossProduct(Dir, Rel);
			if (FMath::Abs(Lat) < Width)
			{
				Side += (Lat >= 0.f ? 1.f : -1.f) * (1.f - Along / 230.f);
			}
		}
	};
	if (const AGRCrowd* C = Crowd.Get())
	{
		for (const AGRWarga* Other : C->People)
		{
			if (Other != this && Other->bActive)
			{
				Avoid(Other->GetActorLocation(), 85.f);
			}
		}
		for (const TWeakObjectPtr<AActor>& Npc : C->StoryPeople)
		{
			if (Npc.IsValid())
			{
				Avoid(Npc->GetActorLocation(), 90.f);
			}
		}
	}
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Avoid(Player->GetActorLocation(), 100.f);
	}
	const FVector2D Left(Dir.Y, -Dir.X);
	const FVector2D Steer = (Dir + Left * FMath::Clamp(Side, -1.f, 1.f) * 0.9f).GetSafeNormal();
	AddMovementInput(FVector(Steer, 0.f), 1.f);

	Stuck = GetVelocity().SizeSquared2D() < FMath::Square(20.f) ? Stuck + DeltaSeconds : 0.f;
	if (Stuck > 1.2f)
	{
		NextTarget(true);
	}
}

void AGRWarga::TickChat(float DeltaSeconds)
{
	const FRotator Face(0.f, (LookAt - GetActorLocation()).Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 4.f));
	ActivityTime -= DeltaSeconds;
	if (ActivityTime <= 0.f)
	{
		LeaveActivity();
		return;
	}
	TalkTimer -= DeltaSeconds;
	AGRCrowd* C = Crowd.Get();
	if (TalkTimer > 0.f || BarkHide > 0.f || !C)
	{
		return;
	}
	TalkTimer = Rng.FRandRange(3.f, 8.f);
	// One speaker at a time; a topic gets an answer from someone else in the group.
	TArray<AGRWarga*> Mates;
	for (AGRWarga* Other : C->People)
	{
		if (Other != this && Other->bActive && Other->Mode == EMode::Chat && Other->Group == Group)
		{
			if (Other->BarkHide > 0.f)
			{
				return;
			}
			Mates.Add(Other);
		}
	}
	if (Mates.IsEmpty())
	{
		LeaveActivity();
		return;
	}
	Say(C->PickChatLine(!bReply), bReply ? 2.5f : 4.f);
	if (!bReply)
	{
		AGRWarga* Answer = Mates[Rng.RandRange(0, Mates.Num() - 1)];
		Answer->bReply = true;
		Answer->TalkTimer = FMath::Min(Answer->TalkTimer, 1.f);
	}
	bReply = false;
}

void AGRWarga::TickSit(float DeltaSeconds)
{
	ActivityTime -= DeltaSeconds;
	if (ActivityTime <= 0.f)
	{
		StandUp();
		return;
	}
	AGRCrowd* C = Crowd.Get();
	if (!C || !C->Benches.IsValidIndex(Bench))
	{
		return;
	}
	// Two on a bench pass the time talking.
	AGRWarga* Mate = C->Benches[Bench].Sitters[1 - Seat].Get();
	TalkTimer -= DeltaSeconds;
	if (Mate && Mate->bActive && TalkTimer <= 0.f && BarkHide <= 0.f && Mate->BarkHide <= 0.f)
	{
		TalkTimer = Rng.FRandRange(5.f, 12.f);
		Say(C->PickChatLine(!bReply), bReply ? 2.5f : 4.f);
		if (!bReply)
		{
			// The mate answers once this line is done.
			Mate->bReply = true;
			Mate->TalkTimer = FMath::Min(Mate->TalkTimer, 1.f);
		}
		bReply = false;
	}
}

void AGRWarga::TickDodge(const APawn* Player)
{
	const AGRCharacter* Bayu = Cast<AGRCharacter>(Player);
	if (!Bayu || !Bayu->IsRiding() || Mode == EMode::Sit || Mode == EMode::Down || DodgeCooldown > 0.f)
	{
		return;
	}
	const float RideSpeed = Bayu->GetRideSpeed();
	if (RideSpeed < 300.f)
	{
		return;
	}
	const FVector2D Heading = FVector2D(Bayu->GetActorForwardVector()).GetSafeNormal();
	const FVector2D Rel(GetActorLocation() - Bayu->GetActorLocation());
	const float Along = FVector2D::DotProduct(Rel, Heading);
	const float Lat = FVector2D::CrossProduct(Heading, Rel);
	if (Along < 0.f || Along > RideSpeed * 1.4f || FMath::Abs(Lat) > 140.f)
	{
		return;
	}
	// Jump to whichever side of the scooter's path they are already on.
	const FVector2D Away = FVector2D(-Heading.Y, Heading.X) * (Lat >= 0.f ? 1.f : -1.f);
	Group = -1;
	bCrossing = false;
	Target = GetActorLocation() + FVector(Away * 280.f, 0.f);
	Mode = EMode::Walk;
	WalkTime = 0.f;
	Stuck = 0.f;
	Dodge = 1.1f;
	DodgeCooldown = 3.f;
	Say(PickOne(Rng, DodgeLines), 2.2f);
}

void AGRWarga::NextTarget(bool bBlocked)
{
	AGRCrowd* C = Crowd.Get();
	if (!C)
	{
		return;
	}
	WalkTime = 0.f;
	Stuck = 0.f;
	bCrossing = false;
	const FVector Loc = GetActorLocation();
	const FVector2D Here(Loc);
	auto Go = [this](const FVector2D& P, float Z, bool bCross)
	{
		Target = FVector(P, Z + CapsuleHalf);
		bCrossing = bCross;
		Mode = EMode::Walk;
	};

	int32 Axis = 0;
	float Side = 1.f;
	float Along = 0.f;
	if (C->PavementAt(Here, Axis, Side, Along) && (Role != ERole::Stroller || Rng.FRand() < 0.8f))
	{
		// Now and then a stroller crosses to the other pavement, only with no car close.
		if (Role == ERole::Stroller && !bBlocked && Rng.FRand() < 0.12f && FMath::Abs(Along) > 4000.f)
		{
			const FVector2D Across = C->PavementPoint(Axis, -Side, Along + Rng.FRandRange(-150.f, 150.f), PavementLine + Rng.FRandRange(-60.f, 60.f));
			float Z = 0.f;
			if (C->IsWalkable(Across, Z) && !C->CarNear((Here + Across) * 0.5f, 3500.f))
			{
				Go(Across, Z, true);
				return;
			}
		}
		if (bBlocked || Rng.FRand() < 0.18f)
		{
			StripDir = -StripDir;
		}
		const float Sign = Along >= 0.f ? 1.f : -1.f;
		for (int32 Try = 0; Try < 2; ++Try)
		{
			const float Length = Role == ERole::Jogger ? Rng.FRandRange(3000.f, 6000.f)
				: Role == ERole::Vendor ? Rng.FRandRange(500.f, 1400.f) : Rng.FRandRange(1000.f, 3200.f);
			const float Far = FMath::Clamp(FMath::Abs(Along) + StripDir * Length, AlongNear, AlongFar);
			if (FMath::Abs(Far - FMath::Abs(Along)) < 300.f)
			{
				// End of the pavement: turn around.
				StripDir = -StripDir;
				continue;
			}
			const FVector2D P = C->PavementPoint(Axis, Side, Sign * Far, PavementLine + Rng.FRandRange(-70.f, 70.f));
			float Z = 0.f;
			if (C->IsWalkable(P, Z) && C->ClearPath(this, Loc, FVector(P, Z + CapsuleHalf), false))
			{
				Go(P, Z, false);
				return;
			}
			StripDir = -StripDir;
		}
	}

	// Lanes, yards and the market: somewhere in sight, roughly ahead.
	const float Heading = GetActorRotation().Yaw;
	for (int32 Try = 0; Try < 10; ++Try)
	{
		const float Yaw = bBlocked || Try >= 5 ? Rng.FRandRange(0.f, 360.f) : Heading + Rng.FRandRange(-75.f, 75.f);
		const float Dist = Role == ERole::Jogger ? Rng.FRandRange(1500.f, 3000.f) : Rng.FRandRange(500.f, 2000.f);
		const FVector2D P = Here + FVector2D(FMath::Cos(FMath::DegreesToRadians(Yaw)), FMath::Sin(FMath::DegreesToRadians(Yaw))) * Dist;
		float Z = 0.f;
		if (C->IsWalkable(P, Z) && C->ClearPath(this, Loc, FVector(P, Z + CapsuleHalf), false))
		{
			Go(P, Z, false);
			return;
		}
	}
	Mode = EMode::Pause;
	Wait = Rng.FRandRange(1.f, 3.f);
}

void AGRWarga::SitDown(int32 InBench, int32 InSeat)
{
	AGRCrowd* C = Crowd.Get();
	if (!C || !C->Benches.IsValidIndex(InBench))
	{
		return;
	}
	Bench = InBench;
	Seat = InSeat;
	C->Benches[Bench].Sitters[Seat] = this;
	Mode = EMode::Sit;
	ActivityTime = Rng.FRandRange(40.f, 140.f);
	TalkTimer = Rng.FRandRange(3.f, 10.f);
	// The capsule overlaps the bench block (WorldDynamic): ignore it, or movement pushes them up onto the seat.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	GetCharacterMovement()->StopMovementImmediately();
	SetSeatedLook(true);
	PlayPose(SitAnim);
}

void AGRWarga::StandUp()
{
	if (AGRCrowd* C = Crowd.Get(); C && C->Benches.IsValidIndex(Bench) && C->Benches[Bench].Sitters[Seat] == this)
	{
		C->Benches[Bench].Sitters[Seat] = nullptr;
	}
	Bench = -1;
	SetSeatedLook(false);
	PlayPose(nullptr);
	// Step forward off the bench before furniture blocks the capsule again.
	FurnitureIgnore = 1.5f;
	Target = GetActorLocation() + GetActorForwardVector() * 100.f;
	Mode = EMode::Walk;
	WalkTime = 0.f;
	Stuck = 0.f;
	bCrossing = false;
}

void AGRWarga::LeaveActivity()
{
	if (Mode == EMode::Sit)
	{
		StandUp();
		return;
	}
	Group = -1;
	bReply = false;
	NextTarget(false);
}

// ---------------------------------------------------------------------------------------------------------------
// AGRCrowd

AGRCrowd::AGRCrowd()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

AGRCrowd* AGRCrowd::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AGRCrowd> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void AGRCrowd::BeginPlay()
{
	Super::BeginPlay();
	Rng.Initialize(20261104);
	NextSeed = 1000;
	PlaceBenches();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < PoolSize; ++i)
	{
		if (AGRWarga* Person = GetWorld()->SpawnActor<AGRWarga>(AGRWarga::StaticClass(), FTransform(FVector(0.f, 0.f, -30000.f)), Params))
		{
			Person->Crowd = this;
			Person->Deactivate();
			People.Add(Person);
		}
	}
	RefreshKeepOut();
}

FVector AGRCrowd::SeatFeet(const FBench& B, int32 InSeat) const
{
	return B.Feet + FRotator(0.f, B.Yaw, 0.f).RotateVector(FVector(0.f, InSeat == 0 ? -45.f : 45.f, 0.f));
}

void AGRCrowd::PlaceBenches()
{
	FRandomStream Spots(20261111);
	for (int32 Axis = 0; Axis < 2; ++Axis)
	{
		for (int32 SideI = -1; SideI <= 1; SideI += 2)
		{
			for (int32 Dir = -1; Dir <= 1; Dir += 2)
			{
				for (float A = 3600.f + Spots.FRandRange(0.f, 1500.f); A < AlongFar - 500.f; A += Spots.FRandRange(3200.f, 4600.f))
				{
					const FVector2D P = PavementPoint(Axis, SideI, Dir * A, BenchLine);
					// Facing the road.
					const float Yaw = Axis == 0 ? (SideI > 0 ? 180.f : 0.f) : (SideI > 0 ? -90.f : 90.f);
					float Z = 0.f;
					if (!GroundAt(GetWorld(), P, Z) || Z > 30.f)
					{
						continue;
					}
					// A stall, a lamp or a bus stop already there: skip this spot.
					const FQuat Rot = FRotator(0.f, Yaw, 0.f).Quaternion();
					const FVector Middle = FVector(P, Z + 50.f) + Rot.RotateVector(FVector(-8.f, 0.f, 0.f));
					if (BodyBlocked(GetWorld(), Middle, Rot, FCollisionShape::MakeBox(FVector(40.f, 110.f, 35.f))))
					{
						continue;
					}
					AddBench(FVector(P, Z), Yaw);
				}
			}
		}
	}
}

void AGRCrowd::AddBench(const FVector& Feet, float Yaw)
{
	TArray<FGRPropPart> Parts;
	if (!GRPropModels::Build(TEXT("bangku"), TArray<float>(), FString(), 0, Parts))
	{
		return;
	}
	FBench& B = Benches.AddDefaulted_GetRef();
	B.Feet = Feet;
	B.Yaw = Yaw;
	const FRotator Rot(0.f, Yaw, 0.f);
	for (const FGRPropPart& Part : Parts)
	{
		// One instanced mesh per color (a bench is all boxes): sixty benches cost two draw calls.
		TObjectPtr<UInstancedStaticMeshComponent>& Mesh = BenchMeshes.FindOrAdd(Part.Color.ToFColor(true).ToPackedARGB());
		if (!Mesh)
		{
			Mesh = NewObject<UInstancedStaticMeshComponent>(this);
			Mesh->SetMobility(EComponentMobility::Movable);
			Mesh->SetupAttachment(RootComponent);
			Mesh->SetStaticMesh(GRStoryUtil::LoadShape(Part.Shape));
			Mesh->SetMaterial(0, GRStoryUtil::MakeColorMaterial(this, Part.Color));
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->RegisterComponent();
		}
		const FTransform Transform(Rot.Quaternion() * Part.Rot.Quaternion(), Feet + Rot.RotateVector(Part.Loc), Part.Size / 100.f);
		B.Instances.Add({ Mesh.Get(), Mesh->AddInstance(Transform, true), Transform });
		if (Part.bCollide)
		{
			// The seat: a solid block from the ground up, like seats in AGRStoryProp.
			const float Top = Part.Loc.Z + Part.Size.Z * 0.5f;
			UBoxComponent* Block = NewObject<UBoxComponent>(this);
			Block->SetupAttachment(RootComponent);
			Block->SetBoxExtent(FVector(Part.Size.X * 0.5f + 10.f, Part.Size.Y * 0.5f + 10.f, Top * 0.5f));
			Block->SetWorldLocationAndRotation(Feet + Rot.RotateVector(FVector(Part.Loc.X, Part.Loc.Y, Top * 0.5f)), Rot);
			Block->SetCollisionProfileName(TEXT("BlockAllDynamic"));
			Block->CanCharacterStepUpOn = ECB_No;
			Block->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			Block->SetCanEverAffectNavigation(false);
			Block->RegisterComponent();
			B.Block = Block;
		}
	}
}

void AGRCrowd::RefreshKeepOut()
{
	KeepOut.Reset();
	StoryPeople.Reset();
	Rooms.Reset();
	TArray<FBox> Props;
	for (TActorIterator<AGRNpc> It(GetWorld()); It; ++It)
	{
		if (!It->IsHidden())
		{
			const FVector L = It->GetActorLocation();
			KeepOut.Add(FVector(L.X, L.Y, 450.f));
			StoryPeople.Add(*It);
		}
	}
	for (TActorIterator<AGRStoryProp> It(GetWorld()); It; ++It)
	{
		if (!It->IsHidden())
		{
			const FBox Box = It->GetComponentsBoundingBox(false);
			if (Box.IsValid)
			{
				Props.Add(Box.ExpandBy(40.f));
			}
		}
	}
	for (TActorIterator<AGRTopDownRoomVolume> It(GetWorld()); It; ++It)
	{
		const FBox Box = It->GetComponentsBoundingBox(true);
		if (Box.IsValid)
		{
			Rooms.Add(Box.ExpandBy(150.f));
		}
	}
	for (TActorIterator<AGRStealthLevel> It(GetWorld()); It; ++It)
	{
		const FBox Box = It->GetComponentsBoundingBox(true);
		if (Box.IsValid)
		{
			Rooms.Add(Box.ExpandBy(150.f));
		}
	}

	// Put benches away where this chapter's story props or people stand.
	for (FBench& B : Benches)
	{
		const FVector Middle = B.Feet + FRotator(0.f, B.Yaw, 0.f).RotateVector(FVector(-8.f, 0.f, 0.f));
		const FBox Area = FBox::BuildAABB(Middle, FVector(110.f, 110.f, 100.f));
		bool bHide = false;
		for (const FBox& Prop : Props)
		{
			bHide |= Prop.IntersectXY(Area);
		}
		for (const FVector& K : KeepOut)
		{
			bHide |= FVector2D::DistSquared(FVector2D(Middle), FVector2D(K)) < FMath::Square(250.f);
		}
		if (bHide == B.bHidden)
		{
			continue;
		}
		B.bHidden = bHide;
		for (const FBenchInstance& Inst : B.Instances)
		{
			if (UInstancedStaticMeshComponent* Mesh = Inst.Mesh.Get())
			{
				FTransform T = Inst.Transform;
				if (bHide)
				{
					T.SetScale3D(FVector::ZeroVector);
				}
				Mesh->UpdateInstanceTransform(Inst.Index, T, true, true);
			}
		}
		if (UPrimitiveComponent* Block = B.Block.Get())
		{
			Block->SetCollisionEnabled(bHide ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		}
		for (const TWeakObjectPtr<AGRWarga>& Sitter : B.Sitters)
		{
			if (AGRWarga* Person = Sitter.Get())
			{
				Person->StandUp();
			}
		}
	}
}

void AGRCrowd::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Timer -= DeltaSeconds;
	KeepOutTimer -= DeltaSeconds;
	if (Timer > 0.f)
	{
		return;
	}
	Timer = 0.5f;
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	const FVector Center = Player->GetActorLocation();
	// A teleport (story jump, waking up at home): fill the new place at once, even in view.
	const bool bJumped = FVector::DistSquared2D(Center, LastPlayerPos) > FMath::Square(4000.f);
	LastPlayerPos = Center;
	if (bJumped || KeepOutTimer <= 0.f)
	{
		KeepOutTimer = 4.f;
		RefreshKeepOut();
	}

	// Recycle everyone Bayu left behind, and anyone who fell out of the world.
	for (AGRWarga* Person : People)
	{
		const FVector L = Person->GetActorLocation();
		if (Person->bActive && (FVector::DistSquared2D(L, Center) > FMath::Square(RecycleRange) || L.Z < -1000.f))
		{
			Person->Deactivate();
		}
	}
	int32 Vendors = 0;
	int32 Active = CountActive(Vendors);
	const int32 Want = TargetPopulation();
	// Late in the evening the streets empty: send the extras home while nobody watches.
	if (Active > Want + 2)
	{
		for (AGRWarga* Person : People)
		{
			if (Person->bActive && Person->Mode != AGRWarga::EMode::Chat && Person->Mode != AGRWarga::EMode::Down
				&& Person->Mode != AGRWarga::EMode::Angry && !Person->DriverCar.IsValid() && !InView(Person->GetActorLocation() - FVector(0.f, 0.f, CapsuleHalf)))
			{
				Person->Deactivate();
				break;
			}
		}
	}
	int32 Budget = bJumped ? PoolSize : 2;
	for (int32 Try = 0; Try < Budget * 2 && Budget > 0 && Active < Want; ++Try)
	{
		if (SpawnActivity(Center, bJumped))
		{
			--Budget;
		}
		Active = CountActive(Vendors);
	}
}

void AGRCrowd::ReportIncident(float Diawasi, const FVector& Where, const FString& What)
{
	const double Now = GetWorld()->GetTimeSeconds();
	Incidents.RemoveAll([Now](double T) { return Now - T > 90.0; });
	Incidents.Add(Now);
	// Once is an accident. Again and again within a minute and a half, and people start to remember the face.
	const int32 Count = Incidents.Num();
	const bool bRiot = Count >= 3 && Now - LastRiot > 60.0;
	if (bRiot)
	{
		LastRiot = Now;
	}
	const float Delta = FMath::RoundToFloat(Diawasi * FMath::Min(1.f + 0.5f * (Count - 1), 3.f)) + (bRiot ? 5.f : 0.f);
	UE_LOG(LogGRCrowd, Display, TEXT("GRINSIDEN %s ke-%d dalam 90 dtk, Diawasi +%.0f%s"), *What, Count, Delta, bRiot ? TEXT(", KERUSUHAN") : TEXT(""));
	UGameInstance* GI = GetGameInstance();
	if (UGRStorySubsystem* Story = GI->GetSubsystem<UGRStorySubsystem>())
	{
		Story->Run(FString::Printf(TEXT("notifburuk %s Diawasi +%.0f."),
			bRiot ? TEXT("Kerusuhan! Warga ramai mengerubungi Bayu, ada yang merekam.") : *What, Delta));
	}
	if (UGRStatusSubsystem* Status = GI->GetSubsystem<UGRStatusSubsystem>())
	{
		Status->AddDiawasi(Delta, true);
	}
	// People nearby turn round and shout; in a riot the whole street crowds in on Bayu.
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const float Reach = bRiot ? 2200.f : 1200.f;
	int32 Left = bRiot ? 14 : 3;
	for (AGRWarga* Person : People)
	{
		if (Left > 0 && Person->bActive && Person->Mode != AGRWarga::EMode::Down && Person->Mode != AGRWarga::EMode::Angry
			&& FVector::DistSquared2D(Person->GetActorLocation(), Where) < FMath::Square(Reach))
		{
			Person->GetAngry(Player, Rng.FRandRange(3.f, 6.f) + (bRiot ? 4.f : 0.f), AGRWarga::EAnger::Bystander);
			Person->bApproach = bRiot && Person->Mode == AGRWarga::EMode::Angry;
			--Left;
		}
	}
}

bool AGRCrowd::DriverOut(AGRTrafficCar* Car)
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Car || !Player || FVector::DistSquared2D(Car->GetActorLocation(), Player->GetActorLocation()) > FMath::Square(3000.f))
	{
		return false;
	}
	AGRWarga* Driver = TakeFromPool();
	if (!Driver)
	{
		return false;
	}
	const FTransform Frame(FRotator(0.f, Car->GetActorRotation().Yaw, 0.f), Car->GetActorLocation());
	const int32 Hour = HourNow(this);
	// Right-hand drive: the driver's door is on the right. A car up against something lets them out on the left.
	for (const float Side : { 1.f, -1.f })
	{
		const FVector Door(Car->GetHalfLength() * 0.15f, Side * (Car->GetHalfWidth() + 70.f), 0.f);
		const FVector Spot = Frame.TransformPosition(Door);
		float Z = 0.f;
		if (!GroundAt(GetWorld(), FVector2D(Spot), Z)
			|| BodyBlocked(GetWorld(), FVector(Spot.X, Spot.Y, Z + CapsuleHalf + 2.f), FQuat::Identity, FCollisionShape::MakeCapsule(38.f, CapsuleHalf - 4.f)))
		{
			continue;
		}
		Driver->Dress(NextSeed++, AGRWarga::ERole::Stroller, Hour >= 20 || Hour < 5);
		Driver->Activate(FVector(Spot.X, Spot.Y, Z), (Player->GetActorLocation() - Spot).Rotation().Yaw);
		Driver->DriverCar = Car;
		Driver->DriverDoor = FVector(Door.X, Door.Y, CapsuleHalf);
		Driver->GetAngry(Player, 6.5f, AGRWarga::EAnger::Driver);
		Car->HoldForDriver(30.f);
		UE_LOG(LogGRCrowd, Display, TEXT("GRWARGA sopir %s turun dari %s"), *Driver->GetName(), *Car->GetName());
		return true;
	}
	return false;
}

int32 AGRCrowd::TargetPopulation() const
{
	const int32 Hour = HourNow(this);
	if (Hour >= 6 && Hour < 20)
	{
		return 36;
	}
	if (Hour >= 20 && Hour < 22)
	{
		return 16;
	}
	if (Hour == 5)
	{
		return 14;
	}
	return 6;
}

int32 AGRCrowd::CountActive(int32& OutVendors) const
{
	int32 Active = 0;
	OutVendors = 0;
	for (const AGRWarga* Person : People)
	{
		if (Person->bActive)
		{
			++Active;
			OutVendors += Person->Role == AGRWarga::ERole::Vendor ? 1 : 0;
		}
	}
	return Active;
}

AGRWarga* AGRCrowd::TakeFromPool()
{
	for (AGRWarga* Person : People)
	{
		if (!Person->bActive)
		{
			return Person;
		}
	}
	return nullptr;
}

bool AGRCrowd::SpawnActivity(const FVector& Around, bool bAnywhere)
{
	using EMode = AGRWarga::EMode;
	using ERole = AGRWarga::ERole;
	// Close enough that the streets around Bayu look busy, far enough to appear out of sight.
	const float MinR = bAnywhere ? 300.f : 2500.f;
	const float MaxR = bAnywhere ? 4500.f : 5200.f;
	const bool bOutOfSight = !bAnywhere;
	const int32 Hour = HourNow(this);
	const bool bNight = Hour >= 20 || Hour < 5;
	int32 Vendors = 0;
	CountActive(Vendors);
	const float JogShare = (Hour >= 5 && Hour < 8) || (Hour >= 16 && Hour < 18) ? 0.15f : 0.04f;
	const float Roll = Rng.FRand();

	if (Roll < 0.2f)
	{
		// A few people chatting.
		FVector Middle;
		if (!FindSpot(Around, MinR, MaxR, bOutOfSight, Rng.FRand() < 0.4f, Middle))
		{
			return false;
		}
		const int32 Count = Rng.FRand() < 0.6f ? 2 : 3;
		const float Ring = Count == 2 ? 55.f : 68.f;
		const float Base = Rng.FRandRange(0.f, 360.f);
		const float Time = Rng.FRandRange(25.f, 70.f);
		const int32 Id = NextGroup++;
		TArray<AGRWarga*> Placed;
		for (int32 i = 0; i < Count; ++i)
		{
			const float Yaw = Base + 360.f * i / Count;
			const FVector2D P = FVector2D(Middle) + FVector2D(FMath::Cos(FMath::DegreesToRadians(Yaw)), FMath::Sin(FMath::DegreesToRadians(Yaw))) * Ring;
			float Z = 0.f;
			AGRWarga* Person = IsWalkable(P, Z) && !PersonNear(FVector(P, Z), 80.f, nullptr) ? TakeFromPool() : nullptr;
			if (!Person)
			{
				continue;
			}
			Person->Dress(NextSeed++, ERole::Stroller, bNight);
			Person->Activate(FVector(P, Z), Yaw + 180.f);
			Person->Mode = EMode::Chat;
			Person->Group = Id;
			Person->LookAt = FVector(Middle.X, Middle.Y, Z + CapsuleHalf);
			Person->ActivityTime = Time + Rng.FRandRange(-3.f, 3.f);
			Person->TalkTimer = Rng.FRandRange(0.5f, 4.f);
			Placed.Add(Person);
		}
		if (Placed.Num() == 1)
		{
			Placed[0]->LeaveActivity();
		}
		return !Placed.IsEmpty();
	}
	if (Roll < 0.35f)
	{
		// Someone (or two) on a bench.
		TArray<int32> Free;
		for (int32 i = 0; i < Benches.Num(); ++i)
		{
			const FBench& B = Benches[i];
			const float D = FVector::Dist2D(B.Feet, Around);
			if (!B.bHidden && !B.Sitters[0].IsValid() && !B.Sitters[1].IsValid() && D > MinR && D < MaxR
				&& !(bOutOfSight && InView(B.Feet)) && !PersonNear(B.Feet, 150.f, nullptr))
			{
				Free.Add(i);
			}
		}
		if (Free.IsEmpty())
		{
			return false;
		}
		const int32 Pick = Free[Rng.RandRange(0, Free.Num() - 1)];
		const int32 Count = Rng.FRand() < 0.45f ? 2 : 1;
		const int32 First = Count == 2 ? 0 : Rng.RandRange(0, 1);
		for (int32 s = First; s < First + Count; ++s)
		{
			AGRWarga* Person = TakeFromPool();
			if (!Person)
			{
				break;
			}
			Person->Dress(NextSeed++, ERole::Stroller, bNight);
			Person->Activate(SeatFeet(Benches[Pick], s), Benches[Pick].Yaw);
			Person->SitDown(Pick, s);
		}
		return true;
	}
	ERole NewRole = ERole::Stroller;
	if (Roll < 0.35f + JogShare)
	{
		NewRole = ERole::Jogger;
	}
	else if (Roll < 0.42f + JogShare && Vendors < MaxVendors && (!bNight || Vendors == 0))
	{
		NewRole = ERole::Vendor;
	}
	FVector Feet;
	if (!FindSpot(Around, MinR, MaxR, bOutOfSight, NewRole != ERole::Stroller || Rng.FRand() < 0.6f, Feet))
	{
		return false;
	}
	AGRWarga* Person = TakeFromPool();
	if (!Person)
	{
		return false;
	}
	Person->Dress(NextSeed++, NewRole, bNight);
	Person->Activate(Feet, Rng.FRandRange(0.f, 360.f));
	Person->NextTarget(false);
	return true;
}

bool AGRCrowd::FindSpot(const FVector& Around, float MinR, float MaxR, bool bOutOfSight, bool bPavement, FVector& OutFeet) const
{
	for (int32 Try = 0; Try < 30; ++Try)
	{
		const float Angle = Rng.FRandRange(0.f, 2.f * PI);
		FVector2D P = FVector2D(Around) + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Rng.FRandRange(MinR, MaxR);
		if (bPavement)
		{
			// Snap to the nearest of the four pavements.
			const int32 Axis = FMath::Abs(FMath::Abs(P.X) - PavementLine) < FMath::Abs(FMath::Abs(P.Y) - PavementLine) ? 0 : 1;
			const float Along = Axis == 0 ? P.Y : P.X;
			const float Side = (Axis == 0 ? P.X : P.Y) >= 0.f ? 1.f : -1.f;
			if (FMath::Abs(Along) < AlongNear || FMath::Abs(Along) > AlongFar)
			{
				continue;
			}
			P = PavementPoint(Axis, Side, Along, PavementLine + Rng.FRandRange(-80.f, 80.f));
			const float D = FVector2D::Distance(P, FVector2D(Around));
			if (D < MinR * 0.7f || D > MaxR * 1.3f)
			{
				continue;
			}
		}
		float Z = 0.f;
		if (!IsWalkable(P, Z))
		{
			continue;
		}
		const FVector Feet(P, Z);
		if (PersonNear(Feet, 160.f, nullptr) || (bOutOfSight && InView(Feet)))
		{
			continue;
		}
		OutFeet = Feet;
		return true;
	}
	return false;
}

bool AGRCrowd::IsRoad(const FVector2D& P) const
{
	const float AX = FMath::Abs(P.X);
	const float AY = FMath::Abs(P.Y);
	return AX < RoadEdge || AY < RoadEdge || P.SizeSquared() < FMath::Square(2600.f)
		|| FMath::Abs(AX - RingRoadLine) < 450.f || FMath::Abs(AY - RingRoadLine) < 450.f;
}

bool AGRCrowd::IsWalkable(const FVector2D& P, float& OutZ) const
{
	if (IsRoad(P) || FMath::Max(FMath::Abs(P.X), FMath::Abs(P.Y)) > MapLimit)
	{
		return false;
	}
	for (const FBox& Room : Rooms)
	{
		if (P.X > Room.Min.X && P.X < Room.Max.X && P.Y > Room.Min.Y && P.Y < Room.Max.Y)
		{
			return false;
		}
	}
	for (const FVector& K : KeepOut)
	{
		if (FVector2D::DistSquared(P, FVector2D(K)) < FMath::Square(K.Z))
		{
			return false;
		}
	}
	// Open ground (not a table, a roof edge or a tree top), with room for a body above it.
	if (!GroundAt(GetWorld(), P, OutZ) || OutZ > 45.f || OutZ < -60.f)
	{
		return false;
	}
	return !BodyBlocked(GetWorld(), FVector(P, OutZ + 100.f), FQuat::Identity, FCollisionShape::MakeCapsule(42.f, 60.f));
}

bool AGRCrowd::InView(const FVector& Feet) const
{
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Camera)
	{
		return false;
	}
	const FVector Eye = Camera->GetCameraLocation();
	const FVector Head = Feet + FVector(0.f, 0.f, 150.f);
	const FVector To = Head - Eye;
	const float Dist = To.Size();
	if (Dist < 1200.f)
	{
		return true;
	}
	if (Dist > 12000.f || FVector::DotProduct(Camera->GetCameraRotation().Vector(), To / Dist) < 0.6f)
	{
		return false;
	}
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByChannel(Hit, Eye, Head, ECC_Visibility, FCollisionQueryParams(SCENE_QUERY_STAT(GRCrowdView), false), BodyResponse());
}

bool AGRCrowd::PersonNear(const FVector& P, float Radius, const AGRWarga* Except) const
{
	for (const AGRWarga* Person : People)
	{
		if (Person != Except && Person->bActive && FVector::DistSquared2D(Person->GetActorLocation(), P) < FMath::Square(Radius))
		{
			return true;
		}
	}
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	return Player && FVector::DistSquared2D(Player->GetActorLocation(), P) < FMath::Square(Radius);
}

bool AGRCrowd::PavementAt(const FVector2D& P, int32& OutAxis, float& OutSide, float& OutAlong) const
{
	for (int32 Axis = 0; Axis < 2; ++Axis)
	{
		const float Lateral = Axis == 0 ? P.X : P.Y;
		const float Along = Axis == 0 ? P.Y : P.X;
		if (FMath::Abs(FMath::Abs(Lateral) - PavementLine) < PavementHalf + 20.f
			&& FMath::Abs(Along) > AlongNear - 300.f && FMath::Abs(Along) < AlongFar + 1000.f)
		{
			OutAxis = Axis;
			OutSide = Lateral >= 0.f ? 1.f : -1.f;
			OutAlong = Along;
			return true;
		}
	}
	return false;
}

FVector2D AGRCrowd::PavementPoint(int32 Axis, float Side, float Along, float Lateral) const
{
	return Axis == 0 ? FVector2D(Side * Lateral, Along) : FVector2D(Along, Side * Lateral);
}

bool AGRCrowd::ClearPath(const AGRWarga* Who, const FVector& From, const FVector& To, bool bMayCrossRoad) const
{
	if (!bMayCrossRoad)
	{
		// Someone already standing on the road may walk off it, but nobody wanders onto it.
		bool bOffRoad = !IsRoad(FVector2D(From));
		const float Length = FVector::Dist2D(From, To);
		const int32 Samples = FMath::Max(1, FMath::CeilToInt(Length / 100.f));
		for (int32 i = 1; i <= Samples; ++i)
		{
			const bool bRoad = IsRoad(FVector2D(FMath::Lerp(From, To, float(i) / Samples)));
			if (bRoad && bOffRoad)
			{
				return false;
			}
			bOffRoad |= !bRoad;
		}
	}
	// Knee to chest height: lamps, trees, walls, benches, stalls. Kerbs are lower.
	FCollisionQueryParams Query(SCENE_QUERY_STAT(GRCrowdPath), false, Who);
	FHitResult Hit;
	const FVector Knee(0.f, 0.f, -30.f);
	return !GetWorld()->SweepSingleByChannel(Hit, From + Knee, To + Knee, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(35.f), Query, BodyResponse());
}

bool AGRCrowd::CarNear(const FVector2D& P, float Radius) const
{
	for (TActorIterator<AGRTrafficCar> It(GetWorld()); It; ++It)
	{
		if (FVector2D::DistSquared(FVector2D(It->GetActorLocation()), P) < FMath::Square(Radius))
		{
			return true;
		}
	}
	return false;
}

FString AGRCrowd::PickChatLine(bool bOpener) const
{
	if (!bOpener)
	{
		return PickOne(Rng, ReplyLines);
	}
	// A third of the topics are what the town talks about this chapter.
	const UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>();
	if (Story && Rng.FRand() < 0.35f)
	{
		TArray<FString> Lines;
		for (const FGRBarkDef& Bark : Story->GetData().Barks)
		{
			if (Story->Check(Bark.If))
			{
				Lines.Add(Bark.Text);
			}
		}
		if (!Lines.IsEmpty())
		{
			return Lines[Rng.RandRange(0, Lines.Num() - 1)];
		}
	}
	return PickOne(Rng, OpenerLines);
}

void AGRCrowd::Lineup()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	using ERole = AGRWarga::ERole;
	const ERole Roles[] = { ERole::Stroller, ERole::Stroller, ERole::Stroller, ERole::Stroller, ERole::Stroller, ERole::Jogger, ERole::Vendor };
	const FVector Ahead = Player->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right(-Ahead.Y, Ahead.X, 0.f);
	const float Facing = (-Ahead).Rotation().Yaw;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Roles); ++i)
	{
		const FVector2D P(Player->GetActorLocation() + Ahead * 400.f + Right * (i - 3) * 130.f);
		float Z = 0.f;
		AGRWarga* Person = TakeFromPool();
		if (!Person || !GroundAt(GetWorld(), P, Z))
		{
			continue;
		}
		Person->Dress(NextSeed++, Roles[i], false);
		Person->Activate(FVector(P, Z), Facing);
		Person->Wait = 1e6f;
		// They stand their ground, even in front of the scooter (for testing the crash reactions too).
		Person->DodgeCooldown = 1e6f;
		if (Roles[i] == ERole::Vendor)
		{
			// The vendor pushes the cart a few steps toward Bayu, to show the pushing walk.
			Person->Target = FVector(P, Z + CapsuleHalf) - Ahead * 300.f;
			Person->Mode = AGRWarga::EMode::Walk;
		}
	}
	// And two on the nearest bench.
	int32 Nearest = INDEX_NONE;
	for (int32 i = 0; i < Benches.Num(); ++i)
	{
		if (!Benches[i].bHidden && (Nearest == INDEX_NONE
			|| FVector::DistSquared2D(Benches[i].Feet, Player->GetActorLocation()) < FVector::DistSquared2D(Benches[Nearest].Feet, Player->GetActorLocation())))
		{
			Nearest = i;
		}
	}
	for (int32 s = 0; Nearest != INDEX_NONE && s < 2; ++s)
	{
		if (AGRWarga* Sitter = Benches[Nearest].Sitters[s].Get())
		{
			Sitter->Deactivate();
		}
		if (AGRWarga* Person = TakeFromPool())
		{
			Person->Dress(NextSeed++, ERole::Stroller, false);
			Person->Activate(SeatFeet(Benches[Nearest], s), Benches[Nearest].Yaw);
			Person->SitDown(Nearest, s);
			Person->ActivityTime = 1e6f;
		}
	}
}

void AGRCrowd::LogCrowd() const
{
	int32 Modes[6] = {};
	int32 Roles[3] = {};
	int32 Active = 0;
	int32 OnRoad = 0;
	for (const AGRWarga* Person : People)
	{
		if (Person->bActive)
		{
			++Active;
			++Modes[int32(Person->Mode)];
			++Roles[int32(Person->Role)];
			OnRoad += IsRoad(FVector2D(Person->GetActorLocation())) ? 1 : 0;
		}
	}
	int32 Hidden = 0;
	for (const FBench& B : Benches)
	{
		Hidden += B.bHidden ? 1 : 0;
	}
	UE_LOG(LogGRCrowd, Warning, TEXT("GRWARGA aktif %d/%d target %d | jalan %d diam %d ngobrol %d duduk %d jatuh %d marah %d | joging %d pedagang %d | di jalan raya %d | bangku %d (%d disimpan)"),
		Active, People.Num(), TargetPopulation(), Modes[0], Modes[1], Modes[2], Modes[3], Modes[4], Modes[5], Roles[1], Roles[2], OnRoad, Benches.Num(), Hidden);
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	for (const FBox& Room : Rooms)
	{
		UE_LOG(LogGRCrowd, Warning, TEXT("GRWARGA ruang (%.0f, %.0f) - (%.0f, %.0f)"), Room.Min.X, Room.Min.Y, Room.Max.X, Room.Max.Y);
	}
	if (Player)
	{
		// Why spots around Bayu are refused: road, room, story keep-out, no ground, high ground, blocked.
		int32 Why[7] = {};
		FRandomStream Probe(7);
		for (int32 i = 0; i < 400; ++i)
		{
			const FVector2D P = FVector2D(Player->GetActorLocation()) + FVector2D(Probe.FRandRange(-6000.f, 6000.f), Probe.FRandRange(-6000.f, 6000.f));
			int32 Reason = 6;
			if (IsRoad(P))
			{
				Reason = 0;
			}
			else if (Rooms.ContainsByPredicate([&](const FBox& R) { return P.X > R.Min.X && P.X < R.Max.X && P.Y > R.Min.Y && P.Y < R.Max.Y; }))
			{
				Reason = 1;
			}
			else if (KeepOut.ContainsByPredicate([&](const FVector& K) { return FVector2D::DistSquared(P, FVector2D(K)) < FMath::Square(K.Z); }))
			{
				Reason = 2;
			}
			else
			{
				float Z = 0.f;
				if (!GroundAt(GetWorld(), P, Z))
				{
					Reason = 3;
				}
				else if (Z > 45.f || Z < -60.f)
				{
					Reason = 4;
				}
				else if (BodyBlocked(GetWorld(), FVector(P, Z + 100.f), FQuat::Identity, FCollisionShape::MakeCapsule(42.f, 60.f)))
				{
					Reason = 5;
				}
			}
			++Why[Reason];
		}
		UE_LOG(LogGRCrowd, Warning, TEXT("GRWARGA titik: jalan %d ruang %d jaga %d tanpa-tanah %d tinggi %d terhalang %d bisa %d"), Why[0], Why[1], Why[2], Why[3], Why[4], Why[5], Why[6]);
	}
	const TCHAR* ModeNames[] = { TEXT("jalan"), TEXT("diam"), TEXT("ngobrol"), TEXT("duduk"), TEXT("jatuh"), TEXT("marah") };
	for (int32 i = 0; i < People.Num(); ++i)
	{
		const AGRWarga* Person = People[i];
		if (Person->bActive && Player && FVector::Dist2D(Person->GetActorLocation(), Player->GetActorLocation()) < 3500.f)
		{
			const FVector L = Person->GetActorLocation();
			const float HeadZ = Person->GetMesh()->GetBoneLocation(TEXT("head")).Z - (L.Z - CapsuleHalf);
			UE_LOG(LogGRCrowd, Warning, TEXT("GRWARGA %2d %-7s (%.0f, %.0f, %.0f) ke (%.0f, %.0f) laju %.0f kepala %.1f skala %.2f %s%s%s"), i, ModeNames[int32(Person->Mode)],
				L.X, L.Y, L.Z, Person->Target.X, Person->Target.Y, Person->GetVelocity().Size2D(), HeadZ, Person->GetMesh()->GetRelativeScale3D().Z,
				Person->GetMesh()->GetSkeletalMeshAsset() == Person->QuinnMesh ? TEXT("quinn") : TEXT("manny"), Person->bCrossing ? TEXT(" nyeberang") : TEXT(""),
				Person->Role == AGRWarga::ERole::Vendor ? TEXT(" pedagang") : Person->Role == AGRWarga::ERole::Jogger ? TEXT(" joging") : TEXT(""));
		}
	}
}

#undef LOCTEXT_NAMESPACE
