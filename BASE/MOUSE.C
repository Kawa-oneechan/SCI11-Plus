//	mouse.c

//KAWA WAS HERE
//-------------
// > General cleanup -- just some tabs.

#include "graph.h"
#include "mouse.h"

bool mouseIsDebug = FALSE;
word buttonState = 0;


//Return interrupt level position in local coords of mouse in the point
word RGetMouse(RPoint *pt)
{
	pt->v = mouseY - rThePort->origin.v;
	pt->h = mouseX - rThePort->origin.h;
	return buttonState;
}


//Return interrupt level position in global coords of mouse in the point
word CurMouse(RPoint *pt)
{
	pt->v = mouseY;
	pt->h = mouseX;
	return buttonState;
}

