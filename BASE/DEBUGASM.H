//	debugasm.h

//KAWA WAS HERE
//-------------
// > General cleanup

#ifndef DEBUGASM_H
#define DEBUGASM_H

void DebugInit(void);
void DebugOn(void);
void DebugOff(void);
void RDebugger(void);
KERNEL(StackUsage);
uint StackUsage(int);
void ShowFreeList(strptr);
void ProfileHandler(void);

#endif

