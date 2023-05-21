/* TYPES.H
 */

//KAWA WAS HERE
//-------------
// > General cleanup

#ifndef	TYPES_H
#define	TYPES_H

//KAWA WAS HERE
//Use this to not feel as tempted to use args directly.
#define KERNEL(x)		void K##x (word *args)
typedef signed short	int16;
typedef signed long		int32;
typedef unsigned short	uint16;
typedef unsigned long	uint32;

typedef signed char		sbyte;
typedef signed short	word;
typedef unsigned char	byte;
//typedef unsigned char	uchar;
typedef unsigned short	uword;
typedef unsigned short	uint;
typedef unsigned long	ulong;

typedef	unsigned short	ObjID;

typedef	void*			voidptr;
typedef	char*			strptr;
typedef	byte*			memptr;

typedef	int				bool;

typedef	void(*fptr)(void);
typedef	bool(*boolfptr)();

typedef	void _far*		farptr;
typedef	byte _far*		Hunkptr;
typedef	uword _far*		Dictptr;
typedef	Hunkptr*		Handle;

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef	unsigned int	size_t;
#endif


/*
//Shorthand for unsigned values.
typedef	unsigned char	uchar;
typedef	unsigned int	uint;
typedef	unsigned long	ulong;

//These should always be 8 bit values.
typedef	char			byte;
typedef	uchar			ubyte;

//These should always be 16 bit values.
typedef	int				word;
typedef	uint			uword;

//This type should be able to hold the number of chars addressable by the machine.
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef	uint			size_t;
#endif

//Boolean type
typedef	int				bool;

//Some pointer types
typedef	void			*voidptr;
typedef	char			*strptr;
typedef	ubyte			*memptr;

//A pointer to a function
typedef	void			(*fptr)(void);
typedef	bool			(*boolfptr)();

//far hunk stuff
typedef	void _far		*farptr;
typedef	ubyte _far		*Hunkptr;
typedef	uword _far		*Dictptr;
typedef	Hunkptr	    	*Handle;

//Pseudo-machine object (or other address) references
typedef	uword			ObjID;
*/
#define	true			1
#define	false			0

#ifdef NULL
#undef NULL
#endif
#define	NULL			0
#define	LNULL			0L

#define	NULL_OBJ		((Obj*)0)

#define	forever			for (;;)
#define	global

#define min(a,b)		((a) < (b) ? (a) : (b))
#define max(a,b)		((a) > (b) ? (a) : (b))

#endif

