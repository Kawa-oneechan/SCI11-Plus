// SOUND.C - SCI Sound Manager

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments
// > Removed AMIGA code

#include "audio.h"
#include "config.h"
#include "errmsg.h"
#include "fileio.h"
#include "intrpt.h"
#include "kernel.h"
#include "memmgr.h"
#include "midi.h"
#include "object.h"
#include "pmachine.h"
#include "resname.h"
#include "resource.h"
#include "restypes.h"
#include "selector.h"
#include "sound.h"
#include "stdio.h"
#include "string.h"
#include "volload.h"

static Handle near soundDrv;
static int near numberOfVoices;
static int near numberOfDACs;
static int near devID;

global bool soundOn = true;

static struct
{
	uint count;
	uint func;
	uint arg2;
	uint arg3;
	uint arg4;
} audArgs;


global bool InitSoundDriver()
{
	int patchNum;
	Handle patchHandle;

	//Load sound driver
#ifdef IBM
	if ((soundDrv = LoadHandle(soundDriver)) == (Handle)NULL)
	{
		RAlert(E_NO_AUDIO_DRVR, soundDriver);
		return false;
	}
	LockHandle(soundDrv);
#endif

	//Load patch (if one is needed)
	//If bit 7 of the patch number is set, then the patch will need to be locked permanently in hunk
	patchNum = DoSound(SPatchReq, (void far*)*(soundDrv), (int far*)&numberOfVoices, (int far*)&numberOfDACs, (int far*)&devID);

	if (patchNum != -1)
	{
		if ((patchNum & 0x7f) == 10)
			patchHandle = DoLoad(RES_PATCH, (patchNum & 0x7f));
		else
			patchHandle = ResLoad(RES_PATCH, (patchNum & 0x7f));
		if (patchNum & 0x80)
		{
			ResLock(RES_PATCH, (patchNum & 0x7f), true);
			LockHandle(patchHandle);
		}
	}

	//Initialize sound driver
#ifdef IBM
	if (DoSound(SInit, (char far*)*(patchHandle), 0) == -1)
	{
		DisposeHandle(soundDrv);
		RAlert(E_NO_MUSIC);
		return false;
	}
#endif

	InitList(&soundList);
	InstallServer(SoundServer,1);

	DoSound(SProcess, true);

	return true;
}


global void TermSndDrv()
{
	KillAllSounds();
	DoSound(STerminate);
}


global void KillAllSounds()
{
	Sound *sn;
	Handle theHandle;

	//Stop and delete each node in the sound list
	while (!EmptyList(&soundList))
	{
		sn = (Sound*)Native(FirstNode(&soundList));
		if (sn->sSample)
		{
			audArgs.count = 2;
			audArgs.func = STOP;
			audArgs.arg2 = sn->sNumber;
			KDoAudio((word*)&audArgs);
		}
		else
		{
			DoSound(SEnd, (char far*)sn);
			ResLock(RES_SOUND, sn->sNumber, false);
			if (theHandle = (Handle)Native(GetProperty((Obj*)Native(sn->sKey), s_handle)))
			{
				if ((int)theHandle != 1)
				{
					CriticalHandle(theHandle, false);
					UnlockHandle(theHandle);
				}
			}
		}
		DeleteNode(&soundList,Pseudo(sn));
	}
}


global void RestoreAllSounds()
{
	Sound *sn;
	Obj *soundObj;
	Handle sHandle;
	int soundId;

	//For every node on the sound list, load the resource in the s_number property.
	//If the sState property of the node is non-zero, restart the sound using the SRestore function in MIDI.S
	sn = (Sound*)Native(FirstNode(&soundList));

	while (sn)
	{
		soundObj = (Obj*)Native(GetKey(Pseudo(sn)));
		soundId = GetProperty(soundObj, s_number);
		if (sn->sSample)
		{
			//put sample stuff here
		}
		else
		{
			if (soundId)
				ResLoad(RES_SOUND, soundId);
			if (sn->sState)
			{
				sHandle = ResLoad(RES_SOUND, soundId);
				CriticalHandle(sHandle, true);
				ResLock(RES_SOUND, soundId, true);

				SetProperty(soundObj, s_handle, (uint)Pseudo(sHandle));
				sn->sPointer = (char far*)sHandle;
				DoSound(SRestore, (char far*)sn);
				//if (sn->sSample)
				//	LockHandle(sHandle);
				UpdateCues(soundObj);
			}
		}

		sn = (Sound*)Native(NextNode(Pseudo(sn)));
	}

	//Reset the default reverb mode
	DoSound(SSetReverb, reverbDefault);
}


//Return RES_AUDIO if we have a DAC and there is an audio sample for this resId, else return RES_SOUND.
global byte GetSoundResType(uint resId)
{
	if (audioDrv && !audNone && (ResCheck(RES_AUDIO, resId) || ResCheck(RES_WAVE, resId)))
		return RES_AUDIO;
	else
		return RES_SOUND;
}

//Kernel Functions
global KERNEL(DoSound)
{
	Obj *soundObj = (Obj*)Native(arg(2));

	switch(arg(1))
	{
		case MASTERVOL:
			if (argCount == 1)
				acc = DoSound(SMasterVol, 255);
			else
				acc = DoSound(SMasterVol, arg(2));
			break;
		case SOUNDON:
			if (argCount == 1)
				acc = DoSound(SSoundOn, 255);
			else
				acc = DoSound(SSoundOn, arg(2));
			break;
		case RESTORESND:
			break;
		case NUMVOICES:
			acc = numberOfVoices;
			break;
		case NUMDACS:
			audArgs.func = DACFOUND;
			KDoAudio((word*)&audArgs);
			break;
		case SUSPEND:
			SuspendSounds(arg(2));
			break;
		case INITSOUND:
			InitSnd(soundObj);
			break;
		case KILLSOUND:
			KillSnd(soundObj);
			break;
		case PLAYSOUND:
			PlaySnd(soundObj, arg(3));
			break;
		case STOPSOUND:
			StopSnd(soundObj);
			break;
		case PAUSESOUND:
			PauseSnd(soundObj, arg(3));
			break;
		case FADESOUND:
			FadeSnd(soundObj, arg(3), arg(4), arg(5), arg(6));
			break;
		case HOLDSOUND:
			HoldSnd(soundObj, arg(3));
			break;
		case SETVOL:
			SetSndVol(soundObj, arg(3));
			break;
		case SETPRI:
			SetSndPri(soundObj, arg(3));
			break;
		case SETLOOP:
			SetSndLoop(soundObj, arg(3));
			break;
		case UPDATECUES:
			UpdateCues(soundObj);
			break;
		case MIDISEND:
			MidiSend(soundObj, arg(3), arg(4), arg(5), arg(6));
			break;
		case SETREVERB:
			if (argCount == 1)
				acc = DoSound(SSetReverb, 255);
			else
				acc = DoSound(SSetReverb, arg(2));
			break;
		case CHANGESNDSTATE:
			//This function is on it's way out (after KQ5 cd and Jones cd ship
			//KAWA WAS HERE just to express amusement at the above.
			ChangeSndState(soundObj);
			break;
	}
}


//Use the SProcess function of MIDI.S to ignore or honor calls to SoundServer.
//A true value in onOff will cause sounds to be suspended, while a false value will cause them to continue
global void SuspendSounds(int onOff)
{
	DoSound(SProcess, !onOff);
}


//Allocate a sound node for the object being initialized if there isn't already one), load the sound resource
//specified in the s_number property, and set up the node properties
global void InitSnd(Obj *soundObj)
{
	Sound *sn;
	int soundId;
	byte sample;

	soundId = GetProperty(soundObj, s_number);

	if (GetSoundResType(soundId) == RES_AUDIO)
		sample = 1; //We have a DAC and there is a sample for this sound
	else
	{
		//No DAC or no sample.
		//First, filter out "not found" condition in case there is no MIDI file but there is a sample file.
		if (!ResCheck(RES_SOUND, soundId) && (ResCheck(RES_AUDIO, soundId) || ResCheck(RES_WAVE, soundId)))
			return;
		sample = 0;
	}

	if (!sample && soundId)
		ResLoad(RES_SOUND, soundId);

	if (!(GetProperty(soundObj, s_nodePtr)))
	{
		if ((sn = NeedPtr(sizeof(Sound))) == NULL)
			return;
		ClearPtr(sn);
		AddToEnd(&soundList, (ObjID)Pseudo(sn), Pseudo(soundObj));
		SetProperty(soundObj, s_nodePtr, (uint)Pseudo(sn));
	}
	else
	{
		sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr));
		StopSnd(soundObj);
	}

	sn->sSample = sample;
	sn->sLoop = 0;
	if ((char)GetProperty(soundObj, s_loop) == (char)-1)
		sn->sLoop = 1;
	sn->sPriority = (char)GetProperty(soundObj, s_priority);
	sn->sVolume = (char)GetProperty(soundObj, s_vol);
	sn->sSignal = 0;
	sn->sDataInc = 0;
}


//Stop the sound and delete it's node
global void KillSnd(Obj *soundObj)
{
	Sound *sn;

	sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr));
	StopSnd(soundObj);

	if (sn)
	{
		DeleteNode(&soundList,Pseudo(sn));
		DisposePtr(sn);
	}

	SetProperty(soundObj, s_nodePtr, 0);
}


//Load the resource in the s_number property of the object, lock the resource, and start the sound playing
global void PlaySnd(Obj *soundObj, int how)
{
	Sound *sn;
	Handle sHandle;
	int soundId;
	LoadLink far **scan;

	if (!GetProperty(soundObj, s_nodePtr))
		InitSnd(soundObj);

	if (sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr)))
	{
		if (GetProperty(soundObj, s_handle))
			StopSnd(soundObj);

		soundId = GetProperty(soundObj, s_number);
		//why is the following not set in InitSnd? TM - 7/17/92
		sn->sNumber = soundId;

		if (GetSoundResType(soundId) == RES_AUDIO)
			sn->sSample = 1;
		else
			sn->sSample = 0;

		if (!sn->sSample)
		{
			if (!ResCheck(RES_SOUND, soundId) && (ResCheck(RES_AUDIO, soundId) || ResCheck(RES_WAVE, soundId)))
			{
				sn->sSample = 1;
				SetProperty(soundObj, s_signal, -1);
				return;
			}
			if ((sHandle = ResLoad(RES_SOUND, soundId)) == NULL)
				return;
			CriticalHandle(sHandle, true);
			ResLock(RES_SOUND, soundId, true);
		}
		else
		{
			//if resource loaded...
			if ((scan = FindResEntry(RES_AUDIO, soundId)) || (scan = FindResEntry(RES_WAVE, soundId)))
				sHandle = (*scan)->lData.data; //pass handle
			else
				sHandle = (Handle)1; //else stream from disk
		}

		SetProperty(soundObj, s_handle, (uint)Pseudo(sHandle));
		SetProperty(soundObj, s_signal, 0);
		SetProperty(soundObj, s_min, 0);
		SetProperty(soundObj, s_sec, 0);
		SetProperty(soundObj, s_frame, 0);

		sn->sPriority = (char) GetProperty(soundObj, s_priority);

		//The following few lines should be removed after KQ5 cd and Jones cd ship....
		//KAWA WAS HERE again.
		sn->sVolume = (char)GetProperty(soundObj, s_vol);
		if (GetProperty(soundObj, s_loop) == -1)
			sn->sLoop = true;
		else
			sn->sLoop = false;

		//In the future, the volume property will be set to a value that will be passed into this function

		sn->sPointer = (char far*)sHandle;
		if (!sn->sSample)
		{
			ChangeSndState(soundObj);
			DoSound(SPlay, (char far*)sn,how);
		}
		else
		{
			audArgs.count = 3;
			audArgs.func = PLAY;
			audArgs.arg2 = GetProperty(soundObj, s_number);
			audArgs.arg3 = GetProperty(soundObj, s_loop);
			if ((uint)sHandle == 1)
				audArgs.arg4 = 0; //tells audio.c to play from disk
			else
				audArgs.arg4 = (uint)sHandle;
			KDoAudio((word*)&audArgs);
		}

		SetProperty(soundObj, s_priority, (int) sn->sPriority);
	}
	else
		SetProperty(soundObj, s_signal, -1);
}


global void StopSnd(Obj *soundObj)
{
	Sound *sn, *searchSn;

	/* Search every sound in the sound list to make sure that
	 * there are no other sounds playing off of the same resource
	 * as the one we are now stopping. If there is, then we
	 * do not want to unlock the resource when we stop the sound
	 */

	if (sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr)))
	{
		if (sn->sSample)
		{
			audArgs.count = 2;
			audArgs.func = STOP;
			audArgs.arg2 = sn->sNumber;
			KDoAudio((word*)&audArgs);
			SetProperty(soundObj, s_handle, 0);
			SetProperty(soundObj, s_signal, -1);
			return;
		}
		searchSn = (Sound*)Native(FirstNode(&soundList));
		while (searchSn)
		{
			if ((searchSn != sn) && (searchSn->sPointer == sn->sPointer) && GetProperty((Obj*)Native(searchSn->sKey), s_handle))
				break;
			searchSn = (Sound*)Native(NextNode(Pseudo(searchSn)));
		}

		DoSound(SEnd, (char far*)sn);

		if ((!searchSn) && GetProperty(soundObj, s_handle))
		{
			Handle theHandle;

			ResLock(RES_SOUND, sn->sNumber, false);
			if (theHandle = (Handle) Native(GetProperty(soundObj, s_handle)))
			{
				if ((int)theHandle != 1)
				{
					CriticalHandle(theHandle, false);
					UnlockHandle(theHandle);
				}
			}
		}
	}

	SetProperty(soundObj, s_handle, 0);
	SetProperty(soundObj, s_signal, -1);
}


global void PauseSnd(Obj *soundObj, int stopStart)
{
	Sound *sn;

	/* If the Object parameter is 0, then we want to pause/unpause
	 * every node. If it is nonzero, pause/unpause only the node
	 * belonging to that object
	 */
	if (!(soundObj))
		DoSound(SPause, (char far*)0, stopStart);
	else if (sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr)))
		DoSound(SPause, (char far*)sn, stopStart);
}


//Fade the node belonging to the object specified using the fade function of MIDI.S
global void FadeSnd(Obj *soundObj, int newVol, int fTicks, int fSteps, int fEnd)
{
	Sound *sn;

	if (fEnd)
		newVol += 128;

	if (sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr)))
		DoSound(SFade, (char far*)sn,newVol,fTicks,fSteps);
}


/* Set the hold property of the sound node belonging to the
 * specified object. This is another type of loop setting,
 * in which sounds can be looped in the middle, and continue
 * on towards the end after being released
 */
global void HoldSnd(Obj *soundObj, int where)
{
	Sound *sn;

	if (sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr)))
		DoSound(SHold, (char far*)sn,where);
}


//Change the volume of the sound node and the sound object to the value passed in
global void SetSndVol(Obj *soundObj, int newVol)
{
	Sound *sn;

	if (sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr)))
	{
		if (sn->sVolume != ((char) newVol))
		{
			DoSound(SChangeVol, (char far*)sn,newVol);
			SetProperty(soundObj, s_vol,newVol);
		}
	}
}


/* Set the priority of the sound node to the value in newPri.
 * If the value is -1, then simply clear the fixed priority
 * flag in the sound node and in the flags property of the
 * sound object. If it is not -1, then set both of those
 * flags
 */
global void SetSndPri(Obj *soundObj, int newPri)
{
	Sound *sn;

	if (sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr)))
	{
		if (newPri == -1)
		{
			sn->sFixedPri = false;
			SetProperty(soundObj, s_flags, (GetProperty(soundObj, s_flags) & (-1 - mFIXEDPRI)));
		}
		else
		{
			sn->sFixedPri = true;
			SetProperty(soundObj, s_flags, (GetProperty(soundObj, s_flags) | mFIXEDPRI));
			DoSound(SChangePri, (char far*)sn,newPri);
		}
	}
}


//Set the loop property of the sound node and the sound object to the value passed in.
global void SetSndLoop(Obj *soundObj, int newLoop)
{
	Sound *sn;

	if (sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr)))
	{
		if (newLoop == -1)
		{
			SetProperty(soundObj, s_loop, -1);
			sn->sLoop = true;
		}
		else
		{
			SetProperty(soundObj, s_loop,1);
			sn->sLoop = false;
		}
	}
}


/* Copy the current cue, clock, and volume information from the
 * sound node to it's object. This should be called every game
 * cycle (in the sounds check: method), or the time and cue
 * information in the sound object will be wrong
 */
global void UpdateCues(Obj *soundObj)
{
	Sound *sn;
	int min, sec, frame, signal;

	if (sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr)))
	{
		if (sn->sSample)
		{
			signal = AudioLoc(); //don't call KDoAudio since it sets acc
			if (signal == -1)
				StopSnd(soundObj);
		}
		else
		{
			signal = DoSound(SGetSignalRset, (char far*)sn);
			//signal = sn->sSignal;
			//sn->sSignal = 0;
			switch(signal)
			{
				case 0xff:
					StopSnd(soundObj);
					break;
				case 0x00:
					if ((unsigned int)GetProperty(soundObj, s_dataInc) != sn->sDataInc)
					{
						SetProperty(soundObj, s_dataInc, sn->sDataInc);
						SetProperty(soundObj, s_signal, (sn->sDataInc + 127));
					}
					break;
				default:
					SetProperty(soundObj, s_signal, signal);
			}

			DoSound(SGetSYMPTE, (char far*)sn, (char far*)&min, (char far*)&sec, (char far*)&frame);
			SetProperty(soundObj, s_min,min);
			SetProperty(soundObj, s_sec, sec);
			SetProperty(soundObj, s_frame,frame);

			SetProperty(soundObj, s_vol, (int) sn->sVolume);
		}
	}
}


//Send MIDI a command to any channel of the node belonging to the specified object
global void MidiSend(Obj *soundObj, int channel, int command, int value1, int value2)
{
	Sound *sn;

	channel--;
	if (command == PBEND)
	{
		if (value1 > 8191)
			value1 = 8191;
		if (value1 < -8192)
			value1 = -8192;
	}
	else
	{
		if (value1 > 127)
			value1 = 127;
		if (value1 < 0)
			value1 = 0;
	}

	if (value2 > 127)
		value2 = 127;
	if (value2 < 0)
		value2 = 0;

	if (sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr)))
	{
		switch(command)
		{
			case NOTEOFF:
				DoSound(SNoteOff, (char far*)sn, channel, value1, value2);
				break;
			case NOTEON:
				DoSound(SNoteOn, (char far*)sn, channel, value1, value2);
				break;
			case CONTROLLER:
				DoSound(SController, (char far*)sn, channel, value1, value2);
				break;
			case PCHANGE:
				DoSound(SPChange, (char far*)sn, channel, value1);
				break;
			case PBEND:
				DoSound(SPBend, (char far*)sn, channel, value1 + 8192);
				break;
		}
	}
}


//Update the sLoop, sVolume, and sPriority properties of the sound node to
//what is currently in those properties of object which the node belongs to
global void ChangeSndState(Obj *soundObj)
{
	//This function is on it's way out (after KQ5 cd and Jones cd ship)
	//KAWA WAS HERE Y'ALL
	Sound *sn;

	if (sn = (Sound*)Native(GetProperty(soundObj, s_nodePtr)))
	{
		sn->sLoop = 0;
		if (GetProperty(soundObj, s_loop) == -1)
			sn->sLoop = 1;

		if (sn->sVolume != (char) GetProperty(soundObj, s_vol))
			DoSound(SChangeVol, (char far*)sn, GetProperty(soundObj, s_vol));

		if (sn->sPriority != (char) GetProperty(soundObj, s_priority))
			DoSound(SChangePri, (char far*)sn, GetProperty(soundObj, s_priority));
	}
}

