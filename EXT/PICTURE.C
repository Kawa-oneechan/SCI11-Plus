/* PICTURE - Certain picture related functions */

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments

#include "picture.h"
#include "kernel.h"
#include "grtypes.h"
#include "palette.h"
#include "intrpt.h"
#include "graph.h"
#include "window.h"
#include "resource.h"

RWindow *picWind;
RRect picRect = { 10, 0, 200, 320 };
word picNotValid = 0;
word showStyle = 0;
uword showMap = VMAP;

static byte priTable[200];
static word priHorizon, priBottom;

static void near HWipe(int, int, int);
static void near VWipe(int, int, int);
static void near Iris(int, int, int);
static void near Shutter(int, int, int, int);
static void near Dissolve(int, int);
static void near ShowBlackBits(RRect *, word);

global void InitPicture() { }

#define	xOrg 160
#define	yOrg 95

#define	WIPEWIDE 8
#define	WIPEHIGH 5
#define	SCRNHIGH 190
#define	SCRNWIDE 320
#define	XWIPES 40
#define	YWIPES 40

#define	RANDMASK 0x240

global void ShowPic(word mapSet, word style)
{
	RRect r;
	RGrafPort *oldPort;
	int i;
	RPalette workPalette;

	RGetPort(&oldPort);
	RSetPort((RGrafPort*)picWind);

	RHideCursor();

	if (palVaryOn)
	{
		//calculate the new palette
		PaletteUpdate(NOSETCLUT,0); //updates sysPalette
	}

	switch (style & ~PICFLAGS)
	{
		case PIXELDISSOLVE:
			if(style & BLACKOUT)
				Dissolve((mapSet | PDFLAG | PDBOFLAG), true);
			SetCLUT((RPalette far*)&sysPalette, false);
			Dissolve((mapSet | PDFLAG), false);
			break;
			//else fall through and do DISSOLVE
		case DISSOLVE:
			if(style & BLACKOUT)
				Dissolve(mapSet, true);
			SetCLUT((RPalette far*)&sysPalette, false);
			Dissolve(mapSet, false);
			break;

		case IRISOUT:
			if(style & BLACKOUT)
				Iris(-1, mapSet, true);
			SetCLUT((RPalette far*)&sysPalette, false);
			Iris(1, mapSet, 0);
		break;

		case IRISIN:
			if(style & BLACKOUT)
				Iris(1, mapSet, true);
			SetCLUT((RPalette far*)&sysPalette, false);
			Iris(-1, mapSet, 0);
			break;

		case WIPERIGHT:
			if(style & BLACKOUT)
				HWipe(-WIPEWIDE, mapSet, true);
			SetCLUT((RPalette far*)&sysPalette, false);
			HWipe(WIPEWIDE, mapSet, false);
			break;

		case WIPELEFT:
			if(style & BLACKOUT)
				HWipe(WIPEWIDE, mapSet, true);
			SetCLUT((RPalette far*)&sysPalette, false);
			HWipe(-WIPEWIDE, mapSet, false);
			break;

		case WIPEDOWN:
			if(style & BLACKOUT)
				VWipe(-WIPEHIGH, mapSet, true);
			SetCLUT((RPalette far*)&sysPalette, false);
			VWipe(WIPEHIGH, mapSet, false);
			break;

		case WIPEUP:
			if(style & BLACKOUT)
				VWipe(WIPEHIGH, mapSet, true);
			SetCLUT((RPalette far*)&sysPalette, false);
			VWipe(-WIPEHIGH, mapSet, false);
			break;

		case HSHUTTER:
			if(style & BLACKOUT)
				Shutter(-WIPEWIDE, 0, mapSet, true);
			SetCLUT((RPalette far*)&sysPalette, false);
			Shutter(WIPEWIDE, 0, mapSet, false);
			break;

		case VSHUTTER:
			if(style & BLACKOUT)
				Shutter(0, -WIPEHIGH, mapSet, true);
			SetCLUT((RPalette far*)&sysPalette, false);
			Shutter(0, WIPEHIGH, mapSet, false);
			break;

		case FADEOUT:
			//fade out current palette
			GetCLUT((RPalette far*)&workPalette);
			for (i = 100; i >= 0; i -= 10)
			{
				SetPalIntensity((RPalette far*)&workPalette, 1, 255, i);
				SetCLUT((RPalette far*)&workPalette, false);
			}

			//change the visible data
			SetPalIntensity((RPalette far*)&sysPalette, 0, 256, 0);
			SetCLUT((RPalette far*)&sysPalette, false);
			RSetRect(&r, 0, 0, rThePort->portRect.right, rThePort->portRect.bottom);
			ShowBits(&r, mapSet);

			//now ramp-up and assert system palette
			for (i = 10; i <= 100; i += 10)
			{
				SetPalIntensity((RPalette far *) &sysPalette, 0, 256, i);
				SetCLUT((RPalette far*)&sysPalette, false);
			}
			break;

		case SCROLLRIGHT:
		case SCROLLLEFT:
		case SCROLLUP:
		case SCROLLDOWN:
			SetCLUT((RPalette far*)&sysPalette, false);
			ShiftScreen(rThePort->portRect.top + rThePort->origin.v,
				rThePort->portRect.left   + rThePort->origin.h,
				rThePort->portRect.bottom + rThePort->origin.v,
				rThePort->portRect.right  + rThePort->origin.h,
				(style & ~PICFLAGS) - SCROLLRIGHT);
			break;

		default: //Simply draw the whole screen
			SetCLUT((RPalette far*)&sysPalette, false);
			RSetRect(&r, 0, 0, rThePort->portRect.right, rThePort->portRect.bottom);
			ShowBits(&r, mapSet);
			break;
	}

	RShowCursor();
	RSetPort(oldPort);
}


//return priority corresponding to this point
global word CoordPri(int y)
{
	if (y >= priBottom)
		y = priBottom - 1;
	return (int)priTable[y];
}


//return lowest Y of this priority band
global word PriCoord(int p)
{
	word i;

	if (p >= 15)
		return priBottom;

	for (i = 0 ; i < priBottom; i++)
	{
		if (priTable[i] >= (byte) p)
			break;
	}
	return i;
}


//Each priority band to be set in the table is specified in data
global void PriBands(uint far *priPtr)
{
	int	priBands;
	int	priority;
	uint n;

	//set up priTable[], priHorizon, priBottom
	priBands = 14;
	priority = 0;
	n = 0;

	while (priority < priBands)
	{
		while (n < *priPtr)
		{
			priTable[n] = (byte)priority;
			++n;
		}
		++priPtr;
		++priority;
	}
	while (n < 200)
	{
		priTable[n] = (byte)priority;
		++n;
	}

	priBottom = 190;
	priHorizon = PriCoord(1);
}


static void near HWipe(int wipeDir, int mapSet, int black)
{
	RRect r;
	int i;
	ulong ticks;

	RSetRect(&r, 0, 0, WIPEWIDE, rThePort->portRect.bottom);
	if (wipeDir < 0)
		ROffsetRect(&r, rThePort->portRect.right - WIPEWIDE, 0);

	for(i = 0; i < XWIPES; i++)
	{
		if (black)
			ShowBlackBits(&r, mapSet);
		else
			ShowBits(&r, mapSet);
		ROffsetRect(&r, wipeDir, 0);
		//Wait until enough time has passed
		ticks = RTickCount();
		while(ticks == RTickCount());
	}
}


static void near VWipe(int wipeDir, int mapSet, int black)
{
	RRect r;
	int i;
	ulong ticks;

	RSetRect(&r, 0, 0, rThePort->portRect.right, WIPEHIGH);
	if (wipeDir < 0)
		ROffsetRect(&r, 0, rThePort->portRect.bottom - WIPEHIGH);

	for (i = 0; i < YWIPES; i++)
	{
		if (black)
			ShowBlackBits(&r, mapSet);
		else
			ShowBits(&r, mapSet);
		ROffsetRect(&r, 0, wipeDir);
		//Wait until enough time has passed
		ticks = RTickCount();
		while(ticks == RTickCount());
	}
}


static void near Iris(int dir, int mapSet, int black)
{
	RRect r;
	int i,j,done;
	ulong ticks;

	if (dir > 0)
		i = 1;
	else
		i = XWIPES / 2;

	for (done = 0; done < (XWIPES / 2); done++)
	{
		for (j = 0; j < 4; j++)
		{
			//make the rectangle for this show
			switch(j)
			{
				case 0:	//top bar
					RSetRect(&r, 0, 0, WIPEWIDE * i * 2, WIPEHIGH );
					MoveRect(&r, xOrg - (i * WIPEWIDE),  yOrg - (i * WIPEHIGH));
					break;

				case 1:	//left bar
					RSetRect(&r, 0, 0, WIPEWIDE, WIPEHIGH * (2 * (i - 1)));
					MoveRect(&r, xOrg - (i * WIPEWIDE),  yOrg - (i * WIPEHIGH) + WIPEHIGH);
					break;

				case 2:	//bottom bar
					RSetRect(&r, 0, 0, WIPEWIDE * i * 2, WIPEHIGH );
					MoveRect(&r, xOrg - (i * WIPEWIDE),  yOrg + (i * WIPEHIGH) - WIPEHIGH);
					break;

				case 3:	//right bar
					RSetRect(&r, 0, 0, WIPEWIDE, WIPEHIGH * (2 * (i - 1)));
					MoveRect(&r, xOrg + (i * WIPEWIDE) - WIPEWIDE,  yOrg - (i * WIPEHIGH) + WIPEHIGH);
					break;
			}
			if (black)
				ShowBlackBits(&r, mapSet);
			else
				ShowBits(&r, mapSet);
		}
		i += dir;
		//Wait until enough time has passed
		ticks = RTickCount();
		while(ticks == RTickCount());
	}
}


//bring in random rectangles or pixels
static void near Dissolve(int mapSet, int black)
{
	word  i, j, x, y, rand = 1234;
	RRect r;
	ulong ticks;

	if (mapSet & PDFLAG)
	{
		RSetRect(&r, rThePort->portRect.left,  rThePort->portRect.top, rThePort->portRect.right, rThePort->portRect.bottom);
		ShowBits(&r, mapSet);
		return;
	}

	rand &= RANDMASK;
	for (j = 0; j < 64; j++)
	{
		for (i = 0; i < 16; i++)
		{
			if (rand & 1)
			{
				rand /= 2;
				rand ^= RANDMASK;
			}
			else
			{
				rand /= 2;
			}

			//use rand to pick a rectangle
			x = rand % 40;
			y = rand / 40;
			RSetRect(&r, x * 8, y * 8, (x * 8) + 8, (y * 8) + 8);
			if (black)
				ShowBlackBits(&r, mapSet);
			else
				ShowBits(&r, mapSet);
		}
		// Wait until enough time has passed
		ticks = RTickCount();
		while(ticks == RTickCount());
	}

	/* do rectangle 0 */
	RSetRect(&r, 0 * 8, 0 * 8, (0 * 8) + 8, (0 * 8) + 8);
	if (black)
		ShowBlackBits(&r, mapSet);
	else
		ShowBits(&r, mapSet);
}


static void near Shutter(int hDir, int vDir, int mapSet, int black)
{
	RRect r1, r2;
	int i;
	ulong ticks;

	if (hDir)
	{
		//horizontal shutter
		RSetRect(&r1, 0, 0, WIPEWIDE, rThePort->portRect.bottom);
		RSetRect(&r2, 0, 0, WIPEWIDE, rThePort->portRect.bottom);
		if (hDir > 0)
		{
			MoveRect(&r1, xOrg - WIPEWIDE, 0);
			MoveRect(&r2, xOrg, 0);
		}
		else
		{
			MoveRect(&r2, rThePort->portRect.right - WIPEWIDE, 0);
		}
	}
	else
	{
		RSetRect(&r1, 0, 0, rThePort->portRect.right, WIPEHIGH);
		RSetRect(&r2, 0, 0, rThePort->portRect.right, WIPEHIGH);
		if (vDir > 0)
		{
			MoveRect(&r1, 0, yOrg - WIPEHIGH);
			MoveRect(&r2, 0, yOrg);
		}
		else
		{
			MoveRect(&r2, 0, rThePort->portRect.bottom - WIPEHIGH);
		}
	}
	for (i = 0; i < XWIPES / 2; i++)
	{
		if (black)
		{
			ShowBlackBits(&r1, mapSet);
			ShowBlackBits(&r2, mapSet);
		}
		else
		{
			ShowBits(&r1, mapSet);
			ShowBits(&r2, mapSet);
		}

		//move the rectangles
		ROffsetRect(&r1, -(hDir), -vDir);
		ROffsetRect(&r2, hDir, vDir);
		//Wait until enough time has passed
		ticks = RTickCount();
		while(ticks == RTickCount());
	}
}


static void near ShowBlackBits(RRect *r, word mapSet)
{
	Handle saveBits;

	saveBits = SaveBits(r, mapSet);
	RFillRect(r, mapSet, 0, 0, 0);
	ShowBits(r, mapSet);
	RestoreBits(saveBits);
}

