//	pmachine.c

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments
// > Disable stamp checking

#include "language.h"
#include "object.h"
#include "pmachine.h"
#include "sciglobs.h"
#include "selector.h"
#include "start.h"

bool gameStarted = false;
Obj *theGame;

void PMachine()
{
	//Load the class table, allocate the p-machine stack

	Script*	script;
	word startMethod;

	theGame = 0;

	if (!gameStarted)
	{
		LoadClassTbl();
		pmCode = (fptr)GetDispatchAddrInHeapFromC;
		restArgs = 0;
		pStack = NeedPtr(PSTACKSIZE);
		pStackEnd = pStack + PSTACKSIZE;
		FillPtr(pStack, 'S');
	}

#ifdef DEBUG
	ssPtr = &sendStack[-2];
#endif

	scriptHandle = 0;
	GetDispatchAddrInHeapFromC(0, 0, &object, &script);
	theGame = object;
	scriptHandle = script->hunk;
	globalVar = script->vars;

	GetLanguage();

	pmsp = pStack;

	if (!gameStarted)
	{
		gameStarted = true;
		startMethod = s_play;
	}
	else
		startMethod = s_replay;

	InvokeMethod(object, startMethod, 0, 0);
}
