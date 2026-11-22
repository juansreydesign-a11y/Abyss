; Optional Inno Setup installer template.
; Build the VST3 first, then adjust Source to the actual build output.
[Setup]
AppName=ABYSS
AppVersion=1.0.0
DefaultDirName={commoncf}\VST3
DefaultGroupName=ABYSS
OutputDir=.
OutputBaseFilename=AbyssVocal-Setup
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
Uninstallable=yes

[Files]
Source: "..\build\AbyssVocal_artefacts\Release\VST3\ABYSS.vst3\*"; DestDir: "{app}\ABYSS.vst3"; Flags: recursesubdirs createallsubdirs
