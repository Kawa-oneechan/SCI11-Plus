// SCI.H

//KAWA WAS HERE
//-------------
// > General cleanup

#ifndef SCI_H
#define SCI_H

#ifndef SETJMP_H
	#include "setjmp.h"
#endif

extern jmp_buf restartBuf;
extern char whereDisk;
extern char where[];

void main(int argc, char *argv[]);

#endif

