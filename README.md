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
| `World/` | `GRNpc` (warga dan petugas SPN: jalan-jalan, patroli, celetukan, pandangan senter), `GRTraffic` (lalu lintas: mobil dan angkot lajur kiri, bundaran Tugu, tabrakan berfisika), `GRCrowd` (warga kota: jalan di trotoar, nyeberang, ngobrol, duduk di bangku, joging, pedagang gerobak), `GRFacades` (detail sementara bangunan: pintu, jendela, rolling door, kanopi, papan nama toko, atap genteng, tandon), `GRStealthLevel` (minigame menyusup di ruangan asli: rak, bukti, satpam dari grid JSON), `GRStoryProp` (benda atau zona dari data), `GRMapSubsystem` (kawasan, penanda, jejak bangunan untuk peta), `GRDistrictVolume`, `GRMapMarkerComponent`/`GRMapMarker`, `GRMapBounds`, `GRDayNightController` (matahari, bulan, kabut asap, lampu jalan), `GRBed` (tidur) |
| `Story/` | Sistem cerita berbasis data: `GRStoryTypes` (struktur `bab1.json`), `GRStorySubsystem` (variabel, syarat, aksi, quest, energi, order ojol, kotak pesan, dompet), `GRStoryDirector` (memunculkan NPC/prop/zona, memasang lalu lintas dan warga, event, membuka dialog/cutscene/minigame) |
| `UI/` (tambahan) | `GRBubbles` (balon bicara di atas kepala untuk celetukan NPC, warga, dan sopir; digambar HUD), `GRWargaChat` (menu ngobrol dengan warga) |
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
| Naik/turun motor (dekat motor yang diparkir) | F, atau E di motor | Y / Segitiga |
| Di motor: gas / rem dan mundur / belok | W / S / A, D | Stik kiri |
| Di motor: ngebut | Shift kiri | Klik stik kiri |
| Lanjut dialog | E, Spasi, Enter, klik | A / Silang |
| Jeda | Esc atau P (P untuk Play di editor) | Start |
| Kembali di menu | Esc | B / Lingkaran |

Motor: makin kencang, makin lebar beloknya, dan motor miring ke dalam tikungan. Tabrak dinding atau orang berarti berhenti. Turun saat masih jalan: motor direm dulu sampai berhenti. Motor tetap terparkir di tempat Bayu turun (tanda M di peta) dan kembali ke halaman rumah setiap pagi.

Lalu lintas: mobil dan angkot berjalan di lajur kiri, memutari Tugu searah jarum jam (mendahulukan yang sudah di bundaran), lalu putar balik di pinggir kota. Mereka menjaga jarak, berhenti untuk Bayu dan orang di jalan, membunyikan klakson, lalu menyalip pelan motor parkir atau orang yang diam di lajur. Menabrak mobil: di bawah 16 km/j motor memantul dan kamera bergetar; sampai 27 km/j energi -3; di atas itu Bayu terpental (ragdoll), motor jatuh, energi -8, lalu Bayu bangun di samping motor. Mobil yang ditabrak terdorong secara fisika (bisa menyenggol mobil lain), lalu sopirnya kembali ke lajur.

Warga kota: sekitar 36 orang hidup di sekitar Bayu pada siang hari (16 menjelang malam, 6 larut malam). Mereka berjalan di trotoar dan gang, kadang menyeberang saat tidak ada mobil dekat, ngobrol berdua atau bertiga, duduk di bangku trotoar, joging pagi dan sore, atau mendorong gerobak bakso, sate, dawet, gorengan, siomay, dan nasi goreng malam hari (kedua tangan di stang gerobak, animasi `MM_GR_Gerobak` dan `MM_GR_GerobakJalan`). Obrolan memakai celetukan bab yang sedang berjalan. Tekan E untuk membuka menu ngobrol di samping warga (permainan tetap jalan): Sapa (dibalas lambaian, animasi `MM_GR_Lambai`), Tanya kabar (celetukan bab), Tanya jalan ke tujuan (arah mata angin dan jarak ke target quest), dan khusus pedagang Beli jajanan (Rp5.000, energi +8). Tombol angka 1 sampai 5 juga bisa. Saat Bayu lewat berjalan kaki, sebagian warga berhenti, menoleh, melambai, dan menyapa. Yang sedang berhenti kadang main HP (`MM_GR_Ponsel`, HP kecil di antara kedua tangan). Semua celetukan tampil sebagai balon bicara yang muncul dengan animasi; teriakan bergetar dan berbingkai warna. Tombol interaksi tampil sebagai tombol E kecil di samping benda atau orang yang dituju. Mereka melompat minggir kalau Bayu ngebut ke arah mereka dengan motor, dan menjauh dari NPC cerita supaya adegan dan tombol E tetap milik cerita. Warga muncul di luar pandangan kamera dan didaur ulang saat jauh. Baju, celana, kulit, dan sepatu diwarnai oleh material `M_GR_Warga` (dibuat `Tools/Python/make_warga_material.py`; hapus file `.uasset`-nya dulu sebelum menjalankan ulang).

Tabrakan dan kerusuhan: motor yang menabrak warga di atas 450 cm/s membuat mereka terpental dan jatuh (ragdoll), lalu bangun perlahan dan marah: menghadap Bayu, mengacungkan kepalan (animasi `MM_GR_Marah`), dan memaki. Tabrakan pelan hanya mendorong mereka. Mobil yang ditabrak Bayu berhenti, sopirnya turun dari pintu kanan untuk memarahi Bayu, lalu kembali ke mobil dan jalan lagi. Setiap insiden menaikkan Diawasi (warga jatuh +6, terserempet +3, mobil +2 sampai +6). Insiden beruntun dalam 90 detik dikali hingga 3 kali lipat, dan insiden ketiga memicu kerusuhan: Diawasi +5 lagi dan warga sekitar mengerubungi Bayu.

Nabrak orang saat jalan kaki: berjalan menyenggol warga membuat mereka kaget (animasi `MM_GR_Kaget`, tangan terangkat dan mundur selangkah), lalu menegur Bayu, dan Bayu minta maaf. Berlari menyeruduk membuat mereka terdorong dan marah. Sekali dua kali masih aman. Skor tabrakan dihitung dalam 15 detik (senggol 1, seruduk 2); begitu mencapai 5 (misalnya tiga kali menyeruduk), warga melapor: Diawasi +2 (senggol) atau +3 (seruduk), lewat aturan insiden di atas. Setelah lapor, skor mulai dari nol lagi.

Diawasi tinggi di jalan: warga yang berpapasan melirik Bayu sambil bergumam (animasi `MM_GR_Lirik`, badan sedikit memalingkan diri, kepala menoleh). Jumlah yang melirik sekaligus: Diawasi 50 satu orang, 60 dua, 70 tiga, 80 lima, dan makin tinggi makin cepat bergantian. Mulai Diawasi 80 satu warga membuntuti Bayu (90: dua). Pembuntut lebih lambat dari Bayu berjalan (420 dan 300 cm/s), jadi jalan terus atau lari (Shift) membuat jarak melebar; lolos bila jarak lebih dari 17 m, Bayu naik motor, masuk ruangan, atau jalannya terhalang. Kalau Bayu tetap dalam 12 m selama 25 detik, pembuntut menelepon SPN: Diawasi +4. Yang menyapa Bayu juga berkurang saat Diawasi 50 ke atas.

Patroli SPN: di sekitar Bayu selalu ada petugas SPN berseragam biru tua dan bertopi, satu berjalan sendiri dan satu berpasangan pada siang hari (malam hari dua pasang, dini hari satu patroli). Pasangan berjalan berdampingan, berhenti berjaga dengan tangan di belakang sambil memandang jalan (`MM_GR_Siaga`), dan sesekali mengobrol. Mereka bisa disapa dengan E. Saat Diawasi 65 ke atas, petugas yang melihat Bayu melapor lewat radio (`MM_GR_Radio`) dan membuntuti dari jauh; berlari di depan mereka memicu pengejaran, dan setelah 25 detik dekat mereka melaporkan Bayu (Diawasi +3). Saat Diawasi 85 ke atas, mereka menunjuk dan berteriak "Hei! Berhenti!" selama sekejap, lalu mengejar (560 cm/s, lebih cepat dari Bayu berjalan, lebih lambat dari Bayu berlari); petugas kedua memotong dari samping. Bayu lolos bila jaraknya lebih dari 20 m, tidak terlihat beberapa detik, naik motor, atau masuk ruangan. Kalau tertangkap, Bayu angkat tangan (`MM_GR_Menyerah`), petugas memegangnya (`MM_GR_Tangkap`), pasangannya melapor lewat radio, warga sekitar menonton, lalu aksi `onCaught` bab berjalan (sama seperti Diawasi penuh: dibawa ke pos, denda, Diawasi turun). Setelah itu SPN tenang selama 60 detik.

Model orang dari Tripo: Bayu, keluarganya, NPC cerita, warga, dan SPN memakai model Tripo yang sudah di-rig di `Art/<Nama>/` (lihat `Art/_skrip_rig/BACA.txt`). Manekin UE tetap menjalankan semua animasi (ABP_Unarmed dan semua `MM_GR_*`) tetapi tidak digambar; tubuh Tripo di atasnya menyalin posenya tiap frame lewat retarget runtime (`World/GRBody.*`, `UGRBodyAnimInstance` menjalankan node Retarget Pose From Mesh dengan `RTG_GR_MannyKeTripo`, tanpa IK). Pilihan model: NPC cerita menurut nama di data bab (`GRBody::ForStoryNpc`, misalnya "Pak Darmo" ke `N01_Pak_Darmo`, "Ibu" ke `C02_Ibu_Bayu`, SPN ke `N07_Aparat_patroli`), figuran tanpa nama dan warga dari kumpulan warga (`GRBody::ForTownsperson`). Model yang belum diimpor jatuh kembali ke manekin berwarna. Topi warna penanda NPC dan topi/jilbab tempelan warga disembunyikan karena model Tripo sudah lengkap.

Menambah atau memperbarui karakter: (1) rig di Blender seperti biasa; (2) ekspor dengan `blender -b Art/<Nama>/<Nama>_rig_detail.blend --python Art/_skrip_rig/exp_ue.py -- Art/_ekspor_ue/<Nama> <Nama> 40000 2048` (buang shape key wajah, decimate ke 40 ribu segitiga, tekstur 2K, satuan cm); (3) jalankan `Tools/Python/import_tripo_chars.py` di editor (set `GR_REIMPORT=1` untuk menimpa yang sudah ada); (4) daftarkan nama di `GRBody.cpp`. Bayu memakai `Art/C01_Bayu/C01_Bayu_rig.blend` (Tripo terbaru `c00.glb`, rig dasar, 80 ribu segitiga).

Gerak lebih halus: warga mempercepat dan mengerem perlahan (akselerasi 700, rem lembut), berbelok lebih pelan, dan perpindahan pose dicampur 0,4 detik; saat berlari (dikejar, menghindar) akselerasi dan belok dipercepat. NPC cerita juga mengerem lembut.

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
| `GRMotor` | Naik/turun motor di mana saja (motor didatangkan ke Bayu) |
| `GRTekan W 1.5` | Tahan tombol selama 1,5 detik, untuk tes otomatis (jalan, minigame, menu) |
| `GRBab bab2` | Lompat ke bab tertentu (bab sebelumnya dianggap selesai) |
| `GRCek "syarat"` | Cek syarat cerita, mis. `GRCek "q.Pagi>=2 && uang>10000"` (hasil di log) |
| `GRPos 8740 -9000 0` | Pindahkan pemain ke x, y (cm) menghadap yaw, untuk screenshot otomatis |
| `GRPosisi` | Catat posisi pemain, kecepatan dan kemiringan motor, jarak motor parkir, dan NPC duduk yang bergeser dari kursinya |
| `GRKamera 90 -10 380` | Arahkan kamera dari yaw/pitch relatif terhadap Bayu pada jarak tertentu, untuk screenshot otomatis |
| `GRMobil 1600 90 10` | Taruh mobil terdekat 16 m di depan Bayu, diputar 90 derajat, diam 10 detik (uji tabrakan) |
| `GRLalin` | Catat posisi, kecepatan, dan status semua mobil, serta mobil yang tumpang tindih |
| `GRWarga` | Catat jumlah warga per kegiatan, warga di dekat Bayu, dan alasan titik sekitar ditolak |
| `GRWargaBaris` | Jejerkan warga diam (tidak menghindar) di depan Bayu dan dudukkan dua di bangku terdekat (cek tampilan, uji tabrakan) |
| `GRPolisi` | Munculkan satu patroli SPN berdua dan satu petugas sendiri di dekat Bayu |
| `GRTubuh` | Catat ke log tubuh Tripo di sekitar Bayu: model, material, tinggi kepala dan kaki (cek retarget) |
| `GRGerobak` | Pindahkan Bayu ke samping pedagang gerobak terdekat dan catat animasi serta posisi pegangan tangannya |

## Map

- `L_KotaSentosa`: kota pertama (World Partition, 1 km). Dibuat ulang dengan `Tools/BuildKotaSentosa.bat` (hapus dulu map lama dan folder `__ExternalActors__`/`__ExternalObjects__` miliknya). Lima kawasan: Kampung Gang, Pasar, Kawasan Sekolah, Pusat Pemerintahan, Jalan Raya.
- Bangunan masih kotak sementara (bangunan asli dibuat terpisah). `World/GRFacades` menambah detail saat game jalan dari kotak bangunan di level: pintu dan jendela, ruko dengan rolling door atau etalase terbuka, kanopi warna, papan nama toko fiktif, atap genteng untuk rumah kampung, dan tandon air di atap datar. Semua instanced tanpa collision, jadi aman diganti bangunan asli. Material `M_PrototypeGrid` harus mencentang "Used with Instanced Static Meshes" (sekarang sudah). Interior rumah Bayu (karpet, TV, foto) dan Dapur PPR (kompor besar, meja masak, rak bahan) ada di `Content/Data/furniture.json` dari `Tools/Python/make_furniture.py`.
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
- NPC bergerak tangan dan mengangguk saat baris dialognya tampil, juga saat bark sambil diam. Animasi `MM_GR_Talk`, `MM_GR_Sit`, `MM_GR_SitTalk`, `MM_GR_Marah` (warga marah), `MM_GR_Lirik` (melirik Bayu), `MM_GR_Kaget` (kaget disenggol), `MM_GR_Siaga` (petugas berjaga, tangan di belakang), `MM_GR_Radio` (bicara ke radio bahu), `MM_GR_Tangkap` (petugas memegang), dan `MM_GR_Menyerah` (Bayu angkat tangan) dibuat dari `MM_Idle` oleh `Tools/Python/make_talk_anims.py`. Pedagang mendorong gerobak dengan `MM_GR_Gerobak` dan `MM_GR_GerobakJalan` dari skrip itu juga (dari `MM_Idle` dan `MF_Unarmed_Walk_Fwd`, tangan di-IK ke stang; ubah stang di `AGRWarga::Dress`, ubah juga `PUSH_GRIP`). Skrip yang sama membuat `MM_GR_Ride` (Bayu di motor) dengan IK dua tulang ke titik jok, dek kaki, dan stang model `motor`; ubah model motor, ubah juga konstanta `RIDE_*` di skrip lalu jalankan ulang.
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
    Characters/      hasil Tripo (Bayu, keluarga, NPC), M_GR_Tripo, IK_GR_*, RTG_GR_MannyKeTripo
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
