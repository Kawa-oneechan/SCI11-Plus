/*
 **TEXT - Miscellaneous text routines.
**
*/

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments
// > Basic UTF-8
// > Add SCI32-style |c| and |f| commands that don't use lookup tables as |C| and |F|

#include "kawa.h"
#include "text.h"
#include "types.h"
#include "ctype.h"
#include "grtypes.h"
#include "resource.h"
#include "string.h"
#include "event.h"
#include "language.h"
#include "graph.h"
#include "dialog.h"
#include "stdio.h"
#include "errmsg.h"
#include "memmgr.h"

byte textColors[MAXTEXTCOLORS];
uint textFonts[MAXTEXTFONTS];
byte lastTextColor;
byte lastTextFont;
word *newRect;
word rectIndex;


global KERNEL(TextColors)
{
	int	i;
	byte *color = textColors;

	for (i = 1; i <= argCount; i++)
		*color++ = (byte)arg(i);
	lastTextColor = (byte)argCount;
}


global KERNEL(TextFonts)
{
	int	i;
	uint *font = textFonts;

	for (i = 1; i <= argCount; i++)
		*font++ = (uint)arg(i);
	lastTextFont = (byte)argCount;
}


#define DEFWIDE	192

//make r large enough to hold text
global void RTextSize(RRect *r, strptr text, word font, word def)
{
	word length, longest, count, height, oldFont, defaultFont;
	strptr str, first;

	//we are sizing this text in the font requested
	oldFont = GetFont();

	if (font != -1)
		RSetFont(defaultFont = font);
	else
		defaultFont = oldFont;

	r->top = 0;
	r->left = 0;

	if (def < 0)
	{
		//we don't want word wrap
		r->bottom = GetHighest(text, strlen(text), defaultFont);
		r->right = RStringWidth(text);
	}
	else
	{
		if (!def)
			r->right = DEFWIDE; //use default width
		else
			r->right = def;

		//get a local pointer to text
		str = text;
		height = 0;
		longest = 0;
		while (*str)
		{
			first = str;
			height += GetHighest(first, (count = GetLongest(&str, r->right, defaultFont)), defaultFont);
			length = RTextWidth(first , 0, count, defaultFont);
			if (length > longest)
				longest = length;
		}
		if (!def && r->right > longest)
			r->right = longest;
		r->bottom = height;
	}

	//restore old font
	RSetFont(oldFont);
}

#ifdef UTF8
short UTF8FontHack(short c)
{
	//Handle stupid space-saving hack
	if (c >= 0x2000 && c <= 0x2044) //General Punctuation overlaps Combining
		c = c - 0x2000 + 0x300;		//Diacritical Marks.
	if (c >= 0x3000 && c <= 0x30FF) //CJK Symbols and Kana overlap Latin Ext-B,
		c = c - 0x3000 + 0x200;		//IPA Extensions, and Spacing Modifiers.
	//Actual font mapping is thus:
	//0x0000 C0 and Basic Latin
	//0x0080 C1 and Latin Supplement
	//0x0100 Latin Extended-A
	//0x0180 Latin Extended-B (free space)
	//0x0200 CJK Symbols and Punctuation
	//0x0240 Hiragana
	//0x02A0 Katakana
	//0x0300 General Punctuation
	//0x0370 Greek and Coptic
	//0x0400 Cyrillic
	return c;
}
#endif

//Return total width of this text
global word RTextWidth(strptr text, int index, int count, int defaultFont)
{
	int width = 0, newNum, action;
	short c;
	int oldFont = GetFont();
	char *str = text + index;

	RSetFont(defaultFont);

	while (count-- > 0 && (*str != 0))
	{
		c = *str;

		if(c == CTRL_CHAR)
		{
			newNum = 0;
			str++;
			if(!(count-- > 0)) break;

			if((*str == 'c' || *str == 'f' || *str == 'C' || *str == 'F') && count-- > 0)
			{
				action = *str;
				str++;
				while(*str >= '0' && *str <= '9' && count)
				{
					newNum *= 10;
					newNum += *str++ - '0';
					count--;
				}
				if(count > 0)
				{
					switch(action)
					{
						case 'c':
						case 'C':
							break;
						case 'f':
							RSetFont(textFonts[newNum]);
							break;
						case 'F':
							RSetFont(newNum);
							break;
					}
				}
			}

			while(count > 0 && *str != CTRL_CHAR)
			{
				count--;
				str++;
			}

		}
		else
		{
#ifdef UTF8
			str = GetUTF8Char(str) - 1;
			count -= UTF8Count - 1;
			c = UTF8FontHack(UTF8Char);
#endif
			width += RCharWidth(c);
		}

		str++;
	}
	RSetFont(oldFont);
	return width;
}

//return count of chars that fit in pixel length
global int GetLongest(strptr *str, int max, int defaultFont)
{
	strptr last, first;
	short c;
	word count = 0, lastCount = 0;
	first = last = *str;

	//find a HARD terminator or LAST SPACE that fits on line
	while (1)
	{
		c = *(*str);

		if (c == 0x0d)
		{
			if (*(*str + 1) == 0x0a)
				(*str)++; //so we don't see it later
			if (lastCount &&  max < RTextWidth(first , 0, count, defaultFont))
			{
				*str = last;
				return lastCount;
			}
			else
			{
				(*str)++; //so we don't see it later
				return count; //caller sees end of string
			}
		}
		if (c == 0x0a)
		{
			if ((*(*str + 1) == 0x0d) && (*(*str + 2) != 0x0a)) //by Corey for 68k
				(*str)++; //so we don't see it later
			if (lastCount &&  max < RTextWidth(first , 0, count, defaultFont))
			{
				*str = last;
				return lastCount;
			}
			else
			{
				(*str)++; //so we don't see it later
				return count; //caller sees end of string
			}
		}

		if (c == '\0')
		{
			if (lastCount && max < RTextWidth(first , 0, count, defaultFont))
			{
				*str = last;
				return lastCount;
			}
			else
			{
				return count; //caller sees end of string
			}
		}

		if (c == ' ') //check word wrap
		{
			if (max >= RTextWidth(first , 0, count, defaultFont))
			{
				last = *str;
				++last; //so we don't see space again
				lastCount = count;
			}
			else
			{
				*str = last;
				//eliminate trailing spaces
				while (**str == ' ')
					++(*str);
				return lastCount;
			}
		}

		//all is still cool
#ifdef UTF8
		GetUTF8Char(*str);
		count += UTF8Count;
		(*str) += UTF8Count;
#else
		++count;
		(*str)++;
#endif

		{
			//we may never see a space to break on
			if (!lastCount &&  RTextWidth(first, 0, count, defaultFont) > max)
			{
				last += --count;
				*str = last;
				return count;
        	}
		}
	}
}


//Search string for font control and return point size of tallest font
global int GetHighest(strptr str, int cnt, int defaultFont)
{
	int oldFont, pointSize, start = cnt - 2, setFont, newFont, anyFont;

	oldFont = GetFont();
	pointSize = GetPointSize();

	while (cnt--)
	{
		if (*str++ == CTRL_CHAR)
		{
			//Hit control code: check for font control
			//If font control found, adjust pointSize if a taller font
			//is requested.
			if (*str == 'f' || *str == 'F')
			{
				if (!cnt--) break;
				anyFont = (*str == 'F');
				str++;
				setFont = (cnt == start); //If font control at start of line, set pointSize even if smaller than default.
				if (*str == CTRL_CHAR)
				{
					RSetFont(defaultFont);
				}
				else
				{
					newFont = 0;
					while (cnt-- && (*str >= '0') && (*str <= '9'))
					{
						newFont *= 10;
						newFont += *str++ - '0';
					}
					if (!++cnt) break;
					if (anyFont)
						RSetFont(newFont);
					else
						RSetFont(textFonts[newFont]);
				}
				if (setFont || (pointSize < GetPointSize()))
					pointSize = GetPointSize();
			}
			if (!cnt--) break;
			while ((*str++ != CTRL_CHAR) && cnt--);
		}
	}
	RSetFont(oldFont);
	return pointSize;
}


//put the text to the box in mode requested
global word* RTextBox(strptr text, int show, RRect *box, word mode, word font)
{
	strptr first, str;
	word length, height = 0, wide, xPos, count, oldFont, curFont, pointSize;
	word defaultFont, defaultFore,i;

	rectIndex = 0;
	newRect = (word*)RNewPtr((((strlen(text) / 7) * 4 + 1) * sizeof(word)));

	//we are printing this text in the font requested
	oldFont = GetFont();

	if (font != -1)
		RSetFont(defaultFont = font);
	else
		defaultFont = oldFont;
	defaultFore = (byte)rThePort->fgColor;

	wide = box->right - box->left;
	str = text;

	while (*str)
	{
		first = str;
		curFont = GetFont(); //RTextWidth may change our font, so we must save it.
		pointSize = GetHighest(first, (count = GetLongest(&str, wide, defaultFont)), defaultFont);
		length = RTextWidth(first , 0, count, defaultFont);
		RSetFont(curFont);

		//determine justification and draw the line
		switch (mode)
		{
			case TEJUSTCENTER:
				xPos = (wide - length) / 2;
				break;
			case TEJUSTRIGHT:
				xPos = (wide - length);
				break;
			case TEJUSTLEFT:
			default:
				xPos = 0;
		}
		RMoveTo(box->left + xPos, box->top + height);
		if (show)
			ShowText(first, 0, count, defaultFont, defaultFore);
		else
			RDrawText(first, 0, count, defaultFont, defaultFore);
		height += pointSize;
	}

	//restore old font
	RSetFont(oldFont);
	for (i = 0; i < rectIndex; i++)
	{
		newRect[i*4+0] += rThePort->origin.h;
		newRect[i*4+1] += rThePort->origin.v;
		newRect[i*4+2] += rThePort->origin.h;
		newRect[i*4+3] += rThePort->origin.v;
	}
	newRect[rectIndex*4] = 0x7777;
	if (rectIndex == 0)
	{
		DisposePtr(newRect);
		newRect = NULL;
	}
	return newRect;
}


global int RStringWidth(strptr str)
{
	return RTextWidth(str, 0, strlen(str), GetFont());
}


//draw and show this string
global void DrawString(strptr str)
{
	RDrawText(str, 0, strlen(str), GetFont(), rThePort->fgColor);
}


//draw and show this string
global void ShowString(strptr str)
{
	ShowText(str, 0, strlen(str), GetFont(), rThePort->fgColor);
}


global void RDrawText(strptr str, int first, int cnt, int defaultFont, int defaultFore)
{
	char code;
	int param;
	strptr last;
#ifdef DEBUG
	strptr chkParam;
#endif
	word TogRect;

	TogRect  = 0;

	str += first;
	last = str + cnt;

	while (str < last)
	{
		if (*str == CTRL_CHAR)
		{
			str++;

			//Hit control code: do control function
			if ((code = *str++) == CTRL_CHAR)
			{
				RDrawChar(CTRL_CHAR); // No control code found -> want CTRL_CHAR
				continue;
			}

			if (*str == CTRL_CHAR)
			{
				// No parameter following control code
				str++;
				param = -1;
			}
			else
			{
				param = 0;
#ifdef DEBUG
				chkParam = str;
#endif
				while ((*str >= '0') && (*str <= '9'))
				{
					param *= 10;
					param += (*str++) - '0';
				}
#ifdef DEBUG
				if (str == chkParam)
				{
					//No number found
					Panic(E_TEXT_PARAM, code, *str);
				}
#endif
				while ((str < last) && (*str++ != CTRL_CHAR)) ;
			}

			switch(code)
			{
				//rectangle
				case 'r':
					if (TogRect)
					{
						newRect[rectIndex*4+2] = rThePort->pnLoc.h;
						newRect[rectIndex*4+3] = rThePort->pnLoc.v + rThePort->fontSize;
						rectIndex++;
					}
					else
					{
						newRect[rectIndex*4+0] = rThePort->pnLoc.h;
						newRect[rectIndex*4+1] = rThePort->pnLoc.v;
					}
					TogRect = !TogRect;
					break;
				//Color
				case 'c':
					if (param == -1)
					{
						//No param = set color to default color
						PenColor(defaultFore);
					}
					else
					{
						//Param = index in textColors table
						if (param < lastTextColor)
							PenColor(textColors[param] & 0xff);
#ifdef DEBUG
						else
							Panic(E_TEXT_COLOR, param);
#endif
					}
					break;
				case 'C':
					if (param == -1)
					{
						//No param = set color to default color
						PenColor(defaultFore);
					}
					else
					{
						//Param = actual palette index!
						PenColor(param & 0xff);
					}
					break;
				//Font
				case 'f':
					if (param == -1)
					{
						//No param = set font to default
						RSetFont(defaultFont);
					}
					else
					{
						//Param = index in textFonts table
						if (param < lastTextFont)
							RSetFont(textFonts[param]);
#ifdef DEBUG
						else
							Panic(E_TEXT_FONT, param);
#endif
					}
					break;
				case 'F':
					if (param == -1)
					{
						//No param = set font to default
						RSetFont(defaultFont);
					}
					else
					{
						//Param = actual font number!
						RSetFont(param);
					}
					break;
#ifdef DEBUG
				default:
					Panic(E_TEXT_CODE, code);
					break;
#endif
			}
		}
		else
		{
#ifdef UTF8
			str = GetUTF8Char(str);
			RDrawChar(UTF8FontHack(UTF8Char));
#else
			RDrawChar(*str++);
#endif
		}
	}
}


global void ShowText(strptr str, int first, int cnt, int defaultFont, int defaultFore)
{
	RRect r;

	r.top = rThePort->pnLoc.v;
	r.bottom = r.top + GetHighest(str + first, cnt, defaultFont);
	r.left = rThePort->pnLoc.h;
	RDrawText(str, first, cnt, defaultFont, defaultFore);

	r.right = rThePort->pnLoc.h;
	ShowBits(&r, VMAP);
}
