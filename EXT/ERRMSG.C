/****************************************************************************
 errmsg.c		Chad Bye, Larry Scott, Mark Wilden

 Text error message access

 (c) Sierra On-Line, 1991
*****************************************************************************/

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments
// > DoAlert says it waits for a CLICK, but only checks for keyboard events.
// > SetAlertProc tried to set a functino with a different prototype.
// > Internalized all the error messages. For no good reason. Define INTERNALERRORS to use.

#include "kawa.h"
#include "types.h"
#include "memmgr.h"
#include "dialog.h"
#include "errmsg.h"
#include "string.h"
#include "dos.h"
#include "stdio.h"
#include "start.h"
#include "text.h"
#include "dialog.h"
#include "stdlib.h"

#define	ERRBUFSIZE	400
#define	FILEBUFSIZE	1000

int	theViewNum, theLoopNum, theCelNum; // for debugging

bool (*alertProc)(strptr) = (bool(*)(strptr))DoPanic;

#ifndef INTERNALERRORS
static char errMsgBuf[ERRBUFSIZE];
static char *errMsgFile = "INTERP.ERR";

static char* DoReadErrMsg(int errnum, char *textBuf, int msgfile);
static int OpenErrMsgFile(void);

void InitErrMsgs()
{
	char *tmpBuf;
	int	i, fd;

	if ((fd = OpenErrMsgFile()) == -1)
	{
		sprintf(errMsgBuf, "Can't find %s", errMsgFile);
		DoPanic(errMsgBuf);
	}

	//Pre-load error messages that cannot be loaded later
	tmpBuf = errMsgBuf;
	for(i = 1; i <= E_LASTPRELOAD; i++) //First messages reserved for preloading
	{
		if (!DoReadErrMsg(i, tmpBuf, fd))
			break;
		while (*tmpBuf++) ;
		lseek(fd, 0L, LSEEK_BEG); //Messages not necessarily in order
	}
	close(fd);
}
#else
void InitErrMsgs() { }
#endif

void SetAlertProc(boolfptr func)
{
	alertProc = (bool(*)(strptr))func; //KAWA WAS HERE
}


bool RAlert(int errnum, ...)
{
	va_list	ap;
	char msg[ERRBUFSIZE];
	char buf[ERRBUFSIZE];

	va_start(ap,errnum);
	vsprintf(msg, ReadErrMsg(errnum, buf), ap);
	va_end(ap);

	return alertProc(msg);
}


void Panic(int errnum, ...)
{
	va_list	ap;
	char msg[ERRBUFSIZE];
	char buf[ERRBUFSIZE];

	va_start(ap, errnum);
	vsprintf(msg, ReadErrMsg(errnum, buf), ap);
	va_end(ap);

	sprintf(msg + strlen(msg), ReadErrMsg(E_PANIC_INFO, buf), thisScript, ip);
	DoPanic(msg);
}


global void DoPanic(strptr text)
{
	panicStr = text;
	exit(1);
}


global bool DoAlert(strptr text) //put up alert box and wait for a click
{
	word ret = 0;
	RRect ar;
	REventRecord event;
	Handle savedBits;
	RGrafPort *oldPort;

	RGetPort(&oldPort);
	RSetPort(wmgrPort);
	PenColor(0);
	RTextSize(&ar, text, 0, 0);
	if (ar.bottom > 100) RTextSize(&ar, text, 0, 300);
	CenterRect(&ar);
	RInsetRect(&ar, -MARGIN, -MARGIN);
	savedBits = SaveBits(&ar, VMAP);
	RFillRect(&ar, VMAP, 255);
	RFrameRect(&ar);
	ShowBits(&ar, VMAP);
	RInsetRect(&ar, MARGIN, MARGIN);
	RTextBox(text, 1, &ar, TEJUSTLEFT, 0); //changed from TEJUSTCENTER -PG
	while (1)
	{
		RGetNextEvent(allEvents, &event);
		if (event.type == keyDown){
			if (event.message == 27)
				break;
			if (event.message == 0x0d){
				ret = 1;
				break;
			}
		}
		if (event.type == mouseDown) break; //KAWA WAS HERE to add click support.
		//I mean shit, it SAYS "waits for a CLICK" up there, right?
	}

	//repair the damage
	RestoreBits(savedBits);
	RInsetRect(&ar, -MARGIN, -MARGIN);
	ShowBits(&ar, VMAP);
	RSetPort(oldPort);
	return (ret);
}

#ifdef INTERNALERRORS

//Try to keep these A) in the correct order and B) in purely ASCII so UTF8 and SBCS can use the same source.
const char *InterpErr[] =
{
	//INTASM.S
	"DOS Error:\n%s\nENTER to retry\nESC to %s", //1 E_DISK_ERROR
	"cancel", //2 E_CANCEL (used in E_DISK_ERROR for non-fatal errors)
	"quit", //3 E_QUIT (used in E_DISK_ERROR for fatal errors)

	"Disk is write protected", //4 E_DISK_ERROR_MSG
	"Unknown unit", //5
	"Not ready", //6
	"Invalid command", //"?", //7
	"CRC error", //8
	"Bad request structure length", //"?", //9
	"Seek error", //10
	"Unknown disk format", //"", //11
	"Sector not found", //12
	"Printer out of paper", //"?", //13
	"Write error", //14
	"Read error", //15
	"General failure", //16

	//ALTRES.C
	"Your extra memory has been found to be incompatible with this game.\n\nPlease run INSTALL and choose NOT to use extra memory.\n\n[0x%x %d %d]", //17 E_ARM_CHKSUM

	"", //18
	"", //19
	"", //20
	"", //21
	"", //22
	"", //23
	"", //24 E_LASTPRELOAD -- with interp.err enabled, the preceeding messages are preloaded.
	//Apparently, disk errors are considered more likely.

	//DEBUG.C
#ifndef NOOOPS
	"You did something that we weren't expecting.\nWhatever it was, you don't need to do it to finish the game.\nTry taking a different approach to the situation.\nError %d.  SCI Version %s", //25 E_OOPS_TXT
	"Oops!", //26 E_OOPS
#else
	"", //25
	"", //26
#endif
	"Debug not available! error #%d", //27 E_NO_DEBUG

	//AUDIO.C
	"Can't find audio driver '%s'!", //28 E_NO_AUDIO_DRVR
	"Unable to initialize your audio hardware.", //29 E_NO_AUDIO

	"", //"Unable to locate the cd-language map file.", //30 E_NO_CD_MAP (unused?)
	"", //"Please insert\nDisk %d in drive %s\nand press ENTER.%s", //31 E_INSERT_DISK (unused, SCI11 is single-volume)
	"", //"Unable to locate the language map file.", //32 E_NO_LANG_MAP (unused?)
	"", //"Please insert\nStartup Disk in drive %s\nand press ENTER.%s", //33 E_INSERT_STARTUP (unused)

	//EVENT.C
	"No keyboard driver!", //34 E_NO_KBDDRV

	//RESOURCE.C
	"Out of hunk!", //35 E_NO_HUNK
	"Out of handles!", //36 E_NO_HANDLES

	//SCI.C
	"Couldn't install video driver!", //37 E_NO_VIDEO

	//SOUND.C
	"", //"Couldn't find %s.", //38 E_CANT_FIND (switching this to use E_NO_AUDIO_DRVR, okay?)
	"", //"Couldn't find PatchBank.", //39 E_NO_PATCH (unused?)
	"Unable to initialize your music hardware", //40 E_NO_MUSIC
	"", //"Can't load %s!", //41 E_CANT_LOAD (unused?)

	//VOLLOAD.C
	"Can't find configuration file: %s\nPlease run the Install program.\n", //42 E_NO_INSTALL
	"Resource type mismatch", //43 E_RESRC_MISMATCH
	"%s not found.", //44 E_NOT_FOUND
	"Wrong resource type!\nLooking for $%x #%d", //45 E_WRONG_RESRC
	"", //"Load error: %s\nRETURN continues\nESC quits", //46 E_LOAD_ERROR (unused?)

	//INTRPT.C
	"Maximum number of servers exceeded.\n", //47 E_MAX_SERVE
	"", //"No Debugger Available.", //48 E_NO_DEBUGGER (unused?)

	//CONFIG.C
	"Error: minHunk value is MISSING in your \"%s\" file.\nExample: minHunk = 100k", //49 E_NO_MINHUNK

	//SCI.C
	"You need %U more bytes of free memory available to run this game.\nIf you have any resident software loaded please remove it and try again.", //50 E_NO_MEMORY
	"Out of configuration storage.", //51 E_NO_CONF_STORAGE (unused, uses E_CONFIG_STORAGE instead)
	"", //"Bad picture code", //52 E_BAD_PIC (unused?)

	//RESOURCE.C
	"Out of hunk\nloading %s", //53 E_NO_HUNK_RES

	//DEBUG.C
	"Could not open resource use log file \"%s\"\n", //54 E_NO_RES_USE
	"Could not open hunk use log file \"%s\"\n", //55 E_NO_HUNK_USE

	//ALTRES.C
	"ARM read error", //56 E_ARM_READ
	"ARM handle < 0", //57 E_ARM_HANDLE

	//VOLLOAD.C
	"Can't find a 'where' file.", //58 E_NO_WHERE (used instead of E_NO_INSTALL in debug builds)

	//ERRMSG.C
	"%s View: %d  Loop: %d  Cel: %d\n", //59 E_VIEW

	//PK.C
	"Error on Explode", //60 E_EXPLODE

	//CELS.S
	"Build Line Error ", //61 E_BUILD_LINE
	"Mirror Line Error", //62 E_MIRROR_BUILD
	"Eat Line Error", //63 E_EAT_LINE
	"Invalid Line Length", //64 E_LINE_LENGTH
	"Invalid rectangle!", //65 E_BAD_RECT

	//CONFIG.C
	"\nError: invalid value in your \"%s\" file:\naudioSize = %s\n", //66 E_AUDIOSIZE
	"Error: minHunk was not specified IN KILOBYTES in your \"%s\" file.\nExample: minHunk = 100k\n", //67 E_BAD_MINHUNK

	//EXTMEM.S
	"ExtMem error %d", //68 E_EXT_MEM

	//GETPATH.C
	"Poly Avoider generated too large of path!", //69 E_POLY_AVOID
	"Poly Merge generated too large of path!", //70 E_POLY_MERGE
	"Too many patches!", //71 E_MAX_PATCHES
	"Too many Points!", //72 E_MAX_POINTS
	"Poly Avoider internal error!", //73 E_AVOID

	//GRAPH256.S
	"DrawPic - Not Enough Hunk.", //74 E_DRAWPIC
	"Overran stack in Fill.", //75 E_FILL
	"SaveBits - Not Enough Hunk.", //76 E_SAVEBITS

	//KERNEL.C
	"Night Palette Not Found!", //77 E_NO_NIGHT_PAL (used but badly phrased)
	"Magnification factor out of range", //78 E_MAGNIFY_FACTOR
	"Invalid Magnification Cel Type", //79 E_MAGNIFY_CEL
	"Invalid Source view size or destination port size for magnification factor", //80 E_MAGNIFY_SIZE

	//LANGUAGE.C
	"%d is not a supported language code", //81 E_BAD_LANG

	//MEMMGR.S
	"Out of heap space.", //82 E_NO_HEAP
	"Zero Heap Allocation Request.", //83 E_HEAP_ALLOC
	"Zero Hunk Allocation Request.", //84 E_HUNK_ALLOC
	"Out of hunk space.", //85 E_NO_HUNK_SPACE
	"Returning free hunk!", //86 E_RET_HUNK

	//MENU.C
	"AddMenu not supported.", //87 E_ADDMENU
	"DrawMenuBar not supported.", //88 E_DRAWMENU
	"SetMenu not supported.", //89 E_SETMENU
	"GetMenu not supported.", //90 E_GETMENU
	"MenuSelect not supported.", //91 E_MENUSELECT

	//MESSAGE.C
	"Invalid Message() function: %d", //92 E_MESSAGE

	//MOTION.C
	"bresen failed!", //93 E_BRESEN

	//PALETTE.C
	"Invalid Palette", //94 E_BAD_PALETTE

	//PMACHASM.S
	"Executing in disposed script number %d.", //95 E_DISPOSED_SCRIPT

	//SCALE.C
	"Divide by zero in GetNewScaleFactors", //96 E_SCALEFACTOR

	//STARTASM.S
	"Can't allocate heap.", //97 E_NO_HEAP_MEM

	//TEXT.C
	"Invalid parameter following text code: |%c%c", //98 E_TEXT_PARAM
	"Color index out of range: |c%d|", //99 E_TEXT_COLOR
	"Font index out of range: |f%d|", //100 E_TEXT_FONT
	"Invalid text code: |%c", //101 E_TEXT_CODE

	"", //"Zero size received", //102 E_ZERO_SIZE (unused?)
	"", //"Free pages error (< 0)", //103 E_FREE_PAGES (unused?)
	"", //"Bad handle returned", //104 E_BAD_HANDLE_OUT (unused?)
	"", //"Bad handle", //105 E_BAD_HANDLE (unused?)
	"", //"Too many pages free", //106 E_MAX_PAGES (unused?)

	//WINDOW.C
	"Can't open window.", //107 E_OPEN_WINDOW

	//XMS.S
	"XMS read error: %x\nlen %d src %d dst %d", //108 E_XMM_READ
	"XMS write error: %x\nlen %d src %d dst %d", //109 E_XMM_WRITE

	//ERRMSG.C
	"\nScript #: %d, IP: %x\n", //110 E_PANIC_INFO

	//CONFIG.C
	"Out of configuration storage.", //111 E_CONFIG_STORAGE

	//RESNAME.C
	"Config file error: no '%c' in '%s'", //112 E_CONFIG_ERROR

	//MESSAGE.C
	"Message file has not been updated to version 4.", //113 E_BADMSGVERSION
	"Message stack overflow:  references are nested beyond %d levels", //114 E_MSGSTACKOVERFLOW
	"", //"Detected Sample data in *.SND file.", //115 E_SAMPLE_IN_SND (unused?)
	"Message stack stack overflow:  stacks are stacked beyond %d levels", //116 E_MSGSTACKSTACKOVERFLOW
	"Mismatched (Message MsgPop)", //117 E_MSGSTACKSTACKUNDERFLOW

	//KERNEL.C
	"Not given enough points to make a polygon.", //118 E_BAD_POLYGON

	//DEBUG.C and OBJASM.S
	"Invalid property accessed", //119 E_INVALID_PROPERTY
};

char* ReadErrMsg(int errnum, char *textBuf)
{
	return (char*)InterpErr[errnum - 1];
}

#else

static char* DoReadErrMsg(int errnum, char *textBuf, int msgfile)
{
	int	num;
	char fileBuf[FILEBUFSIZE];
	char *filePtr = fileBuf;
	char *linePtr = fileBuf;
	char *textPtr = textBuf;
	int	count;

	*textPtr = 0;
	count = read(msgfile, fileBuf, FILEBUFSIZE);
	while (count)
	{
		if (!strncmp(linePtr, "\\\\", 2)) {
			//found '\\', so read message #
			linePtr += 2;
			num = 0;
			while ((*linePtr >= '0') && (*linePtr <= '9'))
			{
				num *= 10;
				num += *linePtr++ - '0';
			}

			//found error message
			if (num == errnum)
			{
				//advance to next line
				while (count)
				{
					if (!--count)
						count = read(msgfile, (filePtr = fileBuf), FILEBUFSIZE);
					if (*filePtr++ == '\n') break;
				}
				linePtr = filePtr;

				while (count)
				{
					//if terminating '\\' is found return error message
					if (!strncmp(linePtr, "\\\\", 2))
					{
						//terminate the string
						*(textPtr - 1) = 0;
						return textBuf;
					}

					//advance to next line while copying to textBuf
					while (count)
					{
						if (!--count)
							count = read(msgfile, (filePtr = fileBuf), FILEBUFSIZE);
						*textPtr++ = *filePtr;
						if (*filePtr++ == '\n') break;
					}
					linePtr = filePtr;
				}
			}
		}

		//advance to next line
		while (count)
		{
			if (!--count)
				count = read(msgfile, (filePtr = fileBuf), FILEBUFSIZE);
			if (*filePtr++ == '\n') break;
		}
		linePtr = filePtr;
	}
	return NULL;
}


static int OpenErrMsgFile(void)
{
	int	msgfile;
	char path[100];

	if ((msgfile=open(errMsgFile, 0)) == -1)
	{
		GetExecDir(path);
		strcat(path, errMsgFile);

		//read string #errnum from message file and return pointer to it
		return open(path, 0);
	}
	return msgfile;
}


char* ReadErrMsg(int errnum, char *textBuf)
{
	char *tmpBuf;
	int	j;
	int	fd;

	tmpBuf = errMsgBuf;
	if (errnum <= E_LASTPRELOAD)
	{
		for(j = 1; j < errnum; j++)
			while (*tmpBuf++) ;
		return tmpBuf;
	}
	else
	{
		if ((fd = OpenErrMsgFile()) == -1)
		{
			sprintf(textBuf, "Can't find %s", errMsgFile);
			return textBuf;
		}
		if (!DoReadErrMsg(errnum, textBuf, fd))
			sprintf(textBuf, "Error #%d not found in file %s", errnum, errMsgFile);
		close(fd);
		return textBuf;
	}
}
#endif

global void PanicMsgOutput(int errNum)
{
	char str[80];

	Panic(E_VIEW, ReadErrMsg(errNum, str), theViewNum, theLoopNum, theCelNum);
}

