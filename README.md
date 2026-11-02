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

## Struktur kode (`Source/GemahRipah`)

| Folder | Isi |
|---|---|
| `Core/` | `GRGameMode`, `GRPlayerController` (input dan mode kamera), `GRCheatManager` (perintah debug), `GRGameplayTags` (tag native), `GRScriptLibrary` (helper Blueprint/Python) |
| `Player/` | `GRCharacter` (kamera orang ketiga ala Genshin, lari, zoom), `GRInteractionComponent` |
| `Interaction/` | Interface `GRInteractable` dan `GRInteractableActor` (interaksi berbasis data untuk prototipe) |
| `Camera/` | `GRTopDownRoomVolume`: kamera berpindah ke top-down saat pemain masuk ruangan |
| `Systems/` | Subsystem yang bertahan antar-map: status (uang, Suara, Diawasi), aturan ("Pengumuman Pemerintah"), progres (flag cerita, kota), dan save |
| `UI/` | `GRHUD`: HUD sementara berbasis Canvas, nanti diganti UMG |

Keputusan teknis:

- Logika ditulis di C++, karena Blueprint berupa file biner yang sulit di-review. Blueprint dipakai untuk penyesuaian konten saja.
- Input Enhanced Input dibuat di C++ (`GRPlayerController::BuildInputAssets`), tanpa aset `.uasset`.
- Satu kota = satu map. Daftar kota ada di `Config/DefaultGame.ini` bagian `GRCitySettings`, atau di Project Settings > Game > Gemah Ripah Kota.
- Aturan baru dibuat sebagai Data Asset `GRRegulationData` di `/Game/GemahRipah/Data/Regulations`.

## Kontrol

| Aksi | Keyboard/mouse | Gamepad |
|---|---|---|
| Jalan | WASD | Stik kiri |
| Kamera | Mouse | Stik kanan |
| Zoom | Roda mouse | - |
| Lari | Shift kiri | Klik stik kiri |
| Lompat | Spasi | A / Silang |
| Interaksi | E | X / Kotak |

## Perintah debug (konsol `~`, saat Play)

| Perintah | Fungsi |
|---|---|
| `GRStatus` | Tampilkan uang, meter, aturan, flag, kota |
| `GRUang 10000` / `GRSuara 10` / `GRDiawasi 10` | Ubah status |
| `GRUmumkanLarangan` | Umumkan aturan uji "Larangan Berkumpul" |
| `GRCabutSemuaAturan` | Cabut semua aturan |
| `GRAksi Aksi.Kumpul.Demo` | Laporkan aksi ke sistem aturan |
| `GRFlag Flag.Bab1.BuktiDapur` | Tambah flag cerita |
| `GRBukaKota <Id>` / `GRKota <Id>` | Buka atau pindah kota |
| `GRSimpan` / `GRMuat` | Simpan atau muat `Slot0` |

## Map dev (`/Game/GemahRipah/Maps/L_Dev_Sandbox`)

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
    Data/Regulations aset GRRegulationData
  ThirdParty/
    Sketchfab/       aset gratis, satu folder per kode aset (A02, B01, ...)
    Fab/
  Characters/        Mannequin template UE (placeholder)
  LevelPrototyping/  material dan mesh grey-box template UE
```

Setiap aset pihak ketiga wajib dicatat di `Docs/CREDITS.md`.
