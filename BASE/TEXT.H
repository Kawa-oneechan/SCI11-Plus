//KAWA WAS HERE
//-------------
// > General cleanup

#ifndef	TEXT_H
#define	TEXT_H

#include "types.h"		//for typedef strptr
#include "grtypes.h"	//for typedef RRect

//text justification
#define TEJUSTLEFT		0
#define TEJUSTCENTER	1
#define TEJUSTRIGHT		-1

//Defines for text strings with special control codes (fonts, colors, etc.)
#define	MAXTEXTFONTS	100
#define	MAXTEXTCOLORS	100
#define	CTRL_CHAR		'|'

extern byte textColors[];
extern uint textFonts[];
extern byte lastTextColor;
extern byte lastTextFont;
extern word *newRect;
extern word rectIndex;

extern KERNEL(TextColors);
extern KERNEL(TextFonts);
extern void RTextSize(RRect*, strptr, word, word);
extern int GetLongest(strptr*, int, int);
extern int GetHighest(strptr, int, int);
extern word*RTextBox(strptr, int, RRect*, word, word);
extern int RStringWidth(strptr);
extern void DrawString(strptr);
extern void ShowString(strptr);
extern void RDrawText(strptr, int, int, int, int);
extern void ShowText(strptr, int, int, int, int);

#endif

