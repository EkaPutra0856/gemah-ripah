# Gemah Ripah

Game open world satir di Unreal Engine 5.8. Pemain adalah Bayu, pengemudi ojol di Kota Sentosa, Republik Gemah Ripah (negara fiktif), yang mengejar keadilan dan kebebasan.

Target saat ini: *vertical slice* Bab 1 "Asap dan Piring".

## Kebutuhan

- Unreal Engine 5.8 di `E:\UE_5.8`
- Visual Studio 2022 (toolset MSVC 14.44)

## Cara pakai

| Tugas | Cara |
|---|---|
| Compile | Tutup editor, jalankan `Tools\Build.bat` |
| Buka editor | Klik dua kali `GemahRipah.uproject` |
| Buat ulang map dev | Hapus `L_Dev_Sandbox`, tutup editor, jalankan `Tools\BuildDevMap.bat` |
| Buat ulang map menu | Hapus `L_MainMenu`, tutup editor, jalankan `Tools\BuildMainMenu.bat` |
| Tes otomatis + screenshot | Jalankan game dengan `-GRScript=<file>`. Tiap baris: `<detik> <perintah>`, `<detik> shot <nama>` (hasil di `Saved/AutoShots`), atau `<detik> quit` |

## Struktur kode (`Source/GemahRipah`)

| Folder | Isi |
|---|---|
| `Core/` | `GRGameMode`, `GRPlayerController` (input, mode kamera, jeda), `GRMenuGameMode`/`GRMenuPlayerController` (map menu), `GRGameInstance` (loading screen, dev script), `GRGameUserSettings` (pengaturan), `GRCheatManager` (perintah debug), `GRGameplayTags`, `GRScriptLibrary` |
| `Data/` | `GRDataSubsystem`: membaca JSON di `Content/Data/` (aturan, tips loading, kredit) |
| `Player/` | `GRCharacter` (kamera orang ketiga ala Genshin, lari, zoom), `GRInteractionComponent` |
| `Interaction/` | Interface `GRInteractable` dan `GRInteractableActor` (interaksi berbasis data untuk prototipe) |
| `Camera/` | `GRTopDownRoomVolume`: kamera berpindah ke top-down saat pemain masuk ruangan |
| `Systems/` | Subsystem yang bertahan antar-map: status (uang, Suara, Diawasi), aturan ("Pengumuman Pemerintah"), progres (flag cerita, kota), save (otomatis + 3 slot), alur game (game baru, lanjut, menu), notifikasi |
| `UI/` | UMG dibangun di C++: tumpukan layar (`GRUISubsystem`), menu utama, jeda, simpan/muat, pengaturan, kredit, konfirmasi, HUD (`GRHUDWidget`), loading screen (Slate) |

Keputusan teknis:

- Logika ditulis di C++, karena Blueprint berupa file biner yang sulit di-review. Blueprint dipakai untuk penyesuaian konten saja.
- Input Enhanced Input dibuat di C++ (`GRPlayerController::BuildInputAssets`), tanpa aset `.uasset`.
- Satu kota = satu map. Daftar kota ada di `Config/DefaultGame.ini` bagian `GRGameSettings`, atau di Project Settings > Game > Gemah Ripah.
- Konten (aturan, tips, kredit, nanti dialog dan quest) ditulis sebagai JSON di `Content/Data/`. Teks bisa diubah tanpa compile; muat ulang saat main dengan `GRReloadData`.
- UI dibangun di C++ tanpa Widget Blueprint, gayanya diatur di `UI/GRUIStyle.h`.

## Kontrol

| Aksi | Keyboard/mouse | Gamepad |
|---|---|---|
| Jalan | WASD | Stik kiri |
| Kamera | Mouse | Stik kanan |
| Zoom | Roda mouse | - |
| Lari | Shift kiri | Klik stik kiri |
| Lompat | Spasi | A / Silang |
| Interaksi | E | X / Kotak |
| Jeda | Esc atau P (P untuk Play di editor) | Start |
| Kembali di menu | Esc | B / Lingkaran |

## Perintah debug (konsol `~`, saat Play)

| Perintah | Fungsi |
|---|---|
| `GRStatus` | Tampilkan uang, meter, aturan, flag, kota |
| `GRUang 10000` / `GRSuara 10` / `GRDiawasi 10` | Ubah status |
| `GRUmumkan Aturan.PasalKaretKritik` | Umumkan aturan dari `regulations.json` (`GRUmumkanLarangan` = Larangan Berkumpul) |
| `GRCabutSemuaAturan` | Cabut semua aturan |
| `GRAksi Aksi.Kumpul.Demo` | Laporkan aksi ke sistem aturan |
| `GRFlag Flag.Bab1.BuktiDapur` | Tambah flag cerita |
| `GRBukaKota <Id>` / `GRKota <Id>` | Buka atau pindah kota |
| `GRSimpan [Slot1..3/Auto]` / `GRMuat [slot]` | Simpan atau muat (bawaan `Slot1`) |
| `GRUI menu/jeda/simpan/muat/pengaturan/kredit/tutup` | Buka layar UI |
| `GRNewGame` / `GRMenu` | Mulai game baru / kembali ke menu utama |
| `GRReloadData` | Muat ulang JSON di `Content/Data/` |
| `GRNotif <teks>` | Tes notifikasi |

## Map

- `L_MainMenu`: map awal game. Jalan berasap dilihat dari satu kamera, menu utama di atasnya.

### Map dev (`/Game/GemahRipah/Maps/L_Dev_Sandbox`)

Map *grey-box* hasil `Tools/Python/build_dev_map.py`. Isinya:

- Gang kampung dengan langit berasap oranye.
- Titik "Antar order" untuk menambah uang.
- Titik "Ngobrol dengan warga" berupa aksi `Aksi.Kumpul`. Aksi ini melanggar aturan setelah `GRUmumkanLarangan`.
- Ruang warnet top-down dengan PC untuk "Unggah kritik".

## Struktur konten (rencana)

```
Content/
  GemahRipah/
    Maps/            map per kota + sandbox dev
    Characters/      hasil Tripo (Bayu, keluarga, NPC)
    Environment/     bangunan, properti
    Vehicles/
    UI/              ikon, potret dialog, logo (dari Gemini)
  ThirdParty/
    Sketchfab/       aset gratis, satu folder per kode aset (A02, B01, ...)
    Fab/
  Data/              JSON konten (aturan, tips, kredit, dialog, quest)
  Characters/        Mannequin template UE (placeholder)
  LevelPrototyping/  material dan mesh grey-box template UE
```

Setiap aset pihak ketiga wajib dicatat di `Docs/CREDITS.md`.
