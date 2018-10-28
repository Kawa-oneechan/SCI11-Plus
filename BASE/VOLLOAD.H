//KAWA WAS HERE
//-------------
// > General cleanup

#ifndef VOLLOAD_H
#define VOLLOAD_H

#include "types.h"

extern Handle LoadResMap(strptr);
extern bool FindDirEntry(ulong*, byte, uword, int*);
extern bool FindDirEntryMap(ulong*, byte, uword, Handle);
extern Handle DoLoad(byte, uint);
extern void InitResource(strptr);

#endif

