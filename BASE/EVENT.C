// EVENT - Event manager routines

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments

#include "config.h"
#include "driver.h"
#include "errmsg.h"
#include "event.h"
#include "eventasm.h"
#include "intrpt.h"
#include "kernel.h"
#include "memmgr.h"
#include "mouse.h"
#include "pmachine.h"
#include "resource.h"
#include "script.h"
#include "selector.h"
#include "start.h"
#include "stdlib.h"
#include "string.h"

//Handle and pointer to keyboard driver.
Handle kbdHandle, joyHandle;
Hunkptr keyboard, joystick;

static REventRecord* near bump(REventRecord*);


//init manager for num events
void InitEvent(word num)
{
	//Setup the event queue.
	evQueue = (REventRecord*)NeedPtr(num * sizeof(REventRecord));
	evHead = evTail = evQueue;
	evQueueEnd = (evQueue + num);

	//Load and initialize the keyboard driver.
	if (kbdDriver == NULL || (kbdHandle = LoadHandle(kbdDriver)) == NULL)
	{
		RAlert(E_NO_KBDDRV);
		exit(1);
	}
	else
	{
		LockHandle(kbdHandle);
		keyboard = *kbdHandle;
		KeyboardDriver(D_INIT);
		InstallServer(PollKeyboard, 1);
	}

	//If there is a joystick, load and initialize the joystick driver.
	if (joyDriver != NULL && (joyHandle = LoadHandle(joyDriver)) != NULL)
	{
		LockHandle(joyHandle);
		joystick = *joyHandle;
		JoystickDriver(D_INIT);
	}
}


void EndEvent()
{
	if (joyHandle)
		JoystickDriver(D_TERMINATE);
	DisposeServer(PollKeyboard);
	KeyboardDriver(D_TERMINATE);
	DisposeMouse();
}


//return next event to user
bool RGetNextEvent(word mask, REventRecord *event)
{
	word ret = 0;
	REventRecord *found;

	if (joyHandle)
		PollJoystick();

	found = evHead;
	//scan all events in evQueue
	while (found != evTail)
	{
		if (found->type & mask)
		{
			ret = 1;
			break;
		}
		found = bump(found);
	}

	if (ret)
	{
		//give it to him and blank it out
		memcpy((memptr)event, (memptr)found, sizeof(REventRecord));
		found->type = nullEvt;
		evHead = bump(evHead);
	}
	else
	{
		MakeNullEvent(event); //use his storage
	}
	return(ret);
}


//Flush all events specified by mask from buffer.
void RFlushEvents(word mask)
{
	REventRecord event;

	while (RGetNextEvent(mask, &event))
		;

	if (mask & (keyDown | keyUp))
		FlushKeyboard();
}


//return but don't remove
bool REventAvail(word mask, REventRecord *event)
{
	word ret = 0;
	REventRecord *found;

	found = evHead;
	//scan all events in evQueue
	while (found != evTail)
	{
		if (found->type & mask)
		{
			ret = 1;
			break;
		}
		found = bump(found);
	}

	//a null REventRecord pointer says just return result
	if (event)
	{
		if (ret)
			memcpy((memptr) event, (memptr) found, sizeof(REventRecord));
		else
			MakeNullEvent(event);
	}

	return(ret);
}


//look for any mouse ups
bool RStillDown()
{
	return !REventAvail(mouseUp, 0);
}


//add event to evQueue at evTail, bump evHead if == evTail
void RPostEvent(REventRecord *event)
{
	event->when = RTickCount();
	memcpy((memptr) evTail, (memptr) event, sizeof(REventRecord));
	evTail = bump(evTail);
	if (evTail == evHead) //throw away oldest
		evHead = bump(evHead);
}


//give him current stuff
void MakeNullEvent(REventRecord *event)
{
	event->type = 0;
	CurMouse(&(event->where));
	event->modifiers = GetModifiers();
}


KERNEL(MapKeyToDir)
{
	Obj *SCIEvent;
	REventRecord event;

	SCIEvent = (Obj*)Native(*(args + 1));
	ObjToEvent(SCIEvent, &event);
	MapKeyToDir(&event);
	EventToObj(&event, SCIEvent);
	acc = Pseudo(SCIEvent);
}


REventRecord* MapKeyToDir(REventRecord *event)
{
	KeyboardDriver(INP_MAP, event);
	return event;
}


void EventToObj(REventRecord *evt, Obj *evtObj)
{
	IndexedProp(evtObj, evType) = evt->type;
	IndexedProp(evtObj, evMod) = evt->modifiers;
	IndexedProp(evtObj, evMsg) = evt->message;
	IndexedProp(evtObj, evX) = evt->where.h;
	IndexedProp(evtObj, evY) = evt->where.v;
}


void ObjToEvent(Obj *evtObj, REventRecord *evt)
{
	evt->type = IndexedProp(evtObj, evType);
	evt->modifiers = IndexedProp(evtObj, evMod);
	evt->message = IndexedProp(evtObj, evMsg);
	evt->where.h = IndexedProp(evtObj, evX);
	evt->where.v = IndexedProp(evtObj, evY);
}


KERNEL(Joystick)
{
	switch (arg(1))
	{
		case JOY_RPT_INT:
			acc = JoystickDriver(JOY_RPT_INT, arg(2));
	}
}


//move evQueue pointer to next slot
static REventRecord* near bump(REventRecord *ptr)
{
	if (++ptr == evQueueEnd)
		ptr = evQueue;
	return ptr;
}

