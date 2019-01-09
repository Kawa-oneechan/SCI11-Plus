// SYNC.H

//KAWA WAS HERE
//-------------
// > General cleanup

#if !defined(SYNC_H)
#define SYNC_H

#if !defined(TYPES_H)
	#include "types.h"
#endif
#if !defined(OBJECT_H)
	#include "object.h"
#endif

//sync cue structure
typedef	struct Sync
{
	uint time;	//absolute frame time to cue
	uint cue;	//cue number to send at frame time
} Sync;

enum syncFuncs
{
	STARTSYNC,
	NEXTSYNC,
	STOPSYNC,
	QUEUESYNC
};

KERNEL(DoSync);
void StartSync(Obj *theSync, uint num);
void StartSync36(Obj*, uint, byte, byte, byte, byte);
void QueueSync(Obj *theSync, uint num);
void QueueSync36(Obj*, uint, byte, byte, byte, byte);
void NextSync(Obj *theSync);
void StopSync(void);
ulong FindSync36Entry(uint, byte, byte, byte, byte, uint*);

#endif

