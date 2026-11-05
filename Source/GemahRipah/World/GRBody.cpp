#include "World/GRBody.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "Retargeter/IKRetargeter.h"

void UGRBodyAnimInstance::Follow(UIKRetargeter* Retargeter, USkeletalMeshComponent* Source)
{
	Node.IKRetargeterAsset = Retargeter;
	Node.RetargetFrom = ERetargetSourceMode::CustomSkeletalMeshComponent;
	Node.SourceMeshComponent = Source;
	Node.bSuppressWarnings = true;
	if (FIKRetargetProcessor* Processor = Node.GetRetargetProcessor())
	{
		Processor->SetNeedsInitialized();
	}
}

FAnimInstanceProxy* UGRBodyAnimInstance::CreateAnimInstanceProxy()
{
	return new FGRBodyAnimProxy(this, &Node);
}

void UGRBodyAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FGRBodyAnimProxy*>(InProxy);
}

namespace GRBody
{
USkeletalMesh* Find(const FString& Model)
{
	if (Model.IsEmpty())
	{
		return nullptr;
	}
	static TMap<FString, TWeakObjectPtr<USkeletalMesh>> Cache;
	if (const TWeakObjectPtr<USkeletalMesh>* Hit = Cache.Find(Model))
	{
		if (Hit->IsValid())
		{
			return Hit->Get();
		}
	}
	const FString Path = FString::Printf(TEXT("/Game/GemahRipah/Characters/%s/SK_%s.SK_%s"), *Model, *Model, *Model);
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	Cache.Add(Model, Mesh);
	return Mesh;
}

bool Apply(ACharacter* Owner, TObjectPtr<USkeletalMeshComponent>& Body, USkeletalMesh* Model)
{
	USkeletalMeshComponent* Rig = Owner ? Owner->GetMesh() : nullptr;
	if (!Rig)
	{
		return false;
	}
	static TWeakObjectPtr<UIKRetargeter> Retargeter;
	if (!Retargeter.IsValid())
	{
		Retargeter = LoadObject<UIKRetargeter>(nullptr, TEXT("/Game/GemahRipah/Characters/RTG_GR_MannyKeTripo.RTG_GR_MannyKeTripo"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (!Model || !Retargeter.IsValid())
	{
		Rig->SetVisibility(true);
		if (Body)
		{
			Body->SetVisibility(false);
			Body->SetComponentTickEnabled(false);
		}
		return false;
	}

	if (!Body)
	{
		Body = NewObject<USkeletalMeshComponent>(Owner, TEXT("TripoBody"));
		Body->SetupAttachment(Rig);
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Body->SetGenerateOverlapEvents(false);
		Body->bReceivesDecals = false;
		// Same place and facing as the mannequin: both models face +Y with their feet at 0.
		Body->SetRelativeTransform(FTransform::Identity);
		Body->RegisterComponent();
		Body->AddTickPrerequisiteComponent(Rig);
	}
	if (Body->GetSkeletalMeshAsset() != Model)
	{
		Body->SetSkeletalMeshAsset(Model);
	}
	if (!Cast<UGRBodyAnimInstance>(Body->GetAnimInstance()))
	{
		Body->SetAnimInstanceClass(UGRBodyAnimInstance::StaticClass());
	}
	if (UGRBodyAnimInstance* Anim = Cast<UGRBodyAnimInstance>(Body->GetAnimInstance()))
	{
		Anim->Follow(Retargeter.Get(), Rig);
	}
	Body->SetVisibility(true);
	Body->SetComponentTickEnabled(true);
	// The mannequin is the puppeteer: unseen, but its pose must stay current for the body to copy.
	Rig->SetVisibility(false);
	Rig->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	return true;
}

namespace
{
	struct FNamed
	{
		const TCHAR* Key;
		const TCHAR* Model;
	};
	// First match wins: the chapter data names people "Pak Darmo (Kelurahan)", "Bu Ratna (Guru)", ...
	const FNamed StoryCast[] = {
		{ TEXT("Bayu"), TEXT("C01_Bayu") },
		{ TEXT("Bu Sri"), TEXT("C02_Ibu_Bayu") },
		{ TEXT("Dinda"), TEXT("C03_Adik_Bayu") },
		{ TEXT("Rudi"), TEXT("C04_Dimas") },
		{ TEXT("Pak Darmo"), TEXT("N01_Pak_Darmo") },
		{ TEXT("dr. Lestari"), TEXT("N02_Pegawai_kelurahan_wanita") },
		{ TEXT("Lurah"), TEXT("N03_Lurah") },
		{ TEXT("Pak Hendro"), TEXT("N03_Lurah") },
		{ TEXT("Wiratama"), TEXT("N04_Pejabat_pria") },
		{ TEXT("Komandan Wiryo"), TEXT("N08_Komandan_Wiryo") },
		{ TEXT("Bang Jefri"), TEXT("N09_Intel") },
		{ TEXT("Pak Hartono"), TEXT("N10_Pak_Hartono") },
		{ TEXT("Bu Ratna"), TEXT("N11_Bu_Ratna") },
		{ TEXT("Satpam Dapur"), TEXT("N12_Petugas_dapur_pria") },
		{ TEXT("Murid"), TEXT("N14_Siswi_SD") },
		{ TEXT("Orang tua murid"), TEXT("N56_Ibu_muda") },
		{ TEXT("Relawan"), TEXT("N36_Mahasiswa") },
		{ TEXT("Pak RT"), TEXT("N15_Bapak_warga") },
		{ TEXT("Koh Liong"), TEXT("N15_Bapak_warga") },
		{ TEXT("Bu Yati"), TEXT("N16_Ibu_warga") },
		{ TEXT("Mak Ijah"), TEXT("N17_Nenek") },
		{ TEXT("Pak Harso"), TEXT("N18_Kakek") },
		{ TEXT("Mas Joko"), TEXT("N19_Pemuda") },
		{ TEXT("Bu Tini"), TEXT("N20_Pedagang_pasar") },
		{ TEXT("Pedagang"), TEXT("N20_Pedagang_pasar") },
		{ TEXT("Pak Udin"), TEXT("N21_Ojol_bapak") },
		{ TEXT("Ojol"), TEXT("N21_Ojol_bapak") },
		{ TEXT("Satpam"), TEXT("N42_Satpam_kompleks") },
		{ TEXT("Sari"), TEXT("N25_Wartawan") },
		{ TEXT("Petugas"), TEXT("N07_Aparat_patroli") },
		{ TEXT("SPN"), TEXT("N07_Aparat_patroli") },
	};
	// Townspeople and unnamed extras ("Warga", "Pembeli", "Pemilih", ...).
	const TCHAR* Strollers[] = {
		TEXT("N15_Bapak_warga"), TEXT("N16_Ibu_warga"), TEXT("N17_Nenek"), TEXT("N18_Kakek"),
		TEXT("N19_Pemuda"), TEXT("N21_Ojol_bapak"), TEXT("N24_Buruh"), TEXT("N16_Ibu_warga"), TEXT("N20_Pedagang_pasar"),
		// NPC dunia (B3-B6) and pedestrians (P1-P4), Gemini lineups of three.
		TEXT("N32_Pegawai_kantor_pria"), TEXT("N33_Pegawai_kantor_wanita"), TEXT("N34_Kurir_paket"),
		TEXT("N35_Petugas_kebersihan"), TEXT("N36_Mahasiswa"), TEXT("N37_Mahasiswi"),
		TEXT("N38_Kuli_panggul"), TEXT("N40_Pemuda_gondrong"), TEXT("N41_Ibu_PKK"),
		TEXT("N42_Satpam_kompleks"), TEXT("N43_Montir_bengkel"), TEXT("N47_Ibu_berjilbab"),
		TEXT("N49_Bapak_pulang_masjid"), TEXT("N50_Wanita_karier"), TEXT("N51_Pemuda_bertopi"),
		TEXT("N52_Nenek_berjilbab"), TEXT("N53_Pria_flanel"), TEXT("N54_Siswi_SMP"),
		TEXT("N55_Turis_lokal"), TEXT("N56_Ibu_muda"), TEXT("N57_Kakek_kaos_oblong"),
	};
	const TCHAR* Joggers[] = {
		TEXT("N19_Pemuda"), TEXT("N24_Buruh"), TEXT("N25_Wartawan"),
		TEXT("N46_Bapak_jogging"), TEXT("N48_Remaja_hoodie"), TEXT("N36_Mahasiswa"),
	};
	const TCHAR* Vendors[] = {
		TEXT("N20_Pedagang_pasar"), TEXT("N21_Ojol_bapak"), TEXT("N15_Bapak_warga"), TEXT("N39_Penjual_jamu"),
	};
}

FString ForStoryNpc(const FString& Name, const FString& Kind, const FString& Id)
{
	if (Kind == TEXT("spn"))
	{
		return TEXT("N07_Aparat_patroli");
	}
	// "Ibu" alone is Bayu's mother (bab1: ibu_pagi, ibu_rumah, ibu_pkm).
	if (Name == TEXT("Ibu") || Id.StartsWith(TEXT("ibu_")))
	{
		return TEXT("C02_Ibu_Bayu");
	}
	for (const FNamed& Entry : StoryCast)
	{
		if (Name.Contains(Entry.Key, ESearchCase::CaseSensitive))
		{
			return Entry.Model;
		}
	}
	return Strollers[GetTypeHash(Id) % UE_ARRAY_COUNT(Strollers)];
}

FString ForTownsperson(int32 Role, int32 Pick)
{
	Pick = FMath::Abs(Pick);
	switch (Role)
	{
	case 1: return Joggers[Pick % UE_ARRAY_COUNT(Joggers)];
	case 2: return Vendors[Pick % UE_ARRAY_COUNT(Vendors)];
	case 3: return TEXT("N07_Aparat_patroli");
	default: return Strollers[Pick % UE_ARRAY_COUNT(Strollers)];
	}
}
}
