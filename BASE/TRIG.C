//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments

#include "trig.h"
#include "types.h"
#include "math.h"

static int near getMajor(int,int,int,int);
static int near prorateATan(long);

static long near SinArray[]=
{
	   0,  872, 1736, 2588,
	3420, 4226, 5000, 5736,
	6428, 7071, 7660, 8192,
	8660, 9063, 9397, 9659,
	9848, 9962, 10000
};

static long near ATanArray[]=
{
	 875, 1763, 2679, 3640,
	4663, 5774, 7002, 8391,
	10000
};


global long ScaledSin(word a)
{
	uint entry;
	long entryValue, deltaValue;

	if (a < 0) return ScaledSin(a + 360);

	if (a <= 90) //1st quadrant, interpolate
	{
		entry = (uint)(a / TrigStep);
		entryValue = SinArray[entry];
		deltaValue = SinArray[entry+1] - entryValue;

		return (long)(
			entryValue +
			(
				(deltaValue * (long)(a % TrigStep) + ((long)TrigStep / 2L))
				/ (long)TrigStep
			)
		);
	}
	if (a <= 180) return  ScaledSin(180 - a); //2nd
	if (a <= 270) return -ScaledSin(a - 180); //3rd
	if (a <= 360) return -ScaledSin(360 - a); //4th

	return ScaledSin(a - 360);
}


global long ScaledCos(word a)
{
	if (a <    0) return  ScaledCos(a + 360);
	if (a <=  90) return  ScaledSin( 90 - a); //1st quadrant
	if (a <= 180) return -ScaledCos(180 - a); //2nd
	if (a <= 270) return -ScaledCos(a - 180); //3rd
	if (a <= 360) return  ScaledCos(360 - a); //4th

	return ScaledCos(a - 360);
}


//returns heading from (x1,y1) to (x2,y2) in degrees (always in the range 0-359)
global int ATan(int x1, int y1, int x2, int y2)
{
	int major;
	major = getMajor(x1,y1,x2,y2);
	//0 <= major <= 90
	if (x2 < x1)
	{
		if (y2 <= y1)
			major += 180;
		else
			major = 180 - major;
	}
	else
	{
		if (y2 < y1)
			major = 360 - major;
		if (major == 360)
			major = 0;
	}
	return(major);
}


//returns angle in the range 0-90
static int near getMajor(int x1, int y1, int x2, int y2)
{
	long deltaX, deltaY, index;
	int major;

	deltaX = (long)(abs(x2 - x1));
	deltaY = (long)(abs(y2 - y1));

	//if (x1,y1) = (x2,y2) return 0

	if ((deltaX == 0) && (deltaY == 0))
		return(0);

	if (deltaY <= deltaX)
	{
		index = ((TrigScale * deltaY) / deltaX);
		if (index < 1000L)
			major = (int)((57L * deltaY + (deltaX / 2L)) / deltaX);
		else
			major = prorateATan(index);
	}
	else
		major = 90 - getMajor(y1,x1,y2,x2);

	return(major);
}


static int near prorateATan(long index)
{
	int i = 0;
	while (ATanArray[i] < index) ++i;
	return ((5 * i) + (int)(((5L * (index - ATanArray[i - 1])) + ((ATanArray[i] - ATanArray[i - 1]) / 2)) / (ATanArray[i] - ATanArray[i - 1])));
}

