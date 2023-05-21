/* RESTART
 * Procedures used in restarting a game, either from the start of the game
 * or from a saved game.
 */

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments

#include "restart.h"
#include "graph.h"
#include "script.h"
#include "start.h"
#include "setjmp.h"
#include "resource.h"
#include "kernel.h"
#include "memmgr.h"
#include "pmachine.h"
#include "animate.h"
#include "sound.h"
#include "menu.h"
#include "debug.h"
#include "script.h"
#include "sci.h"
#include "midi.h"
#include "palette.h"

int	gameRestarted;


global KERNEL(RestartGame)
{
	//Get rid of our internal stuff.
	static int varyList[] = { 1, PALVARYKILL };

	DoSound(SProcess, false);
	gameRestarted = 1;
	gameStarted = false;

	//Turn off PalVary

	if (palVaryOn)
	{
		palVaryOn = false;
		palettePercent = 0;
		KPalVary(varyList);
	}

	KillAllSounds();
	DisposeAllScripts();
	DisposeLastCast();
	//SetDebug(false);
	//free all memory
	ResUnLoad(RES_MEM, ALL_IDS);

	//Restore the heap and saved vars to what they were before the call to PMachine.
	RestartHeap();

	//Regenerate the 'patchable resources' table
	InitPatches();

	InitMenu();
	DoSound(SProcess, true);
	DoSound(SSetReverb, 0);
	reverbDefault = 0;

	//Now restore the stack and restart the PMachine.
	longjmp(restartBuf, 1);
}


global KERNEL(GameIsRestarting)
{
	acc = gameRestarted;
	if (argCount && arg(1) == false)
		gameRestarted = false;
}

