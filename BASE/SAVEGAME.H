//KAWA WAS HERE
//-------------
// > General cleanup

#if !defined(SAVEGAME_H)
#define SAVEGAME_H

#if !defined(TYPES_H)
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

