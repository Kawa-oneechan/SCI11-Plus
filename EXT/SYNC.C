/*********************************************
FILE    :sync.c
PURPOSE :'Lip'-sync animation to audio
*********************************************/

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments
// > FindSync36Entry returned signed -1, should be ulong.
// > NextSync -set- unsigned things to -1.

#include "audio.h"
#include "dialog.h"
#include "fileio.h"
#include "intrpt.h"
#include "io.h"
#include "kernel.h"
#include "object.h"
#include "pmachine.h"
#include "resname.h"
#include "resource.h"
#include "restypes.h"
#include "savevars.h"
#include "sci.h"
#include "selector.h"
#include "stdio.h"
#include "string.h"
#include "sync.h"

static bool near sync36, qsync36[MAXQ];
static bool near syncing = false;
static Handle near syncHandle = NULL, qsyncHandle[MAXQ];
static uint near syncNum, qsyncNum[MAXQ], syncIndex, locCnt, qndx, qnum;


global KERNEL(DoSync)
{
	switch (arg(1))
	{
		case QUEUESYNC:
			if (argCount < 6)
				QueueSync((Obj*)Native(arg(2)), arg(3));
			else if (argCount == 6)
				QueueSync36((Obj*)0, arg(2), (byte)arg(3), (byte)arg(4), (byte)arg(5), (byte)arg(6));
			else
				QueueSync36((Obj*)Native(arg(2)), arg(3), (byte)arg(4), (byte)arg(5), (byte)arg(6), (byte)arg(7));
			break;
		case STARTSYNC:
			if (syncHandle != NULL)
				StopSync();
			if (argCount < 7)
				StartSync((Obj*)Native(arg(2)), arg(3));
			else
				StartSync36((Obj*)Native(arg(2)), arg(3), (byte)arg(4), (byte)arg(5), (byte)arg(6), (byte)arg(7));
			locCnt = 0;
			qnum = 0;
			break;
		case NEXTSYNC:
			NextSync((Obj*)Native(arg(2)));
			break;
		case STOPSYNC:
			StopSync();
			for (; qnum < qndx; qnum++)
				if (qsync36[qnum])
					ResUnLoad(RES_MEM, (uword)qsyncHandle[qnum]);
				else
					ResUnLoad(RES_SYNC, qsyncNum[qnum]);
			qnum = qndx = 0;
			break;
	}
}


global void StartSync(Obj *theSync, uint num)
{
	IndexedProp(theSync, syncCue) = -1;
	if ((syncHandle = ResLoad(RES_SYNC, num)) != NULL)
	{
		ResLock(RES_SYNC, syncNum = num, true);
		LockHandle(syncHandle);
		IndexedProp(theSync, syncCue) = 0;
		syncIndex = 0;
		syncing = true;
		sync36 = false;
	}
}


global void QueueSync(Obj *theSync, uint num)
{
	if (theSync)
		IndexedProp(theSync, syncCue) = -1;
	if (qndx < MAXQ && (qsyncHandle[qndx] = ResLoad(RES_SYNC, num)) != NULL)
	{
		ResLock(RES_SYNC, qsyncNum[qndx] = num, true);
		LockHandle(qsyncHandle[qndx]);
		qsync36[qndx++] = false;
		if (theSync)
			IndexedProp(theSync, syncCue) = 0;
	}
}


global void StartSync36(Obj *theSync, uint module, byte noun, byte verb, byte cond, byte sequ)
{
	int fd;
	uint len, typeHdrlen;
	long start, saveOffset;
	char pathName[64];

	IndexedProp(theSync, syncCue) = -1;
	MakeName36(RES_SYNC, pathName, module, noun, verb, cond, sequ);
	if ((fd = ROpenResFile(RES_SYNC, 0, pathName)) != CLOSED)
	{
		start = 0L;
		len = (uint)filelength(fd);
	}
	else
	{
		if ((start = FindSync36Entry(module, noun, verb, cond, sequ, &len)) == -1L)
			return;
		fd = audVolFd;
		saveOffset = lseek(fd, 0L, LSEEK_CUR);
	}
	if ((syncHandle = ResLoad(RES_MEM, len)) != NULL)
	{
		lseek(fd, start, LSEEK_BEG);
		read(fd, &typeHdrlen, 2);
		if (typeHdrlen == RES_SYNC)
		{
			ReadDos(fd, (byte far*)(*syncHandle), len - 2);
			IndexedProp(theSync, syncCue) = 0;
			syncIndex = 0;
			syncing = true;
			sync36 = true;
		}
	}
	if (fd == audVolFd)
		lseek(fd, saveOffset, LSEEK_BEG);
	else
		close(fd);
}


global void QueueSync36(Obj *theSync, uint module, byte noun, byte verb, byte cond, byte sequ)
{
	int fd;
	uint len, typeHdrlen;
	long start, saveOffset;
	char pathName[64];

	if (theSync)
		IndexedProp(theSync, syncCue) = -1;
	MakeName36(RES_SYNC, pathName, module, noun, verb, cond, sequ);
	if ((fd = ROpenResFile(RES_SYNC, 0, pathName)) != CLOSED)
	{
		start = 0L;
		len = (uint)filelength(fd);
	}
	else
	{
		if ((start = FindSync36Entry(module, noun, verb, cond, sequ, &len)) == -1L)
			return;
		fd = audVolFd;
		saveOffset = lseek(fd, 0L, LSEEK_CUR);
	}
	if (qndx < MAXQ && (qsyncHandle[qndx] = ResLoad(RES_MEM, len)) != NULL)
	{
		lseek(fd, start, LSEEK_BEG);
		read(fd, &typeHdrlen, 2);
		if (typeHdrlen == RES_SYNC)
		{
			ReadDos(fd, (byte far*)(*qsyncHandle[qndx]), len - 2);
			qsync36[qndx++] = true;
			if (theSync)
				IndexedProp(theSync, syncCue) = 0;
		}
	}
	if (fd == audVolFd)
		lseek(fd, saveOffset, LSEEK_BEG);
	else
		close(fd);
}


ulong FindSync36Entry(uint module, byte noun, byte verb, byte cond, byte sequ, uint *len)
{
	long offset;
	Handle map;
	char far *ptr36;
	ResAud36Entry far *entry36;

	if (audVolFd == CLOSED)
		return (ulong)-1; //KAWA WAS HERE -- why was this -1L when the return type is ulong?

	if (!ResCheck(RES_MAP, module))
		return (ulong)-1; //KAWA WAS HERE
	map = ResLoad(RES_MAP, module);

	ptr36 = (char far*)*map;
	offset = *(ulong far*)ptr36;
	ptr36 += 4;

	for (entry36 = (ResAud36Entry far*)ptr36; entry36->flag.sequ != 255; entry36 = (ResAud36Entry far*)ptr36)
	{
		offset += ((ulong)entry36->offsetMSB << 16) + (ulong)entry36->offsetLSW;
		if (entry36->noun == noun && entry36->verb == verb && entry36->cond == cond && (entry36->flag.sequ & SEQUMASK) == sequ)
			if ((entry36->flag.sync & SYNCMASK) && (*len = GetWord(entry36->syncLen)))
				return offset;
			else
				break;
		ptr36 += sizeof(ResAud36Entry);
		if (!(entry36->flag.sync & SYNCMASK))
			ptr36 -= sizeof(entry36->syncLen);
		if (!(entry36->flag.rave & RAVEMASK))
			ptr36 -= sizeof(entry36->raveLen);
	}
	return (ulong)-1; //KAWA WAS HERE
}


global void NextSync(Obj *theSync)
{
	Sync tsync;
	Hunkptr	dp;

	if (syncHandle == NULL || syncIndex == (uword)-1)
		return;
	dp = *syncHandle;
	tsync.time = *(dp + (syncIndex++)) + *(dp + (syncIndex++)) * 256;
	if (tsync.time == (uword)-1)
	{
		StopSync();
		if (qnum < qndx)
		{
			syncHandle = qsyncHandle[qnum];
			sync36 = qsync36[qnum];
			syncNum = qsyncNum[qnum];
			locCnt += GetAudQCnt(qnum++);
			syncIndex = 0;
			NextSync(theSync);
			return;
		}
		else
			qnum = qndx = locCnt = 0;
		syncIndex = tsync.cue = (unsigned)-1; //KAWA WAS HERE
	}
	else
		tsync.cue = *(dp+(syncIndex++)) + *(dp + (syncIndex++)) * 256;
	IndexedProp(theSync, syncTime) = tsync.time + locCnt;
	IndexedProp(theSync, syncCue) = tsync.cue;
}


global void StopSync()
{
	if (syncHandle == NULL)
		return;
	if (sync36)
		ResUnLoad(RES_MEM, (uword)syncHandle);
	else
		ResUnLoad(RES_SYNC, syncNum);
	syncHandle = NULL;
	syncing = false;
}

