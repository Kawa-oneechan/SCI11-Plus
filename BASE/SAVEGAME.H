//KAWA WAS HERE
//-------------
// > General cleanup

#ifndef SAVEGAME_H
#define SAVEGAME_H

#ifndef TYPES_H
	#include "types.h"
#endif

ulong GetSaveLength(void);
void MakeFileName(strptr where, strptr name, int num);
void MakeDirName(strptr where, strptr name);
KERNEL(SaveGame);
KERNEL(RestoreGame);
KERNEL(GetSaveFiles);
KERNEL(GetSaveDir);
KERNEL(CheckSaveGame);

extern char	saveDir[];

#endif

