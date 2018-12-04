#include "types.h"
#include "ctype.h"
#include "kernel.h"
#include "pmachine.h"
#include "resource.h"
#include "stdio.h"
#include "errmsg.h"
#include "memmgr.h"

enum {
	ARRAYNEW,
	ARRAYSIZE,
	ARRAYAT,
	ARRAYATPUT,
	ARRAYFREE,
	ARRAYFILL,
	ARRAYCPY,
	ARRAYCMP,
	ARRAYDUP,
	ARRAYGETDATA
};

enum {
	INTARRAY,
	IDARRAY,
	BYTEARRAY,
	STRARRAY,
	DWORDARRAY,
	LASTARRAYTYPE,
};

typedef struct {
	int elementSize;
	int size;
} arrayHdr;

int arrayElementSizes[] =
{
	2, // INTARRAY
	2, // IDARRAY
	1, // BYTEARRAY
	1, // STRARRAY
	4  // DWORDARRAY
};

arrayHdr* array;

void* ArrayCalc(int index)
{
	char* base = (char*)array;
	if (index < 0 || index >= array->size)
		DoPanic("Index out of range.");
	return base + sizeof(arrayHdr) + (array->elementSize * index);
}

void ArrayResize(int newSize)
{
	arrayHdr* ret;
	char* i;
	char* j;
	int oldByteSize, newByteSize;
	if (newSize < 0)
		DoPanic("ArrayResize: passed size is negative.");
	newSize++;
	oldByteSize = sizeof(arrayHdr) + (array->size * array->elementSize);
	newByteSize = sizeof(arrayHdr) + (newSize * array->elementSize);
	if (newByteSize > oldByteSize)
	{
		ret = NeedPtr(newByteSize);
		//ret->size = newSize;
		i = (char*)ret;
		j = (char*)array;
		while (newByteSize--)
			*i++ = *j++;
		array = ret;
	}
	if (newSize > array->size)
		array->size = newSize;
}

arrayHdr* MakeArray(int size, int type)
{
	arrayHdr* ret = NeedPtr(sizeof(arrayHdr) + (size * arrayElementSizes[type]));
	ret->elementSize = arrayElementSizes[type];
	ret->size = size;
	return ret;
}

int ArrayAt(int index)
{
	int ret = 0;
	//ArrayResize(index);
	void *data = ArrayCalc(index);
	switch (array->elementSize)
	{
		case 1:
			ret = (int)*(unsigned char*)data;
			break;
		case 2:
			ret = (int)*(int*)data;
			break;
		case 4:
			DoAlert("Can't do long arrays, sorry.");
			break;
		default:
			DoAlert("Weird array element size.");
			break;
	}
	return ret;
}

void ArraySetAt(int index, int value)
{
	void *data;
	ArrayResize(index);
	data = ArrayCalc(index);
	switch (array->elementSize)
	{
		case 1:
			*(unsigned char*)data = (unsigned char)value;
			break;
		case 2:
			*(int*)data = (int)value;
			break;
		case 4:
			DoAlert("Can't do long arrays, sorry.");
			break;
		default:
			DoAlert("Weird array element size.");
			break;
	}
}

void ArrayAtPut(int index, int count, void *values)
{
	void *data;
	int *curValue;
	int *intPtr;
	char *charPtr;
	ArrayResize(index + count);
	data = ArrayCalc(index);
	curValue = (int*)values;
	switch (array->elementSize)
	{
		case 1:
			charPtr = (char*)data;
			while (count--)
				*charPtr++ = (char)*curValue++;
			break;
		case 2:
			intPtr = (int*)data;
			while (count--)
				*intPtr++ = (int)*curValue++;
			break;
		case 4:
			DoAlert("Can't do long arrays, sorry.");
			break;
		default:
			DoAlert("Weird array element size.");
			break;
	}
}

void ArrayFill(int startIndex, int length, int value)
{
	int index;
	int endIndex = -1;
	if (length == -1)
		length = array->size - startIndex;
	if (length < 1)
		return;
	endIndex = startIndex + length;
	for (index = startIndex; index <= endIndex; index++)
		ArraySetAt(index, value);
	/*
	void* data;
	int* intData;
	char* charData;
	int endIndex = -1;
	if (length == -1)
		length = array->size - startIndex;
	if (length < 1)
		return;
	endIndex = startIndex + length;
	ArrayResize(endIndex);
	data = ArrayCalc(startIndex);
	switch (array->elementSize)
	{
		case 1:
			//*(unsigned char*)data = (unsigned char)value;
			charData = (char*)data;
			while (length--)
			{
				*charData++ = (char)value;
				charData++;
			}
			break;
		case 2:
			intData = (int*)data;
			while (length--)
				*intData++ = (int)value;
			break;
		case 4:
			DoAlert("Can't do long arrays, sorry.");
			break;
		default:
			DoAlert("Weird array element size.");
			break;
	}
	*/
}

void ArrayCopy(int destIndex, void* source, int srcIndex, int length)
{
	arrayHdr* srcArray = (arrayHdr*)source;
	arrayHdr* tempArray;
	char* srcPtr;
	char* destPtr;
	int byteSize;
	if (length == -1)
		length = array->size - srcIndex;
	if (length < 1)
		return;
	if (array->elementSize != srcArray->elementSize)
	{
		DoPanic("ArrayCopy: source and destination have different elementSize.");
		return;
	}
	ArrayResize(destIndex + length);
	destPtr = (char*)ArrayCalc(destIndex);
	//cheat a bit cos we're not array objects now
	tempArray = array;
	array = srcArray;
	ArrayResize(srcIndex + length);
	srcPtr = (char*)ArrayCalc(srcIndex);
	//switch back
	srcArray = array;
	array = tempArray;
	byteSize = length * array->elementSize;
	while (byteSize--)
		*destPtr++ = *srcPtr++;
}

arrayHdr* DuplicateArray()
{
	int byteSize = sizeof(arrayHdr) + (array->size * array->elementSize);
	void* dup = NeedPtr(byteSize);
	char* i = (char*)dup;
	char* j = (char*)array;
	while (byteSize--)
		*i++ = *j++;
	return dup;
}

global KERNEL(Array)
{
	array = (arrayHdr*)Native(arg(2));
	switch(arg(1))
	{
		case ARRAYNEW:
			// arg(2) = array size
			// arg(3) = array type
			//TODO: strings
			//if (arg(3) == STRARRAY) ...
			array = MakeArray(arg(2), arg(3));
			ArrayFill(0, arg(2), 0);
			acc = (int)array;
			break;
		case ARRAYSIZE:
			acc = array->size;
			break;
		case ARRAYAT:
			acc = ArrayAt(arg(3));
			break;
		case ARRAYATPUT:
			ArrayAtPut(arg(3), argCount - 3, &arg(4));
			//ArraySetAt(arg(3), arg(4));
			acc = (int)array;
			break;
		case ARRAYFREE:
			DisposePtr(Native(arg(2)));
			break;
		case ARRAYFILL:
			// arg(2) = array datablock
			// arg(3) = position in array to start fill
			// arg(4) = length to fill
			// arg(5) = value to fill with
			// fill a portion of the passed array with a specific value
			ArrayFill(arg(3), arg(4), arg(5));
			acc = (int)array;
			break;
		case ARRAYCPY:
			// arg(2) = destination array datablock
			// arg(3) = position in destination array to start copy
			// arg(4) = source array datablock
			// arg(5) = position in source array to start copy
			// arg(6) = length to copy
			// copy data between two arrays
			ArrayCopy(arg(3), Native(arg(4)), arg(5), arg(6));
			acc = (int)array;
			break;
		case ARRAYCMP:
			//wasn't in the C++ version either lol.
			break;
		case ARRAYDUP:
			acc = (int)DuplicateArray();
			break;
		case ARRAYGETDATA:
			DoPanic("ArrayGetData unimplemented.");
			break;
	}
}
