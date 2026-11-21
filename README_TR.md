# MADENCITUNE

Mor arayüzlü, kazma knoblu auto-tune (VST3 + Standalone).

## Exe kurulum dosyasını almak (en kolay: GitHub)
1. github.com'da boş bir repo aç, bu klasörün içindekileri yükle (.github klasörü dahil).
2. Repo > Actions > "Build MADENCITUNE" > Run workflow (push'ta kendiliğinden de çalışır).
3. Bitince sayfanın altındaki Artifacts'tan **MADENCITUNE_Setup** indir > zip içinden `MADENCITUNE_Setup.exe`.
4. Çalıştır: `C:\Program Files\Common Files\VST3\MADENCITUNE.vst3` içine kurar. DAW'da plugin taramasını yenile.

## Kendi bilgisayarında derlemek
Visual Studio 2022 (C++ ile masaüstü), CMake, Git ve Inno Setup 6 kur, sonra `build_windows.bat` çalıştır.
Çıktı: `dist\MADENCITUNE_Setup.exe`

## Kontroller
- SPEED: 400 ms -> 0 -> -10. Negatife indikçe tuning sertleşir (-10 = en sert, robot).
- FORMANT: -12..+12. -12'de "MADENCI MOD", sola doğru "madencileştirme", sağa doğru "teknoşlaştırma".
- VIBRATO: 0-200 %. Note Transition, Tolerance, Correction, Range, Scale/Key, REF Hz, Mix, Output.
- Klavye: nota aç/kapat. MIDI notası basılı tutulursa sadece o notalara tuning yapar.
