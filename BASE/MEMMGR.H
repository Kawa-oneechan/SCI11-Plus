// MEMMGR.H

//KAWA WAS HERE
//-------------
// > General cleanup
// > Removed 68000 code

#ifndef MEMMGR_H
#define MEMMGR_H

#ifndef TYPES_H
	#include "types.h"
#endif
#ifndef LIST_H
	#include "list.h"
#endif

#define Native(x)		((strptr) (x))
#define NativeFar(x)	((byte _far *) (x))
#define Pseudo(addr)	((ObjID) (addr))

/* These are the 8086 versions.  Since the files are stored in 8086
 * format (low byte first), nothing needs to be done.
 */
//These access a given memory address (directly/through a pointer).
#define	GetWord(w)		(w)
#define	GetWordP(p)		(*((word*)(p)))
#define	GetFarLongP(p)	(*((long _far*)(p)))

enum memFuncs
{
	LARGESTPTR,
	FREEHEAP,
	LARGESTHANDLE,
	FREEHUNK,
	TOTALHUNK
};

//Free list structure for pointers
typedef struct block_s
{
	word size;				//size of pointer
	struct block_s*	next;	//pointer to next free block in list
} block;

extern block *freeList;
extern size_t heapAvail;
extern size_t hunkAvail;
extern memptr heapBase;
extern memptr handleBase;

void InitMem(word maxHunkSize);
Handle GetHandle(void);
void* NeedPtr(size_t);
void* RNewPtr(size_t);
void ClearPtr(void*);
void FillPtr(void*, byte);
void DisposePtr(void*);
void* ReallocatePtr(void*, size_t);
size_t LargestPtr(void);
size_t FreeHeap(void);
size_t PtrSize(void *);
void SaveHeap(void);
void RestoreHeap(void);
void RestartHeap(void);
Handle NeedHandle(size_t);
Handle RNewHandle(size_t);
void ClearHandle(Handle);
void FillHandle(Handle, byte);
void DisposeHandle(Handle);
void* ReallocateHandle(Handle, size_t);
uint32 LargestHandle(void);
size_t FreeHunk(void);
size_t HandleSize(Handle);
int HandleLocked(Handle);
void LockHandle(Handle);
void UnlockHandle(Handle);
void CriticalHandle(Handle theHandle, bool critical);
void PackHandles(void);
void* CopyHandle(Handle, void*);
size_t StackLeft(void);
void* hunkcpy(voidptr dst, farptr src, size_t len);
int CheckLoadLinks(void);

void ShowHunks(void);
void ShowHandles(void);

#endif

