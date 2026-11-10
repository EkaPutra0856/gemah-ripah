#include "World/GRCrowd.h"
#include "World/GRSfx.h"
#include "World/GRVfx.h"
#include "World/GROjol.h"
#include "World/GRCrossings.h"
#include "UI/GRBubbles.h"
#include "Camera/GRTopDownRoomVolume.h"
#include "Player/GRCharacter.h"
#include "Story/GRStoryDirector.h"
#include "Story/GRStorySubsystem.h"
#include "Story/GRStoryTypes.h"
#include "Story/GRStoryUtil.h"
#include "Systems/GRClockSubsystem.h"
#include "Systems/GRStatusSubsystem.h"
#include "Systems/GRSuaraSubsystem.h"
#include "UI/GRUISubsystem.h"
#include "UI/GRWargaChat.h"
#include "World/GRBody.h"
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
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LandscapeProxy.h"
#include "Engine/LocalPlayer.h"
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
	// A sale from the cart (AGRWarga::TickServe), seconds from StartServe.
	constexpr float ServeShow = 1.6f;   // the parcel comes out in the vendor's hand
	constexpr float ServeToss = 2.4f;   // thrown over the cart
	constexpr float ServeCatch = 2.95f; // in Bayu's hand
	constexpr float ServeEatEnd = 5.6f;
	constexpr float ServeKeepEnd = 4.4f;

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
	/** Muttering about Bayu while eyeing him. */
	const FColor WatchColor(235, 205, 130);
	/** The glance anim turns chest, neck and head this far right; the body turns away by as much. */
	constexpr float GlanceTurn = 56.f;
	const TCHAR* const WatchLines[] = {
		TEXT("Itu kan orang yang lagi diawasi..."), TEXT("Ssst, itu dia orangnya."), TEXT("Jangan dekat-dekat, nanti ikut didata."),
		TEXT("Mukanya mirip yang di berita."), TEXT("Hm... mencurigakan."), TEXT("Katanya dia provokator, ya?"),
	};
	const TCHAR* const BumpLines[] = {
		TEXT("Eh, hati-hati, Mas!"), TEXT("Aduh, jalan lihat-lihat, dong."), TEXT("Ih, nyenggol."), TEXT("Waduh! Pelan-pelan, Mas."),
	};
	const TCHAR* const SorryLines[] = { TEXT("Eh, maaf!"), TEXT("Maaf, maaf!"), TEXT("Sori, nggak lihat.") };
	const TCHAR* const PoliceChatLines[] = {
		TEXT("Sepi hari ini, Ndan."), TEXT("Katanya bulan ini target razia naik lagi."), TEXT("Kopi di pos habis, siapa yang beli?"),
		TEXT("Awasi yang bergerombol, ya."), TEXT("Siap, laksanakan."), TEXT("Nanti malam kita jaga Tugu."), TEXT("Daftar pantauan baru turun tadi pagi."),
	};
	const TCHAR* const FanLines[] = {
		TEXT("Eh, itu Mas Bayu yang kemarin!"), TEXT("Mas Bayu! Sehat, Mas?"), TEXT("Semangat, Mas!"), TEXT("Kami dukung, Mas."),
		TEXT("Terima kasih ya, Mas."), TEXT("Hati-hati di jalan, Mas Bayu."),
	};
	const TCHAR* const MarchLines[] = {
		TEXT("Kami di belakangmu, Mas."), TEXT("Biar mereka lihat kita banyak."), TEXT("Jalan terus, Mas!"), TEXT("Kali ini kami tidak diam."),
	};
	const TCHAR* const DefenderLines[] = {
		TEXT("Pak, dia warga sini!"), TEXT("Salahnya apa, Pak?"), TEXT("Surat tugasnya mana, Pak?"), TEXT("Kami rekam ya, Pak!"), TEXT("Jangan main tangkap, Pak!"),
	};
	const TCHAR* const RadioLines[] = {
		TEXT("Pos, ini patroli. Target terlihat, ganti."), TEXT("Orang di daftar pantauan lewat sini, ganti."), TEXT("Kami ikuti dari jauh, ganti."),
	};
	const TCHAR* const ChaseLines[] = {
		TEXT("Hei! Berhenti!"), TEXT("Jangan lari kamu!"), TEXT("Berhenti! SPN!"), TEXT("Tangkap dia!"), TEXT("Minggir, minggir!"),
	};
	const TCHAR* const PoliceGreetLines[] = {
		TEXT("Selamat siang. Ada perlu apa?"), TEXT("Jangan keluyuran malam, Mas. Jam malam berlaku."), TEXT("Kalau lihat yang mencurigakan, lapor, ya."),
		TEXT("Mas ini yang sering ikut kumpul-kumpul itu, kan?"),
	};

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

	/** Height of the landscape at P (0 where there is none): the town rolls up and down a few metres. */
	float LandAt(const UWorld* World, const FVector2D& P)
	{
		static TWeakObjectPtr<ALandscapeProxy> Land;
		if (!Land.IsValid() || Land->GetWorld() != World)
		{
			TActorIterator<ALandscapeProxy> It(const_cast<UWorld*>(World));
			Land = It ? *It : nullptr;
		}
		const TOptional<float> Z = Land.IsValid() ? Land->GetHeightAtLocation(FVector(P, 0.f)) : TOptional<float>();
		return Z.IsSet() ? Z.GetValue() : 0.f;
	}

	bool GroundAt(const UWorld* World, const FVector2D& P, float& OutZ)
	{
		FHitResult Hit;
		const float Land = LandAt(World, P);
		if (!World->LineTraceSingleByChannel(Hit, FVector(P, FMath::Max(Land, 0.f) + 400.f), FVector(P, FMath::Min(Land, 0.f) - 200.f), ECC_Visibility, FCollisionQueryParams(SCENE_QUERY_STAT(GRCrowdGround), false)))
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
	Move->RotationRate = FRotator(0.f, 260.f, 0.f);
	Move->MaxWalkSpeed = Speed;
	// Ease into a walk and out of it instead of starting and stopping dead (Tick raises these for running).
	Move->MaxAcceleration = 700.f;
	Move->bUseSeparateBrakingFriction = true;
	Move->BrakingFriction = 2.5f;
	Move->BrakingDecelerationWalking = 300.f;
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
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> WaveSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Lambai.MM_GR_Lambai"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> PhoneSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Ponsel.MM_GR_Ponsel"));
	WaveAnim = WaveSeq.Object;
	PhoneAnim = PhoneSeq.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> GlanceSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Lirik.MM_GR_Lirik"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> StartleSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Kaget.MM_GR_Kaget"));
	GlanceAnim = GlanceSeq.Object;
	StartleAnim = StartleSeq.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> GuardSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Siaga.MM_GR_Siaga"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> RadioSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Radio.MM_GR_Radio"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> GrabSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Tangkap.MM_GR_Tangkap"));
	GuardAnim = GuardSeq.Object;
	RadioAnim = RadioSeq.Object;
	GrabAnim = GrabSeq.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PhoneCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	PhoneProp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhoneProp"));
	PhoneProp->SetStaticMesh(PhoneCube.Object);
	PhoneProp->SetupAttachment(RootComponent);
	PhoneProp->SetUsingAbsoluteLocation(true);
	PhoneProp->SetUsingAbsoluteRotation(true);
	PhoneProp->SetUsingAbsoluteScale(true);
	PhoneProp->SetWorldScale3D(FVector(0.012f, 0.075f, 0.15f));
	PhoneProp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PhoneProp->SetCastShadow(false);
	PhoneProp->SetHiddenInGame(true);
	FoodProp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FoodProp"));
	FoodProp->SetupAttachment(RootComponent);
	FoodProp->SetUsingAbsoluteLocation(true);
	FoodProp->SetUsingAbsoluteRotation(true);
	FoodProp->SetUsingAbsoluteScale(true);
	FoodProp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FoodProp->SetHiddenInGame(true);
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

}

void AGRWarga::Dress(int32 Seed, ERole InRole, bool bNight)
{
	Rng.Initialize(Seed);
	Role = InRole;
	if (!Cast<UMaterialInstanceDynamic>(PhoneProp->GetMaterial(0)))
	{
		PhoneProp->SetMaterial(0, GRStoryUtil::MakeColorMaterial(this, FLinearColor(0.04f, 0.05f, 0.07f), 1.f));
	}
	const bool bFemale = Rng.FRand() < 0.5f;
	USkeletalMeshComponent* Body = GetMesh();
	USkeletalMesh* Want = bFemale ? QuinnMesh.Get() : MannyMesh.Get();
	if (Want && Body->GetSkeletalMeshAsset() != Want)
	{
		Body->SetSkeletalMeshAsset(Want);
	}
	// A new mesh restarts the anim instance, and with it any pose montage.
	CurrentPose = nullptr;
	// The look: a Tripo model (true height, own clothes, hair, hat or hijab) when imported, otherwise the
	// mannequin in random colors and headwear.
	const FString Model = GRBody::ForTownsperson(static_cast<int32>(Role), Rng.RandRange(0, 9999));
	const bool bTripo = GRBody::Apply(this, TripoBody, GRBody::Find(Model));
	// The Tripo model decides who they look like (an ojol passenger is dressed to match the name on the order).
	bIsFemale = bFemale;
	if (bTripo)
	{
		static const TCHAR* const Women[] = { TEXT("Ibu"), TEXT("Nenek"), TEXT("wanita"), TEXT("Mahasiswi"), TEXT("Siswi"), TEXT("jamu") };
		bIsFemale = false;
		for (const TCHAR* Word : Women)
		{
			bIsFemale |= Model.Contains(Word, ESearchCase::CaseSensitive);
		}
	}
	const float Scale = bTripo ? 1.f : Rng.FRandRange(0.93f, 1.04f) * (bFemale ? 0.97f : 1.f);
	Body->SetRelativeScale3D(FVector(Scale));

	// Clothes: one material over the whole mannequin, colored by height (shoes, trousers, shirt, skin).
	// SPN wear the navy uniform with black boots.
	const bool bPolice = Role == ERole::Police;
	const FLinearColor Shirt = GRStoryUtil::ParseColor(bPolice ? TEXT("#22305A") : Role == ERole::Jogger ? PickOne(Rng, SportColors) : PickOne(Rng, ShirtColors));
	const FLinearColor Pants = GRStoryUtil::ParseColor(bPolice ? TEXT("#1A2238") : PickOne(Rng, PantsColors));
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
		Clothes->SetScalarParameterValue(TEXT("ShoeZ"), bPolice ? 22.f : 9.f);
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
	if (bPolice)
	{
		// Service cap with a stiff peak.
		const FLinearColor Cap = GRStoryUtil::ParseColor(TEXT("#1C2747"));
		PutPiece(Headwear, TEXT("cylinder"), FVector(-1.f, 0.f, Top - 2.f), FVector(0.23f, 0.22f, 0.1f), Cap);
		PutPiece(HeadExtra, TEXT("cube"), FVector(12.f, 0.f, Top - 6.f), FVector(0.13f, 0.19f, 0.015f), GRStoryUtil::ParseColor(TEXT("#111111")));
	}
	else if (Role == ERole::Vendor && !bFemale && Roll < 0.6f)
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
	if (bTripo)
	{
		Headwear->SetVisibility(false);
		HeadExtra->SetVisibility(false);
	}
	BarkZ = Top + 40.f;
	BubbleZ = BarkZ;

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

	Speed = Role == ERole::Jogger ? Rng.FRandRange(320.f, 380.f) : bVendor ? Rng.FRandRange(70.f, 85.f) : bPolice ? Rng.FRandRange(112.f, 125.f) : Rng.FRandRange(105.f, 150.f);
	StripDir = Rng.FRand() < 0.5f ? 1.f : -1.f;
}

void AGRWarga::Activate(const FVector& Feet, float Yaw)
{
	bSupporter = false;
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
	bToZebra = false;
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
	WaveTime = 0.f;
	NoticeCooldown = Rng.FRandRange(4.f, 15.f);
	bPhone = false;
	PhoneProp->SetHiddenInGame(true);
	GlanceTime = 0.f;
	GlanceCooldown = Rng.FRandRange(0.f, 10.f);
	StartleTime = 0.f;
	FollowLost = 0.f;
	FollowClose = 0.f;
	ShoveCooldown = 0.f;
	Partner = nullptr;
	bLeader = true;
	ChaseLost = 0.f;
	ChaseShout = 0.f;
	ChaseCooldown = 0.f;
	RadioTime = 0.f;
	AngryAt = nullptr;
	DriverCar = nullptr;
	UGRBubbleSubsystem::Hide(this);
	SetSeatedLook(false);
	bOjol = false;
	bOjolWalk = false;
	bOjolSeated = false;
}

void AGRWarga::Deactivate()
{
	if (bOjolSeated)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	bOjol = false;
	bOjolWalk = false;
	bOjolSeated = false;
	StopRagdoll();
	if (AGRTrafficCar* Car = DriverCar.Get())
	{
		// Recycled while out of the car: it drives on without them.
		Car->HoldForDriver(0.f);
	}
	DriverCar = nullptr;
	AngryAt = nullptr;
	// A pair split by recycling: the one left patrols alone.
	if (AGRWarga* Mate = Partner.Get(); Mate && Mate->Partner == this)
	{
		Mate->Partner = nullptr;
		Mate->bLeader = true;
	}
	Partner = nullptr;
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
	UGRBubbleSubsystem::Hide(this);
}

void AGRWarga::Say(const FString& Text, float Duration, const FColor& Color)
{
	UGRBubbleSubsystem::Show(this, Text, Duration, FLinearColor(Color), BubbleZ, Color != FColor::White);
	BarkHide = Duration;
}

bool AGRWarga::KnockedBy(AActor* Culprit, float Closing, const FVector& Direction, float Diawasi, const TCHAR* What)
{
	if (!bActive || Mode == EMode::Down || HitCooldown > 0.f || bOjolSeated)
	{
		return false;
	}
	if (bOjol)
	{
		// The customer Bayu came to fetch, run over at the kerb: the trip is off.
		OjolRelease();
		if (UGROjolSubsystem* Ojol = UGROjolSubsystem::Get(this))
		{
			Ojol->PassengerHit(this);
		}
	}
	HitCooldown = 1.f;
	GRSfx::PlayAt(this, EGRSfx::TabrakBadan, GetActorLocation(), FMath::Clamp(Closing / 500.f, 0.5f, 1.f), FMath::FRandRange(0.95f, 1.15f));
	if (UGRVfxSubsystem* Vfx = UGRVfxSubsystem::Get(this))
	{
		Vfx->Dust(GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), Closing >= 450.f ? 45.f : 28.f);
	}
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
			: Anger == EAnger::Defender ? PickOne(Rng, DefenderLines)
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
	if (Anger != EAnger::Bystander && Anger != EAnger::Defender)
	{
		Say(PickOne(Rng, CalmLines), 2.2f);
	}
	NextTarget(false);
}

void AGRWarga::StartGreet()
{
	StartGlance(false);
	WaveTime = 2.2f;
	UE_LOG(LogGRCrowd, Display, TEXT("GRWARGA %s menyapa Bayu"), *GetName());
	Say(PickOne(Rng, FanLines), 2.4f);
}

void AGRWarga::StartSupport()
{
	bSupporter = true;
	Mode = EMode::Follow;
	FollowLost = 0.f;
	GlanceCooldown = 30.f;
	Group = -1;
	bCrossing = false;
	SupportShout = Rng.FRandRange(2.f, 6.f);
	UE_LOG(LogGRCrowd, Display, TEXT("GRWARGA %s berjalan bersama Bayu"), *GetName());
	Say(PickOne(Rng, MarchLines), 2.4f);
}

void AGRWarga::TickSupport(float DeltaSeconds, const APawn* Player)
{
	const AGRCharacter* Bayu = Cast<AGRCharacter>(Player);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const FVector To = Bayu ? Bayu->GetActorLocation() - GetActorLocation() : FVector::ZeroVector;
	const float Dist = To.Size2D();
	const UGRSuaraSubsystem* Suara = UGRSuaraSubsystem::Get(this);
	const bool bGone = !Bayu || Bayu->IsRiding() || Bayu->IsInTopDownMode() || Dist > 1700.f || !Suara || !Suara->HasTier(EGRSuaraTier::Gerakan);
	FollowLost = bGone ? FollowLost + DeltaSeconds : 0.f;
	if (FollowLost > 1.5f)
	{
		bSupporter = false;
		Move->MaxWalkSpeed = Speed;
		NextTarget(false);
		return;
	}
	if (Dist > 300.f)
	{
		// Keeps up with him walking and falls behind when he runs.
		Move->MaxWalkSpeed = FMath::FInterpTo(Move->MaxWalkSpeed, Dist > 700.f ? 460.f : 340.f, DeltaSeconds, 3.f);
		AddMovementInput(To.GetSafeNormal2D(), 1.f);
	}
	SupportShout -= DeltaSeconds;
	if (SupportShout <= 0.f)
	{
		SupportShout = Rng.FRandRange(9.f, 16.f);
		Say(PickOne(Rng, MarchLines), 2.2f);
	}
}

void AGRWarga::StartGlance(bool bMutter)
{
	GlanceResume = Mode == EMode::Pause || Mode == EMode::Chat ? Mode : EMode::Walk;
	Mode = EMode::Glance;
	UE_LOG(LogGRCrowd, Display, TEXT("GRWARGA %s melirik Bayu"), *GetName());
	GlanceTime = Rng.FRandRange(1.8f, 2.8f);
	GlanceCooldown = 30.f;
	if (bMutter && Rng.FRand() < 0.5f)
	{
		// Muttered, not shouted: no shake, and the glance stays the pose.
		UGRBubbleSubsystem::Show(this, PickOne(Rng, WatchLines), 2.2f, FLinearColor(WatchColor), BubbleZ, false);
	}
}

void AGRWarga::TickGlance(float DeltaSeconds, const APawn* Player)
{
	GlanceTime -= DeltaSeconds;
	if (Player)
	{
		// The body turns away by as much as the anim turns the head, so the eyes land on him.
		const float Yaw = (Player->GetActorLocation() - GetActorLocation()).Rotation().Yaw - GlanceTurn;
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Yaw, 0.f), DeltaSeconds, 5.f));
	}
	if (GlanceTime <= 0.f)
	{
		Mode = GlanceResume;
		WalkTime = 0.f;
		Stuck = 0.f;
	}
}

void AGRWarga::StartFollow()
{
	Mode = EMode::Follow;
	FollowLost = 0.f;
	FollowClose = 0.f;
	GlanceCooldown = 30.f;
	Group = -1;
	bCrossing = false;
	UE_LOG(LogGRCrowd, Display, TEXT("GR%s %s membuntuti Bayu"), IsPolice() ? TEXT("SPN") : TEXT("WARGA"), *GetName());
	if (IsPolice())
	{
		// Calls it in first, then keeps an eye on him from a distance.
		RadioTime = 3.f;
		UGRBubbleSubsystem::Show(this, PickOne(Rng, RadioLines), 3.f, FLinearColor(WatchColor), BubbleZ, false);
	}
	UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>();
	if (Story && IsLeader())
	{
		Story->Run(IsPolice() ? TEXT("notif Petugas SPN membuntutimu. Menjauh pelan-pelan; lari di depan mereka bikin dikejar.")
			: TEXT("notif Ada warga yang membuntutimu. Jalan cepat atau lari supaya lolos."));
	}
}

void AGRWarga::TickFollow(float DeltaSeconds, const APawn* Player)
{
	if (bSupporter)
	{
		TickSupport(DeltaSeconds, Player);
		return;
	}
	const AGRCharacter* Bayu = Cast<AGRCharacter>(Player);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const FVector Loc = GetActorLocation();
	const FVector To = Bayu ? Bayu->GetActorLocation() - Loc : FVector::ZeroVector;
	const float Dist = To.Size2D();
	const bool bPolice = IsPolice();
	// An officer tailing him sees him break into a run: now it is a chase.
	if (bPolice && Bayu && !Bayu->IsRiding() && Dist < 1100.f && Bayu->GetVelocity().SizeSquared2D() > FMath::Square(600.f))
	{
		StartChase(true);
		if (AGRWarga* Mate = Partner.Get(); Mate && Mate->bActive && Mate->Mode != EMode::Down && Mate->Mode != EMode::Chase)
		{
			Mate->StartChase(false);
		}
		return;
	}
	// Slower than Bayu walking and far slower than him running: keep moving and the gap only grows.
	// Officers hang further back than curious townspeople.
	const float Keep = bPolice ? 650.f : 400.f;
	const bool bMoving = GetVelocity().SizeSquared2D() > FMath::Square(40.f);
	const bool bGo = Dist > (bMoving ? Keep : Keep + 120.f);
	if (bGo)
	{
		const float Want = Dist > Keep + 500.f ? (bPolice ? 400.f : 420.f) : (bPolice ? 280.f : 300.f);
		Move->MaxWalkSpeed = FMath::FInterpTo(Move->MaxWalkSpeed, Want, DeltaSeconds, 3.f);
		AddMovementInput(To.GetSafeNormal2D(), 1.f);
	}
	else if (Bayu)
	{
		const float Yaw = To.Rotation().Yaw - GlanceTurn;
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Yaw, 0.f), DeltaSeconds, 4.f));
	}
	// Shaken off: far ahead, on the scooter, indoors, or the way is blocked.
	Stuck = bGo && !bMoving ? Stuck + DeltaSeconds : 0.f;
	const bool bLost = !Bayu || Bayu->IsRiding() || Bayu->IsInTopDownMode() || Dist > 1700.f || Stuck > 2.f;
	FollowLost = bLost ? FollowLost + DeltaSeconds : 0.f;
	if (FollowLost > 1.5f)
	{
		UE_LOG(LogGRCrowd, Display, TEXT("GRWARGA %s kehilangan jejak Bayu"), *GetName());
		Move->MaxWalkSpeed = Speed;
		UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>();
		if (Story && IsLeader())
		{
			Story->Run(bPolice ? TEXT("notif Kamu lolos dari pengawasan SPN.") : TEXT("notif Kamu lolos dari warga yang membuntuti."));
		}
		NextTarget(false);
		return;
	}
	FollowClose += Dist < 1200.f ? DeltaSeconds : 0.f;
	if (FollowClose < 25.f)
	{
		return;
	}
	// Stuck to him long enough: out comes the phone (or the radio).
	UE_LOG(LogGRCrowd, Display, TEXT("GRWARGA %s melaporkan Bayu"), *GetName());
	Move->MaxWalkSpeed = Speed;
	UGRBubbleSubsystem::Show(this, bPolice ? TEXT("Pos, target masih di sini. Mohon arahan, ganti.") : TEXT("Halo, Pak SPN? Orangnya ada di sini..."),
		3.5f, FLinearColor(WatchColor), BubbleZ, false);
	UGameInstance* GI = GetGameInstance();
	const bool bReport = !bPolice || IsLeader();
	if (UGRStatusSubsystem* Status = GI->GetSubsystem<UGRStatusSubsystem>(); Status && bReport)
	{
		Status->AddDiawasi(bPolice ? 3.f : 4.f, true);
	}
	if (UGRStorySubsystem* Story = GI->GetSubsystem<UGRStorySubsystem>(); Story && bReport)
	{
		Story->Run(bPolice ? TEXT("notifburuk Petugas SPN melaporkan gerak-gerikmu. Diawasi +3.") : TEXT("notifburuk Warga yang membuntutimu melapor ke SPN. Diawasi +4."));
	}
	Mode = EMode::Pause;
	bPhone = !bPolice;
	RadioTime = bPolice ? 3.5f : 0.f;
	ChaseCooldown = bPolice ? 30.f : ChaseCooldown;
	Wait = 6.f;
}

void AGRWarga::Bumped(APawn* Player, const FVector& Direction, bool bHard)
{
	AGRCrowd* C = Crowd.Get();
	const float Streak = C ? C->NoteBump(bHard) : 0.f;
	const FVector Dir = Direction.GetSafeNormal2D();
	if (AGRCharacter* Bayu = Cast<AGRCharacter>(Player))
	{
		// The bump costs him his momentum too.
		Bayu->GetCharacterMovement()->Velocity *= 0.35f;
		if (Rng.FRand() < 0.6f)
		{
			UGRBubbleSubsystem::Show(Bayu, PickOne(Rng, SorryLines), 1.4f);
		}
	}
	if (Streak >= 5.f)
	{
		// Again and again in a few seconds: nobody thinks it was an accident any more.
		KnockedBy(Player, bHard ? 300.f : 220.f, Dir, bHard ? 3.f : 2.f, TEXT("Bayu menabrak-nabrak orang di trotoar."));
		return;
	}
	if (Mode == EMode::Angry)
	{
		AngryTime = FMath::Max(AngryTime, 2.5f);
		Say(PickOne(Rng, ShoveLines), 1.8f, AngryColor);
		return;
	}
	if (bHard)
	{
		// Ran into: shoved aside and cross, but once is not a report.
		KnockedBy(Player, 260.f, Dir, 0.f);
		return;
	}
	// Brushed past: a flinch and a step back, then a word with him.
	Dodge = 0.f;
	StartleTime = 0.9f;
	LaunchCharacter(Dir * 180.f, true, false);
	TalkPause = FMath::Max(TalkPause, 1.6f);
	Say(PickOne(Rng, BumpLines), 2.f);
}

void AGRWarga::TickPartner(float DeltaSeconds)
{
	const AGRWarga* Lead = Partner.Get();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Mode = Lead->Mode == EMode::Pause ? EMode::Pause : EMode::Walk;
	// Half a step behind the leader's shoulder, matching the leader's pace: the right one, unless a wall is there.
	const FVector LeadAt = Lead->GetActorLocation();
	FVector Slot = LeadAt + Lead->GetActorRightVector() * 95.f - Lead->GetActorForwardVector() * 15.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GRSpnPartner), false, this);
	Params.AddIgnoredActor(Lead);
	if (GetWorld()->LineTraceTestByChannel(LeadAt, Slot + (Slot - LeadAt).GetSafeNormal2D() * 40.f, ECC_Visibility, Params))
	{
		Slot = LeadAt - Lead->GetActorRightVector() * 95.f - Lead->GetActorForwardVector() * 15.f;
	}
	// Caught on something: fall in right behind the leader instead.
	if (Stuck > 1.f)
	{
		Slot = LeadAt - Lead->GetActorForwardVector() * 90.f;
	}
	const FVector2D To(Slot - GetActorLocation());
	const float Dist = To.Size();
	const float LeadSpeed = Lead->GetVelocity().Size2D();
	if (Dist > 1500.f)
	{
		// Split up (a wall, a crowd): patrol alone from here.
		Partner = nullptr;
		bLeader = true;
		NextTarget(false);
		return;
	}
	if (Dist > 35.f)
	{
		Move->MaxWalkSpeed = FMath::FInterpTo(Move->MaxWalkSpeed, FMath::Clamp(LeadSpeed + Dist * 0.8f, 60.f, 280.f), DeltaSeconds, 4.f);
		AddMovementInput(FVector(To / Dist, 0.f), FMath::Clamp(Dist / 120.f, 0.35f, 1.f));
		Stuck = Dist > 150.f && GetVelocity().SizeSquared2D() < FMath::Square(30.f) ? Stuck + DeltaSeconds : FMath::Max(0.f, Stuck - DeltaSeconds * 0.5f);
	}
	else if (LeadSpeed < 20.f)
	{
		// Standing with the leader: face the same way, or face them while one of the two is talking.
		const float Yaw = Lead->BarkHide > 0.f || BarkHide > 0.f ? (Lead->GetActorLocation() - GetActorLocation()).Rotation().Yaw : Lead->GetActorRotation().Yaw;
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Yaw, 0.f), DeltaSeconds, 3.f));
	}
}

void AGRWarga::TickPolicePause(float DeltaSeconds, const APawn* Player)
{
	Wait -= DeltaSeconds;
	TalkTimer -= DeltaSeconds;
	AGRWarga* Mate = Partner.Get();
	if (Mate && Mate->bActive)
	{
		// Small talk between the two now and then, turned toward each other.
		if (TalkTimer <= 0.f && BarkHide <= 0.f && Mate->BarkHide <= 0.f)
		{
			TalkTimer = Rng.FRandRange(4.f, 9.f);
			(Rng.FRand() < 0.5f ? this : Mate)->Say(PickOne(Rng, PoliceChatLines), 3.f);
		}
		if (BarkHide > 0.f || Mate->BarkHide > 0.f)
		{
			const FRotator Face(0.f, (Mate->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0.f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 3.f));
		}
	}
	if (Wait <= 0.f)
	{
		NextTarget(false);
	}
}

void AGRWarga::StartChase(bool bShout)
{
	if (Mode == EMode::Sit)
	{
		StandUp();
	}
	UE_LOG(LogGRCrowd, Display, TEXT("GRSPN %s mengejar Bayu"), *GetName());
	Mode = EMode::Chase;
	ChaseLost = 0.f;
	ChaseShout = Rng.FRandRange(2.2f, 3.f);
	if (bShout)
	{
		Say(TEXT("Hei! Berhenti!"), 1.8f, AngryColor);
	}
	Group = -1;
	bCrossing = false;
	TalkPause = 0.f;
	Dodge = 0.f;
	StartleTime = 0.f;
	Stuck = 0.f;
	// A moment to point and shout before the run: Bayu gets a head start.
	Wait = bShout ? 0.8f : 1.f;
}

void AGRWarga::TickChase(float DeltaSeconds, APawn* Player)
{
	AGRCharacter* Bayu = Cast<AGRCharacter>(Player);
	if (!Bayu)
	{
		EndChase(true);
		return;
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const FVector Loc = GetActorLocation();
	const float Dist = FVector::Dist2D(Bayu->GetActorLocation(), Loc);
	if (Wait > 0.f)
	{
		Wait -= DeltaSeconds;
		const FRotator Face(0.f, (Bayu->GetActorLocation() - Loc).Rotation().Yaw, 0.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 6.f));
		return;
	}
	// Picks up speed over a second instead of bolting: faster than Bayu walking, slower than him running.
	Move->MaxWalkSpeed = FMath::FInterpTo(Move->MaxWalkSpeed, 560.f, DeltaSeconds, 2.5f);
	// Run at where he is going; the second officer of a pair cuts in from the side.
	FVector Aim = Bayu->GetActorLocation() + Bayu->GetVelocity() * FMath::Clamp(Dist / 700.f, 0.1f, 0.6f);
	if (!IsLeader() && Dist > 450.f)
	{
		const FVector Ahead = (Aim - Loc).GetSafeNormal2D();
		Aim += FVector(-Ahead.Y, Ahead.X, 0.f) * 200.f;
	}
	AddMovementInput((Aim - Loc).GetSafeNormal2D(), 1.f);
	ChaseShout -= DeltaSeconds;
	if (ChaseShout <= 0.f)
	{
		ChaseShout = Rng.FRandRange(2.2f, 3.4f);
		Say(PickOne(Rng, ChaseLines), 1.8f, AngryColor);
	}
	AGRCrowd* C = Crowd.Get();
	if (Dist < 125.f && !Bayu->IsRiding() && !Bayu->IsKnockedDown() && C && !C->IsArresting())
	{
		C->StartArrest(this);
		return;
	}
	// Lost him: out of sight for a while, far ahead, off on the scooter, indoors, or stuck behind something.
	bool bSees = false;
	if (Dist < 2600.f)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GRSpnChase), false, this);
		Params.AddIgnoredActor(Bayu);
		bSees = !GetWorld()->LineTraceTestByChannel(Loc + FVector(0.f, 0.f, 60.f), Bayu->GetActorLocation() + FVector(0.f, 0.f, 60.f), ECC_Visibility, Params);
	}
	Stuck = GetVelocity().SizeSquared2D() < FMath::Square(40.f) ? Stuck + DeltaSeconds : 0.f;
	const bool bGone = Dist > 2000.f || Bayu->IsInTopDownMode() || (Bayu->IsRiding() && Dist > 700.f) || (!bSees && Dist > 900.f) || Stuck > 1.5f;
	ChaseLost = bGone ? ChaseLost + DeltaSeconds : 0.f;
	if (ChaseLost > 2.5f)
	{
		EndChase(true);
	}
}

void AGRWarga::EndChase(bool bLost)
{
	UE_LOG(LogGRCrowd, Display, TEXT("GRSPN %s berhenti mengejar"), *GetName());
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	ChaseCooldown = 25.f;
	// Catch their breath and call it in.
	Mode = EMode::Pause;
	Wait = Rng.FRandRange(3.f, 4.5f);
	Stuck = 0.f;
	if (bLost)
	{
		RadioTime = 3.f;
		UGRBubbleSubsystem::Show(this, Rng.FRand() < 0.5f ? TEXT("Pos, target lolos, ganti.") : TEXT("Ke mana dia..."), 2.5f, FLinearColor(WatchColor), BubbleZ, false);
	}
}

void AGRWarga::TickArrest(float DeltaSeconds, const APawn* Player)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const AGRCrowd* C = Crowd.Get();
	const AGRWarga* Holder = C ? C->Arrester.Get() : nullptr;
	if (!Player || !Holder)
	{
		Mode = EMode::Pause;
		Wait = 2.f;
		Move->MaxWalkSpeed = Speed;
		return;
	}
	// The one who caught him holds him; the other comes round to his other side.
	const bool bHolder = Holder == this;
	const FVector Bayu = Player->GetActorLocation();
	const FVector Spot = bHolder ? Bayu : Bayu + (Bayu - Holder->GetActorLocation()).GetSafeNormal2D() * 115.f;
	const FVector2D To(Spot - GetActorLocation());
	const float Dist = To.Size();
	const float Stand = bHolder ? 85.f : 30.f;
	if (Dist > Stand)
	{
		Move->MaxWalkSpeed = FMath::FInterpTo(Move->MaxWalkSpeed, FMath::Clamp(Dist * 1.5f, 70.f, 320.f), DeltaSeconds, 5.f);
		AddMovementInput(FVector(To / Dist, 0.f), 1.f);
	}
	if (Dist < Stand + 60.f)
	{
		const FRotator Face(0.f, (Bayu - GetActorLocation()).Rotation().Yaw, 0.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 6.f));
	}
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

void AGRWarga::OjolStart()
{
	bOjol = true;
	bOjolWalk = false;
	bOjolSeated = false;
	Mode = EMode::Pause;
	Wait = 1.f;
	OjolPose(1);
}

void AGRWarga::OjolPose(int32 Kind)
{
	PlayPose(Kind == 1 ? PhoneAnim.Get() : Kind == 2 ? WaveAnim.Get() : nullptr);
}

void AGRWarga::OjolWalkTo(const FVector& Where)
{
	OjolTarget = Where;
	bOjolWalk = true;
	OjolWalkTime = 0.f;
	PlayPose(nullptr);
}

void AGRWarga::OjolFace(const FVector& Where)
{
	const FVector To = (Where - GetActorLocation()).GetSafeNormal2D();
	if (!To.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
	}
}

void AGRWarga::TickOjol(float DeltaSeconds)
{
	BarkHide -= DeltaSeconds;
	if (bOjolSeated || !bOjolWalk)
	{
		return;
	}
	OjolWalkTime += DeltaSeconds;
	const FVector2D To(OjolTarget - GetActorLocation());
	if (To.Size() < 40.f || OjolWalkTime > 5.f)
	{
		bOjolWalk = false;
		GetCharacterMovement()->StopMovementImmediately();
		return;
	}
	GetCharacterMovement()->MaxWalkSpeed = 180.f;
	AddMovementInput(FVector(To.GetSafeNormal(), 0.f), 1.f);
}

void AGRWarga::OjolBoard(USceneComponent* SeatPart, const FTransform& Relative, bool bSideSaddle, UAnimSequenceBase* RidePose)
{
	bOjolWalk = false;
	bOjolSeated = true;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->StopMovementImmediately();
	Move->DisableMovement();
	Move->SetComponentTickEnabled(false);
	SetActorEnableCollision(false);
	AttachToComponent(SeatPart, FAttachmentTransformRules::KeepWorldTransform);
	SetActorRelativeTransform(Relative);
	PlayPose(bSideSaddle || !RidePose ? SitAnim.Get() : RidePose);
	SetSeatedLook(true);
	UGRBubbleSubsystem::Hide(this);
}

void AGRWarga::OjolAlight(const FVector& Feet, float Yaw)
{
	if (bOjolSeated)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	bOjolSeated = false;
	SetActorEnableCollision(true);
	SetActorLocationAndRotation(Feet + FVector(0.f, 0.f, CapsuleHalf + 2.f), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::ResetPhysics);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->SetComponentTickEnabled(true);
	Move->SetMovementMode(MOVE_Walking);
	Move->Velocity = FVector::ZeroVector;
	PlayPose(nullptr);
	SetSeatedLook(false);
}

void AGRWarga::OjolFall(const FVector& Velocity)
{
	OjolAlight(GetActorLocation() - FVector(0.f, 0.f, CapsuleHalf + 40.f), GetActorRotation().Yaw);
	OjolRelease();
	StartFall(Velocity);
}

void AGRWarga::OjolRelease()
{
	if (bOjolSeated)
	{
		OjolAlight(GetActorLocation() - FVector(0.f, 0.f, CapsuleHalf), GetActorRotation().Yaw);
	}
	bOjol = false;
	bOjolWalk = false;
	Mode = EMode::Pause;
	Wait = Rng.FRandRange(1.5f, 3.f);
	PlayPose(nullptr);
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
		AnimInstance->StopSlotAnimation(0.4f, TEXT("DefaultSlot"));
		return;
	}
	// The pushing walk comes from a walk cycle, which can carry root motion; the crowd code moves people itself.
	AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	CurrentMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(Anim, TEXT("DefaultSlot"), 0.4f, 0.4f, Rate, 10000);
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
	BubbleZ = bSeated ? BarkZ - 42.f : BarkZ;
	if (Clothes)
	{
		// Seated, the waist sits just above the thighs and the neck drops with the pelvis.
		Clothes->SetScalarParameterValue(TEXT("HipZ"), bSeated ? 58.f : HipZ);
		Clothes->SetScalarParameterValue(TEXT("NeckZ"), bSeated ? NeckZ - 42.f : NeckZ);
	}
}

FText AGRWarga::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return Role == ERole::Vendor ? LOCTEXT("GreetVendor", "Sapa pedagang") : Role == ERole::Police ? LOCTEXT("GreetSpn", "Sapa petugas") : LOCTEXT("GreetWarga", "Sapa warga");
}

bool AGRWarga::CanInteract_Implementation(AActor* Interactor) const
{
	if (!CanChat() || Dodge > 0.f || bOjol)
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
	HoldChat(4.5f);
	Say(Role == ERole::Vendor ? TEXT("Ya, Mas? Mau jajan?") : Rng.FRand() < 0.5f ? TEXT("Eh, Mas Bayu. Ada apa?") : TEXT("Iya, Mas?"), 2.5f);
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	UGRUISubsystem* UI = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UGRUISubsystem>() : nullptr;
	if (UGRWargaChatScreen* Screen = UI ? UI->PushScreen<UGRWargaChatScreen>() : nullptr)
	{
		Screen->SetWarga(this);
	}
}

FText AGRWarga::GetChatTitle() const
{
	return Role == ERole::Vendor ? LOCTEXT("ChatVendor", "Pedagang Gerobak")
		: Role == ERole::Jogger ? LOCTEXT("ChatJogger", "Warga yang Lagi Joging")
		: Role == ERole::Police ? LOCTEXT("ChatSpn", "Petugas SPN")
		: LOCTEXT("ChatWarga", "Warga Sentosa");
}

bool AGRWarga::CanChat() const
{
	return bActive && !IsHidden() && Mode != EMode::Down && Mode != EMode::Angry && Mode != EMode::Chase && Mode != EMode::Arrest
		&& !(IsPolice() && Mode == EMode::Follow);
}

void AGRWarga::HoldChat(float Seconds)
{
	TalkPause = FMath::Max(TalkPause, Seconds);
}

void AGRWarga::Chat(int32 Topic)
{
	HoldChat(5.f);
	const int32 Hour = HourNow(this);
	TArray<FString> Lines;
	if (Topic == 0)
	{
		// Hello back, with a wave (seated people and vendors keep their hands where they are).
		if (Mode != EMode::Sit && Role != ERole::Vendor)
		{
			WaveTime = 2.4f;
		}
		Lines.Add(Hour >= 4 && Hour < 11 ? TEXT("Pagi, Mas Bayu! Sehat?") : Hour < 15 ? TEXT("Siang, Mas. Panas, ya.")
			: Hour < 18 ? TEXT("Sore, Mas Bayu. Salam buat Ibu.") : TEXT("Malam, Mas. Jangan kemalaman, banyak razia."));
	}
	else if (Topic == 1)
	{
		if (Role == ERole::Vendor)
		{
			Lines.Add(TEXT("Laris, Mas. Tapi minyak goreng naik lagi, untung tipis."));
			Lines.Add(TEXT("Katanya mau ada penertiban PKL. Kami mau dagang di mana?"));
			Lines.Add(TEXT("Murah, Mas. Belum kena pajak."));
		}
		else if (Role == ERole::Police)
		{
			for (const TCHAR* Line : PoliceGreetLines)
			{
				Lines.Add(Line);
			}
		}
		else if (Role == ERole::Jogger)
		{
			Lines.Add(TEXT("Ayo lari, Mas! Biar kuat ngadepin harga."));
			Lines.Add(TEXT("Udaranya sesak, Mas. ISPU merah terus, tapi lari harus jalan."));
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
	}
	else if (Topic == 2)
	{
		// Directions to the current goal by compass point, the way people here give them.
		FVector Goal;
		FText Label;
		const AGRStoryDirector* Director = AGRStoryDirector::Get(this);
		if (!Director || !Director->GetTrackedTarget(Goal, Label))
		{
			Lines.Add(TEXT("Mau ke mana, Mas? Kota ini kecil, jalan aja pasti nyampe."));
		}
		else
		{
			const FVector To = Goal - GetActorLocation();
			const float Meters = To.Size2D() / 100.f;
			static const TCHAR* Points[] = { TEXT("utara"), TEXT("timur laut"), TEXT("timur"), TEXT("tenggara"), TEXT("selatan"), TEXT("barat daya"), TEXT("barat"), TEXT("barat laut") };
			const int32 Point = (FMath::RoundToInt(FMath::RadiansToDegrees(FMath::Atan2(To.Y, To.X)) / 45.f) + 8) % 8;
			if (Meters < 15.f)
			{
				Lines.Add(FString::Printf(TEXT("%s? Lho, itu sudah dekat sini, Mas."), *Label.ToString()));
			}
			else
			{
				const int32 Rounded = Meters < 100.f ? FMath::RoundToInt(Meters / 10.f) * 10 : FMath::RoundToInt(Meters / 50.f) * 50;
				Lines.Add(FString::Printf(TEXT("%s? Ke arah %s, Mas. Kira-kira %d meter dari sini."), *Label.ToString(), Points[Point], Rounded));
			}
		}
	}
	else if (Topic == 3 && Role == ERole::Vendor)
	{
		UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>();
		const UGRStatusSubsystem* Status = GetGameInstance()->GetSubsystem<UGRStatusSubsystem>();
		if (Story && Status && Status->GetUang() >= 5000)
		{
			Story->Run(TEXT("uang -5000 Jajan di gerobak; energi 8"));
			Lines.Add(TEXT("Makasih, Mas! Masih anget, tuh. Langganan, ya."));
		}
		else
		{
			Lines.Add(TEXT("Uangnya kurang, Mas. Ngutang? Wah, jangan deh."));
		}
	}
	if (!Lines.IsEmpty())
	{
		Say(Lines[Rng.RandRange(0, Lines.Num() - 1)], 4.5f);
	}
}

void AGRWarga::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive)
	{
		return;
	}
	if (bOjol)
	{
		TickOjol(DeltaSeconds);
		return;
	}
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Role == ERole::Vendor)
	{
		SteamWait -= DeltaSeconds;
		if (SteamWait <= 0.f)
		{
			SteamWait = FMath::FRandRange(0.3f, 0.5f);
			UGRVfxSubsystem* Vfx = UGRVfxSubsystem::Get(this);
			if (Vfx && Player && Mode != EMode::Down && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) < FMath::Square(3000.f))
			{
				Vfx->Steam(GetActorTransform().TransformPosition(FVector(115.f, 0.f, 45.f)));
			}
		}
	}
	BarkHide -= DeltaSeconds;
	DodgeCooldown -= DeltaSeconds;
	HitCooldown -= DeltaSeconds;
	ShoveCooldown -= DeltaSeconds;
	GlanceCooldown -= DeltaSeconds;
	ChaseCooldown -= DeltaSeconds;
	RadioTime -= DeltaSeconds;
	// Running (a chase, a dodge) starts and turns quickly; strolling eases in and out.
	const bool bRunning = Mode == EMode::Chase || Dodge > 0.f;
	GetCharacterMovement()->MaxAcceleration = bRunning ? 1400.f : 700.f;
	GetCharacterMovement()->RotationRate.Yaw = bRunning ? 420.f : 260.f;
	// Bayu on foot walking into someone startles them; sprinting into them shoves them aside.
	if (const AGRCharacter* Bayu = Cast<AGRCharacter>(Player); Bayu && !Bayu->IsRiding() && ShoveCooldown <= 0.f && Mode != EMode::Down && Mode != EMode::Sit
		&& Mode != EMode::Chase && Mode != EMode::Arrest)
	{
		const FVector2D Rel(GetActorLocation() - Bayu->GetActorLocation());
		const float Closing = FVector2D::DotProduct(FVector2D(Bayu->GetVelocity()), Rel.GetSafeNormal());
		if (Rel.SizeSquared() < FMath::Square(120.f) && Closing > 150.f)
		{
			ShoveCooldown = 3.f;
			Bumped(Player, FVector(Rel.GetSafeNormal(), 0.f), Closing > 560.f);
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
	WaveTime -= DeltaSeconds;
	NoticeCooldown -= DeltaSeconds;
	// Bayu walking past: some look up, stop, wave and say hello.
	if (const AGRCharacter* Bayu = Cast<AGRCharacter>(Player); Bayu && !Bayu->IsRiding() && NoticeCooldown <= 0.f && TalkPause <= 0.f
		&& (Mode == EMode::Walk || Mode == EMode::Pause) && Role != ERole::Jogger && Role != ERole::Police && !DriverCar.IsValid() && Dodge <= 0.f
		&& FVector::DistSquared2D(GetActorLocation(), Bayu->GetActorLocation()) < FMath::Square(300.f))
	{
		NoticeCooldown = Rng.FRandRange(30.f, 60.f);
		// Fewer hellos for a face the town has been told to watch.
		const UGRStatusSubsystem* Status = GetGameInstance()->GetSubsystem<UGRStatusSubsystem>();
		if (Rng.FRand() < (Status && Status->GetDiawasi() >= 50.f ? 0.15f : 0.45f))
		{
			static const TCHAR* Hello[] = { TEXT("Eh, Mas Bayu!"), TEXT("Mari, Mas."), TEXT("Mas Bayu! Sehat?"), TEXT("Monggo, Mas."), TEXT("Salam buat Ibu, Mas!") };
			HoldChat(2.2f);
			WaveTime = Role == ERole::Vendor ? 0.f : 2.2f;
			Say(Hello[Rng.RandRange(0, UE_ARRAY_COUNT(Hello) - 1)], 2.2f);
		}
	}

	if (StartleTime > 0.f)
	{
		// Flinching where they stand (MM_GR_Kaget); the word with him comes after.
		StartleTime -= DeltaSeconds;
	}
	else if (TalkPause > 0.f)
	{
		TalkPause -= DeltaSeconds;
		if (Player && Mode != EMode::Sit)
		{
			const FRotator Face(0.f, (Player->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0.f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 4.5f));
		}
	}
	else
	{
		TickDodge(Player);
		switch (Mode)
		{
		case EMode::Walk:
		case EMode::Pause:
			if (IsPolice() && !IsLeader())
			{
				TickPartner(DeltaSeconds);
			}
			else if (Mode == EMode::Walk)
			{
				TickWalk(DeltaSeconds);
			}
			else if (IsPolice())
			{
				TickPolicePause(DeltaSeconds, Player);
			}
			else
			{
				Wait -= DeltaSeconds;
				if (Wait <= 0.f)
				{
					NextTarget(false);
				}
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
		case EMode::Glance:
			TickGlance(DeltaSeconds, Player);
			break;
		case EMode::Follow:
			TickFollow(DeltaSeconds, Player);
			break;
		case EMode::Chase:
			TickChase(DeltaSeconds, Player);
			break;
		case EMode::Arrest:
			TickArrest(DeltaSeconds, Player);
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
	FitCartToHands(DeltaSeconds);
	TickServe(DeltaSeconds);
}

void AGRWarga::StartServe(bool bEatHere, bool bDrink)
{
	ServeTime = 0.f;
	bServeEat = bEatHere;
	bServed = false;
	ServeLine = 0;
	HoldChat(8.f);
	FoodProp->SetStaticMesh(GRStoryUtil::LoadShape(bDrink ? TEXT("cylinder") : TEXT("cube")));
	GRStoryUtil::ApplyColor(FoodProp, GRStoryUtil::ParseColor(bDrink ? TEXT("#5A3A22") : TEXT("#C8A272")));
	FoodScale = bDrink ? FVector(0.07f, 0.07f, 0.11f) : FVector(0.15f, 0.11f, 0.07f);
	FoodProp->SetHiddenInGame(true);
	Say(TEXT("Siap, Mas! Sebentar, ya."), 1.6f);
}

void AGRWarga::CancelServe()
{
	if (ServeTime < 0.f)
	{
		return;
	}
	ServeTime = -1.f;
	FoodProp->SetHiddenInGame(true);
	if (UGRBodyAnimInstance* Bayu = GRBody::AnimOf(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Bayu->SetHand(1, false, FVector::ZeroVector, 0.3f);
	}
}

int32 AGRWarga::GetServeStage() const
{
	return bServed ? 5 : ServeTime < 0.f ? 0 : ServeTime < ServeShow ? 1 : ServeTime < ServeToss ? 2 : ServeTime < ServeCatch ? 3 : 4;
}

void AGRWarga::TickServe(float DeltaSeconds)
{
	if (ServeTime < 0.f)
	{
		return;
	}
	ACharacter* Bayu = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Bayu || FVector::Dist2D(Bayu->GetActorLocation(), GetActorLocation()) > 450.f)
	{
		CancelServe();
		return;
	}
	ServeTime += DeltaSeconds;
	HoldChat(1.f);
	UGRBodyAnimInstance* Mine = GRBody::AnimOf(this);
	UGRBodyAnimInstance* His = GRBody::AnimOf(Bayu);
	const USkeletalMeshComponent* MyBody = TripoBody && TripoBody->IsVisible() ? TripoBody.Get() : GetMesh();
	const FName MyHand = MyBody == GetMesh() ? FName(TEXT("hand_r")) : FName(TEXT("RightHand"));
	const USkeletalMeshComponent* HisBody = nullptr;
	{
		TInlineComponentArray<USkeletalMeshComponent*> Meshes(Bayu);
		for (const USkeletalMeshComponent* Part : Meshes)
		{
			if (Part->IsVisible() && Part->GetSkeletalMeshAsset())
			{
				HisBody = Part;
				break;
			}
		}
	}
	const bool bHisTripo = HisBody && HisBody->DoesSocketExist(TEXT("RightHand"));
	const FName HisHand = bHisTripo ? FName(TEXT("RightHand")) : FName(TEXT("hand_r"));
	const FName HisHead = bHisTripo ? FName(TEXT("Head")) : FName(TEXT("head"));
	const FName HisShoulder = bHisTripo ? FName(TEXT("RightArm")) : FName(TEXT("upperarm_r"));
	const float End = bServeEat ? ServeEatEnd : ServeKeepEnd;

	// Bayu turns to the cart while he is served.
	const FRotator Face(0.f, (GetActorLocation() - Bayu->GetActorLocation()).Rotation().Yaw, 0.f);
	Bayu->SetActorRotation(FMath::RInterpTo(Bayu->GetActorRotation(), Face, DeltaSeconds, 5.f));

	const FTransform& Me = GetActorTransform();
	const FTransform& Him = Bayu->GetActorTransform();
	auto PlaceFood = [this](const FVector& Where, float Grow)
	{
		FoodProp->SetHiddenInGame(Grow <= 0.f);
		FoodProp->SetWorldLocation(Where);
		FoodProp->SetWorldScale3D(FoodScale * FMath::Max(Grow, 0.01f) * GetActorScale3D().X);
	};
	// The parcel sits in the palm, a little past the wrist.
	auto InHand = [](const USkeletalMeshComponent* Body, FName Hand, const FTransform& Holder)
	{
		return Body ? Body->GetBoneLocation(Hand) + Holder.GetRotation().GetForwardVector() * 6.f - FVector(0.f, 0.f, 2.f) : Holder.GetLocation();
	};

	if (ServeTime < ServeShow)
	{
		// Making it: hands off the bar (UpdatePose plays the talk gestures), the parcel not out yet.
		if (Mine)
		{
			Mine->SetHand(1, false, FVector::ZeroVector, 0.3f);
		}
		FoodProp->SetHiddenInGame(true);
	}
	else if (ServeTime < ServeToss)
	{
		// Holding it out over the cart, at shoulder height.
		if (Mine && MyBody == TripoBody)
		{
			const FVector Shoulder = Me.InverseTransformPosition(TripoBody->GetBoneLocation(TEXT("RightArm")));
			Mine->SetHand(1, true, FVector(Shoulder.X + 38.f, Shoulder.Y + 4.f, Shoulder.Z + 6.f), 0.35f);
		}
		FoodProp->SetWorldRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
		PlaceFood(InHand(MyBody, MyHand, Me), FMath::Clamp((ServeTime - ServeShow) / 0.15f, 0.f, 1.f));
		TossFrom = FoodProp->GetComponentLocation();
		if (ServeLine == 0)
		{
			ServeLine = 1;
			Say(bServeEat ? TEXT("Ini, Mas! Tangkap!") : TEXT("Sudah dibungkus. Tangkap, Mas!"), 1.8f);
		}
	}
	else if (ServeTime < ServeCatch)
	{
		// Tossed over the cart into Bayu's open hand, on a little arc, turning over once (the vendor's hands go back
		// to the handlebar: FitCartToHands).
		if (His && HisBody)
		{
			const FVector Shoulder = Him.InverseTransformPosition(HisBody->GetBoneLocation(HisShoulder));
			His->SetHand(1, true, FVector(Shoulder.X + 34.f, Shoulder.Y - 4.f, Shoulder.Z - 8.f), 0.25f);
		}
		const float T = (ServeTime - ServeToss) / (ServeCatch - ServeToss);
		FVector Where = FMath::Lerp(TossFrom, InHand(HisBody, HisHand, Him), T);
		Where.Z += FMath::Sin(T * PI) * 45.f;
		FoodProp->SetWorldRotation(FRotator(0.f, GetActorRotation().Yaw, 360.f * T));
		PlaceFood(Where, 1.f);
	}
	else
	{
		const float T = ServeTime - ServeCatch;
		const float Hold = End - ServeCatch;
		float Grow = 1.f;
		if (His && HisBody)
		{
			const FVector Shoulder = Him.InverseTransformPosition(HisBody->GetBoneLocation(HisShoulder));
			const FVector Front(Shoulder.X + 30.f, Shoulder.Y - 4.f, Shoulder.Z - 22.f);
			if (bServeEat && T > 0.45f && T < Hold - 0.4f)
			{
				// Three bites: hand up to the mouth and down a little, the parcel getting smaller.
				const FVector Mouth = Him.InverseTransformPosition(HisBody->GetBoneLocation(HisHead)) + FVector(16.f, 2.f, -10.f);
				const float Bite = FMath::Fmod(T - 0.45f, 0.75f) / 0.75f;
				His->SetHand(1, true, Bite < 0.55f ? Mouth : Front, 0.22f);
				Grow = 1.f - 0.28f * FMath::FloorToFloat((T - 0.45f) / 0.75f);
			}
			else if (!bServeEat && T > 0.6f)
			{
				// Into the bag at his hip.
				His->SetHand(1, true, FVector(Shoulder.X + 4.f, Shoulder.Y + 10.f, Shoulder.Z - 55.f), 0.3f);
			}
			else
			{
				His->SetHand(1, true, Front, 0.2f);
			}
		}
		if ((bServeEat && T >= Hold - 0.4f) || (!bServeEat && T >= Hold - 0.3f))
		{
			Grow = 0.f;
		}
		FoodProp->SetWorldRotation(FRotator(0.f, Bayu->GetActorRotation().Yaw, 0.f));
		PlaceFood(InHand(HisBody, HisHand, Him), FMath::Clamp(Grow, 0.f, 1.f));
		if (bServeEat && ServeLine == 1 && T > 1.2f)
		{
			ServeLine = 2;
			Say(TEXT("Gimana, Mas? Enak, toh?"), 2.f);
		}
		if (T >= Hold)
		{
			if (His)
			{
				His->SetHand(1, false, FVector::ZeroVector, 0.35f);
			}
			FoodProp->SetHiddenInGame(true);
			ServeTime = -1.f;
			bServed = true;
			HoldChat(2.f);
		}
	}
}

FString AGRWarga::CartDebug() const
{
	auto Bone = [this](const TCHAR* Name)
	{
		return TripoBody && TripoBody->GetSkeletalMeshAsset() ? GetActorTransform().InverseTransformPosition(TripoBody->GetBoneLocation(Name)) : FVector::ZeroVector;
	};
	return FString::Printf(TEXT("anim=%s v=%.0f mode=%d grip=(%.0f %.0f %.0f) w=%.0f handL=(%.0f %.0f %.0f) handR=(%.0f %.0f %.0f) head=%.0f"),
		CurrentPose ? *CurrentPose->GetName() : TEXT("-"), GetVelocity().Size2D(), int32(Mode), CartGrip.X, CartGrip.Y, CartGrip.Z, CartGripWidth,
		Bone(TEXT("LeftHand")).X, Bone(TEXT("LeftHand")).Y, Bone(TEXT("LeftHand")).Z, Bone(TEXT("RightHand")).X, Bone(TEXT("RightHand")).Y,
		Bone(TEXT("RightHand")).Z, Bone(TEXT("Head")).Z);
}

void AGRWarga::FitCartToHands(float DeltaSeconds)
{
	// Parts laid out in Dress: 10 is the handlebar, 11 and 12 the side bars joining it to the cart body (front at 70).
	UGRBodyAnimInstance* Anim = TripoBody ? Cast<UGRBodyAnimInstance>(TripoBody->GetAnimInstance()) : nullptr;
	const bool bPushing = CurrentPose && (CurrentPose == CartAnim || CurrentPose == CartWalkAnim);
	if (Role != ERole::Vendor || CartParts.Num() < 13 || !Anim || !TripoBody->IsVisible() || !bPushing)
	{
		if (Anim)
		{
			Anim->SetGrip(false);
		}
		CartGrip = FVector::ZeroVector;
		return;
	}
	// MM_GR_Gerobak and MM_GR_GerobakJalan were made for the mannequin's arms, and the walk swings them besides: the
	// bar goes where this body's hands reach comfortably (elbows bent), and IK in the body's anim puts the hands on it.
	const FTransform& Me = GetActorTransform();
	const FVector ShoulderL = Me.InverseTransformPosition(TripoBody->GetBoneLocation(TEXT("LeftArm")));
	const FVector ShoulderR = Me.InverseTransformPosition(TripoBody->GetBoneLocation(TEXT("RightArm")));
	const float Reach = FVector::Dist(TripoBody->GetBoneLocation(TEXT("LeftArm")), TripoBody->GetBoneLocation(TEXT("LeftForeArm")))
		+ FVector::Dist(TripoBody->GetBoneLocation(TEXT("LeftForeArm")), TripoBody->GetBoneLocation(TEXT("LeftHand")));
	const FVector Shoulders = (ShoulderL + ShoulderR) * 0.5f;
	const FVector Want(Shoulders.X + Reach * 0.5f, 0.f, Shoulders.Z - Reach * 0.62f);
	const float HalfGrip = FMath::Max(FMath::Abs(ShoulderL.Y - ShoulderR.Y) * 0.5f, 12.f);
	// The cart is rigid while the shoulders bob with each step: follow them slowly.
	CartGrip = CartGrip.IsZero() ? Want : FMath::Lerp(CartGrip, Want, 1.f - FMath::Exp(-3.f * DeltaSeconds));
	CartGripWidth = FMath::Max(HalfGrip * 2.f + 16.f, 62.f);
	const float BarX = FMath::Clamp(CartGrip.X, 12.f, 60.f);
	const float BarZ = CartGrip.Z;
	CartParts[10]->SetRelativeLocation(FVector(BarX, 0.f, BarZ));
	CartParts[10]->SetRelativeScale3D(FVector(0.035f, CartGripWidth / 100.f, 0.035f));
	for (int32 k = 0; k < 2; ++k)
	{
		const float PY = k == 0 ? -1.f : 1.f;
		CartParts[11 + k]->SetRelativeLocation(FVector((BarX + 70.f) * 0.5f, PY * 29.f, BarZ));
		CartParts[11 + k]->SetRelativeScale3D(FVector((70.f - BarX) / 100.f, 0.035f, 0.035f));
	}
	// Wrists just behind and above the bar, so the palms close over it.
	Anim->SetGrip(true, FVector(BarX - 5.f, -HalfGrip, BarZ + 2.f), FVector(BarX - 5.f, HalfGrip, BarZ + 2.f));
}

void AGRWarga::UpdatePose()
{
	const bool bTalking = BarkHide > 0.f || TalkPause > 0.f;
	UAnimSequenceBase* Want = nullptr;
	if (Mode == EMode::Down)
	{
		PhoneProp->SetHiddenInGame(true);
		return;
	}
	float Rate = 1.f;
	if (Mode == EMode::Angry)
	{
		// Shaking a fist while standing; walking up to him (or staggering) uses the walk.
		Want = GetVelocity().SizeSquared2D() < FMath::Square(60.f) && GetCharacterMovement()->IsMovingOnGround() ? AngryAnim.Get() : nullptr;
	}
	else if (StartleTime > 0.f && StartleAnim && Role != ERole::Vendor)
	{
		Want = StartleAnim.Get();
	}
	else if (Mode == EMode::Chase && Wait > 0.f)
	{
		// Pointing him out before running.
		Want = AngryAnim.Get();
	}
	else if (IsPolice() && (Mode == EMode::Arrest || Mode == EMode::Pause || Mode == EMode::Follow) && TalkPause <= 0.f
		&& GetVelocity().SizeSquared2D() < FMath::Square(40.f))
	{
		// Holding Bayu, on the radio, eyeing him while tailing, chatting, or on watch with hands behind the back.
		const AGRCrowd* C = Crowd.Get();
		Want = Mode == EMode::Arrest && C && C->Arrester.Get() == this ? GrabAnim.Get()
			: RadioTime > 0.f || Mode == EMode::Arrest ? RadioAnim.Get()
			: Mode == EMode::Follow ? GlanceAnim.Get()
			: bTalking ? TalkAnim.Get() : GuardAnim.Get();
	}
	else if (Role == ERole::Vendor && ServeTime >= 0.f && ServeTime < ServeToss && TalkAnim)
	{
		// Making the order behind the cart.
		Want = TalkAnim.Get();
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
	else if (WaveTime > 0.f && WaveAnim)
	{
		Want = WaveAnim.Get();
	}
	else if (TalkPause > 0.f)
	{
		Want = TalkAnim.Get();
	}
	else if (GlanceAnim && (Mode == EMode::Glance || (Mode == EMode::Follow && GetVelocity().SizeSquared2D() < FMath::Square(40.f))))
	{
		Want = GlanceAnim.Get();
	}
	else if (bTalking && Mode != EMode::Walk)
	{
		Want = TalkAnim.Get();
	}
	else if (Mode == EMode::Pause && bPhone && PhoneAnim)
	{
		Want = PhoneAnim.Get();
	}
	PlayPose(Want, Rate);

	// The phone sits between both hands, screen toward the face.
	const bool bShowPhone = Want == PhoneAnim && PhoneAnim;
	PhoneProp->SetHiddenInGame(!bShowPhone);
	if (bShowPhone)
	{
		const USkeletalMeshComponent* Body = GetMesh();
		const FVector Hands = (Body->GetSocketLocation(TEXT("hand_l")) + Body->GetSocketLocation(TEXT("hand_r"))) * 0.5f;
		const FVector Face = Body->GetSocketLocation(TEXT("head")) + GetActorForwardVector() * 12.f;
		const FVector Normal = (Face - Hands).GetSafeNormal();
		PhoneProp->SetWorldLocationAndRotation(Hands + Normal * 4.f + FVector(0.f, 0.f, 3.f), FRotationMatrix::MakeFromXZ(Normal, FVector::UpVector).Rotator());
	}
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
		if (bToZebra)
		{
			// At the kerb of the crossing: look at the traffic, then go when it is clear (NextTarget).
			bPhone = false;
			Wait = Rng.FRandRange(0.3f, 0.8f);
			return;
		}
		bPhone = Role == ERole::Stroller && Rng.FRand() < 0.35f;
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
		// Someone glancing over at Bayu is still part of the group.
		if (Other != this && Other->bActive && Other->Group == Group
			&& (Other->Mode == EMode::Chat || (Other->Mode == EMode::Glance && Other->GlanceResume == EMode::Chat)))
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
	const bool bOnPavement = C->PavementAt(Here, Axis, Side, Along);
	if (bToZebra && (!bOnPavement || bBlocked))
	{
		bToZebra = false;   // pushed off the pavement or the way is blocked: forget the crossing
	}
	if (bOnPavement && (bToZebra || Role != ERole::Stroller || Rng.FRand() < 0.8f))
	{
		// Now and then a stroller goes over to the other pavement: only on a zebra crossing, never straight across.
		if (!bToZebra && Role == ERole::Stroller && !bBlocked && Rng.FRand() < 0.12f && FMath::Abs(Along) > 4000.f)
		{
			const float Zebra = GRCrossings::Nearest(Along);
			if (FMath::Abs(Zebra - Along) < 4500.f)
			{
				bToZebra = true;
				ZebraWait = 0.f;
				ZebraAlong = Zebra + Rng.FRandRange(-0.6f, 0.6f) * GRCrossings::HalfBand;
			}
		}
		if (bToZebra)
		{
			const FVector2D Kerb = C->PavementPoint(Axis, Side, ZebraAlong, RoadEdge + 90.f);
			float Z = 0.f;
			if (FVector2D::Distance(Here, Kerb) > 120.f)
			{
				// Along the pavement to the crossing first.
				if (C->IsWalkable(Kerb, Z) && C->ClearPath(this, Loc, FVector(Kerb, Z + CapsuleHalf), false))
				{
					Go(Kerb, Z, false);
					return;
				}
				bToZebra = false;
			}
			else
			{
				// At the kerb: wait, facing the road, until no car is near the crossing (cars stop for people on it).
				const FVector2D Across = C->PavementPoint(Axis, -Side, ZebraAlong, RoadEdge + 90.f);
				const FVector2D Middle = (Here + Across) * 0.5f;
				if (!C->IsWalkable(Across, Z) || ZebraWait > 12.f)
				{
					bToZebra = false;
				}
				else if (C->CarNear(Middle, 2600.f))
				{
					const FVector2D Look = Across - Here;
					SetActorRotation(FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(Look.Y, Look.X)), 0.f));
					Mode = EMode::Pause;
					bPhone = false;
					Wait = Rng.FRandRange(0.4f, 0.9f);
					ZebraWait += Wait;
					return;
				}
				else
				{
					bToZebra = false;
					Go(Across, Z, true);
					return;
				}
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
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> SurrenderSeq(TEXT("/Game/GemahRipah/Anims/MM_GR_Menyerah.MM_GR_Menyerah"));
	SurrenderAnim = SurrenderSeq.Object;
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
	TickArrest(DeltaSeconds);
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
	UpdateWatchers(Player, 0.5f);
	UpdateFriends(Player, 0.5f);
	UpdatePolice(Player, 0.5f);
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
		if (Person->bActive && !Person->bOjol && (FVector::DistSquared2D(L, Center) > FMath::Square(RecycleRange) || L.Z < -1000.f))
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
			if (Person->bActive && !Person->bOjol && Person->Mode != AGRWarga::EMode::Chat && Person->Mode != AGRWarga::EMode::Down
				&& Person->Mode != AGRWarga::EMode::Angry && Person->Mode != AGRWarga::EMode::Follow && !Person->IsPolice() && !Person->DriverCar.IsValid() && !InView(Person->GetActorLocation() - FVector(0.f, 0.f, CapsuleHalf)))
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

float AGRCrowd::NoteBump(bool bHard)
{
	// Running into people counts double; a few bumps in a while are just a busy pavement.
	const double Now = GetWorld()->GetTimeSeconds();
	Bumps.RemoveAll([Now](const TPair<double, float>& B) { return Now - B.Key > 15.0; });
	Bumps.Emplace(Now, bHard ? 2.f : 1.f);
	float Score = 0.f;
	for (const TPair<double, float>& B : Bumps)
	{
		Score += B.Value;
	}
	UE_LOG(LogGRCrowd, Display, TEXT("GRTABRAK %s, skor %.0f dalam 15 dtk"), bHard ? TEXT("keras") : TEXT("senggol"), Score);
	// A streak is reported once; the next report takes a new streak.
	if (Score >= 5.f)
	{
		Bumps.Reset();
	}
	return Score;
}

void AGRCrowd::UpdateWatchers(const APawn* Player, float Step)
{
	GlanceGap -= Step;
	FollowGap -= Step;
	const AGRCharacter* Bayu = Cast<AGRCharacter>(Player);
	const UGRStatusSubsystem* Status = GetGameInstance()->GetSubsystem<UGRStatusSubsystem>();
	if (!Bayu || !Status || Bayu->IsRiding() || Bayu->IsInTopDownMode() || Bayu->IsKnockedDown())
	{
		return;
	}
	// Eyes on him at once: Diawasi 50 one, 60 two, 70 three, 80 five. From 80 someone tails him, from 90 two do.
	const float Diawasi = Status->GetDiawasi();
	const int32 Lookers = Diawasi >= 80.f ? 5 : Diawasi >= 70.f ? 3 : Diawasi >= 60.f ? 2 : Diawasi >= 50.f ? 1 : 0;
	const int32 Tails = Diawasi >= 90.f ? 2 : Diawasi >= 80.f ? 1 : 0;
	if (Lookers == 0)
	{
		return;
	}
	int32 Looking = 0;
	int32 Following = 0;
	for (const AGRWarga* Person : People)
	{
		Looking += Person->bActive && Person->Mode == AGRWarga::EMode::Glance ? 1 : 0;
		Following += Person->bActive && Person->Mode == AGRWarga::EMode::Follow && !Person->IsPolice() && !Person->bSupporter ? 1 : 0;
	}
	const FVector Eye = Bayu->GetActorLocation() + FVector(0.f, 0.f, 60.f);
	// The nearest passer-by in sight of him (and of the camera, so the look is not wasted).
	auto Pick = [&](float Reach, bool bGlance) -> AGRWarga*
	{
		AGRWarga* Best = nullptr;
		float BestDist = FMath::Square(Reach);
		for (AGRWarga* Person : People)
		{
			const FVector L = Person->GetActorLocation();
			const float D = FVector::DistSquared2D(L, Eye);
			if (!Person->bActive || D >= BestDist || D < FMath::Square(150.f) || (Person->Mode != AGRWarga::EMode::Walk && Person->Mode != AGRWarga::EMode::Pause && (!bGlance || Person->Mode != AGRWarga::EMode::Chat))
				|| Person->Role == AGRWarga::ERole::Vendor || (!bGlance && (Person->Role == AGRWarga::ERole::Jogger || Person->IsPolice())) || Person->DriverCar.IsValid()
				|| Person->Dodge > 0.f || Person->TalkPause > 0.f || Person->StartleTime > 0.f || (bGlance && Person->GlanceCooldown > 0.f)
				|| !InView(L - FVector(0.f, 0.f, CapsuleHalf)))
			{
				continue;
			}
			FCollisionQueryParams Params(SCENE_QUERY_STAT(GRWargaWatch), false, Person);
			Params.AddIgnoredActor(Bayu);
			if (GetWorld()->LineTraceTestByChannel(L + FVector(0.f, 0.f, 60.f), Eye, ECC_Visibility, Params))
			{
				continue;
			}
			Best = Person;
			BestDist = D;
		}
		return Best;
	};
	if (Following < Tails && FollowGap <= 0.f)
	{
		if (AGRWarga* Person = Pick(1200.f, false))
		{
			Person->StartFollow();
			FollowGap = 20.f;
			return;
		}
	}
	if (Looking < Lookers && GlanceGap <= 0.f)
	{
		if (AGRWarga* Person = Pick(900.f, true))
		{
			Person->StartGlance();
			// The higher Diawasi, the quicker the next one looks: every 5 s at 50, every half second from 80.
			GlanceGap = FMath::GetMappedRangeValueClamped(FVector2f(50.f, 80.f), FVector2f(5.f, 0.f), Diawasi);
		}
	}
}

AGRWarga* AGRCrowd::FindCitizenNear(const FVector& Center, float MinR, float MaxR) const
{
	using EMode = AGRWarga::EMode;
	AGRWarga* Best = nullptr;
	float BestDist = FMath::Square(MaxR);
	for (AGRWarga* Person : People)
	{
		const FVector L = Person->GetActorLocation();
		const float D = FVector::DistSquared2D(L, Center);
		if (!Person->bActive || D >= BestDist || D < FMath::Square(MinR) || Person->IsPolice() || Person->bOjol || Person->bSupporter || Person->DriverCar.IsValid()
			|| Person->Role == AGRWarga::ERole::Vendor || (Person->Mode != EMode::Walk && Person->Mode != EMode::Pause && Person->Mode != EMode::Chat)
			|| Person->Dodge > 0.f || Person->TalkPause > 0.f || Person->StartleTime > 0.f || Person->GlanceCooldown > 0.f || !InView(L - FVector(0.f, 0.f, CapsuleHalf)))
		{
			continue;
		}
		Best = Person;
		BestDist = D;
	}
	return Best;
}

void AGRCrowd::UpdateFriends(const APawn* Player, float Step)
{
	GreetGap -= Step;
	SupportGap -= Step;
	const AGRCharacter* Bayu = Cast<AGRCharacter>(Player);
	const UGRStatusSubsystem* Status = GetGameInstance()->GetSubsystem<UGRStatusSubsystem>();
	const UGRSuaraSubsystem* Suara = GetGameInstance()->GetSubsystem<UGRSuaraSubsystem>();
	if (!Bayu || !Status || !Suara || !Suara->HasTier(EGRSuaraTier::Dikenal) || Bayu->IsRiding() || Bayu->IsInTopDownMode() || Bayu->IsKnockedDown() || IsArresting())
	{
		return;
	}
	const FVector Center = Bayu->GetActorLocation();
	if (GreetGap <= 0.f)
	{
		if (AGRWarga* Person = FindCitizenNear(Center, 150.f, 800.f))
		{
			Person->StartGreet();
			// Every 24 s or so for someone just known, every 7 s for a whole movement.
			GreetGap = FMath::GetMappedRangeValueClamped(FVector2f(20.f, 90.f), FVector2f(24.f, 7.f), Status->GetSuara());
		}
	}
	if (SupportGap <= 0.f && Suara->HasTier(EGRSuaraTier::Gerakan))
	{
		int32 Walking = 0;
		for (const AGRWarga* Person : People)
		{
			Walking += Person->bActive && Person->bSupporter ? 1 : 0;
		}
		SupportGap = 6.f;
		if (Walking < 3)
		{
			if (AGRWarga* Person = FindCitizenNear(Center, 150.f, 1000.f))
			{
				Person->StartSupport();
			}
		}
	}
}

void AGRCrowd::RallyAgainst(AGRWarga* Officer, int32 Count)
{
	using EMode = AGRWarga::EMode;
	for (AGRWarga* Person : People)
	{
		if (Count > 0 && Person->bActive && !Person->IsPolice() && !Person->bOjol && !Person->DriverCar.IsValid() && Person->Role != AGRWarga::ERole::Vendor
			&& (Person->Mode == EMode::Walk || Person->Mode == EMode::Pause || Person->Mode == EMode::Chat || Person->Mode == EMode::Glance || Person->bSupporter)
			&& FVector::DistSquared2D(Person->GetActorLocation(), Officer->GetActorLocation()) < FMath::Square(1400.f))
		{
			Person->bSupporter = false;
			Person->GetAngry(Officer, Rng.FRandRange(4.f, 6.f), AGRWarga::EAnger::Defender);
			--Count;
		}
	}
}

void AGRCrowd::SpawnPoliceNear()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player)
	{
		SpawnPolice(Player->GetActorLocation(), true, true);
		SpawnPolice(Player->GetActorLocation(), true, false);
	}
}

bool AGRCrowd::SpawnPolice(const FVector& Around, bool bAnywhere, bool bPair)
{
	using ERole = AGRWarga::ERole;
	FVector Feet;
	if (!FindSpot(Around, bAnywhere ? 600.f : 2500.f, bAnywhere ? 1600.f : 5200.f, !bAnywhere, true, Feet))
	{
		return false;
	}
	AGRWarga* Lead = TakeFromPool();
	if (!Lead)
	{
		return false;
	}
	const int32 Hour = HourNow(this);
	const bool bNight = Hour >= 20 || Hour < 5;
	const float Yaw = Rng.FRandRange(0.f, 360.f);
	Lead->Dress(NextSeed++, ERole::Police, bNight);
	Lead->Activate(Feet, Yaw);
	Lead->NextTarget(false);
	UE_LOG(LogGRCrowd, Display, TEXT("GRSPN patroli %s di (%.0f, %.0f)"), bPair ? TEXT("berdua") : TEXT("sendiri"), Feet.X, Feet.Y);
	if (!bPair)
	{
		return true;
	}
	// The partner starts at the leader's right shoulder.
	const FVector2D P(Feet + FRotator(0.f, Yaw, 0.f).RotateVector(FVector(0.f, 95.f, 0.f)));
	float Z = 0.f;
	AGRWarga* Mate = IsWalkable(P, Z) ? TakeFromPool() : nullptr;
	if (!Mate)
	{
		return true;
	}
	Mate->Dress(NextSeed++, ERole::Police, bNight);
	Mate->Activate(FVector(P, Z), Yaw);
	Mate->Mode = AGRWarga::EMode::Walk;
	Mate->Partner = Lead;
	Mate->bLeader = false;
	Lead->Partner = Mate;
	Lead->bLeader = true;
	return true;
}

void AGRCrowd::UpdatePolice(const APawn* Player, float Step)
{
	using EMode = AGRWarga::EMode;
	PoliceCalm -= Step;
	PoliceSpawnGap -= Step;
	WarnGap -= Step;
	ShieldGap -= Step;
	const AGRCharacter* Bayu = Cast<AGRCharacter>(Player);
	int32 Units = 0;
	int32 Pairs = 0;
	int32 Chasers = 0;
	int32 Tailing = 0;
	for (const AGRWarga* Person : People)
	{
		if (Person->bActive && Person->IsPolice())
		{
			Chasers += Person->Mode == EMode::Chase || Person->Mode == EMode::Arrest ? 1 : 0;
			Tailing += Person->Mode == EMode::Follow ? 1 : 0;
			if (Person->IsLeader())
			{
				++Units;
				Pairs += Person->Partner.IsValid() ? 1 : 0;
			}
		}
	}
	// One officer alone and one pair by day, two pairs at night, a single patrol in the small hours.
	const int32 Hour = HourNow(this);
	const bool bNight = Hour >= 20 || Hour < 5;
	const int32 WantUnits = Hour >= 1 && Hour < 5 ? 1 : 2;
	if (Bayu && Units < WantUnits && PoliceSpawnGap <= 0.f)
	{
		PoliceSpawnGap = 8.f;
		SpawnPolice(Bayu->GetActorLocation(), false, bNight || Pairs < Units || (Units == 0 && Rng.FRand() < 0.5f));
	}

	UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>();
	if (Story && Chasers > 0 && !bWasChased)
	{
		Story->Run(TEXT("notifburuk SPN mengejarmu! Lari (Shift), jangan sampai tertangkap."));
	}
	else if (Story && Chasers == 0 && bWasChased && !IsArresting())
	{
		Story->Run(TEXT("notif Kamu lolos dari kejaran SPN."));
	}
	bWasChased = Chasers > 0;

	const UGRSuaraSubsystem* Suara = GetGameInstance()->GetSubsystem<UGRSuaraSubsystem>();
	if (Story && Suara && Chasers > 0 && ShieldGap <= 0.f && !IsArresting() && Suara->HasTier(EGRSuaraTier::Dilindungi))
	{
		// The town gets in the way: every officer on the run is held up by the people around them.
		ShieldGap = 25.f;
		bool bHeldUp = false;
		for (AGRWarga* Person : People)
		{
			if (Person->bActive && Person->IsPolice() && Person->Mode == EMode::Chase)
			{
				Person->Wait = 4.f;
				Person->Say(TEXT("Minggir! Jangan halangi petugas!"), 2.2f, AngryColor);
				RallyAgainst(Person, 3);
				bHeldUp = true;
			}
		}
		if (bHeldUp)
		{
			UE_LOG(LogGRCrowd, Display, TEXT("GRSPN dihadang warga"));
			Story->Run(TEXT("notifbaik Warga menghadang SPN! Lari sekarang."));
		}
	}

	const UGRStatusSubsystem* Status = GetGameInstance()->GetSubsystem<UGRStatusSubsystem>();
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	const UGRUISubsystem* UI = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UGRUISubsystem>() : nullptr;
	if (!Bayu || !Status || IsArresting() || PoliceCalm > 0.f || Bayu->IsInTopDownMode() || Bayu->IsKnockedDown() || (UI && UI->HasScreens()))
	{
		return;
	}
	// Diawasi 65: an officer who sees him tails him from a distance. 85: they run him down.
	const float Diawasi = Status->GetDiawasi();
	if (Story && Suara && WarnGap <= 0.f && Diawasi >= 40.f && Suara->HasTier(EGRSuaraTier::Dibela))
	{
		// Someone who trusts him tips him off before the patrol gets close.
		const FVector Here = Bayu->GetActorLocation();
		const bool bNear = People.ContainsByPredicate([&Here](const AGRWarga* Person)
		{
			return Person->bActive && Person->IsPolice() && FVector::DistSquared2D(Person->GetActorLocation(), Here) < FMath::Square(1800.f);
		});
		AGRWarga* Friend = bNear ? FindCitizenNear(Here, 150.f, 1000.f) : nullptr;
		if (Friend)
		{
			WarnGap = 45.f;
			Friend->StartGlance(false);
			Friend->Say(TEXT("Mas Bayu, ada SPN dekat sini. Hati-hati."), 2.8f);
			Story->Run(TEXT("notif Warga berbisik: ada SPN dekat sini."));
		}
	}
	if (Diawasi < 65.f)
	{
		return;
	}
	const FVector Eye = Bayu->GetActorLocation() + FVector(0.f, 0.f, 60.f);
	for (AGRWarga* Officer : People)
	{
		if (!Officer->bActive || !Officer->IsPolice() || !Officer->IsLeader() || Officer->ChaseCooldown > 0.f
			|| (Officer->Mode != EMode::Walk && Officer->Mode != EMode::Pause && Officer->Mode != EMode::Glance && Officer->Mode != EMode::Follow))
		{
			continue;
		}
		const FVector L = Officer->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		const float Dist = FVector::Dist2D(L, Eye);
		// Seen ahead of them, or anywhere close by.
		if (Dist > 1800.f || (Dist > 600.f && FVector::DotProduct(Officer->GetActorForwardVector(), (Eye - L).GetSafeNormal2D()) < 0.2f))
		{
			continue;
		}
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GRSpnSee), false, Officer);
		Params.AddIgnoredActor(Bayu);
		if (GetWorld()->LineTraceTestByChannel(L, Eye, ECC_Visibility, Params))
		{
			continue;
		}
		AGRWarga* Mate = Officer->Partner.Get();
		const bool bMate = Mate && Mate->bActive && Mate->Mode != EMode::Down && Mate->Mode != EMode::Angry && Mate->Mode != EMode::Sit;
		if (Diawasi >= 85.f && !Bayu->IsRiding())
		{
			Officer->StartChase(true);
			if (bMate)
			{
				Mate->StartChase(false);
			}
		}
		else if (Officer->Mode != EMode::Follow && Tailing == 0 && Dist < 1300.f)
		{
			Officer->StartFollow();
			if (bMate)
			{
				Mate->StartFollow();
			}
			++Tailing;
		}
	}
}

void AGRCrowd::StartArrest(AGRWarga* Officer)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	AGRCharacter* Bayu = PC ? Cast<AGRCharacter>(PC->GetPawn()) : nullptr;
	if (!Bayu || !Officer || ArrestTime >= 0.f)
	{
		return;
	}
	if (UGRSuaraSubsystem* Suara = GetGameInstance()->GetSubsystem<UGRSuaraSubsystem>(); Suara && Suara->UseWitness())
	{
		// The witness Bayu called in: phones up, the officers think better of it.
		UE_LOG(LogGRCrowd, Display, TEXT("GRSPN %s batal menangkap, ada saksi"), *Officer->GetName());
		for (AGRWarga* Person : People)
		{
			if (Person->bActive && Person->IsPolice() && Person->Mode == AGRWarga::EMode::Chase)
			{
				Person->EndChase(false);
				Person->ChaseCooldown = 60.f;
			}
		}
		Officer->Say(TEXT("...Kali ini kamu beruntung."), 2.4f, AngryColor);
		RallyAgainst(Officer, 5);
		PoliceCalm = 60.f;
		bWasChased = false;
		return;
	}
	UE_LOG(LogGRCrowd, Display, TEXT("GRSPN %s menangkap Bayu"), *Officer->GetName());
	Arrester = Officer;
	ArrestTime = 0.f;
	// He stops where he is (no dead stop: the run brakes out) and puts his hands up.
	PC->SetIgnoreMoveInput(true);
	if (UAnimInstance* Anim = Bayu->GetMesh()->GetAnimInstance(); Anim && SurrenderAnim)
	{
		Anim->PlaySlotAnimationAsDynamicMontage(SurrenderAnim, TEXT("DefaultSlot"), 0.35f, 0.35f, 1.f, 10000);
	}
	for (AGRWarga* Person : People)
	{
		if (Person->bActive && Person->IsPolice() && (Person == Officer || Person == Officer->Partner.Get() || Person->Mode == AGRWarga::EMode::Chase))
		{
			Person->Mode = AGRWarga::EMode::Arrest;
			Person->TalkPause = 0.f;
			Person->StartleTime = 0.f;
		}
	}
	Officer->Say(TEXT("Diam di tempat! Angkat tangan!"), 2.2f, AngryColor);
	// Passers-by stop and stare.
	int32 Left = 4;
	for (AGRWarga* Person : People)
	{
		using EMode = AGRWarga::EMode;
		if (Left > 0 && Person->bActive && !Person->IsPolice() && (Person->Mode == EMode::Walk || Person->Mode == EMode::Pause || Person->Mode == EMode::Chat)
			&& FVector::DistSquared2D(Person->GetActorLocation(), Bayu->GetActorLocation()) < FMath::Square(1200.f))
		{
			Person->StartGlance();
			--Left;
		}
	}
}

void AGRCrowd::TickArrest(float DeltaSeconds)
{
	if (ArrestTime < 0.f)
	{
		return;
	}
	const float Before = ArrestTime;
	ArrestTime += DeltaSeconds;
	auto Crossed = [Before, this](float T) { return Before < T && ArrestTime >= T; };
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	AGRCharacter* Bayu = PC ? Cast<AGRCharacter>(PC->GetPawn()) : nullptr;
	AGRWarga* Officer = Arrester.Get();
	const bool bHeld = Bayu && Officer && Officer->bActive && Officer->Mode == AGRWarga::EMode::Arrest;
	if (bHeld && ArrestTime < 3.6f)
	{
		// He turns to the officer holding him; the partner radios it in.
		const FRotator Face(0.f, (Officer->GetActorLocation() - Bayu->GetActorLocation()).Rotation().Yaw, 0.f);
		Bayu->SetActorRotation(FMath::RInterpTo(Bayu->GetActorRotation(), Face, DeltaSeconds, 5.f));
		if (Crossed(1.f))
		{
			UGRBubbleSubsystem::Show(Bayu, TEXT("I-iya, Pak... Saya nggak ngapa-ngapain."), 2.f);
		}
		if (AGRWarga* Mate = Officer->Partner.Get(); Crossed(1.7f) && Mate && Mate->Mode == AGRWarga::EMode::Arrest)
		{
			Mate->RadioTime = 2.5f;
			Mate->Say(TEXT("Pos, satu orang diamankan, ganti."), 2.4f);
		}
		if (Crossed(2.4f))
		{
			Officer->Say(TEXT("Sudah, ikut kami ke pos."), 2.f);
		}
		return;
	}
	// Done, or the officer was knocked away (then he simply gets free).
	ArrestTime = -1.f;
	Arrester = nullptr;
	bWasChased = false;
	PoliceCalm = 60.f;
	if (PC)
	{
		PC->SetIgnoreMoveInput(false);
	}
	if (UAnimInstance* Anim = Bayu ? Bayu->GetMesh()->GetAnimInstance() : nullptr; Anim && !Bayu->IsRiding())
	{
		Anim->StopSlotAnimation(0.35f, TEXT("DefaultSlot"));
	}
	for (AGRWarga* Person : People)
	{
		if (Person->bActive && Person->Mode == AGRWarga::EMode::Arrest)
		{
			Person->Mode = AGRWarga::EMode::Pause;
			Person->Wait = 4.f;
			Person->ChaseCooldown = 60.f;
			Person->RadioTime = 0.f;
			Person->GetCharacterMovement()->MaxWalkSpeed = Person->Speed;
		}
	}
	UE_LOG(LogGRCrowd, Display, TEXT("GRSPN penangkapan selesai%s"), bHeld ? TEXT(", Bayu dibawa") : TEXT(", Bayu lepas"));
	UGRStorySubsystem* Story = GetGameInstance()->GetSubsystem<UGRStorySubsystem>();
	if (bHeld && Story)
	{
		GetGameInstance()->GetSubsystem<UGRSuaraSubsystem>()->NoteCaught();
		Story->CancelOrder();
		Story->Run(Story->GetData().OnCaught);
	}
}

void AGRCrowd::ReportIncident(float Diawasi, const FVector& Where, const FString& What)
{
	const double Now = GetWorld()->GetTimeSeconds();
	Incidents.RemoveAll([Now](double T) { return Now - T > 90.0; });
	Incidents.Add(Now);
	// Once is an accident. Again and again within a minute and a half, and people start to remember the face.
	const int32 Count = Incidents.Num();
	UGRSuaraSubsystem* Suara = GetGameInstance()->GetSubsystem<UGRSuaraSubsystem>();
	const bool bRiot = Count >= 3 && Now - LastRiot > 60.0 && !Suara->HasTier(EGRSuaraTier::Gerakan);
	if (bRiot)
	{
		LastRiot = Now;
	}
	const float Delta = FMath::RoundToFloat(Diawasi * FMath::Min(1.f + 0.5f * (Count - 1), 3.f) * Suara->IncidentScale()) + (bRiot ? 5.f : 0.f);
	// People remember who hurt them: the town's trust goes with it.
	const float Trust = bRiot ? 5.f : FMath::Clamp(FMath::RoundToFloat(Diawasi * 0.5f), 1.f, 3.f);
	Suara->Lose(Trust);
	UE_LOG(LogGRCrowd, Display, TEXT("GRINSIDEN %s ke-%d dalam 90 dtk, Diawasi +%.0f%s"), *What, Count, Delta, bRiot ? TEXT(", KERUSUHAN") : TEXT(""));
	UGameInstance* GI = GetGameInstance();
	if (UGRStorySubsystem* Story = GI->GetSubsystem<UGRStorySubsystem>())
	{
		Story->SetVar(TEXT("suara.tabrak"), Story->GetVar(TEXT("suara.tabrak")) + 1);
		Story->Run(FString::Printf(TEXT("notifburuk %s Diawasi +%.0f, Suara -%.0f."),
			bRiot ? TEXT("Kerusuhan! Warga ramai mengerubungi Bayu, ada yang merekam.") : *What, Delta, Trust));
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

bool AGRCrowd::FindPickupSpot(const FVector& Around, float MinR, float MaxR, FVector& OutFeet) const
{
	return FindSpot(Around, MinR, MaxR, false, true, OutFeet) || FindSpot(Around, MinR, MaxR, false, false, OutFeet);
}

AGRWarga* AGRCrowd::SpawnPassenger(const FVector& Feet, float Yaw, int32 Seed, bool bFemale)
{
	AGRWarga* Person = TakeFromPool();
	if (!Person)
	{
		for (AGRWarga* Other : People)
		{
			if (Other->bActive && !Other->bOjol && !Other->IsPolice() && !Other->DriverCar.IsValid() && Other->Mode != AGRWarga::EMode::Arrest
				&& !InView(Other->GetActorLocation() - FVector(0.f, 0.f, CapsuleHalf)))
			{
				Other->Deactivate();
				Person = Other;
				break;
			}
		}
	}
	if (!Person)
	{
		return nullptr;
	}
	const int32 Hour = HourNow(this);
	// The order already named them: dress until the body matches.
	for (int32 Try = 0; Try < 12; ++Try)
	{
		Person->Dress(Seed + Try * 101, AGRWarga::ERole::Stroller, Hour >= 20 || Hour < 5);
		if (Person->IsFemale() == bFemale)
		{
			break;
		}
	}
	Person->Activate(Feet, Yaw);
	Person->OjolStart();
	return Person;
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
		Person->bPhone = i == 1 || i == 3;
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
	int32 Modes[10] = {};
	int32 Roles[4] = {};
	int32 Active = 0;
	int32 OnRoad = 0;
	int32 OnZebra = 0;
	int32 Jaywalk = 0;
	int32 ToZebra = 0;
	for (const AGRWarga* Person : People)
	{
		if (Person->bActive)
		{
			++Active;
			++Modes[int32(Person->Mode)];
			++Roles[int32(Person->Role)];
			const FVector2D P(Person->GetActorLocation());
			OnRoad += IsRoad(P) ? 1 : 0;
			ToZebra += Person->bToZebra ? 1 : 0;
			// On the asphalt of Jl. Raya / Jl. Merdeka (not the roundabout): on a zebra crossing or not.
			for (int32 Axis = 0; Axis < 2; ++Axis)
			{
				const float Across = Axis == 0 ? P.X : P.Y;
				const float Along = Axis == 0 ? P.Y : P.X;
				if (FMath::Abs(Across) < RoadEdge && FMath::Abs(Along) > 2600.f && FMath::Abs(Along) < AlongFar)
				{
					const bool bZebra = FMath::Abs(GRCrossings::Nearest(Along) - Along) < GRCrossings::HalfBand + 60.f;
					OnZebra += bZebra ? 1 : 0;
					Jaywalk += bZebra ? 0 : 1;
				}
			}
		}
	}
	int32 Hidden = 0;
	for (const FBench& B : Benches)
	{
		Hidden += B.bHidden ? 1 : 0;
	}
	UE_LOG(LogGRCrowd, Warning, TEXT("GRWARGA aktif %d/%d target %d | jalan %d diam %d ngobrol %d duduk %d jatuh %d marah %d lirik %d buntuti %d kejar %d tangkap %d | joging %d pedagang %d polisi %d | di jalan raya %d (zebra %d, sembarangan %d, menuju zebra %d) | bangku %d (%d disimpan)"),
		Active, People.Num(), TargetPopulation(), Modes[0], Modes[1], Modes[2], Modes[3], Modes[4], Modes[5], Modes[6], Modes[7], Modes[8], Modes[9], Roles[1], Roles[2], Roles[3], OnRoad, OnZebra, Jaywalk, ToZebra, Benches.Num(), Hidden);
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
	const TCHAR* ModeNames[] = { TEXT("jalan"), TEXT("diam"), TEXT("ngobrol"), TEXT("duduk"), TEXT("jatuh"), TEXT("marah"), TEXT("lirik"), TEXT("buntuti"), TEXT("kejar"), TEXT("tangkap") };
	for (int32 i = 0; i < People.Num(); ++i)
	{
		const AGRWarga* Person = People[i];
		if (Person->bActive && Player && FVector::Dist2D(Person->GetActorLocation(), Player->GetActorLocation()) < 3500.f)
		{
			const FVector L = Person->GetActorLocation();
			const float HeadZ = Person->GetMesh()->GetBoneLocation(TEXT("head")).Z - (L.Z - CapsuleHalf);
			UE_LOG(LogGRCrowd, Warning, TEXT("GRWARGA %2d %-7s (%.0f, %.0f, %.0f) ke (%.0f, %.0f) laju %.0f kepala %.1f skala %.2f %s%s%s"), i, ModeNames[int32(Person->Mode)],
				L.X, L.Y, L.Z, Person->Target.X, Person->Target.Y, Person->GetVelocity().Size2D(), HeadZ, Person->GetMesh()->GetRelativeScale3D().Z,
				Person->GetMesh()->GetSkeletalMeshAsset() == Person->QuinnMesh ? TEXT("quinn") : TEXT("manny"), Person->bCrossing ? TEXT(" nyeberang") : Person->bToZebra ? TEXT(" ke_zebra") : TEXT(""),
				Person->Role == AGRWarga::ERole::Vendor ? TEXT(" pedagang") : Person->Role == AGRWarga::ERole::Jogger ? TEXT(" joging")
				: Person->IsPolice() ? TEXT(" polisi") : TEXT(""));
		}
	}
}

#undef LOCTEXT_NAMESPACE
