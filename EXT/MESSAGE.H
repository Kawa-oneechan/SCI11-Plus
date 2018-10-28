// message.h

//KAWA WAS HERE
//-------------
// > General cleanup

#if !defined(MESSAGE_H)
#define MESSAGE_H

#if !defined(TYPES_H)
	#include "types.h"
#endif

#define MessageMajorVersion	4000

typedef struct {
	byte noun;
	byte verb;
	byte cond;
	byte seq;
	byte talker;
	uint offset;
	byte refNoun;
	byte refVerb;
	byte refCond;
	byte refSeq;
} IndexEntry;

typedef struct
{
	ulong version;
	uint length;	// size of file starting at count field, excluding comments
	uint lastMsgNum;
	uint count;
	IndexEntry entries[1];
} MsgData;

#endif

