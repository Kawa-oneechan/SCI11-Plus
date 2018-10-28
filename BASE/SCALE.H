/*  Scale.h */

//KAWA WAS HERE
//-------------
// > General cleanup

#include "vm_defs.h"

extern unsigned short vm_deltaxs[MAXWIDTH], vm_deltays[MAXHEIGHT];

extern void vm_size(word, word, word, word, word, word, word*, word*, word*, word*);
extern void GetNewScaleFactors(Obj*, int, word*, word*);

