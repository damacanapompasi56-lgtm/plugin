[Setup]
AppName=MADENCITUNE
AppVersion=1.0.0
AppPublisher=Madenci Audio
DefaultDirName={autopf}\MADENCITUNE
DefaultGroupName=MADENCITUNE
OutputDir=..\dist
OutputBaseFilename=MADENCITUNE_Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
PrivilegesRequired=admin
DisableProgramGroupPage=yes
WizardStyle=modern

[Files]
; VST3 plugin -> ortak VST3 klasoru (tum DAW'lar burayi tarar)
Source: "..\build\MadenciTune_artefacts\Release\VST3\MADENCITUNE.vst3\*"; DestDir: "{commoncf64}\VST3\MADENCITUNE.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone (DAW'siz test icin)
Source: "..\build\MadenciTune_artefacts\Release\Standalone\MADENCITUNE.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\MADENCITUNE (Standalone)"; Filename: "{app}\MADENCITUNE.exe"
Name: "{autoprograms}\MADENCITUNE kaldir"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\MADENCITUNE.exe"; Description: "MADENCITUNE'u simdi calistir"; Flags: nowait postinstall skipifsilent unchecked
