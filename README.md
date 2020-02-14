# SCI11+ -- SCI With Bits On

The `BASE` folder contains a cleaned-up version of the SCI11 interpreter source, ready to build. It has no extra features, only some warning-suppressing fixes and a lot of style changes. Oh, and no version stamp checks.
The `EXT` folder contains my personal project, which takes the base version and adds a bunch of new features to it.

Included is a minimal copy of a very old MSVC, required to compile all this. To use this in DOSBox, mount `SCI11` as C: and run `GO.BAT`. `cd BASE` or `cd EXT`, then simply run `make` and sit back. If all goes well you should end up with a `SIERRA.EXE` interpreter file. On a real MS-DOS machine, it's much the same as long as the directories match up.

You may want to edit `INFO.C` to taste.

The following interpreters are available. Invoke `make <target>` with one of the following:

*  `sierra`: no debugger and no menu support.
*  `sci`: debugger, still no menu support.
*  `sierram`: no debugger, but menu support is in.
*  `scitestr`: both debugger and menu are in.
*  `clean`: don't build but remove all objects, map files, and binaries.

If no target is given, `sierra` is the default.

## Hacks

The `EXT` version of SCI11 adds the following tricks:

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
*  **DbugStr outputs to file** instead of a secondary mono monitor. **New:** You can specify which file in `RESOURCE.CFG` via the `log` setting. Also I forgot to mention it takes format strings.
*  **LDM/STM opcodes** as needed by the hottest new shit in SCI Companion.
*  **UTF-8 support**. Font files have 16-bit character counts, and using UTF-8 encoding is the most backwards-compatible way to reach them all. Includes kernel calls equivalent to `mbstowcs`/`wcstombs`. This feature can be easily toggled out by editing `KAWA.H`.
*  **Colorful menus**. Menus, if enabled, are drawn in whatever colors were last used by `DrawStatus`. This feature can be easily toggled out by editing `KAWA.H`.
*  **Full SBCS case mapping**. If you don't enable UTF-8 support, you can specify a casemap file to use, with your choice of DOS-437, Win-1252, or ISO-8859-1.
*  **SCI32 font code rules**. In SCI11, the `|f..|` and `|c..|` control codes you can use in text strings for display use lookup tables set up with the `TextFonts` and `TextColors` kernel calls, so `|f2|` uses the third entry of the font list. In SCI2, they set the font and color directly. In SCI11+, you can use *both*. `|f2|` uses the third entry of the font list, but `|F2|` uses font #2.

![Demonstration of color hacks.](http://helmet.kafuka.org/sci/.images/colorhaxdemo1.gif)
![Demonstration of stroked text.](http://helmet.kafuka.org/sci/.images/dsstroke.png)
![Demonstration of colorful menus.](http://helmet.kafuka.org/sci/.images/colormenus.png)
