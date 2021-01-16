/*	STRING
	******
	String functions (standard C issue and user-defined strings).
	by Jeff Stephenson
	Multilingual conversion by Pablo Ghenis, April 1989

*** IMPORTANT!!!* **

Identical versions of this string.c and string.h should be used to
compile SCI, SC, VC, DC, VCPP and any other SCI-related programs.
If these programs are kept in separate directories make sure to migrate
changes to all others.
Recommendation: keep masters in SCI directory, copy as needed to others.

**************************************************************************/

//KAWA WAS HERE
//-------------
// > General cleanup

#ifndef	STRING_H
#define	STRING_H

#include "types.h"

extern uint strlen(strptr str);
extern strptr strcpy(strptr dest, strptr src);
extern strptr strncpy(strptr dest, strptr src, int n);
extern strptr strdup(strptr str);
extern strptr strcat(strptr dest, strptr src);
extern strptr strncat(strptr dest, strptr src, int n);
extern int strcmp(strptr str1, strptr str2);
extern int strncmp(strptr str1, strptr str2, int n);
extern strptr strlwr(strptr str);
extern strptr strchr(strptr, char);
extern strptr strstr(strptr, strptr);

#ifndef SCI
extern int memcmp(void *buf1, void *buf2, unsigned int count);
#endif

//extern void memcpy(memptr, memptr, int); //in memmgr.s
extern void* memcpy(void *to, void *from, unsigned int count); //in memmgr.s
extern void FarMemcpy(void far *dest, void far *src, int length);

//Our extensions:
extern bool strtrn(strptr, strptr, strptr, strptr);
extern strptr strbcat(strptr dest, strptr src, int count);
extern strptr reverse(strptr str);
extern bool IsPrintStr(strptr str);

#ifdef UTF8
extern short UTF8Char, UTF8Count;
extern char* GetUTF8Char(char*);
extern char* SetUTF8Char(char*, short);
extern void SetUTF8CharAt(char*, int, short);
#endif

#endif

