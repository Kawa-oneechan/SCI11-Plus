# SCI16, Kawa Edition

The `BASE` folder contains a cleaned-up version of the SCI16 interpreter source, ready to build. It has no extra features, only some warning-suppressing fixes and a lot of style changes. Oh, and no version stamp checks.
The `EXT` folder contains my personal project, which takes the base version and adds a bunch of new features to it.

Included is a minimal copy of a very old MSVC, required to compile all this. To use this in DOSBox, mount `SCI16` as C: and run `GO.BAT`. `cd BASE` or `cd EXT`, then simply run `make` and sit back. If all goes well you should end up with two interpreter files, `SIERRA.EXE` and `SCI.EXE`. On a real MS-DOS machine, it's much the same as long as the directories match up.

You may want to edit `INFO.C` to taste.

The following interpreters are available. Edit `MAKEFILE.MAK` line 26 to pick and choose.

*  `sierra`: no debugger and no menu support.
*  `sci`: debugger, stil no menu support.
*  `sierram`: no debugger, but menu support is in.
*  `scitestr`: both debugger and menu are in.


## Hacks

The `EXT` version of SCI16 adds the following tricks:

*  **B800 text screen on exit** (aka `ENDOOM`). Given a vocab resource #184 (get it?), the interpreter will plonk this into video memory on exit, like various games of yore. This vocab should be your standard 80x25 binary textmode screen, with the cursor locations to put any custom quit messages (`SetQuitStr` kernel call) and where to leave the command prompt afterwards. Also to make it a proper resource you must add two header bytes on top, `86 00`. This feature can be easily toggled out by editing `KAWA.H`.
*  **Internalized error messages**. Instead of having a separate `INTERP.ERR` file with all the error message text, these are all embedded to make for a cleaner directory. This feature can be easily toggled out by editing `KAWA.H`.
*  **Friendlier "Oops!" message**. In a debug build, script errors give actually relatively *helpful* messages. In release builds, all you get is the confusingly-worded "Oops! You did something that we weren't expecting." and an error number. This often made the *player* think they'd messed up when "you" in fact referred to the programmer. We now take a compromise, showing an "Oops!" message that properly refers to the programmer *and* states the actual error. This feature can be easily toggled out by editing `KAWA.H`.
*  **A new kernel call** so as to not piggy-back on others. The `Kawa` kernel call has various subcommands, all stupid and dumb except for one. Its name really depends on vocab 999. Numerically, it should go after `DbugStr`, as seen in `KERNDISP.S`.
    *  `(Kawa 0 {text})`: Show `text` in a `DoAlert` message for extra drama.
    *  `(Kawa 1 from to)`: Invert the specified range of the color palette. *Za warudo!*
    *  `(Kawa 2 back text)`: Sets the title bar colors for unskinned windows.
    *  `(Kawa 3)`: Returns 1 if this is a debug build, 2 if it has menu support, 3 if both.
*  **`FileIO` extensions**. Several but not all SCI32 `FileIO` subcommands have been added for convenience.
    *  `(FileIO 13 fd)`: Reads and returns a single byte from `fd`.
    *  `(FileIO 14 fd byte)`: Writes a single byte to `fd`.
    *  `(FileIO 15 fd)`: Reads and returns a two-byte word from `fd`.
    *  `(FileIO 16 fd word)`: Writes a word to `fd`.
*  **Color hacks**. Set a `View` object's `scaleSignal` to activate. If the top byte is `$01`, any non-transparent non-remap pixels will become black. If it's `$02`, they'll be remapped, and if it's set to `$03`, they'll be set *to* the first remap color. In other words, if you set up a remap to make color 253 darken by 25%, this color hack will either darken the entire view by 25%, or turn the view into one big translucent shadow.
*  **Button and edit controls respond to the `back` and `color` properties**. Draw any button in any color without going full custom.
*  **Outlined text rendering**. The `Display` kernel command now has a `#stroke`/`dsSTROKE` parameter that takes a bitfield specifying which of eight parts of an outline to draw around the text.
*  **Ensuring you're trying to run an SCI11 game** by sniffing the format of the `RESOURCE.MAP` file. This feature can be easily toggled out by editing `KAWA.H`.
*  **DbugStr outputs to file** instead of a secondary mono monitor.
*  **LDM/STM opcodes** as needed by the hottest new shit in SCI Companion.

![Demonstration of color hacks.](http://helmet.kafuka.org/sci/.images/colorhaxdemo1.gif)
![Demonstration of stroked text.](http://helmet.kafuka.org/sci/.images/dsstroke.png)
