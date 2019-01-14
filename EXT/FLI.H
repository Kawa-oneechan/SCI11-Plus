#define FLI_FLI 0xAF11
#define FLI_FLC 0xAF12

#define FLI_OK 0
#define FLI_READERROR 1
#define FLI_CORRUPTEDFILE 2
#define FLI_OUTOFMEMORY 4

typedef struct
{
	unsigned long format, numframes, width, height, depth, delay, offframe1, nextframe, offnextframe;
	int rwops;
} FLI_Animation;

extern FLI_Animation *FLI_Open(int rwops, int *error);
extern void FLI_Close(FLI_Animation *flic);
extern int FLI_NextFrame(FLI_Animation *flic);
extern int FLI_Rewind(FLI_Animation *flic);
extern int FLI_Skip(FLI_Animation *flic);

extern int FLI_JustTryToPlay(char* file);