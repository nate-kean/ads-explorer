# ADS Explorer

This is a shell namespace extension that lets you browse the Alternate Data
Streams of a file or folder. It's inspired by Dave Plummer's ZipFolders
and how he did the same for .zip archives. Imbues Windows Explorer with the
ability to see and manage the Alternate Data Streams attached to all NTFS file
objects that are otherwise invisible by usual means.

If I have seen further, it is by standing on the shoulders of giants. This
app would not have been possible without the work of better programmers than
me, including [Calvin Buckley](https://github.com/NattyNarwhal/OpenWindows),
[Pascal Hurni](https://www.codeproject.com/Articles/7973/An-almost-complete-Namespace-Extension-Sample),
[Dave Plummer](https://www.youtube.com/watch?v=aQUtUQ_L8Yk), and
[Michael Dunn](https://www.codeproject.com/Articles/1649/The-Complete-Idiot-s-Guide-to-Writing-Namespace-Ex).

## Building
This is a Visual Studio 2022 project made for MSVC v143. It was developed and
tested on Windows 10.

## Installation
Copy the DLL and TLB to the same folder somewhere and run
`regsvr32 ADSExplorer.dll`.

To uninstall, run `regsvr32 /u ADSExplorer.dll`.

This is going to worm its way into anything with a shell view, so it might
cause stability issues. _Caveat emptor._

## Known issues
* An installer would be nice.
* Modern property handling is very, very basic and intended just to appease
  the tile view subtitles in Vista+. It should probably use fancier behavior,
  but as it is now it delegates as much as it can except the bare minimum to
  the "classic" way of doing it.
* There's probably a better way to do a lot of this, but the sheer obscurity
  of the topic led to some Cargo Cult programming.

## Attributions
See `LICENSE`.

This is based off [OpenWindows by Calvin Buckley](https://github.com/NattyNarwhal/OpenWindows),
which in turn is based off a [DOpus-style favorites manager by Pascal Hurni](https://www.codeproject.com/Articles/7973/An-almost-complete-Namespace-Extension-Sample).
The MIT License was inherited from Buckley who made it so with Hurni's
blessing.

Buckley used some Microsoft code (that is, code from outside the Win32 API
itself) as a polyfill for full functionality when built with older SDKs, and as
the Cargo Cult goes, this may still be the case but I don't actually know.

I looked up SO many magic numbers on [magnumdb.com](https://www.magnumdb.com/).
