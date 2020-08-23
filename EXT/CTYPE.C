//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments
// > Add Win-1252 support

#include "ctype.h"
#include "string.h"
#include "types.h"

#ifdef UTF8

#define euroToLower(c) euroToLowerTbl[(int)c-128]
#define euroToUpper(c) euroToUpperTbl[(int)c-128]
#include "CM_UNI.H"

bool islower(short c)
{
	return ((c >= 'a' && c <= 'z') ||
			((c >= 0x80 && c <= 0x17f) && (euroToLower(c) == c)));
//TODO: #ifndef UTF8_SMALLMAP
}

bool isupper(short c)
{
	return ((c >= 'A' && c <= 'Z') ||
			((c >= 0x80 && c <= 0x17f) && (euroToUpper(c) == c)));
//TODO: #ifndef UTF8_SMALLMAP
}

short _tolower(short c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 'a';
	else if (c >= 0x80 && c <= 0x17f)
		return euroToLower(c);
#ifndef UTF8_SMALLMAP
	else if (c >= 0x370 && c <= 0x52F)
		return euroToLowerTbl[(((int)c-0x370)+0x180)-128];
#endif
	else
		return c;
}

short _toupper(short c)
{
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 'A';
	else if (c >= 0x80 && c <= 0x17f)
		return euroToUpper(c);
#ifndef UTF8_SMALLMAP
	else if (c >= 0x370 && c <= 0x52F)
		return euroToUpperTbl[(((int)c-0x370)+0x180)-128];
#endif
	else
		return c;
}

#else //UTF8

#ifndef CASEMAP

//If we don't use a casemap file, use the old behavior.

#define euroToLower(c) euroToLowerTbl[(int)c-128]

/* this array maps European characters to their lower case version
 * (lower case chars map to themselves)
 * euroToLowerTbl[(int)c-128] is lower case version of c
 * Use euroToLower(c) macro defined above.
 * NOTE: ASCII does not provide upper case versions of all lower case
 * European characters.
 */
static byte near euroToLowerTbl[]=
{
	135, //128 = C cedille
	129,
	130,
	131,
	132,
	133,
	134,
	135,
	136,
	137,
	138,
	139,
	140,
	141,
	132, //142 = dotted A
	134, //143 = accented A
	130, //144 = accented E
	145,
	145, //146 = AE
	147,
	148,
	149,
	150,
	151,
	152,
	148, //153 = dotted O
	129, //154 = dotted U
	155,
	156,
	157,
	158,
	159,
	160,
	161,
	162,
	163,
	164,
	164, //165 = N~
};


bool islower(byte c)
{
	return ((c >= 'a' && c <= 'z') ||
			(iseuro(c) && (euroToLower(c) == c));
}


bool isupper(byte c)
{
	return ((c >= 'A' && c <= 'Z') ||
			(iseuro(c) && (euroToLower(c) != c)));
}


char _tolower(byte c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 'a';
	else if (iseuro(c))
		return euroToLower(c);
	else
		return c;
}


char _toupper(byte c)
{
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 'A';
	else
		//we could do upcase conversion of Euorpean characters,
		//but it seems too much of a pain, soooo....
		return c;
}

#else //CASEMAP

#define euroToLower(c) euroToLowerTbl[(int)c-128]
#define euroToUpper(c) euroToUpperTbl[(int)c-128]

#include CASEMAP

bool islower(byte c)
{
	return ((c >= 'a' && c <= 'z') ||
			(iseuro(c) && (euroToLower(c) == c)));
}

bool isupper(byte c)
{
	return ((c >= 'A' && c <= 'Z') ||
			(iseuro(c) && (euroToUpper(c) == c)));
}

char _tolower(byte c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 'a';
	else if (iseuro(c))
		return euroToLower(c);
	else
		return c;
}


char _toupper(byte c)
{
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 'A';
	else if (iseuro(c))
		return euroToUpper(c);
	else
		return c;
}

#endif //CASEMAP

#endif //UTF8
