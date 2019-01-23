// FARDATA.H

//KAWA WAS HERE
//-------------
// > General cleanup

#ifndef FARDATA_H
#define	FARDATA_H

#ifndef TYPES_H
	#include "types.h"
#endif

int GetFarData(int moduleNum, int entryNum, memptr buffer);
strptr GetFarStr(int moduleNum, int entryNum, strptr buffer);
KERNEL(GetFarText);
strptr GetFarText(uint module, uint offset, strptr buffer);

#endif

