# ADS Explorer

This is a shell namespace extension that lets you browse the
[Alternate Data Streams](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/e2b19412-a925-4360-b009-86e3b8a020c8)
of a file or folder within native Windows Explorer.
[Windows crucially lacks an easy way to do this.](https://vox.veritas.com/kb/articles-backup-and-recovery/what-you-need-to-know-about-alternate-data-streams-in-windows-is-your-data-secur/807740)
This extension is inspired by Dave Plummer's ZipFolders and how he did the same
for ZIP files a few decades ago.
Its function is to imbue Windows Explorer with the ability to see and manage the
Alternate Data Streams attached to all NTFS file objects that are otherwise
invisible by usual means.

If I have seen further, it is by standing on the shoulders of giants. This app
would not have been possible without the work of better programmers than me,
including
[Calvin Buckley](https://cmpct.info/~calvin/Articles/COMAbyss/),
[Pascal Hurni](https://www.codeproject.com/Articles/7973/An-almost-complete-Namespace-Extension-Sample),
[Dave Plummer](https://www.youtube.com/watch?v=aQUtUQ_L8Yk),
and
[Michael Dunn](https://www.codeproject.com/Articles/1649/The-Complete-Idiot-s-Guide-to-Writing-Namespace-Ex).  
This project is most directly a fork of Buckley's [OpenWindows](https://github.com/NattyNarwhal/OpenWindows)
namespace extension.

## Building
This is a Visual Studio 2022 C++ ATL project made for MSVC v143.
It was developed and tested on Windows 10.  
In your Visual Studio Installer it requires:
- MSVC
- C++ ATL
- C++ MFC
- A Windows SDK
- The C++ 2022 redistributable update too I think I'm not sure

The PowerShell scripts for vscode users also use `sudo`:
- [Official `sudo` for Windows 11](https://learn.microsoft.com/en-us/windows/sudo/)
- [Unofficial `sudo` for Windows 10](https://gerardog.github.io/gsudo/)

## Installation
Copy the DLL and TLB to the same folder somewhere and run
`regsvr32 ADSExplorer.dll`.

To uninstall, run `regsvr32 /u ADSExplorer.dll`.

This touches everything with a shell view (e.g., Window Explorer and file
picker dialogs), so it might cause stability issues. _Caveat emptor._

## Known issues
* An installer would be nice.
* Modern property handling is very, very basic and intended just to appease the
  tile view subtitles in Vista+. It should probably use fancier behavior,
  but as it is now it delegates as much as it can except the bare minimum to
  the "classic" way of doing it.
* Not feature complete.
* It would be nice for this to be backwards compatible with the oldest versions
  of Windows possible.

## Attributions
See [`LICENSE`](LICENSE).

This is based off [OpenWindows by Calvin Buckley](https://github.com/NattyNarwhal/OpenWindows),
which is based off a [DOpus-style favorites manager by Pascal Hurni](https://www.codeproject.com/Articles/7973/An-almost-complete-Namespace-Extension-Sample),
which in turn is based off [a namespace shell extension example by Michael Dunn](https://www.codeproject.com/Articles/1649/The-Complete-Idiot-s-Guide-to-Writing-Namespace-Ex).
The MIT License was inherited from Buckley who made it so with Hurni's blessing.

Buckley used some Microsoft code (that is, code from outside the Win32 API
itself) as a polyfill for full functionality when built with older SDKs, and as
the Cargo Cult goes, this may still be the case but I don't actually know.

I looked up SO many magic numbers on [magnumdb.com](https://www.magnumdb.com/).
