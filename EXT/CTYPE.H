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

extern bool	islower(byte c);
extern bool	isupper(byte c);
extern char	_tolower(byte c);
extern char _toupper(byte c);

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

#ifdef WIN1252
#define	iseuro(c) ((c) >= 0x80 && (c) <= 0xff)
#else
#define	iseuro(c) ((c) >= 0x80 && (c) <= 0xa5)
#define	iskana(c) ((c) >= 0xa6 && (c) <= 0xdf)
#define	isgreek(c) ((c) >= 0xe0 && (c) <= 0xee)

#define iskanji(c) \
			((unsigned char)(c) >= 0x81 && \
			 (unsigned char)(c) <= 0x9f || \
			 (unsigned char)(c) >= 0xe0 && \
			 (unsigned char)(c) <= 0xfc)
#define iskanji2(c) \
			((unsigned char)(c) >= 0x40 && \
			 (unsigned char)(c) <= 0x7e || \
			 (unsigned char)(c) >= 0x80 && \
			 (unsigned char)(c) <= 0xfc)

#endif

#endif
