//KAWA WAS HERE
//-------------
// > General cleanup

#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"
#include "grtypes.h"
#include "window.h"
#include "palette.h"

extern bool findRet;
extern int newRoomNum;
extern bool traceOn;
extern bool trace;
extern bool proOn;
extern uword *lastSp;
extern Handle proBuffer;
extern bool isDebugVersion;
extern uint proIndex;
extern uint proBufSize;
extern bool trackResUse;
extern bool trackHunkUse;

extern KERNEL(SetDebug);
extern KERNEL(ShowSends);
extern KERNEL(ShowFree);
extern KERNEL(InspectObj);

extern void ShowSends(bool);
extern void WriteResUse(int, int);
extern void SetDebug(bool);
extern void CheckHunkUse(uint);
extern void Debug(Hunkptr, uword*);
extern void ToggleDebug(void);
extern void Resources(void);
extern RWindow* DebugWindow(strptr, strptr, int, int, int, bool);
extern RWindow* SizedWindow(strptr title, strptr data, bool wait);
extern void PError(memptr, uword*, int, uint, uint);
extern strptr ArgNameRead(strptr);
extern char* ReadMsg(int);

#endif

