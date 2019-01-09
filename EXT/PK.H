//KAWA WAS HERE
//-------------
// > General cleanup

#if !defined(PK_H)
#define PK_H

#if !defined(TYPES_H)
	#include "types.h"
#endif

extern Handle dcompBuffer;

void pkExplode(int fd, byte far *dst, unsigned length);
void pkImplode(int fd, byte far *src, unsigned length);
uint pkImplode2Mem(byte far *dst, byte far *src, unsigned length);

#endif

