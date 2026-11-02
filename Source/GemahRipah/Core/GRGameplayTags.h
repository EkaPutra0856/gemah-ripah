#pragma once

#include "NativeGameplayTags.h"

// Native gameplay tags. Keep them here (text, diffable) instead of DefaultGameplayTags.ini.
namespace GRTags
{
	// Player actions that regulations can forbid.
	GEMAHRIPAH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aksi_Unggah_Kritik);
	GEMAHRIPAH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aksi_Kumpul);
	GEMAHRIPAH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aksi_Kumpul_Demo);
	GEMAHRIPAH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aksi_Bicara_Media);

	// Regulation ids ("Pengumuman Pemerintah").
	GEMAHRIPAH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aturan_LaranganBerkumpul);
	GEMAHRIPAH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aturan_PasalKaretKritik);

	// Story flags, Bab 1 "Asap dan Piring".
	GEMAHRIPAH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Flag_Bab1_AdikKeracunan);
	GEMAHRIPAH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Flag_Bab1_BuktiDapur);
	GEMAHRIPAH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Flag_Bab1_BuktiDiunggah);
	GEMAHRIPAH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Flag_Bab1_AkunHilang);

	// Top-down minigame rooms.
	GEMAHRIPAH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Minigame_DapurPPR);
	GEMAHRIPAH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Minigame_Warnet);
}
