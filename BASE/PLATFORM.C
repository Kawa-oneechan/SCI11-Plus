//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments

#include "kernel.h"
#include "platform.h"
#include "pmachine.h"

typedef enum
{
	Macintosh = 0, //KAWA WAS HERE -- has subops in Mac terps, according to ScummVM?
	Dos,
	DosWindows,
	Amiga,
	WhatAmI, //KAWA WAS HERE -- returns 1 (DOS), might return 2 (Windows) according to SCI Companion
/*	//KAWA WAS HERE -- According to ScummVM:
	IsNotHiRes,
	IsHiRes,
	IsWindows */
} PlatformType;

/*
typedef enum
{
	Windows = 0,
	EGA
	CD
} PlatformCall;
*/

global KERNEL(Platform)
{
	acc = 0;

	if ( argCount < 2 )
		return;

	switch ((PlatformType)arg(1))
	{
		case Dos:
/*
			switch ((PlatformCall)arg(2))
			{
				case Windows:
					break;
				case EGA:
					break;
				case CD:
					break;
			}
			break;
*/
		case Macintosh:
		case DosWindows:
		case Amiga:
			break;
		//Return the platform type that we are
		case WhatAmI:
			acc = (word)Dos;
			break;
/*
		case IsNotHiRes:
			acc = 1;
			break;
		case IsHiRes
		case IsWindows:
			break;
*/
	   }
}
