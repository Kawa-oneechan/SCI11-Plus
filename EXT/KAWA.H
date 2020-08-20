/* KAWA's HACKS -- A REFERENCE GUIDE
 * ---------------------------------
 * ENDOOM
 *   When exiting the game, if there's a 184.VOC with
 *   an 80x25 textmode screen and some coordinates,
 *   show that.
 * NOOOPS
 *   Use better "Oops!" errors that aren't as confusing
 *   to see, and more informative.
 * ENSURESCI11
 *   Run some checks on the RESOURCE.MAP file to see
 *   if it's truly an SCI11 map before doing anything
 *   else.
 * MSG_FIXHEXDIGITS
 *   There's a bug where Message lines with \## escape
 *   codes in them don't decode right -- the decoder
 *   uses "01234567890ABCDEF", so \0A is misread as
 *   character #11 instead of the correct #10.
 * MSG_ALLOWDIGITSINDIRECTIONS
 *   Messages can contain (STAGE DIRECTIONS). Any
 *   parenthetical with lowercase characters or digits
 *   is reconsidered not a stage direction at all.
 *   SCI2 allows digits.
 * UTF8
 *   Allow all 65536 characters in a font file to be
 *   used via UTF-8 encoding.
 * DBUGSTRFILE
 *   Instead of writing to a mono text screen, have
 *   DbugStr write to a text file for later checking,
 *   or for another program to read live. Also makes
 *   DbugStr work with format strings.
 * COLORFULMENU
 *   Menus, if enabled, are drawn in whatever colors
 *   were last used by DrawStatus.
 * ALTCUSTOMWINDOWLOGIC
 *   When set, windows won't draw anything at all if
 *   bit 0x80 (wCustom) is set. Otherwise, only the
 *   exact value 0x80 works. Some Sierra games
 *   mistakenly use 0x81.
 *
 * ERRLANG <file>
 *   Instead of reading error messages from INTERP.ERR,
 *   have them in the program code itself. This makes
 *   the program bigger, but cleans up the game folder.
 * CASEMAP <file>
 *   Use another casemap list instead of the built-in
 *   DOS-437. Only used if UTF8 is *not* defined.
 */

#define ENDOOM
#define NOOOPS
#define ENSURESCI11
#define MSG_FIXHEXDIGITS
#define MSG_ALLOWDIGITSINDIRECTIONS
#define UTF8
#define DBUGSTRFILE
#define COLORFULMENU

#define ERRLANG "ERRMSG_E.H"
#define CASEMAP "CM_1252.H"
