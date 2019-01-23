//KAWA WAS HERE
//-------------
// > General cleanup

#ifndef SETJMP_H
#define SETJMP_H

#ifndef TYPES_H
	#include "types.h"
#endif

#define JMPBUFSIZE 14

typedef uint jmp_buf[JMPBUFSIZE];

int setjmp(jmp_buf); 		//in startasm.s
void longjmp(jmp_buf, int);	//in startasm.s

#endif

