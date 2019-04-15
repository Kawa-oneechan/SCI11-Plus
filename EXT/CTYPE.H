//KAWA WAS HERE
//-------------
// > General cleanup
// > Add Win-1252 support

#ifndef CTYPE_H
#define CTYPE_H

#include "kawa.h"

#ifdef UTF8
	#ifndef WIN1252
		#define WIN1252
	#endif
#endif

#ifndef TYPES_H
	#include "types.h"
#endif

#ifdef UTF8
extern bool islower(short c);
extern bool isupper(short c);
extern short _tolower(short c);
extern short _toupper(short c);
#else
extern bool islower(byte c);
extern bool isupper(byte c);
extern char _tolower(byte c);
extern char _toupper(byte c);
#endif

//need defines to avoid Microsoft C defaults
#define	lower(c) tolower(c)
#define	tolower(c) _tolower(c)
#define	toupper(c) _toupper(c)

#define	isalpha(c) (islower(c) || isupper(c) )
#define	isdigit(c) ((c) >= '0' && (c) <= '9')

#define  isalnum(c) isalpha(c) || isdigit(c)
#define  iscntrl(c) ((c) <= 0x1f || (c) == 0x7f)
#define  isspace(c) strchr(" \t\n\r", c)
#define  ispunct(c) !(isalnum(c) || isspace(c) || iscntrl(c))

#ifdef CASEMAP
#define	iseuro(c) ((c) >= 0x80 && (c) <= 0xff)
#else
#define	iseuro(c) ((c) >= 0x80 && (c) <= 0xa5)
#endif

#endif
