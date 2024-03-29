/*
 * VOLLOAD - Code to load volume based resources
 * Work started 2/25/88 by Robert E. Heitman for Sierra On-Line, Inc.
 */

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments

#include "volload.h"
#include "audio.h"
#include "sync.h"

//SCI-specific headers
#include "audio.h"
#include "config.h"
#include "event.h"
#include "fileio.h"
#include "memmgr.h"
#include "resname.h"
#include "resource.h"
#include "start.h"
#include "errmsg.h"
#include "dialog.h"
#include "implode.h"
#include "pk.h"
#include "armasm.h"

//ANSI standard headers
#include "dos.h"
#include "io.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static char* near resMapName = RESMAPNAME;
static Handle near resourceMap;

static word near resVolFd = CLOSED;

static char* near altMapName = ALTMAPNAME;
static Handle near alternateMap;
static char* near altVolName = ALTVOLNAME;
static word near altVolFd = CLOSED;
static word near altDirNum;


//init global resource list
global void InitResource(strptr where)
{
	int dirNum, fd;
	char mapName[64], volName[64];
	InitList(&loadList);

	if (!ReadConfigFile(where, "RESOURCE.CFG"))
	{
#ifndef DEBUG
		Panic(E_NO_INSTALL, where);
#else
		Panic(E_NO_WHERE, where);
#endif
	}

	InitPatches();

	//resource and resource map files
	//(these files are required for 'volume-based' versions of a game
	//and optional for 'file-based versions of a game)
	sprintf(volName, "%s.%03d", RESVOLNAME, 0);
	resVolFd = open(volName, O_RDONLY);
	resourceMap = LoadResMap(resMapName);

	//optional 'foreign language' map and resource files
	//(per optional patchDir specification)
	dirNum = 0;
	while (patchDir[dirNum])
	{
		sprintf(mapName, "%s%s", patchDir[dirNum], altMapName);
		if ((fd = open(mapName, 0)) >= 0)
		{
			close(fd);
			sprintf(volName, "%s%s", patchDir[dirNum], altVolName);
			break;
		}
		dirNum++;
	}
	if (!patchDir[dirNum])
	{
		strcpy(mapName,altMapName);
		strcpy(volName,altVolName);
	}
	altDirNum = dirNum;
	if (alternateMap = LoadResMap(mapName))
	{
		if (CLOSED == (altVolFd = open(volName, O_RDONLY)))
		{
			DisposeHandle(alternateMap);
			alternateMap = 0;
		}
	}
}


//allocate buffer and load resource map into it
global Handle LoadResMap(char *mapName)
{
	/* For multiple capacity floppy support, we must determine the PROPER
	 * resource map to load. This is because the current map structure aligns
	 * with only one set of volume geography. The assumptions are, that a
	 * user will use his largest capacity floppy drive in conjunction
	 * with a hard disk OR another equal or lower capacity floppy drive OR
	 * use his hard disk exclusively (full install).
	 * Drive capacities and MAP availabilities are examined at startup
	 * to determine which MAP to load
	 */

	int fd, size;
	Handle buffer = 0;

	if ((fd = open(mapName, 0)) >= 0)
	{
		size = (uword) filelength(fd);
		if ((buffer = NeedHandle(size)))
		{
			size = readfar(fd, buffer, size);
			if (size == -1)
			{
				DisposeHandle(buffer);
				buffer = 0;
			}
		}
		close(fd);
	}
	return buffer;
}


struct
{
	unsigned char type;
	unsigned int id;
	unsigned int segmentLength;
	unsigned int length;
	unsigned int compressUsed;
} dataInfo;


//determine if this resource is locateable
global bool ResCheck(byte resType, uint resId)
{
	int dumy;
	ulong dumylong;
	char pathName[64];

	pathName[0] = '\0';

	if (FindDirEntry(&dumylong, resType, resId, &dumy))
		return true;
	if (FindPatchEntry(resType, resId) != -1)
		return true;
	if ((dumy = ROpenResFile(resType, resId, pathName)) != CLOSED)
	{
		close(dumy);
		return true;
	}
	if ((resType == RES_AUDIO || resType == RES_WAVE) && FindAudEntry(resId) != -1L)
		return true;

	return false;
}


global bool ResCheck36(byte resType, uint module, byte noun, byte verb, byte cond, byte sequ)
{
	uint dumy;
	char pathName[64];

	MakeName36(resType, pathName, module, noun, verb, cond, sequ);

	if (resType == RES_AUDIO36)
	{
		if (FindAud36Entry(module, noun, verb, cond, sequ) != -1L)
			return true;
		if ((dumy = ROpenResFile(RES_AUDIO, 0, pathName)) == (uint) CLOSED)
			return false;
		close(dumy);
		return true;
	}

	if (resType == RES_SYNC36)
	{
		if (FindSync36Entry(module, noun, verb, cond, sequ, &dumy) != -1L)
			return true;
		if ((dumy = ROpenResFile(RES_SYNC, 0, pathName)) == (uint) CLOSED)
			return false;
		close(dumy);
		return true;
	}

	return false;
}


//allocate memory and load this resource
global Handle DoLoad(byte resType, uint resId)
{
	ulong offset;
	char fileName[64];
	byte typeLen;
	bool loadedFromFile = false;
	long seek;
	int fd, n;
	Handle outHandle;

	//default return handle to NULL
	outHandle = 0;

	AudioPause(resType, resId);

	//First check to see if the resource is present as a patch file or
	//is in the Alternate Resource Volume (but only if the ARV is at an
	//earlier patchDir path specification than the patch file found).
	if (((n = FindPatchEntry(resType, resId)) >= 0) && (altDirNum >= n || !FindDirEntryMap(&offset, resType, resId, alternateMap)))
		sprintf(fileName, "%s%s", patchDir[n], ResNameMake(resType, resId));
	else
		fileName[0] = '\0';

	if ((fd = ROpenResFile(resType, resId, fileName)) != CLOSED)
	{
		loadedFromFile = true;
		dataInfo.compressUsed = 0;
		dataInfo.length = (uword) filelength(fd) - 2;

		read(fd, &typeLen, 1);
#ifndef DEBUG
		if (resType != typeLen)
		{
			RAlert(E_RESRC_MISMATCH);
#else
		if (resType != typeLen && (resType != RES_XLATE || typeLen != RES_MSG))
		{
			RAlert(E_WRONG_RESRC, resType, resId);
#endif
			close(fd);
			ExitThroughDebug();
		}

		switch(resType)
		{
			case RES_PIC:
				lseek(fd,3L,LSEEK_BEG);
				read(fd,&typeLen,1);
				seek = (long) typeLen + 22L; // current size of picb header
				break;
			case RES_VIEW:
				lseek(fd,3L,LSEEK_BEG);
				read(fd,&typeLen,1);
				seek = (long) typeLen + 22L; // current size of viewb header
				break;
			case RES_PAL:
				lseek(fd,3L,LSEEK_BEG);
				read(fd,&typeLen,1);
				seek = (long) typeLen; // bypass count byte + count
				break;
			default:
				lseek(fd,1L,LSEEK_BEG);
				read(fd,&typeLen,1);
				seek = (long) typeLen;
				break;
		}

		lseek(fd, seek, LSEEK_CUR);
	}
	else
	{
		if (!FindDirEntry(&offset, resType, resId, &fd))
		{
			RAlert(E_NOT_FOUND, ResNameMake(resType, resId));
			ExitThroughDebug();
		}

		//a file is open.
		//seek to indicated offset and read segment header
		lseek(fd, offset, LSEEK_BEG);
		read(fd, (char*)&dataInfo, sizeof(dataInfo));

		//do byte order conversion here
		dataInfo.id = GetWord(dataInfo.id);
		dataInfo.length = GetWord(dataInfo.length);
		dataInfo.compressUsed = GetWord(dataInfo.compressUsed);
	}

	//some one opened the file and set up compression scheme for us
	if (fd)
	{
		//get a handle for the data
		if ((resType == RES_PATCH) && (resId == 10))
		{
			outHandle = GetHandle();
			*outHandle = Get32KEMS();
		}
		else
			outHandle = GetResHandle(dataInfo.length);

		if(dataInfo.compressUsed)
			pkExplode(fd, *outHandle, GetWord(dataInfo.segmentLength));
		else
			readfar(fd, outHandle, dataInfo.length);
	}

	//close this file if we loaded from a file
	if (loadedFromFile)
		close(fd);

	AudioResume();

	//return pointer/handle or 0 for error
	return outHandle;
}


//Search the alternate volume then the base volume for the
//requested resType/resId; return offset and resource handle if found.
global bool FindDirEntry(ulong *offset, byte resType, uword resId, int *resFd)
{
	if (altVolFd != CLOSED && alternateMap && FindDirEntryMap(offset, resType, resId, alternateMap))
	{
		*resFd = altVolFd;
		return true;
	}
	if (resVolFd != CLOSED && resourceMap && FindDirEntryMap(offset, resType, resId, resourceMap))
	{
		*resFd = resVolFd;
		return true;
	}
	return false;
}


global bool FindDirEntryMap(ulong *offset, byte resType, uword resId, Handle resMap)
{
	word firstOffset, lastOffset, midOffset;
	register ResDirEntry far *entry;
	register ResDirHdrEntry far *header;

	if (!resMap)
		return false;

	//find the resource type in the resource map header
	header = (ResDirHdrEntry far*)*resMap;
	while (header->resType != resType)
		if (header->resType == 255)
			return false;
		else
			++header;
	firstOffset = GetWord(header->resTypeOffset);
	lastOffset = GetWord((header+1)->resTypeOffset) - sizeof(ResDirEntry);

	//utilize a binary search to locate the resource id
	while (firstOffset <= lastOffset)
	{
		midOffset = (lastOffset - firstOffset) / 2 / sizeof(ResDirEntry) * sizeof(ResDirEntry) + firstOffset;
		entry = (ResDirEntry far*)((char far*)*resMap + midOffset);
		if (entry->resId == resId)
		{
			//build the found volume offset
			// *offset = GetFarLongP(&entry->volOffset);
			*offset = ((ulong)entry->volWOffset1 << 1) +
			((ulong)entry->volWOffset2 << 9) +
			((ulong)entry->volWOffset3 << 17);
			return true;
		}
		if (entry->resId < resId)
			firstOffset = midOffset + sizeof(ResDirEntry);
		else
			lastOffset = midOffset - sizeof(ResDirEntry);
	}
	return false;
}

