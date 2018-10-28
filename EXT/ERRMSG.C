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

#if !defined(INTERNALERRORS)
static char errMsgBuf[ERRBUFSIZE];
static char* errMsgFile = "INTERP.ERR";

static char* DoReadErrMsg(int errnum, char* textBuf, int msgfile);
static int OpenErrMsgFile(void);

void InitErrMsgs()
{
	char* tmpBuf;
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

#if defined(INTERNALERRORS)

const char* KawaErrors1 =
	"DOS Error:\n%s\nENTER to retry\nESC to %s\0"
	"cancel\0"
	"quit\0"
	"Disk is write protected\0"
	"Unknown unit\0"
	"Not ready\0"
	"?\0"
	"CRC error\0"
	"?\0"
	"Seek error\0"
	"Stampver error\0"
	"Sector not found\0"
	"?\0"
	"Write error\0"
	"Read error\0"
	"General failure\0"
	"Your extra memory has been found to be incompatible with this game.\n\nPlease run INSTALL and choose NOT to use extra memory.\n\n[0x%x %d %d]\0"
	"\0" //18
	"\0" //19
	"\0" //20
	"\0" //21
	"\0" //22
	"\0" //23
	"\0" //24
	"You did something that we weren't expecting.\nWhatever it was, you don't need to do it to finish the game.\nTry taking a different approach to the situation.\nError %d.  SCI Version %s\0"
	"Oops!\0"
	" Debug not available! error #%d\0"
	"Can't find audio driver '%s'!\0"
	"Unable to initialize your audio hardware.\0"
	"Unable to locate the cd-language map file.\0"
	"Please insert\nDisk %d in drive %s\nand press ENTER.%s\0"
	"Unable to locate the language map file.\0"
	"Please insert\nStartup Disk in drive %s\nand press ENTER.%s\0"
	"No keyboard driver!\0"
	"Out of hunk!\0"
	"Out of handles!\0"
	"Couldn't install video driver!\0"
	"Couldn't find %s.\0"
	"Couldn't find PatchBank.\0"
	"Unable to initialize your music hardware\0"
	"Can't load %s!\0"
	"Can't find configuration file: %s\nPlease run the Install program.\n\0"
	"Resource type mismatch\0"
	"%s not found.\0"
	"Wrong resource type!\nLooking for $%x #%d\0"
	"Load error: %s\nRETURN continues\nESC quits\0"
	"Maximum number of servers exceeded.\n\0"
	"No Dubugger Available.\0"
	"Error: minHunk value is MISSING in your \"%s\" file.\nExample: minHunk = 100k\0"
	"You need %U more bytes of free memory available to run this game.\nIf you have any resident software loaded please remove it and try again.\0"
	"Out of configuration storage.\0"
	"Bad picture code\0"
	"Out of hunk\nloading %s\0"
	"could not open resource use log file \"%s\"\n\0"
	"could not open hunk use log file \"%s\"\n\0"
	"ARM read error\0"
	"ARM handle < 0\0"
	"Can't find a 'where' file.\0"
	"%s View: %d  Loop: %d  Cel: %d\n\0"
	"Error on Explode\0"
	"Build Line Error \0"
	"Mirror Line Error\0"
	"Eat Line Error\0"
	"Invalid Line Length\0"
;
const char* KawaErrors2 =
	"INVALID RECTANGLE!\0"
	"\nError: invalid value in your \"%s\" file:\naudioSize = %s\n\0"
	"Error: minHunk was not specified IN KILOBYTES in your \"%s\" file.\nExample: minHunk = 100k\n\0"
	"ExtMem error %d\0"
	"Poly Avoider generated too large of path!\0"
	"Poly Merge generated too large of path!\0"
	"Too many patches!\0"
	"Too many Points!\0"
	"Poly Avoider internal error!\0"
	"DrawPic - Not Enough Hunk.\0"
	"Overran stack in Fill.\0"
	"SaveBits - Not Enough Hunk.\0"
	"Night Palette Not Found!\0"
	"Magnification factor out of range\0"
	"Invalid Magnification Cel Type\0"
	"Invalid Source view size or destination port size for magnification factor\0"
	"%d is not a supported language code\0"
	"Out of heap space.\0"
	"Zero Heap Allocation Request.\0"
	"Zero Hunk Allocation Request.\0"
	"Out of hunk space.\0"
	"Returning free hunk!\0"
	"AddMenu not supported.\0"
	"DrawMenuBar not supported.\0"
	"SetMenu not supported.\0"
	"GetMenu not supported.\0"
	"MenuSelect not supported.\0"
	"Invalid Message() function: %d\0"
	"bresen failed!\0"
	"Invalid Palette\0"
	"Executing in disposed script number %d.\0"
	"Divide by zero in GetNewScaleFactors\0"
	"Can't allocate heap.\0"
	"Invalid parameter following text code: |%c%c\0"
	"Color index out of range: |c%d|\0"
	"Font index out of range: |f%d|\0"
	"Invalid text code: |%c\0"
	"Zero size received\0"
	"Free pages error (< 0)\0"
	"Bad handle returned\0"
	"Bad handle\0"
	"Too many pages free\0"
	"Can't open window.\0"
	"XMS read error: %x\nlen %d src %d dst %d\0"
	"XMS write error: %x\nlen %d src %d dst %d\0"
	"\nScript #: %d, IP: %x\n\0"
	"Out of configuration storage.\0"
	"Config file error: no '%c' in '%s'\0"
	"Message file has not been updated to version 4.\0"
	"Message stack overflow:  references are nested beyond %d levels\0"
	"Detected Sample data in *.SND file.\0"
	"Message stack stack overflow:  stacks are stacked beyond %d levels\0"
	"Mismatched (Message MsgPop)\0"
	"Not given enough points to make a polygon.\0"
	"Invalid property accessed\0"
	"\nStampver error: %U-%U\0"
	"Suspend.\0"
	"Pause.\0"
	"Play.\n\0"
	"\0"
	"\0"
	"\0"
	"\0"
;

char* ReadErrMsg(int errnum, char* textBuf)
{
	char* tmpBuf;
	int	j;
	tmpBuf = (char*)KawaErrors1;
	if (errnum > 64)
	{
		tmpBuf = (char*)KawaErrors2;
		errnum -= 64;
	}
	for(j = 1; j < errnum; j++)
		while (*tmpBuf++);
	return(tmpBuf);
}

#else

static char* DoReadErrMsg(int errnum, char* textBuf, int msgfile)
{
	int	num;
	char fileBuf[FILEBUFSIZE];
	char* filePtr = fileBuf;
	char* linePtr = fileBuf;
	char* textPtr = textBuf;
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
						return(textBuf);
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
	return(NULL);
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
		return(open(path, 0));
	}
	return(msgfile);
}


char* ReadErrMsg(int errnum, char* textBuf)
{
	char* tmpBuf;
	int	j;
	int	fd;

	tmpBuf = errMsgBuf;
	if (errnum <= E_LASTPRELOAD)
	{
		for(j = 1; j < errnum; j++)
			while (*tmpBuf++) ;
		return(tmpBuf);
	}
	else
	{
		if ((fd = OpenErrMsgFile()) == -1)
		{
			sprintf(textBuf, "Can't find %s", errMsgFile);
			return(textBuf);
		}
		if (!DoReadErrMsg(errnum, textBuf, fd))
			sprintf(textBuf, "Error #%d not found in file %s", errnum, errMsgFile);
		close(fd);
		return(textBuf);
	}
}
#endif

global void PanicMsgOutput(int errNum)
{
	char str[80];

	Panic(E_VIEW, ReadErrMsg(errNum, str), theViewNum, theLoopNum, theCelNum);
}

