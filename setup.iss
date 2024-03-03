[Setup]
AppName=ADS Explorer
AppCopyright=Copyright (c) 2024 Nate Kean
AppPublisher=Nate Kean
AppPublisherURL=https://github.com/garlic-os/ads-explorer
AppVersion=1.0.0.0
DefaultDirName={pf}\ADS Explorer
DisableProgramGroupPage=yes
DisableReadyMemo=yes
DisableReadyPage=yes
; if we decide to support itanium, check this
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "ADSExplorer-vc6-x86.dll"; DestDir: "{app}"; DestName: "ADSExplorer.dll"; Flags: regserver 32bit; Check: not Is64BitInstallMode
; does this need regtypelib flag?
Source: "ADSExplorer-vc6-x86.tlb"; DestDir: "{app}"; DestName: "ADSExplorer.tlb"; Flags: 32bit; Check: not Is64BitInstallMode
Source: "ADSExplorer-vc2010-amd64.dll"; DestDir: "{app}"; DestName: "ADSExplorer.dll"; Flags: regserver 64bit; Check: Is64BitInstallMode
Source: "ADSExplorer-vc2010-amd64.tlb"; DestDir: "{app}"; DestName: "ADSExplorer.tlb"; Flags: 64bit; Check: Is64BitInstallMode

