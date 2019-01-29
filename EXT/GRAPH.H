//KAWA WAS HERE
//-------------
// > General cleanup

#ifndef GRAPH_H
#define GRAPH_H

#ifndef GRTYPES_H
#include "grtypes.h"
#endif
#ifndef PALETTE_H
#include "palette.h"
#endif

struct MagCursorStruct
{
	Hunkptr picture;
	Hunkptr cursor;
	byte replaceColor;
	int curTop;
	int curLeft;
	int curBottom;
	int curRight;
	int picXDim;
	int magLevel;
};


struct pseudoCursor
{
	int xDim;
	int yDim;
	int xHot;
	int yHot;
	byte skipColor;
};

extern word NumberColors;
extern byte currentVideoMode;
extern RGrafPort *rThePort;
extern char palVaryOn;
extern char palVaryPause;
extern word palettePercent;
extern word paletteDir;
extern word paletteTime;
extern word paletteStop;
extern word paletteRes;
extern Handle startPalette;
extern Handle targetPalette;
extern Handle newPalette;
extern word NumberColors;
extern char *theFont;
extern short wordBits[];

void GetCLUT(RPalette far*);
void SetCLUT(RPalette far*, int);
int FastMatch(RPalette far*, int, int, int, int, int);
KERNEL(Graph);
void GetCursorData(char*);
void MoveCursorData(char*);
void MoveCursor(int, int);
void SetCursorData(char*);
bool RIsItSkip(Handle viewHandle, uint loop, uint cel, int vOffset, int hOffset);
void UnloadBits(Handle handle);
void PaletteServer(void);
void PaletteUpdate(int, int);
void PaletteCheck(void);
void KillPalServer(void);
int PaletteShell(RPalette far*);

#endif

