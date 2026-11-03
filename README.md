# Gemah Ripah

Game open world satir di Unreal Engine 5.8. Pemain adalah Bayu, pengemudi ojol di Kota Sentosa, Republik Gemah Ripah (negara fiktif), yang mengejar keadilan dan kebebasan.

Semua bab bisa dimainkan: Bab 1 "Asap dan Piring", Bab 2 "Turun ke Jalan", Bab 3 "Pasal Karet", Bab 4 "Proyek Mercusuar", dan Bab 5 "Suara Rakyat" (tamat).

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
| `UI/` | UMG dibangun di C++: tumpukan layar (`GRUISubsystem`), menu utama, jeda, simpan/muat, pengaturan, kredit, konfirmasi, HUD (`GRHUDWidget`), loading screen (Slate). Layar cerita di `GRStoryScreens` (dialog, cutscene, toko, jurnal, akhir bab), `GRPhoneScreens` (ponsel, minigame warnet, minigame menyusup) dan `GRChapterScreens` (Peta Bab). Animasi UI lewat `GRUIAnim` (tween opacity/geser/skala) |

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
| `GRBab bab2` | Lompat ke bab tertentu (bab sebelumnya dianggap selesai) |
| `GRCek "syarat"` | Cek syarat cerita, mis. `GRCek "q.Pagi>=2 && uang>10000"` (hasil di log) |
| `GRPos 8740 -9000 0` | Pindahkan pemain ke x, y (cm) menghadap yaw, untuk screenshot otomatis |

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

## Bab dan Peta Bab

Daftar bab ada di `Content/Data/chapters.json`: nomor, file cerita, judul, ringkasan, ikon, warna, posisi di Peta Bab. Bab dengan `file` kosong tampil sebagai "segera hadir".

- Peta Bab dibuka dari menu jeda atau dari layar akhir bab. Bab berikutnya terbuka setelah bab sebelumnya selesai.
- Pindah bab memuat ulang map kota. Uang, Suara, Diawasi, aturan, dan variabel cerita terbawa (Bab 2 bisa memeriksa `guru_dilindungi` dari Bab 1). Tugas, kotak pesan, jam, dan energi mulai baru.
- Akhir bab: aksi `selesai` menandai bab selesai, menyimpan otomatis, lalu menampilkan baris `ending` dari file bab.
- Teks tugas boleh memuat `{variabel}`, mis. `dukungan {dukungan}/3`.

Ikon UI (aplikasi ponsel, bab, status, HUD, menu, barang, notifikasi) adalah tile berwarna buatan Google Gemini di `Content/Data/Icons` (128 px). Sumbernya di `Tools/IconSheets`, dipotong dengan `Tools/Python/slice_icon_sheet.py`. Dimuat saat game berjalan, tanpa impor ke editor. Barang memakai `item_<id>`, atau field `icon` di data barang.

Potret tokoh juga dari Gemini: `Content/Data/Icons/portrait_<id speaker>.png` (256 px). Dialog `vn` menampilkannya besar di kartu, dialog `3d` sebagai avatar di samping teks, pesan `chat` di judul. Tanpa file potret, kartu menampilkan huruf awal nama.

Detail interaksi:

- Pilihan dialog bernomor dan bisa dipilih dengan tombol 1-5 (atau panah + Enter, atau klik).
- Meter dialog (`meter`, `meterLabel`, `meterColor` opsional, merah bila kosong) bergerak halus dan menampilkan perubahan terakhir (+5 / -8).
- HUD menampilkan penanda tujuan misi dengan jarak. Bila tujuan di luar layar, penanda menempel di tepi layar dengan nama tujuan.
- Hover mouse hanya mengambil fokus tombol bila mouse benar-benar bergerak, jadi tombol yang beranimasi masuk tidak merebut fokus keyboard.

Perabot dan pose:

- Ruangan tempat orang diajak bicara punya meja dan kursi: Kantor Kelurahan, Warnet Jaya, rumah Bayu, Pos Ronda, layanan Puskesmas, meja piket Pos SPN, fotokopi Pak Harso. Datanya di `Content/Data/furniture.json` (dibuat oleh `Tools/Python/make_furniture.py`) dan dipakai semua bab.
- Spawn `prop` dengan `model` dirakit dari bentuk dasar (`World/GRPropModels.cpp`): `meja`, `meja_makan`, `meja_kantor`, `meja_pc`, `meja_kecil`, `kursi`, `kursi_kantor`, `kursi_plastik`, `bangku`, `rak`, `lemari`, `lemari_arsip`, `dapur`, `dispenser`, `pot`, `tenda`, `kotak_suara`, `bilik`, `motor`, `warung`, `lapak`, `stensil`, `papan`, `baliho`, `spanduk`, `jeruji`, `pintu`, `tiang`, `patok`, `sembako`, `panggung`, `ekskavator`. Atap tenda dan tenda warung hilang saat pemain berdiri di bawahnya.
- NPC dengan `"pose": "duduk"` duduk di tempat (taruh `kursi` di posisi yang sama) dan tidak berputar atau berkeliling.
- NPC bergerak tangan dan mengangguk saat baris dialognya tampil, juga saat bark sambil diam. Animasi `MM_GR_Talk`, `MM_GR_Sit`, `MM_GR_SitTalk` dibuat dari `MM_Idle` oleh `Tools/Python/make_talk_anims.py`.
- Perabot di ruangan stealth memakai syarat `!menyusup` (variabel ini 1 selama level stealth berjalan). Syarat `bab>=3` memakai nomor bab.

## Bab 1 "Asap dan Piring"

Seluruh isi bab ada di `Content/Data/bab1.json`. Ubah teks, quest, NPC, atau minigame tanpa compile, lalu `GRReloadData`.

Alur singkat (3 hari):

1. Hari 1 pagi: ngobrol dengan Ibu, ambil motor, antar order pertama, beli beras.
2. Dinda keracunan menu PPR. Bayu ke Puskesmas, Kelurahan (SKTM), sekolah, dan bertemu Bu Ratna.
3. Hari 2 malam: menyusup ke Dapur PPR dan mengambil bukti. Minigame dimainkan di ruangan dapur asli dengan kamera atas: grid `minigames.dapur.grid` jadi rak, sekat, kotak bukti, dan satpam bersenter. Kena senter terlalu lama = ketahuan (coba lagi atau kabur).
4. Hari 3: unggah bukti di Warnet Jaya (minigame warnet). Jam malam dan razia SPN berlaku.
5. Cutscene SPN, interogasi, rapat warga, lalu layar akhir bab.

Kalau Diawasi mencapai 100 atau SPN melihat Bayu saat mencurigakan terlalu lama, Bayu ditangkap (`onCaught`). Kalau Energi habis, Bayu pingsan (`onFaint`).

## Bab 2 "Turun ke Jalan"

Isi di `Content/Data/bab2.json`. Larangan Berkumpul berlaku. Bayu menggalang dukungan warga (Pak Udin, Bu Yati, Bu Tini, Mas Joko, Rudi, Bu Ratna bila dilindungi di Bab 1), mendesain pamflet di warnet, menempelnya di tiga tempat sambil menghindari SPN, lalu memimpin aksi piring kosong dari Tugu ke Gedung Dewan. Negosiasi dengan Komandan Wiryo punya meter ketegangan; hasilnya audit PPR atau pembubaran paksa.

## Bab 3 "Pasal Karet"

Isi di `Content/Data/bab3.json`. Pasal Penghinaan Pejabat berlaku dan Rudi ditahan (dari Bab 2, atau dijemput di awal Bab 3).

1. Siapa yang Melapor: Sari (LBH) di Pos Ronda, jenguk Rudi di sel titipan Pos SPN (Wiryo menawarkan "biaya penangguhan" yang bisa direkam), kunci cadangan dari Pak RT, lalu menyusup ke arsip Kantor Kelurahan setelah pukul 19.00. Minigame `arsip` memakai ruangan kelurahan asli dengan hansip bersenter. Dokumennya: laporan warga No. 47, memo target laporan, daftar pantauan warga.
2. Kabar Gang: Pak Harso (mantan wartawan) di Ruko Blok A, menyusun koran stensilan di mesin stensil (minigame `koran`), lalu membagikannya ke Bu Tini, Mas Joko, dan Bu Yati. Membawa koran = dicurigai SPN; kalau tertangkap, korannya disita dan harus dicetak ulang.
3. Radio Gang 107,7: pemancar rakitan dari Mas Joko, antena di Pos Ronda, siaran malam dengan meter pendengar. Pulang sebelum SPN menemukan sinyal.

Hasil: Rudi bebas lewat praperadilan bila pendengar cukup dan laporan palsunya terbongkar; kalau tidak, ia hanya ditangguhkan. Penutup memberi petunjuk Bab 4 (patok proyek di Pasar Sentosa).

## Bab 4 "Proyek Mercusuar"

Isi di `Content/Data/bab4.json`. Pemkot dan Dewan mengumumkan Menara Gemilang (88 lantai) tepat di atas Pasar Sentosa. Pedagang diberi surat relokasi dan tenggat tujuh hari.

1. Patok Oranye: Bu Tini menunjukkan surat relokasi, Bayu datang ke sosialisasi Pak Hendro (Humas PT Gemilang Mercusuar Abadi) di depan kelurahan, lalu melihat sendiri "Lokasi Strategis" di belakang Perumahan Sentosa Indah: rawa.
2. Siapa yang Untung?: riset bersama Mas Joko di warnet (dialog teka-teki kata kunci) menemukan Rendy Gemilang, 24 tahun, direktur PT, beralamat sama dengan Dapur PPR. Setelah pukul 19.00 Bayu menyusup ke Kantor Pemasaran (minigame `pemasaran`, ruangan bekas Dapur PPR, satpam proyek) dan mengambil foto maket asli, sertifikat lahan, dan daftar pemesan penthouse. Dokumen ditunjukkan ke Sari.
3. Pasar Bertahan: ajak Bu Tini, Koh Liong, dan Mak Ijah bertahan, tidur, lalu jaga gerbang pasar pagi harinya. Dialog `eksekusi` memakai meter ketegangan: siaran langsung, duduk di depan ekskavator, atau bicara baik-baik, lalu dokumen dibacakan.

Hasil: variabel `tekanan` (dokumen yang dibawa ke Sari, tiga pedagang bertahan, siaran langsung; berkurang bila ketegangan terlalu tinggi) menentukan pembongkaran ditunda (`gusur_ditunda`) atau los ikan dirobohkan (`pasar_rusak`). Penutup memberi petunjuk Bab 5 (Ketua Dewan maju jadi Wali Kota, kertas suara dicetak PT yang sama).

## Bab 5 "Suara Rakyat"

Isi di `Content/Data/bab5.json`. Pemilu Wali Kota: Wiratama Gemilang (Ketua Dewan, No. 1) melawan Bu Ratna (guru honorer dari Bab 1, calon independen No. 2).

1. Serangan Fajar: Ibu menerima amplop "uang transpor" (dikembalikan atau dipakai), lalu Bang Jefri dari tim sukses menawari Bayu Rp300.000 untuk mengoordinir ojol. Bayu bisa merekamnya, menolak, atau menerima. Kampanye akbar No. 1 di dekat Tugu bisa didatangi (dialog `wiratama_kampanye`).
2. Surat Suara Gemilang: Pak Harso membocorkan surat suara cadangan yang sudah dicoblos, Pak Darmo sengaja "lupa" mengunci gudang, lalu Bayu menyusup ke gudang logistik Kantor Kelurahan setelah pukul 20.00 (minigame `logistik`, penjaga Linmas yang patroli kolom). Buktinya diunggah dari Warnet Jaya (minigame `laporan`, viral >= 100 memberi `laporan_viral`).
3. Saksi Rakyat: Bayu menjadi saksi Bu Ratna, tidur, lalu menjaga TPS 07 di depan SDN Sentosa 1. Dialog `hitung` memakai meter `suara_ratna` yang naik atau turun karena pilihan dari semua bab: `suara`, `tolak_amplop`, `rudi_bebas` (Bab 3), `gusur_ditunda` (Bab 4), `guru_dilindungi` (Bab 1), `laporan_viral`, keberatan atas surat suara tercoblos, dan rekaman Bang Jefri.

Hasil: `suara_ratna` >= 50 memberi `ratna_menang`, selain itu `ratna_kalah` (gugatan sengketa). Layar akhir merangkum hasil setiap bab, lalu TAMAT.

### Bahasa cerita

Syarat (`if`, `suspicious`): gabungan `&&`, `||`, `!`, dan perbandingan `>= <= == != > <`.

- Nilai bawaan: `day`, `hour`, `malam`, `uang`, `suara`, `diawasi`, `energi`, `order`, `orders_done`, `bab` (nomor bab).
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

Minigame stealth: `room` berisi tag `MinigameId` volume ruangan, atau id spawn yang berdiri di dalam ruangan (contoh `kel_dalam`). `guard` = nama penjaga. Minigame unggah/cetak: `submit` = label tombol terbit.

Bagian file: `ending`, `start`, `onCaught`, `onFaint`, `onMorning`, `suspicious`, `items`, `shop`, `quests`, `speakers`, `dialogs` (gaya `vn`, `3d`, `chat`), `spawns` (`npc`, `spn`, `prop`, `zone`), `events`, `cutscenes`, `orders`, `kicau`, `berita`, `needs`, `barks`, `minigames`.

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
