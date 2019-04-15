// kernel.h
// defines standard parameter macros and declares kernel routines

//KAWA WAS HERE
//-------------
// > General cleanup
// > Standardized kernel call arguments and definitions -- see TYPES.H

#ifndef KERNEL_H
#define KERNEL_H

#ifndef LIST_H
	#include "list.h"
#endif
#ifndef OBJECT_H
	#include "object.h"
#endif

//KAWA WAS HERE
#define arg(n)		*(args + (n))
#define argCount	arg(0)

//pseudo selectors for SetMenu and Display
//must be duplicated by (define)'s in SYSTEM.SH
#define p_at		100
#define p_mode		101
#define p_color		102
#define p_back		103
#define p_style		104
#define p_font		105
#define p_width		106
#define p_save		107
#define p_restore	108
#define p_said		109
#define p_text		110
#define p_key		111
#define p_state		112
#define p_value		113
#define p_dispose	114
#define p_time		115
#define p_title		116
#define p_draw		117
#define p_edit		118
#define p_button	119
#define p_icon		120
#define p_noshow	121
#define p_stroke	122

//Function codes for memory segment operations
#define SAVE_FROM	0
#define RESTORE_TO	1
#define STRING		0

//Function codes for memory allocation operations
#define MNeedPtr	1
#define MNewPtr		2
#define MDisposePtr	3
#define MCopy		4
#define MReadWord	5
#define MWriteWord	6

//Function code for PriCoord operation
#define PTopOfBand	1

//Function codes for KPalVary
#define PALVARYSTART	0
#define PALVARYREVERSE	1
#define PALVARYINFO		2
#define PALVARYKILL		3
#define PALVARYTARGET	4
#define PALVARYNEWTIME	5
#define PALVARYPAUSE	6
#define DOSETCLUT		1
#define NOSETCLUT		0

//Function codes for KRemapColors
#define REMAPOFF			0
#define REMAPPERCENT		1
#define REMAPRANGE			2
#define REMAPGRAY			3
#define REMAPPERCENTGRAY	4

//sortNode used in Sort
typedef struct
{
	Obj *sortObject;
	int sortKey;
} SortNode;

typedef struct
{
	Node node;
	word nVal;
} KNode;

void bsort(SortNode*, int);

KERNEL(AssertPalette);
KERNEL(ResCheck);
KERNEL(Load);
KERNEL(UnLoad);
KERNEL(DrawPic);
KERNEL(PicNotValid);
KERNEL(Show);
KERNEL(Animate);
KERNEL(NewWindow);
KERNEL(DisposeWindow);
KERNEL(DrawControl);
KERNEL(EditControl);
KERNEL(HiliteControl);
KERNEL(SetCursor);
KERNEL(ShakeScreen);
KERNEL(ShiftScreen);
KERNEL(GlobalToLocal);
KERNEL(LocalToGlobal);
KERNEL(GetAngle);
KERNEL(GetDistance);
KERNEL(SinMult);
KERNEL(CosMult);
KERNEL(SinDiv);
KERNEL(CosDiv);
KERNEL(ATan);
KERNEL(GetEvent);
KERNEL(StrCase); //KERNEL(Parse);
KERNEL(StrTrim); //KERNEL(Said);
KERNEL(CantBeHere);
KERNEL(GetTime);
KERNEL(TextSize);
KERNEL(OnControl);
KERNEL(AddToPic);
KERNEL(DrawCel);
KERNEL(NumCels);
KERNEL(NumLoops);
KERNEL(CelWide);
KERNEL(CelHigh);
KERNEL(GetPort);
KERNEL(SetPort);
KERNEL(SetNowSeen);
KERNEL(IsItSkip);
KERNEL(Display);
KERNEL(Random);
KERNEL(NewList);
KERNEL(DisposeList);
KERNEL(NewNode);
KERNEL(FirstNode);
KERNEL(LastNode);
KERNEL(EmptyList);
KERNEL(NextNode);
KERNEL(PrevNode);
KERNEL(IsFirstNode);
KERNEL(IsLastNode);
KERNEL(NodeValue);
KERNEL(AddAfter);
KERNEL(AddToFront);
KERNEL(AddToEnd);
KERNEL(FindKey);
KERNEL(DeleteKey);
KERNEL(Abs);
KERNEL(Sqrt);
KERNEL(ReadNumber);
KERNEL(MemoryInfo);
KERNEL(PalVary);
KERNEL(ScriptID);
KERNEL(DUMMY);
KERNEL(IsObject);
KERNEL(RespondsTo);
KERNEL(Clone);
KERNEL(DisposeClone);
KERNEL(DisposeScript);
KERNEL(Wait);
KERNEL(Format);
KERNEL(StrCmp);
KERNEL(StrLen);
KERNEL(StrCpy);
KERNEL(StrEnd);
KERNEL(StrCat);
KERNEL(Dummy); //KShowObjs;
KERNEL(HaveMouse);
KERNEL(GetCWD);
KERNEL(CoordPri);
KERNEL(StrAt);
KERNEL(FlushResources);
KERNEL(MemorySegment);
KERNEL(Memory);
KERNEL(MergePoly);
KERNEL(AvoidPath);
KERNEL(ListOps);
KERNEL(FileIO);
KERNEL(Sort);
KERNEL(Lock);
KERNEL(ShowMovie);
KERNEL(SetVideoMode);
KERNEL(SetQuitStr);
KERNEL(DbugStr);
KERNEL(DisposeScript);
KERNEL(RemapColors);
KERNEL(Kawa);
KERNEL(Utf8to16);
KERNEL(Utf16to8);

extern int noShowBits;

#endif

