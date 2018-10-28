// RESOURCE.H
//		Header file for resource manager

//KAWA WAS HERE
//-------------
// > General cleanup

#if !defined(RESOURCE_H)
#define RESOURCE_H

#if !defined(TYPES_H)
	#include "types.h"
#endif
#if !defined(FLIST_H)
	#include "flist.h"
#endif
#if !defined(LIST_H)
	#include "list.h"
#endif
#if !defined(RESTYPES_H)
	#include "restypes.h"
#endif

//special resource IDs
#define	ALL_IDS			0xffff

//conditional execution (test or prod)
#define	FILEBASED		0
#define	VOLBASED		1

//special vocab files containing strings/data need by the system
#define	KERNEL_VOCAB	999
#define	OPCODE_VOCAB	998
#define	SELECTOR_VOCAB	997
#define	CLASSTBL_VOCAB	996
#define	HELP_VOCAB		995
#define	PROPOFS_VOCAB	994

//"locked" resources mean not_auto_purgeable
#define	LOCKED			1
#define	UNLOCKED		0

#define	RESMAPNAME		"RESOURCE.MAP"
#define	RESVOLNAME		"RESOURCE"
#define	ALTMAPNAME		"MESSAGE.MAP"
#define	ALTVOLNAME		"RESOURCE.MSG"
#define	AUDVOLNAME		"RESOURCE.AUD"
#define	SFXVOLNAME		"RESOURCE.SFX"

//structure of header information in resource map file
typedef struct
{
	byte resType;
	uword resTypeOffset;
} ResDirHdrEntry;

//structure of information in resource map file
//used for all resources except for audio
typedef struct ResDirEntry
{
	uword resId;
	byte volWOffset1;
	byte volWOffset2;
	byte volWOffset3;
} ResDirEntry;

//structure of information in resource map file
//used for audio type resource only
typedef struct
{
	uword id;
	uword offsetLSW;
	byte offsetMSB;
} ResAudEntry;

//structure of information in resource map file
//used for sync+rave+audio type resource only
typedef struct
{
	byte noun;
	byte verb;
	byte cond;
	union
	{
		byte sequ;
		byte sync;
		byte rave;
	} flag;
	uword offsetLSW;
	byte offsetMSB;
	uword syncLen;
	uword raveLen;
} ResAud36Entry;

#define	SYNCMASK		0x80
#define	RAVEMASK		0x40
#define	SEQUMASK		0x3F

//structure of information in resource patch file
typedef struct
{
	byte patchDir;
	byte resType;
	uword resId;
} ResPatchEntry;

//masks for decoding directory entry
#define	MAXRESOURCES	32

//bytes added for original size and compression scheme
#define	HEADERSIZE		4

typedef int ARMHandle;

typedef struct
{
	Node next;
	byte type;
	byte lock;
	int id;
	union
	{
		Handle data;
		ARMHandle armHandle;
	} lData;
	uword size;
	byte altResMem;
	byte checkSum;		//also used when checkingLoadLinks is TRUE
} LoadLink;

#define	PARAGRAPHSIZE		16

typedef enum { ArrowCursor, DiskCursor, ARMWriteCursor, ARMReadCursor } ResCursor;

//resource cursor types
#define RESCSR_DISKCHANGE	0x01
#define RESCSR_DISKMOVE		0x02
#define RESCSR_ARMCHANGE	0x04
#define RESCSR_ARMMOVE		0x08

extern FList loadList;
extern word resourceCursorType;
extern bool checkingLoadLinks;

LoadLink far** FindResEntry(byte, uword);
Handle GetResHandle(uword);
bool PurgeLast(void);
Handle LoadHandle(strptr fileName);
Handle ResLoad(byte, uint);
Handle ResLoadSound(uint);
void ResLock(byte, uint, bool);
void ResUnLoad(byte, uint);
void SetResCursor(ResCursor);
int FindPatchEntry(byte resType, uword resId);
void InitPatches(void);
bool ResCheck(byte resType, uint resId);
bool ResCheck36(byte resType, uint module, byte noun, byte verb, byte cond, byte sequ);
void MakeName36(char, char*, uint, byte, byte, byte, byte);
void ConvBase36(char*, uint, int);
char GetDigit36(int);

#endif

