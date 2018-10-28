//	start.c
//		startup and shutdown routines

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments

#include "ctype.h"
#include "start.h"
#include "string.h"

//KAWA
#include "kawa.h"
#include "restypes.h"
#include "resource.h"

#define MAX_ON_EXIT	20

//initialize explicitly because SCI won't have done it for us by now
char* argv[10] = { 0 };
int	argc = 0;

static int exitIndex;
static fptr exitProcs[MAX_ON_EXIT];

static void	WriteString(char* str);


void MakeCommandLineArgs(char _far* cmdTail)
{
	int nChars = *cmdTail++;
	char* cp;
	char far* fcp;
	int i;

	if (!nChars)
	return;

	//copy command line to heap
	fcp = cmdTail;
	cp = commandLine;
	for (i = 0; i < nChars; i++)
		*cp++ = *fcp++;
	*cp = 0;

	argv[argc++] = "sci";

	for (cp = commandLine; *cp; )
	{
		//strip leading whitespace
		while (*cp && isspace(*cp))
			cp++;
		if (!*cp)
			break;

		//set argument
		argv[argc++] = cp;

		//find argument end
		while (*cp && !isspace(*cp))
			cp++;
		if (!*cp)
			break;

		//terminate argument
		*cp++ = 0;
	}
}


//Add a procedure to the list of those to be executed on exit
void onexit(fptr func)
{
	if (exitIndex <= MAX_ON_EXIT - 1)
		exitProcs[exitIndex++] = func;
}


void exit(char code)
{
	int i;
#if defined(ENDOOM)
	Handle hB800;
	volatile char far* vidya = (volatile char far*)0xB8000000;
	char far* b800;
	char x, y;
#endif

	for (i = exitIndex - 1; i >= 0; i--)
		exitProcs[i]();

	if (panicStr)
		WriteString(panicStr);
#if defined(ENDOOM)
	else
	{
		if (ResCheck(RES_VOCAB, 0xB8))
		{
			hB800 = ResLoad(RES_VOCAB, 0xB8);
			b800 = (char far*)*hB800;
			for (i = 0; i < 80*25*2; i++)
				*vidya++ = *b800++;
			x = *b800++; y = *b800++;
			if (quitStr)
			{
				gotoxy(x, y);
				WriteString(quitStr);
			}
			x = *b800++; y = *b800++;
			gotoxy(x, y); //note: prompt appears at the *next* line.
		}
		else if (quitStr)
			WriteString(quitStr);
//		else
//			WriteString("(note: no endoom found, no quitstring set.)");
//			WriteString("Couldn't load endscreen.");
	}
#else
	else if (quitStr)
		WriteString(quitStr);
#endif

	ExitFromC(code);
}


//write a text string to the screen on exit
static void WriteString(char* str)
{
	while (*str)
	{
		if (*str == '\n')
			WriteChar('\r');
		WriteChar(*str);
		++str;
	}
}

