/* SAVEGAME
 * Save/restore game kernel functions.
 */

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments
// > A few signed/unsigned mismatches, added casts to fix.

#include "savegame.h"
#include "palette.h"
#include "graph.h"
#include "fileio.h"
#include "pmachine.h"
#include "kernel.h"
#include "string.h"
#include "setjmp.h"
#include "list.h"
#include "window.h"
#include "script.h"
#include "resource.h"
#include "animate.h"
#include "sound.h"
#include "midi.h"
#include "memmgr.h"
#include "debug.h"
#include "object.h"
#include "intrpt.h"
#include "restart.h"
#include "start.h"
#include "dos.h"
#include "stdio.h"
#include "scifgets.h"
#include "event.h"
#include "flist.h"
#include "pk.h"
#include "io.h"
#include "savevars.h"
#include "movie.h"
#include "sci.h"

static void near Save(byte far *start, byte far *end);
static void near Restore(byte far *bp);
static uint near inword(void);
static void near outbyte(byte c);
static void near outword(word w);
static void near outstr(strptr str);
static bool near CheckSaveGame(strptr version);
static int near GetSaveFiles(strptr name, strptr names, int *nums);
static bool near PutSaveFiles(strptr name, strptr desc, int num);

char saveDir[65] = "";

static int near fd; //File descriptor for save/restore.
static char near gameName[] = "%s\\%ssg.%03d";
static char near dirName[] = "%s\\%ssg.dir";
static jmp_buf near saveErr;


void MakeFileName(strptr where, strptr name, int num)
{
	sprintf(where, gameName, CleanDir(saveDir), name, num);
}


void MakeDirName(strptr where, strptr name)
{
	sprintf(where, dirName, CleanDir(saveDir), name);
}


KERNEL(SaveGame)
{
	strptr name, desc;
	int num;
	char file[64];
	strptr gameVer;

	name = Native(arg(1));
	num = arg(2);
	desc = Native(arg(3));
	gameVer = Native(arg(4));
	ResetDisk();
	diskIOCritical = false;

	reverbDefault = DoSound(SSetReverb,254);

	//Try to open the file.
	MakeFileName(file, name, num);
	if ((fd = creat(file, 0)) == -1)
	{
		acc = 0;
		diskIOCritical = true;
		return;
	}
	//Bailout point for errors during actual save.
	if (setjmp(saveErr) != 0)
	{
		close(fd);
		acc = 0;
		diskIOCritical = true;
		return;
	}
	//Write the lengths of saved variables and heap.  These will be used
	//to check that the saved games are restorable in a given interpreter.
	outword(&saveEnd - &saveStart);
	outword(handleBase - heapBase);
	//Write out the version string for the game.
	outstr(gameVer);

	//make sure no events get saved that might cause a just-restored game to
	//get re-restored (like an F7 - ENTER combination)
	RFlushEvents(keyDown | keyUp);

	//Zero out the free heap.
	FreeHeap();

	//Write compressed forms of saved variables and the heap out.
	Save(&saveStart, &saveEnd);
	Save(heapBase, handleBase);
	Save((byte far*)&sysPalette.palIntensity[0], (byte far*)&sysPalette.palIntensity[256]);
	if(palVaryOn)
	{
		Save(*targetPalette,(*targetPalette) + sizeof(RPalette));
		Save(*startPalette,(*startPalette) + sizeof(RPalette));
	}

	close(fd);
	PutSaveFiles(name, desc, num);
	diskIOCritical = true;
	acc = 1;
}


KERNEL(RestoreGame)
{
	char file[64];
	ObjID sp;
	LoadLink _far **res;
	LoadLink _far **next;
	int ticks;
	byte lastVideoMode = currentVideoMode;
	word passVideoMode[2];
	static int varyList[] = {1,PALVARYKILL};

	//Turn off PalVary
	if (palVaryOn)
		KPalVary(varyList);

	//Turn off sound processing, so that we don't get clobbered while
	//reading in over the current sound list.
	DoSound(SProcess, false);
	KillAllSounds();

	ResetDisk();
	DisposeLastCast();
	diskIOCritical = false;

	//Open the savegame file and check its validity.  Bail out if not from
	//this version of the game.
	MakeFileName(file, Native(arg(1)), arg(2));
	if ((fd = open(file, 0)) == -1)
	{
		diskIOCritical = true;
		return;
	}
	if (!CheckSaveGame(Native(arg(3))))
	{
		close(fd);
		diskIOCritical = true;
		return;
	}

	//Search the resource list and reset any hunk script resources.
	for (res = (LoadLink far**)Native(FFirstNode(&loadList)); res != NULL; res = next)
	{
		next = (LoadLink far**)Native(FNextNode(Pseudo(res)));
		if ((*res)->type == RES_SCRIPT)
		{
			if ((*res)->altResMem)
				ResUnLoad(RES_SCRIPT, (*res)->id);
			else
			{
				ResetHunk((HunkRes _far*)*((*res)->lData.data));
				(*res)->lock = UNLOCKED;
			}
		}
	}

	//Restore the variables and heap.
	Restore(&saveStart);
	Restore(heapBase);
	Restore((byte far*)&sysPalette.palIntensity[0]);

	//Do some re-initialization.
	ResUnLoad(RES_MEM, ALL_IDS);

	theGame = 0;

	//Regenerate the 'patchable resources' table
	InitPatches();

	for (sp = FirstNode(&scriptList); sp; sp = NextNode(sp))
		ReloadScript((Script*)Native(sp));

	//Turn on PalVary
	if (palVaryOn)
	{
		targetPalette = ResLoad(RES_MEM, sizeof(RPalette));
		startPalette = ResLoad(RES_MEM, sizeof(RPalette));
		newPalette = ResLoad(RES_MEM, sizeof(RPalette));
		Restore(*targetPalette);
		Restore(*startPalette);

		//get memory for calculated palette
		//set up defaults for calculated palette
		PaletteShell((RPalette far*)*newPalette);

		//calculate timmer interrupt
		//assume max of 64 variations
		ticks = (paletteTime*60 + 32)/64;
		//if time == 0 make the palette shift in one call
		//if EGA make the palette shift in one call
		if ((ticks == 0) || (NumberColors == 16))
		{
			paletteDir = paletteStop;
			ticks = 1;
		}
		//start server
		InstallServer(PaletteServer, ticks);
	}

	if(lastVideoMode != currentVideoMode)
	{
		passVideoMode[0] = 1;
		passVideoMode[1] = currentVideoMode;
		currentVideoMode = lastVideoMode; //to make sure it will reset
		KSetVideoMode(passVideoMode);
	}

	close(fd);
	RestoreAllSounds();

	//Set the flag indicating a restore.
	gameRestarted = 2;
	diskIOCritical = true;
	DoSound(SProcess, true);

	//Now restore the stack and restart the PMachine.
	longjmp(restartBuf, 1);
}


KERNEL(GetSaveDir)
{
	acc = Pseudo(saveDir);
}


KERNEL(CheckSaveGame)
{
	char file[64];

	MakeFileName(file, Native(arg(1)), arg(2));
	diskIOCritical = false;
	if ((fd = open(file, 0)) == -1)
		acc = false;
	else
	{
		acc = CheckSaveGame(Native(arg(3)));
		close(fd);
	}
	diskIOCritical = true;
}


static bool near CheckSaveGame(strptr gameVer)
{
	char vStr[20];
	int saveSize, heapSize;

	if (setjmp(saveErr) != 0) return false;
	saveSize = inword();
	heapSize = inword();
	sci_fgets(vStr, 20, fd);
	return (
		(saveSize == (&saveEnd - &saveStart)) &&
		(heapSize == (handleBase - heapBase)) &&
		(strcmp(vStr, gameVer) == 0)
	);
}


//Add on some extra mem to adjust for approximation errors
#define	ERROR 1024

ulong GetSaveLength()
{
	ulong saveGameLen;
	Handle tmpBuf;
	uint memNeeded;

	//Zero out the free heap.
	FreeHeap();

	//Find out how much memory we need.
	memNeeded = &saveEnd - &saveStart;
	//if ((handleBase - heapBase) > (int)memNeeded) //KAWA WAS HERE
	if ((uint)(handleBase - heapBase) > memNeeded)
		memNeeded = handleBase - heapBase;
	//if ((&sysPalette.palIntensity[256] - &sysPalette.palIntensity[0]) > (int)memNeeded) //KAWA WAS HERE
	if ((uint)(&sysPalette.palIntensity[256] - &sysPalette.palIntensity[0]) > memNeeded)
		memNeeded = &sysPalette.palIntensity[256] - &sysPalette.palIntensity[0];
	if (sizeof(RPalette) > memNeeded)
		memNeeded = sizeof(RPalette);

	tmpBuf = GetResHandle(memNeeded);
	saveGameLen = pkImplode2Mem(*tmpBuf, &saveStart, &saveEnd - &saveStart) + sizeof(int);
	saveGameLen += pkImplode2Mem(*tmpBuf, heapBase, handleBase - heapBase) + sizeof(int);
	saveGameLen += pkImplode2Mem(*tmpBuf, (byte far*)&sysPalette.palIntensity[0], &sysPalette.palIntensity[256] - &sysPalette.palIntensity[0]) + sizeof(int);
	if(palVaryOn)
	{
		saveGameLen += pkImplode2Mem(*tmpBuf, *targetPalette, sizeof(RPalette)) + sizeof(int);
		saveGameLen += pkImplode2Mem(*tmpBuf, *startPalette, sizeof(RPalette)) + sizeof(int);
	}
	DisposeHandle(tmpBuf);

	//Return save game length (approx) + error margin
	return (saveGameLen + ERROR);
}


static void near Save(byte far *start, byte far *end)
{
	long offset1, offset2;

	//Find out where we are in file and save a position for length field
	offset1 = lseek(fd, 0L, LSEEK_CUR);
	outword(0);
	pkImplode(fd, start, (uint)(end - start));

	//Get length of data written and store in front of data
	offset2 = lseek(fd, 0L, LSEEK_CUR);
	lseek(fd, offset1, LSEEK_BEG);
	outword((uint)(offset2 - offset1 - sizeof(uint)));
	lseek(fd, offset2, LSEEK_BEG);
}


static void near Restore(byte far *bp)
{
	//File contains 1 word of data length followed by compressed data
	pkExplode(fd, bp, inword());
}


static uint near inword()
{
	uint w;

	if (ReadDos(fd, (byte far *) &w, 2) != 2)
		longjmp(saveErr, 1);
	return w;
}


static void near outbyte(byte c)
{
	if (write(fd, &c, 1) != 1)
		longjmp(saveErr, 1);
}


static void near outword(word w)
{
	if (write(fd, (memptr) &w, 2) != 2)
		longjmp(saveErr, 1);
}


static void near outstr(strptr str)
{
	size_t n = strlen(str);

	if (write(fd, str, strlen(str)) != n)
		longjmp(saveErr, 1);
	outbyte('\n');
}


#define MAXGAMES 20
#define NAMELEN 36


KERNEL(GetSaveFiles)
{
	acc = GetSaveFiles(Native(arg(1)), Native(arg(2)), (word*)Native(arg(3)));
}


//Read the save-game directory, putting file descriptions in the array
//pointed to by 'names', the file numbers in the array pointed to by 'nums'.
static int near GetSaveFiles(strptr name, strptr names, int *nums)
{
	char file[65];
	int n, tempNum, numSaves;

	ResetDisk();
	diskIOCritical = false;
	//Open the directory.  If we can't do so, we'll fill in the arrays with defaults.
	numSaves = 0;
	MakeDirName(file, name);
	fd = open(file, 0);
	if (fd == -1)
	{
		diskIOCritical = true;
		if (criticalError)
			return (-1);
	}
	else
	{
		if (setjmp(saveErr) != 0)
		{
			diskIOCritical = true;
			close(fd);
			return -1;
		}
		//Fill in the arrays passed to us.
		for (n = 0; (tempNum = inword()) != -1; ++n)
		{
			*nums = tempNum;
			sci_fgets(names, NAMELEN, fd);

			//If we're to check for existence of the save-game file and
			//the file is not present, clear its description.

			//RPoint to next slots in the arrays.
			++numSaves;
			++nums;
			names += NAMELEN;
		}
		close(fd);
	}
	*names = 0;
	diskIOCritical = true;
	return numSaves;
}


static bool near PutSaveFiles(strptr name, strptr desc, int num)
{
	char file[65];
	int	n, numSaves;
	char names[MAXGAMES * NAMELEN + 1];
	int	nums[MAXGAMES];

	ResetDisk();
	//Read in the current save directory.
	numSaves = GetSaveFiles(name, names, nums);
	//Open/create the directory.
	MakeDirName(file, name);
	if ((fd = creat(file, 0)) == -1)
		return false;
	//Now re-write the directory with the requested file first.
	outword(num);
	outstr(desc);
	for (n = 0; n < numSaves; ++n)
	{
		if (num != nums[n])
		{
			outword(nums[n]);
			outstr(&names[n * NAMELEN]);
		}
	}
	outword(-1);
	close(fd);
	return true;
}

