#include "io.h"
#include "stdio.h"
#include "fileio.h"
#include "memmgr.h"
#include "intrpt.h"
#include "fli.h"

#define FLI_COLOR256	4
#define FLI_SS2			7
#define FLI_COLOR		11
#define FLI_LC			12
#define FLI_BLACK		13
#define FLI_BRUN		15
#define FLI_COPY		16
#define FLI_PSTAMP		18

#pragma warning (disable: 4759)

char far* vidya = (char far*)0xA0000000l;

typedef struct
{
	unsigned long size, type, numchunks;
} FLI_Frame;

typedef struct
{
	unsigned long size, type, index;
} FLI_Chunk;

static unsigned char readu8(FLI_Animation *flic)
{
	unsigned char b;
	read(flic->rwops, &b, 1);
	return b;
}

static unsigned short readu16(FLI_Animation *flic)
{
	unsigned short w;
	read(flic->rwops, &w, 2);
	return w;
}

static unsigned long readu32(FLI_Animation *flic)
{
	unsigned long u;
	read(flic->rwops, &u, 4);
	return u;
}

static void memset(int* ptr, int value, unsigned long num)
{
	while (num--)
		*ptr++ = value;
}

static int readheader(FLI_Animation *flic)
{
	lseek(flic->rwops, 4, LSEEK_CUR);

	flic->format = readu16(flic);
	if (flic->format != FLI_FLI && flic->format != FLI_FLC)
		return FLI_CORRUPTEDFILE;

	//Maximum is 4000 for FLI and FLC files.
	flic->numframes = readu16(flic);
	if (flic->numframes > 4000)
		return FLI_CORRUPTEDFILE;

	//Must be 320x200 for FLI files.
	flic->width = readu16(flic);
	flic->height = readu16(flic);
	if (flic->format == FLI_FLI && (flic->width != 320 || flic->height != 200))
		return FLI_CORRUPTEDFILE;

	flic->depth = readu16(flic);
	if (flic->depth != 8)
		return FLI_CORRUPTEDFILE;

	//Skip the flags, it doesn't look like it follows the specs.
	readu16(flic);

	flic->delay = (flic->format == FLI_FLI) ? readu16(flic) : readu32(flic);
	lseek(flic->rwops, (flic->format == FLI_FLI) ? 110 : 108, LSEEK_CUR);
}

static int readframe(FLI_Animation *flic, FLI_Frame *frame)
{
	//Must be less than or equal to 64k in FLI files.
	frame->size = readu32(flic);
	if (flic->format == FLI_FLI && frame->size > 65536)
		return FLI_CORRUPTEDFILE;

	//Must be 0xF1FA in FLI files or 0xF1FA or 0xF100 in FLC files.
	frame->type = readu16(flic);
	if (frame->type != 0xF1FA && (flic->format == FLI_FLC && frame->type != 0xF100))
		return FLI_CORRUPTEDFILE;

	frame->numchunks = readu16(flic);
	lseek(flic->rwops, 8, LSEEK_CUR);
}

static void readchunk(FLI_Animation *flic, FLI_Chunk *chunk)
{
	chunk->size = readu32(flic);
	chunk->type = readu16(flic);
}

static void handlecolor(FLI_Animation *flic, FLI_Chunk *chunk)
{
	int numpackets, index, count;
	unsigned char r, g, b;

	(void)chunk;
	numpackets = readu16(flic);
	index = 0;
	while (numpackets-- > 0)
	{
		//Skip some colors.
		index += readu8(flic);
		//And change some others.
		count = readu8(flic);
		if (count == 0)
			count = 256;
		while (count-- > 0)
		{
			//r, g and b are in the range [0..63].
			r = (char)((unsigned long)readu8(flic)) * 255 / 63;
			g = (char)((unsigned long)readu8(flic)) * 255 / 63;
			b = (char)((unsigned long)readu8(flic)) * 255 / 63;
			//SDL_SetColors(flic->surface, &color, index++, 1);
		}
	}
}

static void handlelc(FLI_Animation *flic, FLI_Chunk *chunk)
{
	int numlines, numpackets, size;
	unsigned char *line, *p;

	(void)chunk;
	//Skip lines at the top of the image.
	line = (unsigned char*)vidya + readu16(flic) * 320;
	numlines = readu16(flic);
	while (numlines-- > 0)
	{
		p = line;
		line += 320;
		//Each line has numpackets changes.
		numpackets = readu8(flic);
		while (numpackets-- > 0)
		{
			//Skip pixels at the beginning of the line.
			p += readu8(flic);
			//size pixels will change.
			size = (char)readu8(flic);
			if (size >= 0)
			{
				read(flic->rwops, (void*)p, size);
			}
			else
			{
				size = -size;
				memset((void*)p, readu8(flic), size);
			}
			p += size;
		}
	}
}

static void handleblack(FLI_Animation *flic, FLI_Chunk *chunk)
{
	(void)chunk;
	memset((void*)vidya, 0, 320ul * 200ul);
	//if (SDL_FillRect(flic->surface, NULL, 0) != 0)
	//	longjmp(flic->error, FLI_SDLERROR);
}

static void handlebrun(FLI_Animation *flic, FLI_Chunk *chunk)
{
	unsigned long numlines;
	signed size;
	unsigned char *p, *next;

	(void)chunk;
	p = (unsigned char*)vidya;
	numlines = flic->height;
	while (numlines-- > 0)
	{
		//The number of packages is ignored, packets run until the next line is reached.
		readu8(flic);
		next = p + 320;
		while (p < next)
		{
			size = (char)readu8(flic);
			if (size < 0)
			{
				size = -size;
				read(flic->rwops, (void*)p, size);
			}
			else
			{
				memset((void*)p, readu8(flic), size);
			}
			p += size;
		}
	}
}

static void handlecopy(FLI_Animation *flic, FLI_Chunk *chunk)
{
	(void)chunk;
	read(flic->rwops, (void*)vidya, flic->width * flic->height);
}

static int handlecolor256(FLI_Animation *flic, FLI_Chunk *chunk)
{
	int numpackets, index, count;
	unsigned char r, g, b;

	(void)chunk;
	if (flic->format == FLI_FLI)
		return FLI_CORRUPTEDFILE;

	numpackets = readu16(flic);
	index = 0;
	while (numpackets-- > 0)
	{
		//Skip some colors.
		index += readu8(flic);
		//And change some others.
		count = readu8(flic);
		if (count == 0)
			count = 256;
		while (count-- > 0)
		{
			//r, g and b are in the range [0..255].
			r = readu8(flic);
			g = readu8(flic);
			b = readu8(flic);
			//SDL_SetColors(flic->surface, &color, index++, 1);
		}
	}
}

static int handless2(FLI_Animation *flic, FLI_Chunk *chunk)
{
	int   numlines, y, code, size;
	unsigned char *p;

	(void)chunk;
	if (flic->format == FLI_FLI)
		return FLI_CORRUPTEDFILE;

	numlines = readu16(flic);
	y = 0;
	while (numlines > 0)
	{
		code = readu16(flic);
		switch ((code >> 14) & 0x03)
		{
			case 0x00:
				p = (unsigned char*)vidya + 320 * y;
				while (code-- > 0)
				{
					p += readu8(flic);
					size = ((char)readu8(flic)) * 2;
					if (size >= 0)
					{
						read(flic->rwops, (void*)p, size);
					}
					else
					{
						size = -size;
						readu8(flic);
						memset((void*)p, readu8(flic), size);
					}
					p += size;
				}
				y++;
				numlines--;
				break;
			case 0x01:
				return FLI_CORRUPTEDFILE;
			case 0x02:
				p = (unsigned char*)vidya + 320 * (y + 1);
				p[-1] = code & 0xFF;
				break;
			case 0x03:
				y += (code ^ 0xFFFF) + 1;
				break;
		}
	}
}

FLI_Animation *FLI_Open(int rwops, int *error)
{
	FLI_Animation *flic;
	FLI_Frame frame;
	int err;

	flic = (FLI_Animation*)NeedPtr(sizeof(FLI_Animation));
	if (flic == 0)
	{
		if (error != 0) *error = FLI_OUTOFMEMORY;
		return 0;
	}
	flic->rwops = rwops;

	readheader(flic);
	//flic->surface = SDL_CreateRGBSurface(SDL_SWSURFACE, flic->width, flic->height, 8, 0, 0, 0, 0);
	//if (flic->surface == NULL)
	//	longjmp(flic->error, FLI_SDLERROR);

	flic->offframe1 = lseek(rwops, 0, LSEEK_CUR);
	readframe(flic, &frame);

	if (frame.type == 0xF100)
	{
		lseek(rwops, frame.size - 16, LSEEK_CUR);
		flic->offframe1 = lseek(rwops, 0, LSEEK_CUR);
		flic->numframes--;
	}

	flic->offnextframe = flic->offframe1;
	flic->nextframe = 1;
	if (error != NULL) *error = FLI_OK;
	return flic;
}

void FLI_Close(FLI_Animation *flic)
{
	if (flic != NULL)
	{
		if (flic->rwops != NULL)
			close(flic->rwops);
		DisposePtr(flic);
	}
}

int FLI_NextFrame(FLI_Animation *flic)
{
	FLI_Frame frame;
	FLI_Chunk chunk;
	unsigned long i;

	lseek(flic->rwops, flic->offnextframe, LSEEK_BEG);
	readframe(flic, &frame);
	//SDL_LockSurface(flic->surface);
	//locked = 1;
	//(void)locked;
	for (i = frame.numchunks; i != 0; i--)
	{
		readchunk(flic, &chunk);
		switch (chunk.type) {
			case FLI_COLOR:
				handlecolor(flic, &chunk);
				break;
			case FLI_LC:
				handlelc(flic, &chunk);
				break;
			case FLI_BLACK:
				handleblack(flic, &chunk);
				break;
			case FLI_BRUN:
				handlebrun(flic, &chunk);
				break;
			case FLI_COPY:
				handlecopy(flic, &chunk);
				break;
			case FLI_COLOR256:
				handlecolor256(flic, &chunk);
				break;
			case FLI_SS2:
				handless2(flic, &chunk);
				break;
			case FLI_PSTAMP:
				break;
			default:
				return FLI_CORRUPTEDFILE;
		}
	}
	//SDL_UnlockSurface(flic->surface);

	//Setup the number and position of next frame. If it wraps, go to the first one.
	/* if (++flic->nextframe > flic->numframes)
	{
		flic->offnextframe = flic->offframe1;
		flic->nextframe = 1;
	}
	else */
		flic->offnextframe += frame.size;
	return FLI_OK;
}

int FLI_Rewind(FLI_Animation *flic)
{
	flic->offnextframe = flic->offframe1;
	flic->nextframe = 1;
	return FLI_OK;
}

int FLI_Skip(FLI_Animation *flic)
{
	FLI_Frame frame;

	lseek(flic->rwops, flic->offnextframe, LSEEK_BEG);
	readframe(flic, &frame);
	if (++flic->nextframe > flic->numframes)
	{
		flic->offnextframe = flic->offframe1;
		flic->nextframe = 1;
	}
	else
		flic->offnextframe += frame.size;
	return FLI_OK;
}
#pragma warning (default: 4759)

static void FLI_Server(void);
static void FLI_SetupServer(void);
static void FLI_RemoveServer(void);

FLI_Animation* flic;
static long startTick;
char movieOn;

#define FLI_FRAMERATE 10

static void FLI_Server(void)
{
	long tick;
	if (movieOn == 0) return;
	tick = RTickCount();
	if (tick >= startTick + FLI_FRAMERATE) //time for display (10/second)
	{
		FLI_NextFrame(flic);
		startTick = tick;

		if (flic->nextframe < flic->numframes)
			FLI_RemoveServer();
	}
}

static void FLI_SetupServer(void)
{
	startTick = 0;
	movieOn = 1;
	InstallServer(FLI_Server, 1);
}

static void FLI_RemoveServer(void)
{
	movieOn = 0;
	DisposeServer(FLI_Server);
}

int FLI_JustTryToPlay(char* file)
{
	int error;
	int rwops = open(file, O_RDONLY);
	if (rwops == -1)
		return -1;
	flic = FLI_Open(rwops, &error);
	if (error)
	{
		close(rwops);
		DoPanic("NOPE!");
	}
	FLI_SetupServer();
	do
	{
		//...
	} while (movieOn);
	FLI_Close(flic);
	close(rwops);
}
