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
| `World/` | `GRNpc` (warga dan petugas SPN: jalan-jalan, patroli, celetukan, pandangan senter), `GRStealthLevel` (minigame menyusup di ruangan asli: rak, bukti, satpam dari grid JSON), `GRStoryProp` (benda atau zona dari data), `GRMapSubsystem` (kawasan, penanda, jejak bangunan untuk peta), `GRDistrictVolume`, `GRMapMarkerComponent`/`GRMapMarker`, `GRMapBounds`, `GRDayNightController` (matahari, bulan, kabut asap, lampu jalan), `GRBed` (tidur) |
| `Story/` | Sistem cerita berbasis data: `GRStoryTypes` (struktur `bab1.json`), `GRStorySubsystem` (variabel, syarat, aksi, quest, energi, order ojol, kotak pesan, dompet), `GRStoryDirector` (memunculkan NPC/prop/zona, lalu lintas, event, membuka dialog/cutscene/minigame) |
| `Camera/` | `GRTopDownRoomVolume`: kamera berpindah ke top-down saat pemain masuk ruangan |
| `Systems/` | Subsystem yang bertahan antar-map: status (uang, Suara, Diawasi), aturan ("Pengumuman Pemerintah"), progres (flag cerita, kota), save (otomatis + 3 slot), alur game (game baru, lanjut, menu), notifikasi |
| `UI/` | UMG dibangun di C++: tumpukan layar (`GRUISubsystem`), menu utama, jeda, simpan/muat, pengaturan, kredit, konfirmasi, HUD (`GRHUDWidget`), loading screen (Slate). Layar cerita di `GRStoryScreens` (dialog, cutscene, toko, jurnal, akhir bab) dan `GRPhoneScreens` (ponsel, minigame warnet, minigame menyusup) |

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
| Peta kota | M | Back / View |
| Ponsel | Tab | D-pad atas |
| Jurnal (tugas, barang, keluarga) | J | D-pad bawah |
| Naik/turun motor | F | Y / Segitiga |
| Lanjut dialog | E, Spasi, Enter, klik | A / Silang |
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
| `GRJam 19.5` | Lompat ke jam tertentu |
| `GRAsap 300` | Paksa nilai ISPU (`GRAsap -1` = kembali ke data) |
| `GRTidur` | Tidur sampai pagi (hari berikutnya + autosave) |
| `GRPetaUlang` | Hitung ulang gambar peta dari bangunan |
| `GRCerita "aksi; aksi"` | Jalankan aksi cerita, mis. `GRCerita "quest Piring; teleport ibu_pkm"` |
| `GRMotor` | Naik/turun motor (sama dengan F) |
| `GRTekan W 1.5` | Tahan tombol selama 1,5 detik, untuk tes otomatis (jalan, minigame, menu) |
| `GRCek "syarat"` | Cek syarat cerita, mis. `GRCek "q.Pagi>=2 && uang>10000"` (hasil di log) |

## Map

- `L_KotaSentosa`: kota pertama (World Partition, 1 km). Dibuat ulang dengan `Tools/BuildKotaSentosa.bat` (hapus dulu map lama dan folder `__ExternalActors__`/`__ExternalObjects__` miliknya). Lima kawasan: Kampung Gang, Pasar, Kawasan Sekolah, Pusat Pemerintahan, Jalan Raya.
- Waktu: satu hari = 24 menit nyata (`Content/Data/world.json`). Asap (ISPU) naik tiap hari; pemerintah selalu mengklaim "udara baik".
- Tidur hanya bisa pukul 18.00 sampai 04.00 di kasur rumah Bayu. Bangun pukul 06.00 dan game tersimpan otomatis.
- `L_MainMenu`: map awal game. Jalan berasap dilihat dari satu kamera, menu utama di atasnya.

### Map dev (`/Game/GemahRipah/Maps/L_Dev_Sandbox`)

Map *grey-box* hasil `Tools/Python/build_dev_map.py`. Isinya:

- Gang kampung dengan langit berasap oranye.
- Titik "Antar order" untuk menambah uang.
- Titik "Ngobrol dengan warga" berupa aksi `Aksi.Kumpul`. Aksi ini melanggar aturan setelah `GRUmumkanLarangan`.
- Ruang warnet top-down dengan PC untuk "Unggah kritik".

## Bab 1 "Asap dan Piring"

Seluruh isi bab ada di `Content/Data/bab1.json`. Ubah teks, quest, NPC, atau minigame tanpa compile, lalu `GRReloadData`.

Alur singkat (3 hari):

1. Hari 1 pagi: ngobrol dengan Ibu, ambil motor, antar order pertama, beli beras.
2. Dinda keracunan menu PPR. Bayu ke Puskesmas, Kelurahan (SKTM), sekolah, dan bertemu Bu Ratna.
3. Hari 2 malam: menyusup ke Dapur PPR dan mengambil bukti. Minigame dimainkan di ruangan dapur asli dengan kamera atas: grid `minigames.dapur.grid` jadi rak, sekat, kotak bukti, dan satpam bersenter. Kena senter terlalu lama = ketahuan (coba lagi atau kabur).
4. Hari 3: unggah bukti di Warnet Jaya (minigame warnet). Jam malam dan razia SPN berlaku.
5. Cutscene SPN, interogasi, rapat warga, lalu layar akhir bab.

Kalau Diawasi mencapai 100 atau SPN melihat Bayu saat mencurigakan terlalu lama, Bayu ditangkap (`onCaught`). Kalau Energi habis, Bayu pingsan (`onFaint`).

### Bahasa cerita

Syarat (`if`, `suspicious`): gabungan `&&`, `||`, `!`, dan perbandingan `>= <= == != > <`.

- Nilai bawaan: `day`, `hour`, `malam`, `uang`, `suara`, `diawasi`, `energi`, `order`, `orders_done`.
- Variabel: nama bebas, `item.X` (jumlah barang), `q.Quest` (langkah quest, 100 = selesai), `ev.Id` (event sudah jalan), `baca.Id`, `order.Id`.

Aksi (`do`, `end`, `win`, `lose`), dipisah `;`:

| Aksi | Contoh |
|---|---|
| Variabel | `set x 1`, `add x 2`, `flag x`, `unflag x` |
| Status | `uang -15000 Beli beras`, `suara 5`, `diawasi 10`, `energi 40`, `item beras 1` |
| Quest | `quest Piring`, `step Piring 3`, `done Piring` |
| Pesan | `notif teks`, `notifbaik teks`, `notifburuk teks`, `pesan id_pesan` |
| Dunia | `umumkan Aturan.X`, `aksi Aksi.X`, `jam 19.5`, `order o_nasi`, `batalorder` |
| Layar | `dialog id`, `cutscene id`, `minigame id`, `toko pasar`, `ponsel`, `selesai` |
| Lain | `teleport spawn_id`, `tidur`, `simpan`, `jika syarat ? aksi` |

Bagian file: `start`, `onCaught`, `onFaint`, `onMorning`, `suspicious`, `items`, `shop`, `quests`, `speakers`, `dialogs` (gaya `vn`, `3d`, `chat`), `spawns` (`npc`, `spn`, `prop`, `zone`), `events`, `cutscenes`, `orders`, `kicau`, `berita`, `needs`, `barks`, `minigames`.

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
