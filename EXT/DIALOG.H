//KAWA WAS HERE
//-------------
// > General cleanup

#ifndef DIALOG_H
#define DIALOG_H

#include "types.h"
#include "grtypes.h"
#include "window.h"
#include "event.h"
#include "pmachine.h"
#include "kernel.h"
#include "intrpt.h"

#define MARGIN 4

extern void InitDialog(boolfptr);
extern void RHiliteControl(Obj*);
extern word *DrawControl(Obj*);
extern word EditControl(Obj*, Obj*);
extern int GetLine(strptr, strptr, int); //accept or modify the string in buffer
extern void RGlobalToLocal(RPoint*);
extern void RLocalToGlobal(RPoint*);
extern int EditText(RRect*, strptr, int, int, REventRecord*);
extern void DrawCursor(RRect*, strptr, int);
extern void EraseCursor(void);

#endif

