#include "World/GRTripoProps.h"

#include "Engine/StaticMesh.h"

namespace
{
	struct FTripoAsset
	{
		const TCHAR* Code;
		const TCHAR* Path;
	};

	const FTripoAsset Assets[] = {
		{ TEXT("P01"), TEXT("PropertiLuar/SM_P01_Tiang_listrik") },
		{ TEXT("P02"), TEXT("PropertiLuar/SM_P02_Tong_sampah_semen") },
		{ TEXT("P03"), TEXT("PropertiLuar/SM_P03_Kursi_plastik") },
		{ TEXT("P04"), TEXT("PropertiLuar/SM_P04_Meja_warung") },
		{ TEXT("P05"), TEXT("PropertiLuar/SM_P05_Karung_beras") },
		{ TEXT("P06"), TEXT("PropertiLuar/SM_P06_Galon_dan_dispenser") },
		{ TEXT("P07"), TEXT("PropertiLuar/SM_P07_Baliho") },
		{ TEXT("P08"), TEXT("PropertiLuar/SM_P08_Spanduk_demo") },
		{ TEXT("P09"), TEXT("PropertiLuar/SM_P09_Pagar_seng") },
		{ TEXT("P10"), TEXT("PropertiLuar/SM_P10_Jemuran") },
		{ TEXT("P11"), TEXT("PropertiLuar/SM_P11_Gerobak_PPR") },
		{ TEXT("P12"), TEXT("PropertiLuar/SM_P12_Tumpukan_ban") },
		{ TEXT("P13"), TEXT("PropertiLuar/SM_P13_Tenda_TPS") },
		{ TEXT("P14"), TEXT("PropertiLuar/SM_P14_Bilik_suara") },
		{ TEXT("P15"), TEXT("PropertiLuar/SM_P15_Kotak_suara") },
		{ TEXT("P16"), TEXT("PropertiLuar/SM_P16_Panggung_kampanye") },
		{ TEXT("P17"), TEXT("PropertiLuar/SM_P17_Patok_gusuran") },
		{ TEXT("P18"), TEXT("PropertiLuar/SM_P18_Lampu_jalan") },
		{ TEXT("P19"), TEXT("PropertiLuar/SM_P19_Pohon_peneduh") },
		{ TEXT("P20"), TEXT("PropertiLuar/SM_P20_Bangku_trotoar") },
		{ TEXT("P21"), TEXT("PropertiLuar/SM_P21_Halte") },
		{ TEXT("P22"), TEXT("PropertiLuar/SM_P22_Rambu_lalu_lintas") },
		{ TEXT("P23"), TEXT("PropertiLuar/SM_P23_Selokan_beton") },
		{ TEXT("P24"), TEXT("PropertiLuar/SM_P24_Lapak_pasar") },
		{ TEXT("R01"), TEXT("PropertiRuangan/SM_R01_Meja_dan_lemari_arsip") },
		{ TEXT("R02"), TEXT("PropertiRuangan/SM_R02_Kompor_besar_dapur") },
		{ TEXT("R03"), TEXT("PropertiRuangan/SM_R03_Rak_ompreng") },
		{ TEXT("R04"), TEXT("PropertiRuangan/SM_R04_Kulkas_rusak") },
		{ TEXT("R05"), TEXT("PropertiRuangan/SM_R05_PC_warnet") },
		{ TEXT("R06"), TEXT("PropertiRuangan/SM_R06_Kasur_tipis") },
		{ TEXT("R07"), TEXT("PropertiRuangan/SM_R07_Meja_interogasi") },
		{ TEXT("R08"), TEXT("PropertiRuangan/SM_R08_Kipas_angin") },
		{ TEXT("R09"), TEXT("PropertiRuangan/SM_R09_Papan_tulis") },
		{ TEXT("R10"), TEXT("PropertiRuangan/SM_R10_Kotak_P3K") },
		{ TEXT("R11"), TEXT("PropertiRuangan/SM_R11_Mesin_stensil") },
		{ TEXT("R12"), TEXT("PropertiRuangan/SM_R12_Jeruji_sel") },
		{ TEXT("R13"), TEXT("PropertiRuangan/SM_R13_Kursi_kantor") },
		{ TEXT("R14"), TEXT("PropertiRuangan/SM_R14_Kursi_kayu") },
		{ TEXT("R15"), TEXT("PropertiRuangan/SM_R15_Meja_makan_kayu") },
		{ TEXT("R16"), TEXT("PropertiRuangan/SM_R16_Lemari_pakaian_kayu") },
		{ TEXT("R17"), TEXT("PropertiRuangan/SM_R17_TV_tabung") },
		{ TEXT("R18"), TEXT("PropertiRuangan/SM_R18_Karpet") },
		{ TEXT("R19"), TEXT("PropertiRuangan/SM_R19_Bingkai_foto_dinding") },
		{ TEXT("R20"), TEXT("PropertiRuangan/SM_R20_Pot_tanaman") },
		{ TEXT("R21"), TEXT("PropertiRuangan/SM_R21_Meja_dapur_stainless") },
	};

	struct FModelSwap
	{
		const TCHAR* Model;
		const TCHAR* Code;
		float Yaw;
	};

	// Basic-shape models with a Tripo look-alike. Left out: vehicles, the dining table (its Tripo mesh brings its own
	// chairs), the notice board, the stall roof of a warung and the shapes without a match.
	const FModelSwap Swaps[] = {
		{ TEXT("kursi_plastik"), TEXT("P03"), 0.f },
		{ TEXT("kursi"), TEXT("R14"), 0.f },
		{ TEXT("kursi_kantor"), TEXT("R13"), 0.f },
		{ TEXT("bangku"), TEXT("P20"), 0.f },
		{ TEXT("meja_kantor"), TEXT("R01"), 0.f },
		{ TEXT("meja_pc"), TEXT("R05"), 0.f },
		{ TEXT("lemari"), TEXT("R16"), 0.f },
		{ TEXT("dispenser"), TEXT("P06"), 0.f },
		{ TEXT("pot"), TEXT("R20"), 0.f },
		{ TEXT("karpet"), TEXT("R18"), 0.f },
		{ TEXT("tv"), TEXT("R17"), 0.f },
		{ TEXT("bingkai"), TEXT("R19"), 0.f },
		{ TEXT("meja_dapur"), TEXT("R21"), 0.f },
		{ TEXT("kompor_besar"), TEXT("R02"), 0.f },
		{ TEXT("rak_dapur"), TEXT("R03"), 0.f },
		{ TEXT("tenda"), TEXT("P13"), 0.f },
		{ TEXT("kotak_suara"), TEXT("P15"), 0.f },
		{ TEXT("bilik"), TEXT("P14"), 0.f },
		{ TEXT("stensil"), TEXT("R11"), 0.f },
		{ TEXT("baliho"), TEXT("P07"), 0.f },
		{ TEXT("spanduk"), TEXT("P08"), 0.f },
		{ TEXT("jeruji"), TEXT("R12"), 0.f },
		{ TEXT("patok"), TEXT("P17"), 0.f },
		{ TEXT("panggung"), TEXT("P16"), 0.f },
		{ TEXT("lapak"), TEXT("P24"), 0.f },
	};
}

UStaticMesh* GRTripoProps::Load(const FString& Code)
{
	static TMap<FString, TWeakObjectPtr<UStaticMesh>> Cache;
	if (TWeakObjectPtr<UStaticMesh>* Found = Cache.Find(Code))
	{
		if (Found->IsValid())
		{
			return Found->Get();
		}
	}
	for (const FTripoAsset& Asset : Assets)
	{
		if (Code == Asset.Code)
		{
			const FString Name = FPaths::GetBaseFilename(Asset.Path);
			const FString Path = FString::Printf(TEXT("/Game/GemahRipah/Props/%s.%s"), Asset.Path, *Name);
			UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
			Cache.Add(Code, Mesh);
			return Mesh;
		}
	}
	return nullptr;
}

FString GRTripoProps::CodeForModel(const FString& Model, float& YawOffset)
{
	for (const FModelSwap& Swap : Swaps)
	{
		if (Model == Swap.Model)
		{
			YawOffset = Swap.Yaw;
			return Swap.Code;
		}
	}
	YawOffset = 0.f;
	return FString();
}
