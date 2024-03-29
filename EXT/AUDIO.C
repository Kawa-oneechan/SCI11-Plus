/* AUDIO.C
 * Audio routines (sampled voice and other sounds).
 */

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments
// > Various unsigned values being set to -1, added casts to fix.

#include "audio.h"
#include "aud.h"
#include "types.h"
#include "armasm.h"
#include "fileio.h"
#include "pmachine.h"
#include "resource.h"
#include "intrpt.h"
#include "string.h"
#include "kernel.h"
#include "stdio.h"
#include "resname.h"
#include "config.h"
#include "restypes.h"
#include "errmsg.h"
#include "dos.h"
#include "io.h"

#define	MAXQ	10

bool audioRdy = false;
int audFd = CLOSED;
int audVolFd = CLOSED;
int sfxVolFd = CLOSED;
uint savelen;
Handle audioDrv = NULL;
Handle cdaudioDrv = NULL;
Handle playPtr = NULL;
bool audNone;
uint module, id;
byte noun, verb, cond, sequ, audVolDrive;
int loop;
ulong wSelect;
uint playingNum = (uint)-1; //KAWA WAS HERE
uint audQcnt[MAXQ], audQndx = 0;

// ARM audio buffering
#define AUDARMALOC 0xF000
#define AUDARMSIZE 0x800

ulong bytesBuffered;
Handle audARMPtr = NULL;
uint audARMROffset, audARMWOffset;
ARMType * audARMType;
ARMHandle audARMHandle = NO_MEMORY;


global bool InitAudioDriver()
{
	uint newSize;
	char pathName[64];
	char *cp;
	Handle oldPtr;
	Hunkptr buffAdr;
	struct {
		char armEnabled;
		char unused;
		char irq;
		char dma;
		void far *armWrite;
		void far *armRead;
	} audArgs;

	//If audio is not requested -- vamboose
	if (audioDriver == NULL)
		return ((bool)(audioDrv = 0));

	//Load the audio driver.
	if ((audioDrv = LoadHandle(audioDriver)) == NULL)
	{
		RAlert(E_NO_AUDIO_DRVR, audioDriver);
		return ((bool)(audioDrv = 0));
	}

	//Initialize the audio driver
	LockHandle(audioDrv);
	audArgs.armEnabled = 0;
	if (AudARMInit())
	{
		audBuffKSize = 2 * AUDARMSIZE / 1024;
		if (audARMHandle != NO_MEMORY)
			audArgs.armEnabled = 'A';
	}
	audArgs.irq = (char)audioIRQ;
	audArgs.dma = (char)audioDMA;
	audArgs.armWrite = (void far*)(AudARMWrite);
	audArgs.armRead = (void far*)(AudARMRead);
	newSize = AudioDrv(A_INIT, (uint)&audArgs);
	if (newSize == 0)
	{
		RAlert(E_NO_AUDIO);
		DisposeHandle(audioDrv);
		return ((bool)(audioDrv = 0));
	}

	if (newSize == (uint) -1)
		audNone = true;
	else
	{
		audNone = false;
		if (newSize & 0x8000) ///Disney Dac flag bit
		{
			SetDisneyInt();
			newSize &= 0x7fff;
		}
		ReallocateHandle(audioDrv, newSize);
	}

	InstallServer(AudioServer, 1);

	if (audBuffKSize != 0)
	{
		if (audBuffKSize >= 64)
			audBuffKSize = 0xfff0;
		else
			audBuffKSize *= 1024;
		playPtr = NeedHandle(audBuffKSize);

		/*	Ensure that this buffer does not straddle a 64K-page boundary
			inorder that it can be used for continuous DMA. If it does so
			stadle, then get a second opinion (which we assume won't!) and
			THEN release the first.
		*/

		buffAdr = *playPtr;
		if (((long)buffAdr >> 16) / 0x1000 != (((long)buffAdr >> 16) + audBuffKSize / 16) / 0x1000)
		{
			newSize = (uint)(((((long)buffAdr >> 16) + 65535L) / 65536L - ((long)buffAdr >> 16) - 2L) * 16L);
			DisposeHandle((void *)playPtr);
			oldPtr = NeedHandle(newSize);
			LockHandle(oldPtr);
			playPtr = NeedHandle(audBuffKSize);
			DisposeHandle((void *)oldPtr);
		}
		LockHandle(playPtr);

		//Open optional Audio sound effects Volume
		sfxVolFd = open(SFXVOLNAME, O_RDONLY);

		//Open optional Base-36 Speech/Sync/Rave Volume
		strcpy(pathName, AUDVOLNAME);
		audVolFd = ROpenResFile(RES_AUDIO, 0, pathName);
		if (cp = strchr(pathName,':'))
			audVolDrive = *(--cp);
		else
			audVolDrive = 0;
	}
	return true;
}


//Terminate the audio.
global void EndAudio()
{
	if (audioDrv) {
		AudioStop();
		AudioDrv(A_TERMINATE, 0);
		DisposeHandle(audioDrv);
		audioDrv = 0;
	}
	if (playPtr) {
		DisposeHandle((void *)playPtr);
		playPtr = NULL;
	}
	if (audARMHandle != NO_MEMORY)
		AudARMTerm();

	if (sfxVolFd != CLOSED)
		close(sfxVolFd);
	if (audVolFd != CLOSED)
		close(audVolFd);
}


global KERNEL(DoAudio)
{
	if (!audioDrv)
	{
		acc = 0;
		return;
	}

	switch (arg(1))
	{
		case QUEUE: //audio playback begins
			if (audQndx < MAXQ)
			{
				SetAudParms(args);
				SelectAudio(1);
				audQcnt[++audQndx] = acc;
			}
			else
				acc = 0;
			break;
		case WPLAY: //buffers are loaded but audio will not be started
			audQndx = 0;
			AudioStop();
			SetAudParms(args);
			if (SelectAudio(0))
			{
				playingNum = arg(2);
				wSelect = ((noun + verb * 256L) * 256 + cond) * 256 + sequ;
				savelen = acc;
				AudioWPlay(); //play sample once only
				audioRdy = true;
			}
			else
				audioRdy = false;
			break;
		case PLAY: //audio playback begins
			audQndx = 0;
			AudioStop();
			SetAudParms(args);
			if (!audioRdy || wSelect != (ulong)((noun + verb*256L)*256 + cond)*256 + sequ) //KAWA WAS HERE
			{
				if (SelectAudio(0))
				{
					playingNum = arg(2);
					AudioPlay();
				}
			}
			else
			{
				AudioPlay();
				acc = savelen;
			}
			audQcnt[0] = acc;
			audioRdy = false;
			break;
		case STOP:
			audQndx = 0;
			if (argCount > 1 && arg(2) != (int)playingNum) //KAWA WAS HERE
				break;
			playingNum = (uint)-1; //KAWA WAS HERE
			audioRdy = false;
			AudioStop();
			break;
		case PAUSE:
			AudioPause(0,0);
			break;
		case RESUME:
			AudioResume();
			break;
		case LOC:
			acc = AudioLoc(); //get number of 60ths seconds played
/* Notes on LOC:
	A return value of -1 indicates that no audio is currently playing.
	A return value of 0 through 65534 indicates the time in sixtieths of
	a second that the current audio selection has been playing for.
*/
			break;
		case RATE:
			AudioRate(arg(2)); //playback at specified rate
			break;
		case VOLUME:
			AudioVol(arg(2));
			break;
		case DACFOUND:
			if (audNone)
				acc = 0;
			else
				acc = 1;
			break;
		case CDREDBOOK:
			KCDAudio(args);
			break;
		default:
			break; //unexpected audio driver function
	}
}


global void SetAudParms(word *args)
{
	if (argCount < 6)
	{
		module = AUDMODNUM;
		id = arg(2);
		if (argCount >= 3)
			loop = arg(3);
		else
			loop = 0;
	}
	else
	{
		module = arg(2);
		noun = (byte)arg(3);
		verb = (byte)arg(4);
		cond = (byte)arg(5);
		sequ = (byte)arg(6);
		loop = 0;
	}
}


//CD-Red book functions
global KERNEL(CDAudio)
{
	switch (arg(2))
	{
		case CDREDBOOK: //load driver
			acc = InitCDAudioDriver();
			break;
		case PLAY: //load driver if needed and play cd-audio
			if (cdaudioDrv != NULL)
				acc = CDAudioPlay(args);
			else
				acc = 0;
			break;
		case STOP: //stop cd-audio and unload driver
			EndCDAudio();
			acc = 1;
			break;
		case PAUSE: //stop cd-audio
			CDAudioDrv(A_STOP, 0);
			acc = 1;
			break;
		case LOC: //report elapsed 60th-secs or -1 if not playing
			acc = CDAudioLoc();
			break;
		default:
			acc = 0;
			break; //unexpected cd-audio driver function
	}
}


//CD-Red book functions
global bool CDAudioPlay(word *args)
{
	struct {
		uint track;
		uint unused;
		ulong start;
		ulong len;
	} audArgs;

	if (argCount < 3)
		return false;
	audArgs.track = arg(3);
	if (argCount >= 4)
		audArgs.start = arg(4) * 75L;
	else
		audArgs.start = 0L;
	if (argCount >= 5)
		audArgs.len = arg(5) * 75L;
	else
		audArgs.len = 0L;
	if (CDAudioDrv(A_SELECT, (uint)(&audArgs)) != 0)
		return false;
	CDAudioDrv(A_PLAY, 0);
	return true;
}


//Determine the cd-audio status.
global int CDAudioLoc()
{
	struct {
		uint ticks;
		ulong bytes;
	} audArgs;

	CDAudioDrv(A_LOC, (uint)(&audArgs));
	return audArgs.ticks;
}


//Load and init the CD-Audio driver
global bool InitCDAudioDriver()
{
	int	t;
	uint newSize;
	char filename[64];
	struct {
		byte drive;
	} audArgs;

	if (cdaudioDrv != NULL)
		return true;

	//Load the cd-audio driver.
	strcpy(filename,audioDriver);
	for (t = strlen(filename) - 1; t >= 0; t--)
		if (filename[t] == '\\' || filename[t] == '/' || filename[t] == ':')
			break;
	filename[++t] = '\0';
	strcat(filename, "AUDCDROM.DRV");
	if ((cdaudioDrv = LoadHandle(filename)) == NULL)
		return false;
	LockHandle(cdaudioDrv);

	//Initialize the cd-audio driver

	//locate the cd-rom drive by finding the dac audio volume
	//if not found, use current drive
	audArgs.drive = audVolDrive;
	newSize = CDAudioDrv(A_INIT, (uint)(&audArgs));
	if (newSize == 0)
	{
		DisposeHandle(cdaudioDrv);
		cdaudioDrv = NULL;
		return false;
	}
	ReallocateHandle(cdaudioDrv, newSize);

	return true;
}


//Terminate the cd-audio driver.
global void EndCDAudio()
{
	if (cdaudioDrv != NULL)
	{
		CDAudioDrv(A_STOP, 0);
		CDAudioDrv(A_TERMINATE, 0);
		DisposeHandle(cdaudioDrv);
		cdaudioDrv = NULL;
	}
}


//Preparte the sound for playback
global void AudioWPlay()
{
	AudioDrv(A_WPLAY, 0);
}


//Play the sound for
global void AudioPlay()
{
	AudioDrv(A_PLAY, 0);
}


//Stop the audio playback
global void AudioStop()
{
	AudioDrv(A_STOP, 0);
}


//Pause audio if CD-Rom
global void AudioPause(byte resType, uint resId)
{
	struct {
		uint type;
		ulong id;
	} audArgs;

	if (audioDrv)
	{
		audArgs.type = (uint)resType;
		audArgs.id = resId;
		AudioDrv(A_PAUSE, (uint)(&audArgs));
	}
}


//Resume audio if paused
global void AudioResume()
{
	if (audioDrv)
		AudioDrv(A_RESUME, 0);
}


//Determine the audio status.
global int AudioLoc()
{
	struct {
		uint ticks;
		ulong bytes;
	} audArgs;

	AudioDrv(A_LOC, (uint)(&audArgs));
	return audArgs.ticks;
}


//Declare the audio playback rate.
global uint AudioRate(uint hertz)
{
	struct {
		int	rate;
	} audArgs;

	audArgs.rate = hertz;
	AudioDrv(A_RATE, (uint)(&audArgs));
	return audArgs.rate;
}


//Declare the audio volume.
global void AudioVol(int vol)
{
	struct {
		int	vol;
	} audArgs;

	audArgs.vol = vol;
	AudioDrv(A_VOLUME, (uint)(&audArgs));
}


global bool SelectAudio(char queue)
{
	int	ndx;
	char pathName[64];
	struct {
		int fd;
		int fdFlag;
		ulong start;
		ulong len;
		Hunkptr buffAdr;
		uint buffLen;
		char loop;
		char flag;
	} audArgs;

	acc = 0; // initial condition: sample is not found
	if (!audBuffKSize)
		return false;	// no audio buffer

	audARMROffset = audARMWOffset = 0;
	if (audFd != CLOSED)
	{
		close(audFd);
		audFd = CLOSED;
	}
	audArgs.fdFlag = -1;
	audArgs.buffAdr = *playPtr;
	audArgs.buffLen = audBuffKSize;
	audArgs.loop = (char)(loop == -1 ? 1:0);
	audArgs.flag = 0; // flag bits will be in audio header
	audArgs.start = 0L;

	if (module == AUDMODNUM)
	{
		// Search for AUD/WAV resource:
		pathName[0] = '\0';

		// FIRST: Search for .AUD file via 'patchDir=' dir list
		if ((ndx = FindPatchEntry(RES_AUDIO, id)) != -1)
		{
			sprintf(pathName, "%s%s", patchDir[ndx], ResNameMake(RES_AUDIO, id));
			audFd = open(pathName, 0);
			audArgs.fd = audFd;
		}
		// SECOND: Search for .WAV file via 'patchDir=' dir list
		else if ((ndx = FindPatchEntry(RES_WAVE, id)) != -1) {
			sprintf(pathName, "%s%s", patchDir[ndx], ResNameMake(RES_WAVE, id));
			audFd = open(pathName, 0);
			audArgs.fd = audFd;
		}
		// THIRD: Search for .AUD file via 'audio=' dir list
		else if ((audFd = ROpenResFile(RES_AUDIO, id, pathName)) != CLOSED)
		{
			audArgs.fd = audFd;
		}
		// FOURTH: Searh for .WAV file via 'wave=' dir list
		else if ((audFd = ROpenResFile(RES_WAVE, id, pathName)) != CLOSED)
		{
			audArgs.fd = audFd;
		}
		// LAST CHANCE: Search for AUD/WAV resource in SFX Resource Volume
		else if ((audArgs.start = FindAudEntry(id)) != -1L)
		{
			audArgs.fd = sfxVolFd;
		}
		// AUD/WAV resource NOT FOUND!
		else
			return false;
	}
	else
	{
		// Search for @ resource:

		// FIRST: Search for @ file via 'audio=' dir list
		MakeName36(RES_AUDIO, pathName, module, noun, verb, cond, sequ);
		if ((audFd = ROpenResFile(RES_AUDIO, 0, pathName)) != CLOSED)
			audArgs.fd = audFd;
		// LAST CHANCE: Search for @ resource in AUD Resource Volume */
		else if ((audArgs.start = FindAud36Entry(module,noun,verb,cond,sequ)) != -1L)
			audArgs.fd = audVolFd;
		// @ resource NOT FOUND!
		else
			return false;
	}

	audArgs.len = filelength(audArgs.fd);
	if (queue)
		acc = AudioDrv(A_QUEUE, (uint)(&audArgs));
	else
		acc = AudioDrv(A_SELECT, (uint)(&audArgs));

	return (acc != 0);
}


ulong FindAudEntry(uint id)
{
	long offset;
	Handle map;
	ResAudEntry far *entry;

	if (sfxVolFd == CLOSED)
		return (ulong)-1L; //KAWA WAS HERE

	if (!ResCheck(RES_MAP, AUDMODNUM))
		return (ulong)-1L; //KAWA WAS HERE
	map = ResLoad(RES_MAP, AUDMODNUM);

	offset = 0L;
	for (entry = (ResAudEntry far *) *map; entry->id != (uint) -1; ++entry)
	{
		offset += ((ulong)entry->offsetMSB << 16) + (ulong)entry->offsetLSW;
		if	(entry->id == id)
			return offset;
	}
	return (ulong)-1L; //KAWA WAS HERE
}


ulong FindAud36Entry(uint module, byte noun, byte verb, byte cond, byte sequ)
{
	long offset;
	Handle map;
	char far *ptr36;
	ResAud36Entry far *entry36;

	if (audVolFd == CLOSED)
		return (ulong)-1L; //KAWA WAS HERE

	if (!ResCheck(RES_MAP, module))
		return (ulong)-1L; //KAWA WAS HERE
	map = ResLoad(RES_MAP, module);

	ptr36 = (char far *)*map;
	offset = *(ulong far *)ptr36;
	ptr36 += 4;

	for (entry36 = (ResAud36Entry far*)ptr36; entry36->flag.sequ != 255; entry36 = (ResAud36Entry far*)ptr36)
	{
		offset += ((ulong)entry36->offsetMSB << 16) + (ulong)entry36->offsetLSW;
		if (entry36->noun == noun && entry36->verb == verb && entry36->cond == cond && (entry36->flag.sequ & SEQUMASK) == sequ)
		{
			if (entry36->flag.sync & SYNCMASK)
			{
				offset += (long)GetWord(entry36->syncLen);
				if (entry36->flag.rave & RAVEMASK)
					offset += (long)GetWord(entry36->raveLen);
			}
			return offset;
		}
		ptr36 += sizeof(ResAud36Entry);
		if (!(entry36->flag.sync & SYNCMASK))
			ptr36 -= sizeof(entry36->syncLen);
		if (!(entry36->flag.rave & RAVEMASK))
			ptr36 -= sizeof(entry36->raveLen);
	}
	return (ulong)-1L; //KAWA WAS HERE
}


uint GetAudQCnt(int ndx)
{
	return audQcnt[ndx];
}


bool AudARMInit()
{
	if ((audARMHandle = AltResMemAlloc(AUDARMALOC,&audARMType)) == NO_MEMORY)
		return false;

	audARMPtr = NeedHandle(AUDARMSIZE);
	return true;
}


void AudARMTerm() {
	ARMFree(audARMType, audARMHandle);
}


bool AudARMRead(uint len, void far *buff)
{
	if (ARMCritical() || (ulong)len > bytesBuffered)
		return false;
	if (ARMRead(audARMType, audARMHandle, audARMROffset, len, buff) == NO_MEMORY)
		return false;
	if ((audARMROffset += len) == AUDARMALOC)
		audARMROffset = 0;
	bytesBuffered -= len;
	return true;
}


bool AudARMWrite(int fdIn, ulong runLen)
{
	uint len;
	ulong reset;
	static ulong bytesToBuffer;
	static int fd;

	if (fdIn != 0)
	{
		fd = fdIn;
		bytesToBuffer = runLen;
		bytesBuffered = 0L;
	}
	else if (!ARMCritical() && bytesToBuffer)
	{
		len = AUDARMSIZE - (audARMWOffset % AUDARMSIZE);
		while (bytesToBuffer && bytesBuffered + len <= AUDARMALOC)
		{
			if (bytesToBuffer < len)
				len = (uint)bytesToBuffer;
			reset = lseek(fd, 0L, LSEEK_CUR);
			if ((len = readfar(fd, audARMPtr, len)) == (uint) -1)
				return false;
			if (len == 0)
				return false;
			if (ARMWrite(audARMType, audARMHandle, audARMWOffset, len, (void far*)*audARMPtr) == NO_MEMORY)
			{
				lseek(fd, reset, LSEEK_BEG);
				return false;
			}
			if ((audARMWOffset += len) == AUDARMALOC)
				audARMWOffset = 0;
			bytesBuffered += len;
			bytesToBuffer -= len;
		}
	}
	return true;
}
