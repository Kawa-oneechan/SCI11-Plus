//script.c
//	routines to handle loading & unloading script modules.

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments
// > DoFixups had segment loss -- CAN'T FIX

/*	The hunk resource has the following format:

		------------------------------
		word: offset of beginning of fixups
		------------------------------
		word: address of script node
		------------------------------
		word: boolean: script has associated far text
		------------------------------
		word: length of dispatch table
		------------------------------
		dispatch table
		------------------------------
		code
		------------------------------
		word: number of fixups
		------------------------------
		fixups
		------------------------------

	The heap resource has the following format:

		------------------------------
		word: offset of beginning of fixups
		------------------------------
		word: number of variables
		------------------------------
		variables
		------------------------------
		objects
		classes
		meta-strings
		near strings
		------------------------------
		word: number of fixups
		------------------------------
		fixups
		------------------------------
*/

#include "debug.h"
#include "errmsg.h"
#include "kernel.h"
#include "pmachine.h"
#include "resource.h"
#include "selector.h"

static void near InitHeapRes(Handle h, Script *sp);
static void near InitHunkRes(Handle h, Script *sp);
static void near InitObj(Obj *op, Script *sp);
static void near DoFixups(uword _far *fixTbl, byte _far *fixBase, byte _far *fixOfs, int sign);
static void near TossScriptObjects(Script *sp);
static Script* near FindScript(int n);
static void near TossScript(Script *sp, bool checkClones);
static void near TossScriptClasses(uint n);
static Obj* near GetClass(int n);


//Initialize the list of loaded scripts.
void InitScripts()
{
	InitList(&scriptList);
}


//Return a pointer to the node for script n. Load the script if necessary.
Script* ScriptPtr(int n)
{
	Script *sp;

	if ((sp = FindScript(n))== NULL)
		sp = LoadScript(n);

	return sp;
}


//Load script number n in hunk space, copy it to the heap, allocate and
//clear its local variables, and return a pointer to its script node.
Script* LoadScript(int n)
{
	Script *sp;
	Handle heap;
	Handle hunk;

	//Allocate and link a new node into the list.
	sp = (Script*)NeedPtr(sizeof(Script));
	ClearPtr(sp);
	AddToFront((List*)&scriptList, Pseudo(sp), n);

	//Load the heap and hunk resources.
	heap = ResLoad(RES_HEAP, n);
	hunk = ResLoad(RES_SCRIPT, n);
	ResLock(RES_SCRIPT, n, true);
	InitHeapRes(heap, sp);
	InitHunkRes(hunk, sp);

	return sp;
}


void ReloadScript(Script *sp)
{
	uint n;
	Handle hunk;

	n = ScriptNumber(sp);
	hunk = ResLoad(RES_SCRIPT, n);
	ResLock(RES_SCRIPT, n, true);
	InitHunkRes(hunk, sp);
}


static void near InitHeapRes(Handle h, Script *sp)
{
	HeapRes _far *heapRes = (HeapRes _far*)*h;
	HeapRes *heap;
	uint heapLen;
	Obj *op;

	//The first word of the heap resource is the offset to the fixup tables
	//for the resource (which are at the end of the resource). This is thus
	//the length of the heap resource. Allocate the space for the heap
	//component and copy it into the space.
	heapLen = heapRes->fixOfs;
	heap = (HeapRes*)NeedPtr(heapLen);
	sp->heap = (HeapRes*)hunkcpy(heap, heapRes, heapLen);
	sp->vars = &heap->vars;

	//Do the necessary fixups on the portion loaded into heap.
	DoFixups((uword _far*)((char _far*)heapRes + heapLen), (byte _far*)heap, (byte _far*)heap, 1);

	//Now initialize the objects in this heap resource.
	for (op = (Obj*)(&heap->vars + heap->numVars); op->objID == OBJID; op = (Obj*)((uword*)op + op->size))
		InitObj(op, sp);
}


static void near InitObj(Obj *op, Script *sp)
{
	Obj *cp;

	//Set the address of the object's superclass (the compiler puts the
	//class number of the superclass in the -super- property)
	op->super = Pseudo(GetClass(op->super));

	if (op->script == OBJNUM)
	{
		//An object's -classScript- points to its superclass's script.
		cp = (Obj*)Native(op->super);
		op->propDict = cp->propDict;
		op->classScript = cp->script;
	}
	else
	{
		//If the object is a class, set its address in the class table (the
		//class number is stored in the -script- property by the compiler).
		classTbl[op->script].obj = Pseudo(op);
		op->classScript = Pseudo(sp);
	}

	//Set the object's '-script-' property to the address of the script
	//node being loaded.
	op->script = Pseudo(sp);
}


static void near InitHunkRes(Handle h, Script *sp)
{
	HunkRes _far *hunk;
	uword _far *fp;

	sp->hunk = h;
	hunk = (HunkRes _far*)*sp->hunk;

	//Point to the script node for this hunk.
	hunk->script = Pseudo(sp);

	//Do the fixups in the hunk resource.
	fp = (uword _far*)(((byte _far*)hunk)+ hunk->fixOfs);
	DoFixups(fp, (byte _far*)hunk, (byte _far*)sp->heap, 1);

	if (hunk->farText)
		ResLoad(RES_TEXT, ScriptNumber(sp));
}


#pragma warning (disable: 4759)//KAWA WAS HERE -- can't FIX this "segment lost in conversion", but I can HIDE it ;)
static void near DoFixups(uword _far *fixTbl, byte _far *fixBase, byte _far *fixOfs, int sign)
{
	uint numFix;
	uword _far *hp;

	for (numFix = *fixTbl++; numFix--; )
	{
		//get pointer to word to be fixed up
		hp = (uword _far*)(fixBase + *fixTbl++);
		*hp = sign > 0 ? Pseudo(*hp + fixOfs):
		(ObjID)((long)*hp - (long)fixOfs);
	}
}
#pragma warning (default: 4759)


//Dispose of the entire script list (for restart game).
void DisposeAllScripts()
{
	Script *sp;

	for (sp = (Script*)Native(FirstNode(&scriptList)); !EmptyList((List*)&scriptList); sp = (Script*)Native(FirstNode(&scriptList)))
		TossScript(sp, false);
}


//Remove script n from the active script list and deallocate the space taken by its code and variables.
void DisposeScript(int n)
{
	Script *sp;

	if ((sp = FindScript(n))!= NULL)
		TossScript(sp, true);
}


static void near TossScript(Script *sp, bool checkClones)
{
	int n = ScriptNumber(sp);

	//Undo the fixups in the hunk resource and then unlock it.
	ResetHunk((HunkRes _far*)*sp->hunk);
	ResLock(RES_SCRIPT, n, false);

	//Dispose the heap resource.
	TossScriptClasses(n);
	TossScriptObjects(sp);
	if (sp->heap)
		DisposePtr(sp->heap);
	if (checkClones && (sp->clones))
		PError(thisIP, pmsp, E_LEFT_CLONE, n, 0);
	DeleteNode((List*)&scriptList, Pseudo(sp));
	DisposePtr(sp);
}


void ResetHunk(HunkRes _far *hunk)
{
	Script *sp;
	uword _far *fp;

	sp = (Script*)Native(hunk->script);
	if (hunk->script)
	{
		hunk->script = 0;
		fp = (uword _far*)(((byte _far*)hunk)+ hunk->fixOfs);
		DoFixups(fp, (byte _far*)hunk, (byte _far*)sp->heap, -1);
	}
}


//Return a pointer to the node for script n if it is in the script list, or NULL if it is not in the list.
static Script* near FindScript(int n)
{
	return (Script*)Native(FindKey(&scriptList, n));
}


static Obj* near GetClass(int n)
{
	if (n == -1)
		return NULL;

	if (!classTbl[n].obj)
	{
		LoadScript(classTbl[n].scriptNum);
		if (!classTbl[n].obj)
			PError(thisIP, pmsp, E_LOAD_CLASS, n, 0);
	}

	return (Obj*)Native(classTbl[n].obj);
}


//Remove all classes belonging to script number n from the class table.
static void near TossScriptClasses(uint n)
{
	uint i;

	for (i = 0 ; i < numClasses ; ++i)
		if (classTbl[i].scriptNum == n)
			classTbl[i].obj = 0;
}


//Scan through the script unmarking the objects.
static void near TossScriptObjects(Script *sp)
{
	Obj *op;

	for (op = (Obj*)(&sp->heap->vars + sp->heap->numVars); op->objID == OBJID; op = (Obj*)((uword*)op + op->size))
		op->objID = 0;
}


KERNEL(DisposeScript)
{
	//allow for return code calculation on disposescript
	//This prevents return
	//(DisposeScript self)
	//(return value)
	//from getting "executing in disposed script" error
	//by using
	//(DisposeScript self value)

	if (argCount == 2)
		acc = arg(2);
	DisposeScript(arg(1));
}

