/* GETPATH.C PATH AROUND AN OBSTACLE POLYGON
**
** Algorithm by Larry Scott
**
** NAME---GETPATH---
** POLYGON must be closed and not self intersecting (JORDAN polygon)
** all points of polygon must lie with screen limits and the polygon
** must be defined in a screen clockwise direction.
*/

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments
// > Reformatting to match SCI32, besides the overloading, to fix FUCKASS BUG.

#include "getpath.h"

#include "start.h"
#include "math.h"
#include "memmgr.h"
#include "window.h"
#include "trig.h"
#include "errmsg.h"
#include "picture.h"

static void near AvoidPath(AvdPoint*, AvdPoint*, polygon*, int, AvdPoint*);
static polyNode* near AvoidPolygons(AvdPoint*, AvdPoint*, polygon*);
static AvdPoint* near CopyPath(AvdPoint*);
static void near DeletePolygon(polygon*, int);
static int near DistanceEstimate(AvdPoint*, AvdPoint*, int*);
static int near DistanceSquared(int, int, int, AvdPoint*, int, AvdPoint*, AvdPoint*, int*);
static int near Dominates(polyPatch*, polyPatch*, AvdPoint*);
static void near EndPath(AvdPoint*, AvdPoint*);
static void near FreeNodePath(polyNode*);
static int near GetPolyDirections(polyNode*);
static int near GetPolyDistance(AvdPoint*);
static int near IntersectPolygon(AvdPoint*, AvdPoint*, AvdPoint*, int, AvdPoint*, AvdPoint*, int*, int*);
static int near IntersectSegments(AvdPoint*, AvdPoint*, AvdPoint*, AvdPoint*, AvdPoint*);
static void near InvertPolygon(AvdPoint*, int);
static int near LineOnScreenEdge(AvdPoint*, AvdPoint*);
static void near OptimizePath(AvdPoint*, AvdPoint*, AvdPoint*, polyNode*, polygon*, int);
static void near MergePolygon(AvdPoint*, int, polygon*);
static long near NearPoint(AvdPoint*, AvdPoint*, int, AvdPoint*, int);
static int near PatchNode(polyPatch*, int, AvdPoint*);
static int near NodeTest(AvdPoint*, AvdPoint*, AvdPoint*, AvdPoint*);
static int near PolygonPath(AvdPoint*, AvdPoint*, AvdPoint*, int, AvdPoint*, AvdPoint*, int*, int*);
static void near ReducePolyList(polygon*, polyNode*);
static void near RemoveNode(polygon*, int);
static void near SetPolyDirections(polyNode*, int, int);
static void near StartPath(AvdPoint*, AvdPoint*);
static void near v_add(AvdPoint*, AvdPoint*, AvdPoint*);
static int near v_cross3rd_comp(AvdPoint*, AvdPoint*);
static long near v_dot(AvdPoint*, AvdPoint*);
static long near v_sizesqrd(AvdPoint*, AvdPoint*);
static void near v_subtract(AvdPoint*, AvdPoint*, AvdPoint*);
static AvdPoint* near CopyPath(AvdPoint *Path);
static int near TestColinear(AvdPoint*, int*);

static AvdPoint picWindPoly[4];


AvdPoint* GetPath(AvdPoint *A, AvdPoint *B, polygon *polylist, int opt)
{
	int pathType;
	int i; //, j, k;
	AvdPoint exitPoint, entryPoint; //, *P; //, Pt;
	AvdPoint I1, I2;
	int nodeI1, nodeI2;
	enum PathStartType pathStart = fromA;
	enum PathEndType pathEnd = toB;

	AvdPoint path[MAXPATH];

	//set screen polygon
	picWindPoly[0].x = picWind->port.portRect.left;
	picWindPoly[0].y = picWind->port.portRect.top;
	picWindPoly[1].x = picWind->port.portRect.right - 1;
	picWindPoly[1].y = picWind->port.portRect.top;
	picWindPoly[2].x = picWind->port.portRect.right - 1;
	picWindPoly[2].y = picWind->port.portRect.bottom - 1;
	picWindPoly[3].x = picWind->port.portRect.left;
	picWindPoly[3].y = picWind->port.portRect.bottom - 1;

	//TAP polygons are invisable when optimization is off
	for (i = 0; polylist[i].n != 0; ++i)
	{
		if (
			(polylist[i].type >= MTP) ||
			((polylist[i].type == TAP) && (opt == FALSE))
			)
		{
			DeletePolygon(polylist, i);
			--i;
		}
	}
	//are we starting in a polygon?
	for (i = 0; polylist[i].n != 0; ++i)
	{
		if (
			PointInterior(A, polylist[i].polyPoints, polylist[i].n) ||
			(
			(polylist[i].type == CAP) &&
			(NearPoint(A, polylist[i].polyPoints, polylist[i].n, &I2, TRUE) < 2)
				)
			)
		{
			switch (polylist[i].type)
			{
			case TAP:
				DeletePolygon(polylist, i);
				--i;
				break;
			case NAP:
				pathStart = fromAtoEXIT;
				if (opt)
					NearPoint(A, polylist[i].polyPoints, polylist[i].n, &exitPoint, TRUE);
				else
					DeletePolygon(polylist, i);
				break;
			case BAP:
				pathStart = fromBAPtoEXIT;
				NearPoint(A, polylist[i].polyPoints, polylist[i].n, &exitPoint, TRUE);
				break;
			case CAP:
				if (!PointInterior(B, polylist[i].polyPoints, polylist[i].n))
				{
					//Point A is interior and point B is exterior
					pathEnd = toENTRY;
					//Find the last exit point
					if (opt)
					{
						NearPoint(B, polylist[i].polyPoints, polylist[i].n, &entryPoint, FALSE);
					}
					else
					{
						IntersectPolygon(A, B, polylist[i].polyPoints, polylist[i].n, &I1, &I2, &nodeI1, &nodeI2);
						entryPoint = I1;
					}
				}
				break;
			}
		}
		else
		{
			//Turn CAP in BAP
			if (polylist[i].type == CAP)
			{
				InvertPolygon(polylist[i].polyPoints, polylist[i].n);
				polylist[i].type = BAP;
				polylist[i].info = polylist[i].info | INVERTED;
			}
		}
	}
	//are we ending in a polygon?
	for (i = 0; polylist[i].n != 0; ++i)
	{
		if (polylist[i].type != CAP && PointInterior(B, polylist[i].polyPoints, polylist[i].n))
		{
			switch (polylist[i].type)
			{
			case TAP:
				pathEnd = toB;
				DeletePolygon(polylist, i);
				--i;
				break;
			case NAP:
				pathEnd = toENTRYtoB;
				if (opt)
					NearPoint(B, polylist[i].polyPoints, polylist[i].n, &entryPoint, TRUE);
				else
					DeletePolygon(polylist, i);
				break;
			case BAP:
				//If pathEnd == toENTRY we must be trying to get out of a CAP
				if (pathEnd != toENTRY)
				{
					pathEnd = toENTRY;
					NearPoint(B, polylist[i].polyPoints, polylist[i].n, &entryPoint, TRUE);
				}
				break;
			}
		}
	}

	//Change saved CAPs into BAPs
	for (i = 0; polylist[i].n != 0; ++i)
		if (polylist[i].type == CAP)
			polylist[i].type = BAP;

	pathType = (pathStart << 2) + pathEnd;
	switch (pathType)
	{
	case 0: //fromA ---> toB
		AvoidPath(A, B, polylist, opt, path);
		break;
	case 1: //fromA ---> toENTRYtoB
		if (opt)
		{
			AvoidPath(A, &entryPoint, polylist, opt, path);
			EndPath(B, path);
		}
		else
			AvoidPath(A, B, polylist, opt, path);
		break;
	case 2: //fromA ---> toENTRY
		AvoidPath(A, &entryPoint, polylist, opt, path);
		break;
	case 4: //fromAtoEXIT ---> toB
		if (opt)
		{
			AvoidPath(&exitPoint, B, polylist, opt, path);
			StartPath(A, path);
		}
		else
			AvoidPath(A, B, polylist, opt, path);
		break;
	case 5: //fromAtoEXIT ---> toENTRYtoB
		if (opt)
		{
			AvoidPath(&exitPoint, &entryPoint, polylist, opt, path);
			StartPath(A, path);
			EndPath(B, path);
		}
		else
			AvoidPath(A, B, polylist, opt, path);
		break;
	case 6: //fromAtoEXIT ---> toENTRY
		if (opt)
		{
			AvoidPath(&exitPoint, &entryPoint, polylist, opt, path);
			StartPath(A, path);
		}
		else
			AvoidPath(A, &entryPoint, polylist, opt, path);
		break;
	case 8: //fromBAPtoEXIT ---> toB
		AvoidPath(&exitPoint, B, polylist, opt, path);
		if (opt)
			StartPath(A, path);
		break;
	case 9: //fromBAPtoEXIT ---> toENTRYtoB
		if (opt)
		{
			AvoidPath(&exitPoint, &entryPoint, polylist, opt, path);
			StartPath(A, path);
			EndPath(B, path);
		}
		else
			AvoidPath(&exitPoint, B, polylist, opt, path);
		break;
	case 10: //fromBAPtoEXIT ---> toENTRY
		AvoidPath(&exitPoint, &entryPoint, polylist, opt, path);
		if (opt)
			StartPath(A, path);
		break;
	}

	/*
	//This may not be needed anymore
	//kludge to prevent deleting of second point
	if (!opt && P[2].x != ENDOFPATH)
	{
		// if first two points are the same and if direction
		// not blocked eliminate duplicate point, otherwise
		// leave duplicate in to stop motion around obstacle.
		if (FALSE && (P[0].x == P[1].x) && (P[0].y == P[1].y))
		{
			Pt = *A;
			if (A->x < B->x) Pt.x += 2;
			if (B->x < A->x) Pt.x -= 2;
			if (A->y < B->y) Pt.y += 2;
			if (B->y < A->y) Pt.y -= 2;
			for (i = 0, j = 0; polylist[i].n != 0; ++i)
			{
				if (PointInterior(&Pt, polylist[i].polyPoints, polylist[i].n))
				{
					j = 1;
					break;
				}
			}

			if (!j)
				for (k = 0; P[k].x != ENDOFPATH; ++k)
					P[k] = P[k + 1];
		}
	}
	*/

	//Re invert previously inverted polygons
	for (i = 0; polylist[i].n != 0; ++i)
		if (polylist[i].info & INVERTED)
			InvertPolygon(polylist[i].polyPoints, polylist[i].n);

	// Allocate memory for the return path, put the points
	// into the allocation and return the ID
	return CopyPath(path);
}

static void near DeletePolygon(polygon *polylist, int i)
{
	for (; polylist[i].n != 0; ++i)
		polylist[i] = polylist[i + 1];
}

static void near StartPath(AvdPoint *A, AvdPoint *P)
{
	int i = 0;
	do
		++i;
	while (P[i].x != ENDOFPATH);
	for (++i; i != 0; --i)
		P[i] = P[i - 1];
	P[0] = *A;
}

static void near EndPath(AvdPoint *B, AvdPoint *P)
{
	int i = 0;
	do
		++i;
	while (P[i].x != ENDOFPATH);
	P[i++] = *B;
	P[i].x = ENDOFPATH;
}

static long near NearPoint(AvdPoint *P, AvdPoint *polygon, int n, AvdPoint *R, int Edge)
{
	int i;
	long d = 0x07FFFFFFF, dot1, dot2, dot3;
	AvdPoint A, B, tmp1, tmp2, tmp3, tmp4;
	int offScreen;

	for (i = 0; i < n; ++i)
	{
		A = polygon[i];
		if (i == n - 1)
			B = polygon[0];
		else
			B = polygon[i + 1];

		if
			(
				Edge
				&&
				(
				(A.x == B.x) && ((A.x == 0) ||
					(A.x == picWindPoly[1].x)) ||
					(A.y == B.y) && ((A.y == 0) ||
					(A.y == picWindPoly[2].y))
					)
				)

		{
			//do nothing, the line is on the screen edge
		}
		else
		{
			//If P is closest to this line save the closest on this line to P

			v_subtract(&B, &A, &tmp1);
			v_subtract(&A, P, &tmp2);
			v_subtract(&B, P, &tmp3);

			if ((v_dot(&tmp1, &tmp2) <= 0) && (v_dot(&tmp1, &tmp3) >= 0))
				//P-R is normal to A-B

				/*
				A
				***************************
				*
				*
				*
				*
				*
				*
				*
				*
				*
				*
				*
				*
				*
				P----------*R
				*
				*
				*
				***************************
				B
				*/

			{
				//Distance P-R is:
				//ABS{[(B-A)^(0, 0, 1)]#(A-P)/|B-A|}

				//Make N = tmp4 normal to A-B
				tmp4.x = tmp1.y;
				tmp4.y = -tmp1.x;
				dot1 = v_dot(&tmp2, &tmp4);
				dot2 = dot1 / (long)DistanceEstimate(&A, &B, &offScreen);
				if (dot2 < 0) dot2 = -dot2;
				if (dot2 < d)
				{
					d = dot2;

					//R = P + [(N#(A-P))/(N#N)]N
					//round to force point exterior to polygon
					dot2 = v_dot(&tmp4, &tmp4);
					dot3 = dot1 * (int)tmp4.x; //was (long)
					R->x = P->x + (int)
						(
						(
							dot3 + sign(dot3) * (dot2 - 1)
							)
							/ dot2
							);
					dot1 = dot1 * (int)tmp4.y; //was (long)
					R->y = P->y + (int)
						(
						(
							dot1 + sign(dot1) * (dot2 - 1)
							)
							/ dot2
							);
				}
			}
			else
			{
				//P-R is not normal to A-B, for example
				/*
				P*
				*
				*
				*
				*
				*
				R * A
				***************************
				*
				*
				*
				*
				*
				*
				*
				*
				*
				*
				*
				*
				*
				*
				*
				*
				*
				***************************
				B
				*/
				dot1 = (long)DistanceEstimate(&A, P, &offScreen);
				dot2 = (long)DistanceEstimate(&B, P, &offScreen);
				//dot1 = v_dot(&tmp2, &tmp2);
				//dot2 = v_dot(&tmp3, &tmp3);
				if (dot1 < d)
				{
					d = dot1;
					*R = A;
				}
				if (dot2 < d)
				{
					d = dot2;
					*R = B;
				}
			}
		}
	}
	return(d);
}

static void near InvertPolygon(AvdPoint *points, int n)
{
	int i, j;
	for (i = 0, j = n - 1; i < j;)
	{
		AvdPoint p = points[i];
		points[i++] = points[j];
		points[j--] = p;
	}
}

static int near TestColinear(AvdPoint *P, int *n)
{
	int i, j;
	int ret_value = 0;
	int nodes;
	AvdPoint P1, P2, P3, tmp1, tmp2;

	nodes = *n;
	P1 = P[0];
	P2 = P[1];
	P3 = P[2];
	for (i = 3; i < (nodes + 2); ++i)
	{
		v_subtract(&P2, &P1, &tmp1);
		v_subtract(&P3, &P2, &tmp2);
		if (v_cross3rd_comp(&tmp1, &tmp2) == 0)
		{
			//count of adjacent colinear line segments encountered
			++ret_value;
			//eliminate the extra node
			--*n;
			for (j = i; j < (nodes + 1); ++j)
				P[j - 2] = P[j - 1];
			--nodes;
			//retry this node
			--i;
			P2 = P3;
		}
		else
		{
			P1 = P2;
			P2 = P3;
		}
		P3 = P[i % nodes];
	}
	return (ret_value);
}

/* PATH AROUND ALL OBSTACLE POLYGONS
*
* NAME---AVDPATH---
*  polygons must be closed and not self intersecting (JORDAN polygon)
*  and defined in a clockwise direction.
* input:
*  line segment A-B & polygonList
* output:
*  path
*
* EXAMPLE:
*  AVOIDPATH(A, B, opt, polyList); polyList = pathM, pathN (any order)
*
*  returns path:
*   NO OPTIMIZATION: opt = FALSE
*    A, I1, N4, N3, N2, I2, I3, M5, M4, M3, M2, I4, B
*   WITH OPTIMIZATION: opt = TRUE
*    A, N4, M5, M4, B
*
*                                              M7 ********* M8
*                                                *        *
*               N0     N1                       *         *
*               ********                       *          * M0
*              *        *                     *            *
*             *          *                   *              *
*            *            *                 *                *
*           *              *               *                  *
*       N5 *                *          M6 *                    *
*          *I1               * I2        I3*                 I4 * M1
*  A-------*------------------*-------------*------------------*----------B
*          *                   *N2           *                * M2
*          *                  *               *               *
*          *                 *                 *              *
*       N4 ****************** N3                *             *
*                                                *            * M3
*                                                 *          *
*                                                  *        *
*                                                   *      *
*                                                 M5 ****** M4
*
* - DESIGNATES SUBTRACTION OF 2 VECTORS (returns a vector)
* + DESIGNATES ADDITION OF 2 VECTORS (returns a vector)
* # DESIGNATES DOT PRODUCT OF 2 VECTORS (returns a scalar)
* ^ DESIGNATES CROSS PRODUCT OF 2 VECTORS (returns a vector)
* | V | DESIGNATES SIZE OF VECTOR V (returns a scalar)
* V(i) DESIGNATES COORDINATE i of VECTOR V (returns a scalar)
* ! DESIGNATES LARGEST POSSIBLE VALUE
*
* polyNode:
*  next :next polyNode
*  prev :previous polyNode
*  I1 :(x, y) value of nearest intersection with A
*  I2 :(x, y) value of nearest intersection with B
*  d :1 normal direction, -1 reverse direction
*  I1 :entry node
*  I2 :exit node
*  poly :pointer to polygon
*
* polyList:
*  polygon0 :pointer to possible polygon obstacle
*  polygon1 :pointer to possible polygon obstacle
*  polygon2 :pointer to possible polygon obstacle
*  .
*  .
*  .
*  polygonN :pointer to possible polygon obstacle
*  0 :indicates end of polyList
*/
static void near AvoidPath(AvdPoint* A, AvdPoint* B, polygon *polylist, int opt, AvdPoint* path)
{
	int i = -1, j = -1;
	AvdPoint startPath;
	polyNode *nodePath;
	polyNode *polygonNodePath;
	polygon dupPolyList[MAXPOLY];
	int totalNodes = 0;
	int polyDirections, firstPolyDirection, bestPolyDirection;
	int polyDistance, bestPolyDistance;

	nodePath = 0;
	//duplicate polylist so that we can eliminate polygons while
	//creating unoptimized path yet still retain original polylist
	//for optimization pass.

	startPath = *A;

	//   HOOK
	// this is a good place to get rid of unwanted polygons ie ctp,cnp etc.
	do
	{
		++i; ++j;
		if (polylist[i].type >= MTP)
			--j;
		else
			dupPolyList[j] = polylist[i];
	} while (polylist[i].n != 0);

	//if (j > MAXPOLYGONS)
	//	msgMgr->Fatal(SrcLoc, Msg_TooManyPolygons);

	while (polygonNodePath = AvoidPolygons(&startPath, B, dupPolyList))
	{
		++totalNodes;
		//add first node in polygonNodePath to Npath
		//note that Npath points to last node until path complete.
		if (nodePath)
		{
			nodePath->next = polygonNodePath;
			polygonNodePath->prev = nodePath;
		}
		// else
		FreeNodePath(polygonNodePath->next);
		polygonNodePath->next = 0;
		nodePath = polygonNodePath;
		startPath = polygonNodePath->I2;


		/* Now reduce list of polygons to only those which can interfere
		* with the path starting from the exit of the first polygon and
		* going to the point B. We only generate a path around the first
		* encountered polygon and then recurse to generate a path around
		* the next etc. until there are no more polygons in the way.
		*/
		ReducePolyList(dupPolyList, polygonNodePath);
	}

	//set nodePath to point to the first node
	if (nodePath)
	{
		while (nodePath->prev != 0)
		{
			nodePath = nodePath->prev;
		}
	}

	//Now chain together the paths around the polygons
	//and optimize out any unnecessary line segments.
	if ((opt > 1) && (totalNodes > 1))
	{
		//optimize as much as possible to the level 2**n
		//set up current path directions and try to improve
		//on the total distance from A to B
		if (totalNodes > MAXOPTIMIZEDNODES)
			totalNodes = MAXOPTIMIZEDNODES;
		polyDirections = GetPolyDirections(nodePath) % (1 << totalNodes);
		bestPolyDirection = polyDirections;
		firstPolyDirection = polyDirections;
		OptimizePath(A, B, path, nodePath, polylist, opt);
		bestPolyDistance = GetPolyDistance(path);
		while (firstPolyDirection != (polyDirections = (polyDirections + 1) % (1 << totalNodes)))
		{
			SetPolyDirections(nodePath, polyDirections, totalNodes);
			OptimizePath(A, B, path, nodePath, polylist, opt);
			polyDistance = GetPolyDistance(path);
			if (bestPolyDistance > polyDistance)
			{
				bestPolyDirection = polyDirections;
				bestPolyDistance = polyDistance;
			}
		}
		SetPolyDirections(nodePath, bestPolyDirection, totalNodes);
	}
	OptimizePath(A, B, path, nodePath, polylist, opt);
	FreeNodePath(nodePath);

	return;
}

static AvdPoint* near CopyPath(AvdPoint *path)
{
	int i;
	AvdPoint *P;
	i = 0;
	do
	{
	} while (path[i++].x != ENDOFPATH);
	//getpath may add two points to the path when A and B in NAP polygons.
	P = (AvdPoint*)RNewPtr((i + 2) * sizeof(AvdPoint));
	i = 0;
	do
	{
		P[i] = path[i];
	} while (path[i++].x != ENDOFPATH);
	return(P);
}

static polyNode* near AvoidPolygons(AvdPoint *A, AvdPoint *B, polygon *polylist)
{
	int n1, n2, d, i;
	AvdPoint I1, I2;
	polyNode* node;
	polyNode* newNode;
	polyNode* nodePath;

	nodePath = 0;
	if (!((A->x == B->x) && (A->y == B->y)))
	{
		for (i = 0; polylist[i].n != 0; ++i)
		{
			if (d = PolygonPath(A, B, polylist[i].polyPoints, polylist[i].n, &I1, &I2, &n1, &n2))
			{
				//this polygon is an obstruction, create path node
				newNode = (polyNode*)RNewPtr(sizeof(polyNode));
				newNode->I1 = I1;
				newNode->I2 = I2;
				newNode->d = d;
				newNode->n1 = n1;
				newNode->n2 = n2;
				newNode->poly = polylist[i].polyPoints;
				newNode->n = polylist[i].n;
				newNode->next = 0;
				//add path node to list
				if (!nodePath)
				{
					//newNode first node in path
					nodePath = newNode;
					newNode->next = 0;
					newNode->prev = 0;
				}
				else
				{
					//insert path node so that the list is ordered from A to B
					for (node = nodePath; TRUE; node = node->next)
					{
						if (
							(
							(node->I1.x != A->x)
								&&
								(abs(newNode->I1.x - A->x) < abs(node->I1.x - A->x))
								)
							||
							(
							(node->I1.y != A->y)
								&&
								(abs(newNode->I1.y - A->y) < abs(node->I1.y - A->y))
								)
							)
						{
							//insert newNode before node in path
							newNode->prev = node->prev;
							newNode->next = node;
							node->prev = newNode;
							if (newNode->prev == 0)
								nodePath = newNode;
							else
								(newNode->prev)->next = newNode;
							break;
						}
						if (node->next == 0) break;
					}
					if (newNode->next == 0)
					{
						//insert newNode at end of list
						node->next = newNode;
						newNode->prev = node;
					}
				}
			}
		}
	}
	return(nodePath);
}

static void near ReducePolyList(polygon *polylist, polyNode *nodePath)
{
	polyNode *temp;
	int deleted;
	AvdPoint I1, I2;
	AvdPoint tmp1, tmp2;

	//Eliminate any overlaped polygons.
	//For example polygon2 can be eliminated in the following case:
	/*
	********************
	*                  *
	*                  *
	*     polygon2     *
	*                 *
	*               *
	*             *
	*********  *           *  **********
	*        *  *         *  *         *
	I1*         *  *I3   I4*  *          *I2
	A-----------*----------*--*-----*--*-----------*---------B
	*           *  *   *  *            *
	*            *  * *  *             *
	*             *  *  *              *
	*              *   *               *
	*               * *                *
	*                *                 *
	*                                  *
	*            polygon1              *
	*                                  *
	************************************
	*/
	//First polygon in polylist is checked againest the others to
	//see if any polygons in the list can be eliminated.
	deleted = 0;
	I2 = nodePath->I2;
	I1 = nodePath->I1;
	nodePath = nodePath->next;
	while (nodePath)
	{
		temp = nodePath->next;
		v_subtract(&I2, &I1, &tmp1);
		v_subtract(&I2, &nodePath->I2, &tmp2);
		if (v_dot(&tmp1, &tmp2) <= 0l)
		{
			//remove nodePath->i polygon from polylist
			RemoveNode(polylist, nodePath->i - deleted);
			++deleted;
		}
		nodePath = temp;
	}
}

static void near RemoveNode(polygon *polylist, int i)
{
	do
	{
		polylist[i] = polylist[i + 1];
		++i;
	} while (polylist[i].n != 0);
}

static void near FreeNodePath(polyNode *nodePath)
{
	polyNode *node, *tmp;
	for (node = nodePath; node != 0;)
	{
		tmp = node->next;
		DisposePtr(node);
		node = tmp;
	}
}

static int near GetPolyDirections(polyNode *nodePath)
{
	int polyDirections = 0;
	int d, i;
	polyNode *node;
	for (node = nodePath, i = 0; (node != 0) && (i <= MAXOPTIMIZEDNODES); ++i)
	{
		d = 1;
		if (node->d < 0)
			d = 0;
		polyDirections += (d << i);
		node = node->next;
	}
	return(polyDirections);
}

static int near GetPolyDistance(AvdPoint* path)
{
	int distance = 0, offScreen = 0;
	int i;
	for (i = 0; path[i + 1].x != ENDOFPATH; ++i)
		distance += DistanceEstimate(&path[i], &path[i + 1], &offScreen);
	if (offScreen)
		return(MAXVALUEINT);
	return(distance);
}

static void near SetPolyDirections(polyNode *nodePath, int polyDirections, int totalNodes)
{
	polyNode *node;
	int i, d;
	for (node = nodePath, i = 0; (node != 0) && (i < totalNodes); ++i)
	{
		d = (polyDirections >> i) & 1;
		if (!d) d = -1;
		if (node->d != d)
		{
			if (node->d == 1)
			{
				node->d = -1;
				node->n1 = (node->n1 - 1 + node->n) % node->n;
				node->n2 = (node->n2 + 1 + node->n) % node->n;
			}
			else
			{
				node->d = 1;
				node->n1 = (node->n1 + 1 + node->n) % node->n;
				node->n2 = (node->n2 - 1 + node->n) % node->n;
			}
		}
		node = node->next;
	}
}

/* OPTIMIZE PATH AROUND ALL OBSTACLE POLYGONS
*
* NAME---OPTPATH---
* input:
*  line segment A-B, path nodes created by AvoidPath, polygonList, opt
* output:
*  path
*/
static void near OptimizePath(AvdPoint *A, AvdPoint *B, AvdPoint* path, polyNode *nodePath, polygon *polylist, int opt)
{
	int i, j, k, x;
	int M, P0, PN, PG;
	polyNode *node;
	AvdPoint I1, I2;
	int nodeI1, nodeI2;


	//Chain path nodes together to make one large path
	//first point is A
	path[0].x = A->x;
	path[0].y = A->y;

	i = 1;
	for (node = nodePath; node != 0;)
	{
		path[i].x = node->I1.x;
		path[i++].y = node->I1.y;
		j = (node->n1 - node->d + node->n) % node->n;

		do
		{
			j = (j + node->d + node->n) % node->n;
			path[i].x = (node->poly)[j].x;
			path[i++].y = (node->poly)[j].y;
		} while (j != node->n2);

		path[i].x = node->I2.x;
		path[i++].y = node->I2.y;
		node = node->next;
	}

	//last point is B
	path[i].x = B->x;
	path[i++].y = B->y;
	path[i].x = ENDOFPATH;
	if (i >= MAXPATH)
		Panic(E_POLY_AVOID);

	//start optimization of path
	if (!opt || (i < 3))
		return;
	else
	{
		//get rid of any adjacent dupicate points
		for (j = 0; path[j].x != ENDOFPATH; ++j)
		{
			if ((path[j].x == path[j + 1].x) && (path[j].y == path[j + 1].y))
			{
				for (k = j; path[k].x != ENDOFPATH; ++k)
					path[k] = path[k + 1];
				--j;
				--i;
			}
		}

		M = i;
		P0 = 0;
		for (PG = 0, PN = M - 1; P0 < M - 2; PG = 0)
		{
			x = FALSE;
			for (; polylist[PG].n != 0; ++PG)
			{
				if (IntersectPolygon(&path[P0],
					&path[PN],
					polylist[PG].polyPoints,
					polylist[PG].n,
					&I1,
					&I2,
					&nodeI1,
					&nodeI2)
					)
				{
					x = TRUE;
					break;
				}
			}
			if (x)
			{
				if (PN > P0 + 2)
					PN -= 1;
				else
				{
					P0 += 1;
					PN = M - 1;
				}
			}
			else
			{
				//eliminate nodes P0+1 through PN-1
				for (i = P0 + 1, j = PN, k = M - PN + 1; k != 0; --k)
				{
					path[i].x = path[j].x;
					path[i].y = path[j].y;
					++i;
					++j;
				}
				M = M - PN + P0 + 1;
				PN = M - 1;
				P0 += 1;
			}
		}
	}
}

AvdPoint* MergePolygons(AvdPoint *poly, polygon *polylist)
{
	AvdPoint P[MAXPATH];
	AvdPoint *newPoly;
	int i, j, n;

	//copy polygon to merge into P
	for (i = 0; (poly[i].x != ENDOFPATH) && (i < MAXPATH); ++i)
		P[i] = poly[i];
	if (i >= MAXPATH)
		Panic(E_POLY_MERGE);

	P[i].x = ENDOFPATH;
	P[i].y = ENDOFPATH;
	n = i;

	//merge given polygon with each polygon in the list
	//A merged polygon in the list will be marked as MTP, MNP, MBP or MCP.
	//The resultant polygon will be marked BAP.

	for (i = 0; polylist[i].n != 0; ++i)
	{
		MergePolygon(P, n, &polylist[i]);
		for (n = 0; P[n].x != ENDOFPATH; ++n);
	}

	// Get rid of duplicate points and colinear lines
	for (i = 0; P[i].x != ENDOFPATH; ++i)
	{
		if (P[i].x == P[i + 1].x && P[i].y == P[i + 1].y)
		{
			for (j = i + 1; P[j].x != ENDOFPATH; ++j)
			{
				P[j].x = P[j + 1].x;
				P[j].y = P[j + 1].y;
			}
			--i;
		}
	}

	TestColinear(P, &i);
	//newPoly[i].x = ENDOFPATH;
	//newPoly[i].y = ENDOFPATH;

	//return the new polygon
	for (i = 0; P[i].x != ENDOFPATH; ++i);
	++i;
	newPoly = (AvdPoint*)RNewPtr((i * sizeof(AvdPoint)));
	j = 0;
	for (i = 0; P[i].x != ENDOFPATH; ++i)
	{
		if ((P[i].x != P[i + 1].x) || (P[i].y != P[i + 1].y))
		{
			newPoly[j] = P[i];
			j++;
		}
	}

	newPoly[i].x = ENDOFPATH;
	newPoly[i].y = ENDOFPATH;

	return (newPoly);
}

static void near MergePolygon(AvdPoint *P, int n, polygon *poly)
{
	int i, j, k, m;
	int addNode;
	int t1, P_i, Q_i, P_j, Q_j;
	int angleIn, angleOut, delta, angle;
	AvdPoint *Q, P_U, Q_U, Result[MAXPATH];
	AvdPoint tmp1, tmp2;
	polyPatch Patch[MAXPATCHES], newPatch;
	int patches = 0, p;

	//HOOK
	//make sure that merged polygons are marked as mtp, mnp etc.
	Q = poly->polyPoints;
	m = poly->n;
	for (P_i = 0; P_i < n; ++P_i)
	{
		for (Q_i = 0; Q_i < m; ++Q_i)
		{
			t1 = IntersectSegments(&P[P_i], &P[(P_i + 1 + n) % n], &Q[Q_i], &Q[(Q_i + 1 + m) % m], &P_U);
			if ((t1 != NOINTERSECT) && (t1 != INTERSECT + INTERSECTD))
			{
				//Is this an exit from P intersection? For example:
				/*
				*P[i+1]
				*
				*
				*
				INSIDE   *   OUTSIDE
				*
				*
				Q[j]---------*---------------->Q[j+1]   This is an exiting intersection
				*
				*
				*
				*
				P[i]*

				*P[i+1]
				*
				*
				*
				INSIDE    *   OUTSIDE
				*
				*
				Q[j+1]<------*-----------------Q[j]     This is an entering intersection
				*
				*
				*
				*
				P[i]*
				*/
				//test for exit or entry
				v_subtract(&P[(P_i + 1 + n) % n], &P[P_i], &tmp1);
				v_subtract(&Q[(Q_i + 1 + m) % m], &Q[Q_i], &tmp2);
				if (v_cross3rd_comp(&tmp1, &tmp2) < 0)
				{
					//now test for clock wise or counter clockwise
					angle = 0;
					angleIn = ATan(Q[Q_i].x, Q[Q_i].y, Q[(Q_i + 1 + m) % m].x, Q[(Q_i + 1 + m) % m].y);
					for (Q_j = Q_i + 1; Q_j <= Q_i + m; ++Q_j)
					{
						angleOut = ATan(Q[(Q_j + m) % m].x, Q[(Q_j + m) % m].y, Q[(Q_j + 1 + m) % m].x, Q[(Q_j + 1 + m) % m].y);
						delta = angleOut - angleIn;
						if (delta > 180) delta -= 360;
						if (delta < -180) delta += 360;
						angle += delta;
						angleIn = angleOut;
						for (P_j = P_i; P_j <= P_i + n; ++P_j)
						{
							t1 = IntersectSegments(&P[(P_j + n) % n], &P[(P_j + 1 + n) % n], &Q[(Q_j + m) % m], &Q[(Q_j + 1 + m) % m], &Q_U);
							if ((t1 != NOINTERSECT) && (t1 != INTERSECT + INTERSECTD))
							{
								//test for exit or entry
								v_subtract(&P[(P_j + 1 + n) % n], &P[(P_j + n) % n], &tmp1);
								v_subtract(&Q[(Q_j + 1 + m) % m], &Q[(Q_j + m) % m], &tmp2);
								if (v_cross3rd_comp(&tmp1, &tmp2) > 0)
									break;
								else
									t1 = NOINTERSECT;
							}
							else
								t1 = NOINTERSECT;
						}
						if (t1 != NOINTERSECT)
							break;
					}
					//we need to find an intersection
					if (t1 != NOINTERSECT)
					{
						//if loop to add is screen clockwise add to P patches
						if (angle > 0)
						{
							if (patches >= MAXPATCHES)
								Panic(E_MAX_PATCHES);
							//add loop to patches
							if (patches == 0)
							{
								Patch[patches].P_i = P_i;
								Patch[patches].Q_i = Q_i;
								Patch[patches].P_U = P_U;
								Patch[patches].P_j = (P_j + n) % n;
								Patch[patches].Q_j = (Q_j + m) % m;
								Patch[patches].Q_U = Q_U;
								Patch[patches++].deleteIt = FALSE;
							}
							else
							{
								newPatch.P_i = P_i;
								newPatch.Q_i = Q_i;
								newPatch.P_U = P_U;
								newPatch.P_j = (P_j + n) % n;
								newPatch.Q_j = (Q_j + m) % m;
								newPatch.Q_U = Q_U;
								for (p = 0; p < patches; p++)
									if (Dominates(&Patch[p], &newPatch, P))
										break;
								if (p == patches)
								{
									Patch[patches].P_i = P_i;
									Patch[patches].Q_i = Q_i;
									Patch[patches].P_U = P_U;
									Patch[patches].P_j = (P_j + n) % n;
									Patch[patches].Q_j = (Q_j + m) % m;
									Patch[patches].Q_U = Q_U;
									Patch[patches++].deleteIt = FALSE;
									for (p = 0; p < patches - 1; p++)
										if (Dominates(&newPatch, &Patch[p], P))
											Patch[p].deleteIt = TRUE;
								}
							}
						}
					}
				}
			}
		}
	}
	if (patches)
	{
		//merge the polygons
		//mark as merged polygon
		poly->type = poly->type + MTP;
		j = 0;
		for (P_i = 0; P_i < n; ++P_i)
		{
			//if P_i is not in a patch add node
			for (i = 0, addNode = TRUE; (i < patches) && (addNode == TRUE); ++i)
				if (Patch[i].deleteIt == FALSE)
					if (PatchNode(&Patch[i], P_i, P))
						addNode = FALSE;
			if (addNode) Result[j++] = P[P_i];
			//if at a patch, add the patch
			for (i = 0; i < patches; ++i)
			{
				if (Patch[i].deleteIt == FALSE)
				{
					if (P_i == Patch[i].P_i)
					{
						if (j >= MAXPATH)
							Panic(E_MAX_POINTS);
						if (!(Patch[i].P_U.x == P[P_i].x && Patch[i].P_U.y == P[P_i].y))
							Result[j++] = Patch[i].P_U;
						for (k = (Patch[i].Q_i + 1 + m) % m; k != Patch[i].Q_j; k = (k + 1 + m) % m, ++j)
							Result[j] = Q[(k + m) % m];
						Result[j++] = Q[Patch[i].Q_j];
						if (!(Patch[i].Q_U.x == Q[Patch[i].Q_j].x && Patch[i].Q_U.y == Q[Patch[i].Q_j].y))
							Result[j++] = Patch[i].Q_U;
					}
				}
			}
		}
		for (i = 0; i < j; ++i)
			P[i] = Result[i];
		if ((P[i - 1].x == P[0].x) && (P[i - 1].y == P[0].y))
			i--;
		P[i].x = ENDOFPATH;
		P[i].y = ENDOFPATH;
	}
}

static int near PatchNode(polyPatch *Patch, int theNode, AvdPoint *thePoly)
{
	int offScreen;
	if (Patch->P_i < Patch->P_j)
		if ((Patch->P_i < theNode) && (theNode <= Patch->P_j))
			return (TRUE);
	if (Patch->P_j < Patch->P_i)
		if ((Patch->P_i < theNode) || (theNode <= Patch->P_j))
			return (TRUE);
	if (Patch->P_i == Patch->P_j)
		if (DistanceEstimate(&Patch->P_U, &thePoly[Patch->P_i], &offScreen) > DistanceEstimate(&Patch->Q_U, &thePoly[Patch->P_i], &offScreen))
			return (TRUE);
	return (FALSE);
}

static int near Dominates(polyPatch *A, polyPatch *B, AvdPoint *poly)
{
	int Api, Bpi, Apj, Bpj, offScreen = 0;

	Api = A->P_i;
	Apj = A->P_j;
	Bpi = B->P_i;
	Bpj = B->P_j;
	if ((A->P_U.x == B->P_U.x) && (A->P_U.y == B->P_U.y) && (A->Q_U.x == B->Q_U.x) && (A->Q_U.y == B->Q_U.y))
		//The same patch!
		return(TRUE);

	if (Api != Apj)
	{
		if (((Api<Apj) && ((Api < Bpi) && (Bpi < Apj))) || ((Api>Apj) && ((Api < Bpi) || (Bpi < Apj))))
			/*
			patch A
			*-----------------------------*
			|                             |
			|    patch B                  |
			|  *----------...             |
			|  |                          |
			|  |                          |
			|  |       ********           |
			|  |      *        *          |
			|  *-----*          *         |
			|       *            *--------*
			|      *              *
			|     *                *
			|     *                 *
			*-----*                  *
			*                   *
			*                  *
			*                 *
			******************
			*/
			return(TRUE);

		if (((Api<Apj) && ((Api < Bpj) && (Bpj < Apj))) || ((Api>Apj) && ((Api < Bpj) || (Bpj < Apj))))
			/*
			patch A
			*-----------------------------*
			|                             |
			|    patch B                  |
			|  *----------*               |
			|             |               |
			|             |               |
			|          ********           |
			|         *        *          |
			|        *          *         |
			|       *            *--------*
			|      *              *
			|     *                *
			|     *                 *
			*-----*                  *
			*                   *
			*                  *
			*                 *
			******************
			*/
			return(TRUE);
	}
	if (Bpi != Bpj)
	{
		if (((Bpi<Bpj) && ((Bpi < Api) && (Api < Bpj))) || ((Bpi>Bpj) && ((Bpi < Api) || (Api < Bpj))))
			/*
			patch B
			*-----------------------------*
			|                             |
			|    patch A                  |
			|  *----------...             |
			|  |                          |
			|  |                          |
			|  |       ********           |
			|  |      *        *          |
			|  *-----*          *         |
			|       *            *--------*
			|      *              *
			|     *                *
			|     *                 *
			*-----*                  *
			*                   *
			*                  *
			*                 *
			******************
			*/
			return(FALSE);
		if (((Bpi<Bpj) && ((Bpi < Apj) && (Apj < Bpj))) || ((Bpi>Bpj) && ((Bpi < Apj) || (Apj < Bpj))))
			/*
			patch B
			*-----------------------------*
			|                             |
			|    patch A                  |
			|  *----------*               |
			|             |               |
			|             |               |
			|          ********           |
			|         *        *          |
			|        *          *         |
			|       *            *--------*
			|      *              *
			|     *                *
			|     *                 *
			*-----*                  *
			*                   *
			*                  *
			*                 *
			******************
			*/
			return(FALSE);
	}
	if (Api != Apj)
	{
		if (Bpi == Bpj)
		{
			if (Api == Bpi)
				if (DistanceEstimate(&poly[A->P_i], &A->P_U, &offScreen) < DistanceEstimate(&poly[A->P_i], &B->P_U, &offScreen))
					/*
					patch A
					*----------------*
					|                |
					|  ************************
					|  *                      *
					|  *                      *-------*
					|  *                      *       |
					|  *                      *       |
					|  *                      *---*p  |
					|  *                      *   |a  |
					|  *                      *   |t  |
					|  *                      *   |c  |
					|  *                      *   |h  |
					|  *                      *   |B  |
					|  *                      *---*   |
					|  *                      *       |
					|  *                      *       |
					|  *                     *        |
					|  *                    *         |
					|  *********************          |
					|                                 |
					*---------------------------------*
					*/
					return(TRUE);
				else
					/*
					patch A
					*----------------*
					|                |
					|  ************************
					|  *                      *
					|  *                      *
					|  *                      *
					|  *                      *
					|  *                      *---*p
					|  *                      *   |a
					|  *                      *   |t
					|  *                      *   |c
					|  *                      *   |h
					|  *                      *   |B
					|  *                      *---*
					|  *                      *
					|  *                      *-------*
					|  *                      *       |
					|  *                      *       |
					|  *                     *        |
					|  *                    *         |
					|  *********************          |
					|                                 |
					*---------------------------------*
					*/
					return(FALSE);
			if (Apj == Bpi)
				if (DistanceEstimate(&poly[A->P_j], &A->Q_U, &offScreen) > DistanceEstimate(&poly[A->P_j], &B->P_U, &offScreen))
					/*
					patch A
					*----------------*
					|                |
					|    patch B     |
					|    *-----*     |
					|    |     |     |
					|    |     |     |
					|  ************************
					|  *                      *
					|  *                      *
					|  *                      *
					|  *                      *---*
					|  *                      *   |
					|  *                      *   |
					|  *                      *   |
					|  *                      *   |
					|  *                     *    |
					|  *                    *     |
					|  *********************      |
					|                             |
					*-----------------------------*
					*/
					return(TRUE);
				else
					/*
					patch A
					*---------*
					|         |
					|         |   patch B
					|         |   *-----*
					|         |   |     |
					|         |   |     |
					|  ************************
					|  *                      *
					|  *                      *
					|  *                      *
					|  *                      *---*
					|  *                      *   |
					|  *                      *   |
					|  *                      *   |
					|  *                      *   |
					|  *                     *    |
					|  *                    *     |
					|  *********************      |
					|                             |
					*-----------------------------*
					*/
					return(FALSE);
		}
		if (Api != Bpi)
			/*
			patch A                            patch B
			*-------------*                    *-------------*
			|             |                    |             |
			|             |                    |             |
			|          ********                |          ********
			|         *        *               |         *        *
			|        *          *              |        *          *
			|       *            *--------*    |       *            *--------*
			|      *              *       |p   |      *              *       |p
			|     *                *      |a   |     *                *      |a
			|     *                 *     |t   |     *                 *     |t
			*-----*                  *    |c   *-----*                  *    |c
			*                   *   |h         *                   *   |h
			*                  *    |B         *                  *    |A
			*                 *-----*          *                 *-----*
			******************                 ******************
			*/
			return(FALSE);
		if (DistanceEstimate(&poly[A->P_i], &A->P_U, &offScreen) < DistanceEstimate(&poly[A->P_i], &B->P_U, &offScreen))
			/*
			patch A
			*-----------------------------*
			|                             |
			|    patch B                  |
			|  *----------*               |
			|  |          |               |
			|  |          |               |
			|  |       ********           |
			|  |      *        *          |
			|  |     *          *         |
			|  |    *            *--------*
			|  |   *              *
			|  |  *                *
			|  *--*                 *
			*-----*                  *
			*                   *
			*                  *
			*                 *
			******************
			*/
			return(TRUE);
		else
			/*
			patch B
			*-----------------------------*
			|                             |
			|    patch A                  |
			|  *----------*               |
			|  |          |               |
			|  |          |               |
			|  |       ********           |
			|  |      *        *          |
			|  |     *          *         |
			|  |    *            *--------*
			|  |   *              *
			|  |  *                *
			|  *--*                 *
			*-----*                  *
			*                   *
			*                  *
			*                 *
			******************
			*/
			return(FALSE);
	}
	if ((Api == Apj) && (Api != Bpi))
	{
		//if A wraps around it dominates B.
		if (DistanceEstimate(&poly[A->P_i], &A->P_U, &offScreen) > DistanceEstimate(&poly[A->P_i], &A->Q_U, &offScreen))
			/*
			patch A
			*--------------------------------*
			|                                |
			|          ********              |
			|         *        *             |
			|        *          *--------*   |
			|       *            *       |p  |
			|      *              *      |a  |
			|     *                *     |t  |
			|     *                 *    |c  |
			*-----*                  *   |h  |
			*                   *  |B  |
			*-----*                  *---*   |
			|     *                 *        |
			|     ******************         |
			|                                |
			*--------------------------------*
			*/
			return(TRUE);
	}

	//if either patches wrap around then their can not be a dominance.
	if (DistanceEstimate(&poly[A->P_i], &A->P_U, &offScreen) > DistanceEstimate(&poly[A->P_i], &A->Q_U, &offScreen))
		/*
		patch A
		*-----------------------------*
		|                             |
		|          ********           |
		|         *        *          |
		|        *          *         |
		|       *            *        |
		|      *              *       |
		|     *                *      |
		|     *                 *     |
		*-----*                  *    |
		*                   *   |
		*-----*                  *    |
		|     *                 *     |
		|     ******************      |
		|                             |
		*-----------------------------*
		*/
		return(FALSE);
	if (DistanceEstimate(&poly[B->P_i], &B->P_U, &offScreen) > DistanceEstimate(&poly[B->P_i], &B->Q_U, &offScreen))
		/*
		patch B
		*-----------------------------*
		|                             |
		|          ********           |
		|         *        *          |
		|        *          *         |
		|       *            *        |
		|      *              *       |
		|     *                *      |
		|     *                 *     |
		*-----*                  *    |
		*                   *   |
		*-----*                  *    |
		|     *                 *     |
		|     ******************      |
		|                             |
		*-----------------------------*
		*/
		return(FALSE);

	if (DistanceEstimate(&poly[B->P_i], &B->P_U, &offScreen) > DistanceEstimate(&poly[B->P_i], &A->P_U, &offScreen))
		if (DistanceEstimate(&poly[B->P_i], &A->Q_U, &offScreen) > DistanceEstimate(&poly[B->P_i], &B->P_U, &offScreen))
			/*
			*************
			*            *
			*------------*             *
			p |            *              *
			a |            *               *
			t |            *                *
			c |   patchB   *                 *
			h |  *---------*                  *
			A |  |         *                   *
			|  *---------*                  *
			*------------*                 *
			******************
			*/
			return(TRUE);
	/*
	********                          ********
	*       *                         *       *
	*------------*        *           *------------*        *
	p |            *         *          |            *         *
	a |            *          *         |            *          *
	t |            *           *        *------------*           *
	c |   patchB   *            *                    *            *
	h |  *---------*             *      *------------*             *
	A |  |         *              *     |            *              *
	|  *---------*             *      |            *             *
	*------------*            *       *------------*            *
	*************                     *************
	*/
	return(FALSE);
}

int PointInterior(AvdPoint *M, AvdPoint *P, int n)
{
	AvdPoint N3, P1, P2, P3, P4;
	AvdPoint tmp1, tmp2;
	int nextnode, intr;
	intr = 0;
	P1 = P[0];
	P2 = P[1];
	nextnode = 3;
	P3 = N3 = P[2];
	if (n > 3)
		P4 = P[3];
	else
		P4 = P[0];
start:
	//If M is interior to some line segment of the polygon consider it as interior.
	if ((((P1.y <= M->y) && (M->y <= P2.y)) || ((P2.y <= M->y) && (M->y <= P1.y))) && (((P1.x <= M->x) && (M->x <= P2.x)) || ((P2.x <= M->x) && (M->x <= P1.x))))
	{
		v_subtract(M, &P1, &tmp1);
		v_subtract(M, &P2, &tmp2);
		if (v_cross3rd_comp(&tmp1, &tmp2) == 0)
			return(TRUE);
	}

	//The alogorithm counts the number of intersections with the
	//given polygon going from M in the negative x direction.
	//If this number is odd M must be interior to the polygon.

	//if M is above or below P1-P2 then there can't be
	//an intersection of the ray from M in the negative x
	//direction with the line segment P1-P2.
	if (((P1.y < M->y) && (M->y < P2.y)) || ((P2.y < M->y) && (M->y < P1.y)))
	{
		/* If P1-P2 or P2-P1 is chosen so that the y delta is
		* positive and this vector is crossed with the vector
		* P1-M then the z component is positive if M lies on
		* the negative side of P1-P2 and negative if M lies
		* on the positive side of P1-P2.
		*/
		v_subtract(&P2, &P1, &tmp1);
		if (tmp1.y < 0)
		{
			tmp1.x = -tmp1.x;
			tmp1.y = -tmp1.y;
		}
		v_subtract(M, &P1, &tmp2);
		if (v_cross3rd_comp(&tmp1, &tmp2) > 0)
			++intr;
	}
	else
	{
		/* If the ray in the negative x direction from M passes
		* through the point P2 then one must look at the next
		* segment of the polygon which is not parallel with the
		* x axis in order to determine if the is an intersection
		* at P2. Note that consecutive line segment are not allowed
		* to be parallel in a given polygon.
		*/
		if ((P2.y == M->y) && (M->x < P2.x))
		{
			/* types of intersections checked here:
			*           *       *    *             *            *
			*           *     *      *             *          *
			*           *   *        *             *        *
			*           * *          *             *      *
			<---------*-----------*------------*****---------******-------------M
			*                             *
			*                              *
			*                               *
			*                                *
			*                                 *

			intersection?  yes          no             yes           no
			*/
			if (P3.y != P2.y)
			{
				if ((long)(P2.y - P1.y) * (long)(P3.y - P2.y) > 0L)
					++intr;
			}
			else
			{
				if ((long)(P2.y - P1.y) * (long)(P4.y - P3.y) > 0L)
					++intr;
			}
		}
	}
	P1 = P2;
	P2 = P3;
	P3 = P4;
	//See if there are any more line segments in the polygon to be tested.
	if ((P3.x == N3.x) && (P3.y == N3.y))
	{
		if (intr & 1)
			return(TRUE);
		else
			return(FALSE);
	}
	else
		P4 = P[(++nextnode) % n];
	goto start;
}

/* POLYPATH.C PATH AROUND AN OBSTACLE POLYGON
*
* Algorithm by Larry Scott
*
* NAME---POLYPATH---
*  POLYGON must be closed and not self intersecting (JORDAN polygon)
* input:
*  line segment A-B & POLYGON P1, P2, ...PN
* output:
*  return 0 polygon not an obstacle, 1 path of nodes returned
*  nodeI1 first node
*  nodeI2 last node
*  direction 1 increasing order, -1 decreasing order
*  I1 intersecting point closest to A
*  I2 intersecting point closest to B
*
* EXAMPLE:
* POLYPATH(A, B, path) returns (1, N4, N2, -1, x1, y1, x2, y2)
*
*                 N0     N1
*                 ********
*                *        *
*               *          *
*              *            *
*             *              *
*         N5 *                *
*            *(x1,y1)=I1       * (x2,y2)=I2
*  A---------*------------------*---------------B
*            *                   *N2
*            *                  *
*            *                 *
*         N4 ****************** N3
*/

static int near PolygonPath(AvdPoint *A, AvdPoint *B, AvdPoint *points, int n, AvdPoint *I1, AvdPoint *I2, int *nodeI1, int *nodeI2)
{
	int distA, distB;
	int offScreenA = FALSE, offScreenB = FALSE;

	if (IntersectPolygon(A, B, points, n, I1, I2, nodeI1, nodeI2) == 0)
		return(0);
	if (*nodeI1 == *nodeI2)
		return(0);
	//nodeI1 = P[0]; first node
	//nodeI2 = P[n - i]; last node
	distA = DistanceSquared(1, *nodeI1 + 1, *nodeI2, points, n, I1, I2, &offScreenA);
	distB = DistanceSquared(-1, *nodeI1, *nodeI2 + 1, points, n, I1, I2, &offScreenB);
	if (offScreenA && (!offScreenB))
	{
		*nodeI2 = (*nodeI2 + 1) % n;
		return(-1);
	}
	if ((!offScreenA) && offScreenB)
	{
		*nodeI1 = (*nodeI1 + 1) % n;
		return(1);
	}
	if (distA < distB)
	{
		*nodeI1 = (*nodeI1 + 1) % n;
		return(1);
	}
	else
	{
		*nodeI2 = (*nodeI2 + 1) % n;
		return(-1);
	}
}

/* d = direction
* nodeF = first node
* nodeL = last node
* P = polygon
* offScreen = pointer to offscreen TRUE or FALSE
*/
static int near DistanceSquared(int direction, int nodeF, int nodeL, AvdPoint *points, int n, AvdPoint *I1, AvdPoint *I2, int *offScreen)
{
	int distance = 0;
	int nodeM;
	AvdPoint P1, P2;
	nodeM = nodeF = nodeF % n;
	nodeL = nodeL % n;
	P1 = points[nodeM];
	for (; nodeM != nodeL;)
	{
		//adding n to mod because % will return a negative
		nodeM = (nodeM + direction + n) % n;
		P2 = points[nodeM];
		//DistanceEstimate is more accurate than the sum
		//of the squares of the distances for deciding
		//which way around a polygon is shorter.
		distance += DistanceEstimate(&P2, &P1, offScreen);
		P1 = P2;
	}
	if (direction > 0)
	{
		distance += DistanceEstimate(I1, &points[nodeF], offScreen);
		distance += DistanceEstimate(&points[nodeL], I2, offScreen);
	}
	else
	{
		distance += DistanceEstimate(I1, &points[nodeF], offScreen);
		distance += DistanceEstimate(&points[nodeL], I2, offScreen);
	}
	return(distance);
}

static int near DistanceEstimate(AvdPoint *P2, AvdPoint *P1, int *offScreen)
{
	int deltaX, deltaY, temp;
	//if line lies on screen edge offScreen set to TRUE
	if (LineOnScreenEdge(P1, P2))
		*offScreen = TRUE;
	/* estimate distance by the formula:
	* If max(deltaX, deltaY) <= (10*min(deltaX, deltaY))/6)
	* then use (13*max(deltaX, deltaY))/10
	* else use max(deltaX, deltaY)
	* This formula guarantees the max error is less than
	* 11% and the average error is about 5%.
	*/
	deltaX = abs(P2->x - P1->x);
	deltaY = abs(P2->y - P1->y);
	if (deltaX > deltaY)
	{
		temp = deltaX;
		deltaX = deltaY;
		deltaY = temp;
	}
	if (((deltaX * 10) / 6) >= deltaY)
		return(((13 * deltaY) / 10));
	else
		return(deltaY);
}

static int near LineOnScreenEdge(AvdPoint *p1, AvdPoint *p2)
{
	if
		(
		(p1->x == p2->x) && (p1->y != p2->y) &&
			(
			(p1->x == 0) ||
				(p1->x == picWindPoly[1].x)
				)
			)
		return(1);
	if
		(
		(p1->y == p2->y) && (p1->x != p2->x) &&
			(
			(p1->y == 0) ||
				(p1->y == picWindPoly[2].y)
				)
			)
		return(1);
	return(0);
}

/* INTPOLY.C INTERSECT LINE SEGMENT AND A POLYGON
*
* Algorithm by Larry Scott
*
* NAME---INTPOLY---
*  POLYGON must be closed and not self intersecting (JORDAN polygon)
* input:
*  line segment A-B & POLYGON P1, P2, ...PN
* output:
*  return number ofintersections
*  I1 intersection closest to A
*  I2 intersection closest to B
*  nodeI1 start node of line intersecting at I1
*  nodeI2 start node of line intersecting at I2
*/
static int near IntersectPolygon(AvdPoint *A, AvdPoint *B, AvdPoint *points, int n, AvdPoint *INT1, AvdPoint *INT2, int *nodeI1, int *nodeI2)
{
	long distA = MAXVALUELONG, distB = MAXVALUELONG, d;
	AvdPoint N1, N2, P1, P2, P3, V, W, F1, tmp1, tmp2, tmp3, tmp4, I1, I2;
	int i, node, nextnode, f1, t1, t2;
	N1 = points[0];
	P1 = N1;
	N2 = points[1];
	P2 = points[1];
	P3 = points[2];
	i = 0; node = 0; //first three nodes already taken
	f1 = IntersectSegments(A, B, &P1, &P2, &V);
	t1 = f1;
	F1 = V;
	nextnode = 2;

start:
	t2 = IntersectSegments(A, B, &P2, &P3, &W);
	if ((t1 == NOINTERSECT) || (t1 == INTERSECT + INTERSECTC))
	{
	getnextnode:
		t1 = t2;
		P1 = P2;
		P2 = P3;
		V = W;
		++node;
		if ((n - 1) <= nextnode)
			if (P3.x == N2.x && P3.y == N2.y)
			{
				*INT1 = I1;
				*INT2 = I2;
				return(i);
			}
			else if (P3.x == N1.x && P3.y == N1.y)
			{
				P3 = N2;
				t2 = f1;
				W = F1;
			}
			else
			{
				P3 = N1;
			}
		else
			P3 = points[++nextnode];
		goto start;
	}
	else
	{
		if (t1 == COLINEAR)
		{
			/* If P1-P2 is colinear with A-B and A-B contains P2, then
			* one of the following cases must apply. This is because
			* all polygon must be defined in a mathematical counter
			* clockwise direction.
			* For example:
			P1      P2                           There is an
			A-----*********----------------------B     intersection at P2
			*
			*
			OUTSIDE    *     INSIDE
			*
			*
			*
			P3



			*
			*
			INSIDE      *
			*   OUTSIDE
			*
			P1     P2*                           There is no
			A-----*********----------------------B     intersection at P2

			IF A=P2 OR B=P2

			P1      P2,A                         There is an
			*********----------------------B     intersection at P2
			*
			*
			OUTSIDE    *     INSIDE
			*
			*
			*
			P3

			P1      P2,A                         There is no
			B-----*********                            intersection at P2
			*
			*
			OUTSIDE    *     INSIDE
			*
			*
			*
			P3

			P1      P2,B                         There is an
			*********----------------------A     intersection at P2
			*
			*
			OUTSIDE    *     INSIDE
			*
			*
			*
			P3

			P3
			*
			*
			INSIDE      *   OUTSIDE
			*
			*
			P1   P2,B*                           There is no
			A-----*********                            intersection at P2
			*/
			//See if A-B contains P2.
			v_subtract(&P2, A, &tmp1);
			v_subtract(&P2, B, &tmp2);
			if (v_dot(&tmp1, &tmp1) <= 2L)
			{
				//A = P2
				//If (P2-P1)#(B-A) > 0 then intersection at A otherwise not an intersection
				v_subtract(&P2, &P1, &tmp3);
				v_subtract(B, A, &tmp4);
				if (v_dot(&tmp3, &tmp4) > 0L)
				{
					V = P2;
					goto intersection;
				}
				else
				{
					goto getnextnode;
				}
			}
			if (v_dot(&tmp2, &tmp2) <= 2L)
			{
				//B = P2
				//If (P2-P1)#(B-A) < 0 then intersection at B otherwise not an intersection
				v_subtract(&P2, &P1, &tmp3);
				v_subtract(B, A, &tmp4);
				if (v_dot(&tmp3, &tmp4) < 0L)
				{
					V = P2;
					goto intersection;
				}
				else
				{
					goto getnextnode;
				}
			}
			//If (P2-A)#(P2-B) < 0 then A-B contains P2.
			if (v_dot(&tmp1, &tmp2) < 0L)
			{
				//If (P2-P1)^(P3-P2) < 0 then P2 is an intersection.
				v_subtract(&P2, &P1, &tmp1);
				v_subtract(&P3, &P1, &tmp2);
				if (v_cross3rd_comp(&tmp1, &tmp2) < 0)
				{
					V = P2;
					goto intersection;
				}
			}
			goto getnextnode;
		}

		if (t1 == INTERSECT + INTERSECTD)
		{
			if (t2 == COLINEAR)
			{
				/* If intersection at P2, A == P2 and P2-P3 is colinear,
				* Then if (P3-P2)#(B-A) < 0 and (P2-P1)^(P3-P2) < 0 then P2 is an intersection.
				* If intersection at P2, A != P2 and P2-P3 is colinear,
				* then if (P2-P1)^(P3-P2) < 0 then P2 is an intersection.
				* Otherwise D is not an intersection.
				* This is because all polygons must be defined in a counter clockwise direction.
				* For example:
				A,P2     P3             There is an no
				B---------------*******               intersection at P2
				*
				*
				OUTSIDE  *   INSIDE
				*
				P1*

				P1*
				*   OUTSIDE
				*
				INSIDE   *
				*P2     P3             There is an
				B---------------*******               intersection at P2
				A

				P1*
				*
				*   OUTSIDE
				INSIDE *
				*P2     P3                  There is no
				*******-------------B      intersection at P2
				A


				A,P2     P3                  There is no
				*******-------------B      intersection at P2
				*
				*
				OUTSIDE *   INSIDE
				*
				P1*

				P1*
				*    OUTSIDE
				*
				INSIDE  *
				*P2    P3                   There is an
				A----------*******-------------B      intersection at P2


				P2     P3
				A----------*******-------------B      There is no
				*                           intersection at P2
				*
				OUTSIDE *   INSIDE
				*
				P1*

				*
				*
				*  OUTSIDE
				INSIDE *
				*P2    P3                   There is an
				B----------*******-------------B      intersection at P2



				P2     P3
				B----------*******-------------B      There is no
				*                           intersection at P2
				*
				OUTSIDE *   INSIDE
				*
				P1*
				*/
				//See if P2 == A
				v_subtract(&P2, A, &tmp1);
				d = v_dot(&tmp1, &tmp1);
				v_subtract(&P2, &P1, &tmp1);
				v_subtract(&P3, &P2, &tmp2);
				if (d <= CLOSETOSQR)
				{
					//if (P3-P2)#(B-A) > 0 no intersection
					v_subtract(B, A, &tmp3);
					if (v_dot(&tmp2, &tmp3) > 0L)
						goto getnextnode;
				}
				//If (P2-P1)^(P3-P2) < 0 then P2 is an intersection.
				if (v_cross3rd_comp(&tmp1, &tmp2) < 0)
					goto intersection;
				else
					goto getnextnode;
			}

			//If intersection at P2 then check crossing node or not.
			//For example:
			/*
			*P3
			*
			*
			*
			P2 *
			A----------*--------------------      Not crossing node
			*
			*
			*
			*
			P1*

			P2
			A----------*--------------------  Crossing node
			* *
			*   *
			*     *
			*       *
			P1*         * P3
			*/
			v_subtract(B, A, &tmp1);
			v_subtract(&P3, A, &tmp2);
			v_subtract(&P1, A, &tmp3);
			if ((v_cross3rd_comp(&tmp1, &tmp2) * v_cross3rd_comp(&tmp1, &tmp3)) > 0)
			{
				//not crossing node
				v_subtract(&P2, A, &tmp4);
				if (v_dot(&tmp4, &tmp4) <= 2L)
				{
					//P2 = A
					/*
					P2
					A*--------------------      There is an
					* *                         intersection at P2
					*   *      INSIDE
					*     *
					*       *
					P1*         * P3

					P2
					A*--------------------      There is no
					* *                         intersection at P2
					*   *     OUTSIDE
					*     *
					*       *
					P3*         * P1
					*/
					if (NodeTest(&tmp1, &P1, &P2, &P3))
						goto intersection;
					else
						goto getnextnode;
				}
				v_subtract(&P2, B, &tmp4);
				if (v_dot(&tmp4, &tmp4) <= 2L)
				{
					//P2 = B
					/*
					P2
					A--------B*                          There is an
					* *                         intersection at P2
					*   *      INSIDE
					*     *
					*       *
					P1*         * P3

					P2
					A---------B*                          There is no
					* *                         intersection at P2
					*   *     OUTSIDE
					*     *
					*       *
					P3*         * P1
					*/
					v_subtract(A, B, &tmp1);
					if (NodeTest(&tmp1, &P1, &P2, &P3))
						goto intersection;
					else
						goto getnextnode;
				}
				/*
				P2
				A----------*--------------------      There is an
				* *                         intersection at P2
				*   *      INSIDE
				*     *
				*       *
				P1*         * P3

				P2
				A----------*--------------------      There is no
				* *                         intersection at P2
				*   *     OUTSIDE
				*     *
				*       *
				P3*         * P1
				*/
				v_subtract(A, &P2, &tmp1);
				if (NodeTest(&tmp1, &P1, &P2, &P3))
					goto intersection;
				else
					goto getnextnode;
			}
			else
			{
				//Crossing node using known inside determine if there is an intersection.
				/*
				*P3
				*
				INSIDE          *
				*
				P2 *
				A----------*--------------------      There is an
				*                           intersection at P2
				*
				*
				*
				P1*

				P3*
				*
				*     OUTSIDE
				INSIDE    *                           There is no
				P2*A--------------B            intersection at P2
				*
				*
				*
				P1*

				P3*
				*
				*     OUTSIDE
				INSIDE    *                     There is an
				B----------P2*A                     intersection at P2
				*
				*
				*
				P1*

				P3*
				*
				*     OUTSIDE
				INSIDE    *                           There is no
				P2*B--------------A            intersection at P2
				*
				*
				*
				P1*
				P3*
				*
				*     OUTSIDE
				INSIDE    *                     There is an
				A----------P2*B                     intersection at P2
				*
				*
				*
				P1*
				*/
				v_subtract(&P2, A, &tmp4);
				if (v_dot(&tmp4, &tmp4) <= 2L)
				{
					//P2 = A
					//If (P2-P1)^(B-A) > 0 then there is an intersection at P2.
					v_subtract(&P2, &P1, &tmp2);
					if (v_cross3rd_comp(&tmp2, &tmp1) > 0)
						goto intersection;
					else
						goto getnextnode;
				}
				v_subtract(&P2, B, &tmp4);
				if (v_dot(&tmp4, &tmp4) <= 2L)
				{
					//P2 = B
					//If (P2-P1)^(B-A) < 0 then there is an intersection at P2.
					v_subtract(&P2, &P1, &tmp2);
					if (v_cross3rd_comp(&tmp2, &tmp1) < 0)
						goto intersection;
					else
						goto getnextnode;
				}
			}
		}

		//If intersection is at A then see if leaving or entering polygon.
		//For example:
		/*
		*P2
		*
		*
		*
		INSIDE    *   OUTSIDE
		*
		*
		*A-------------------      There is no
		*                           intersection at P2
		*
		*
		*
		P1*         *P2
		*
		*
		INSIDE   *    OUTSIDE
		*
		-----------*A                         There is an
		*                           intersection at P2
		*
		*
		*
		P1*
		*/
		if (t1 == INTERSECT + INTERSECTA)
		{
			v_subtract(&P2, &P1, &tmp1);
			v_subtract(B, A, &tmp2);
			if (v_cross3rd_comp(&tmp1, &tmp2) <= 0)
				goto getnextnode;
		}

		//If intersection is at B then see if leaving or entering polygon.
		//For example:
		/*
		*P2
		*
		*
		*
		INSIDE    *   OUTSIDE
		*
		*
		*B-------------------      There is no
		*                           intersection at P2
		*
		*
		*
		P1*         *P2
		*
		*
		INSIDE   *    OUTSIDE
		*
		-----------*B                         There is an
		*                           intersection at P2
		*
		*
		*
		P1*
		*/
		if (t1 == INTERSECT + INTERSECTB)
		{
			v_subtract(&P2, &P1, &tmp1);
			v_subtract(B, A, &tmp2);
			if (v_cross3rd_comp(&tmp1, &tmp2) > 0)
				goto getnextnode;
		}

	intersection:
		//intersection, see if closer to A or B than other intersections
		v_subtract(A, &V, &tmp1);
		if (v_sizesqrd(A, &V) < distA)
		{
			distA = v_sizesqrd(A, &V);
			I1 = V;
			*nodeI1 = node;
		}
		if (v_sizesqrd(B, &V) < distB)
		{
			distB = v_sizesqrd(B, &V);
			I2 = V;
			*nodeI2 = node;
		}
		//count intersections
		++i;
		goto getnextnode;
	}
}

static int near NodeTest(AvdPoint *tmp1, AvdPoint *P1, AvdPoint *P2, AvdPoint *P3)
{
	AvdPoint tmp2, tmp3;

	/*
	P2              tmp1
	*------------------->      There is no
	* *                         intersection at P2
	*   *     OUTSIDE
	*     *
	*       *
	P3*         * P1


	tmp1    P2
	<---------*                          There is an
	* *                         intersection at P2
	*   *      INSIDE
	*     *
	*       *
	P1*         * P3
	*/
	v_subtract(P3, P2, &tmp2);
	v_subtract(P2, P1, &tmp3);
	if (v_cross3rd_comp(&tmp2, &tmp3) > 0)
	{
		if (v_cross3rd_comp(&tmp2, tmp1) > 0)
			return (TRUE);
		else
		{
			if (v_cross3rd_comp(tmp1, &tmp3) < 0)
				return (TRUE);
			else
				return (FALSE);
		}
	}
	else
	{
		if (v_cross3rd_comp(&tmp2, tmp1) > 0)
		{
			if (v_cross3rd_comp(tmp1, &tmp3) < 0)
				return (TRUE);
			else
				return (FALSE);
		}
		else
		{
			return (FALSE);
		}
	}
}

/* INTSEGMS.C INTERSECT TWO LINE SEGMENTS
*
* Algorithm by Larry Scott
*
* NAME---INTSEGMS---
*  POLYGON must be closed and not self intersecting (JORDAN polygon)
* input: 2 line segments A-B & C-D
*  address of where to place intersection point
*  It's assumed that A-B is the motion and C-D
*  is part of a polygon.
*  output: return NOINTERSECT no intersection
*  COLINEAR segments are colinear
* INTERSECT + INTERSECTA intersection exactly at the point A
* INTERSECT + INTERSECTB intersection exactly at the point B
* INTERSECT + INTERSECTC intersection exactly at the point C
* INTERSECT + INTERSECTD intersection exactly at the point D
* INTERSECT + INTERSECTI intersection internal to segments
* interpt intersection point
*/
static int near IntersectSegments(AvdPoint *A, AvdPoint *B, AvdPoint *C, AvdPoint *D, AvdPoint *interpt)
{
	int x1, x2;
	long dot1, dot2, dot3, d;
	AvdPoint U, V, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;

	v_subtract(B, A, &tmp1);
	v_subtract(C, A, &tmp2);
	v_subtract(D, A, &tmp3);

	x1 = v_cross3rd_comp(&tmp1, &tmp2);
	x2 = v_cross3rd_comp(&tmp1, &tmp3);

	//check for colinear
	if (x1 == 0 && x2 == 0)
		return(COLINEAR); //COLINEAR LINE SEGMENTS

						  //check for intersection at C
	if (x1 == 0)
		//C lies on the line through A-B find out if it is in the line segment A-B.
		if (A->x == B->x)
		{
			if (((A->y <= C->y) && (C->y <= B->y)) || ((B->y <= C->y) && (C->y <= A->y)))
			{
				*interpt = *C;
				return(INTERSECT + INTERSECTC);
			}
		}
		else
		{
			if (((A->x <= C->x) && (C->x <= B->x)) || ((B->x <= C->x) && (C->x <= A->x)))
			{
				*interpt = *C;
				return(INTERSECT + INTERSECTC);
			}
		}

	//check for intersection at D
	if (x2 == 0)
		//D lies on the line through A-B find out if it is in the line segment A-B.
		if (A->x == B->x)
		{
			if (((A->y <= D->y) && (D->y <= B->y)) || ((B->y <= D->y) && (D->y <= A->y)))
			{
				*interpt = *D;
				return(INTERSECT + INTERSECTD);
			}
		}
		else
		{
			if (((A->x <= D->x) && (D->x <= B->x)) || ((B->x <= D->x) && (D->x <= A->x)))
			{
				*interpt = *D;
				return(INTERSECT + INTERSECTD);
			}
		}

	//check for intersection at A
	v_subtract(C, D, &tmp4);
	v_subtract(B, C, &tmp5);
	v_subtract(B, D, &tmp6);
	dot3 = v_dot(&tmp4, &tmp4);
	if (dot3 == 0L)
		Panic(E_AVOID);
	if (((d = v_dot(&tmp4, &tmp2)) >= 0) && (v_dot(&tmp4, &tmp3) <= 0))
	{
		tmp7.x = (int)((d*(long)tmp4.x + (long)(sign(tmp4.x)*dot3 / 2)) / dot3);
		tmp7.y = (int)((d*(long)tmp4.y + (long)(sign(tmp4.y)*dot3 / 2)) / dot3);
		v_subtract(&tmp7, &tmp2, &tmp7);
		d = v_dot(&tmp7, &tmp7);
	}
	else
	{
		if ((dot1 = v_dot(&tmp2, &tmp2)) <= (dot2 = v_dot(&tmp3, &tmp3)))
			d = dot1;
		else
			d = dot2;
	}
	if (d <= CLOSETOSQR)
	{
		*interpt = *A;
		return(INTERSECT + INTERSECTA);
	}

	//check for intersection at B
	if (((d = v_dot(&tmp4, &tmp6)) >= 0) && (v_dot(&tmp4, &tmp5) <= 0))
	{
		tmp7.x = (int)((d * (long)tmp4.x + (long)(sign(tmp4.x) * dot3 / 2)) / dot3);
		tmp7.y = (int)((d * (long)tmp4.y + (long)(sign(tmp4.y) * dot3 / 2)) / dot3);
		v_subtract(&tmp7, &tmp6, &tmp7);
		d = v_dot(&tmp7, &tmp7);
	}
	else
	{
		if ((dot1 = v_dot(&tmp5, &tmp5)) <= (dot2 = v_dot(&tmp6, &tmp6)))
			d = dot1;
		else
			d = dot2;
	}
	if (d <= CLOSETOSQR)
	{
		*interpt = *B;
		return(INTERSECT + INTERSECTB);
	}

	//check that C, D on opposite side of line through A-B
	if (x1 * x2 >= 0)
		return(NOINTERSECT);

	//check that A, B on opposite side of line through C-D
	x1 = v_cross3rd_comp(&tmp2, &tmp4);
	x2 = v_cross3rd_comp(&tmp4, &tmp5);
	if (x1 * x2 >= 0)
		return(NOINTERSECT);

	//intersection interior to both line segments
	//calculate intersection point

	//U is normal to C-D pointing toward the outside of the polygon
	U.x = -tmp4.y;
	U.y = tmp4.x;
	//U dot (A-C)
	dot1 = v_dot(&U, &tmp2);
	//U dot (A-B)
	dot2 = v_dot(&U, &tmp1);
	dot3 = dot2;
	if (dot1 < 0)
		dot1 = -dot1;
	if (dot2 < 0)
		dot2 = -dot2;

	if (dot3 > 0)
	{
		//round toward outside of polygon
		tmp6.x = (int)(((long)tmp1.x * dot1 + (long)sign(tmp1.x) * (dot2 - 1)) / dot2);
		tmp6.y = (int)(((long)tmp1.y * dot1 + (long)sign(tmp1.y) * (dot2 - 1)) / dot2);
		v_add(A, &tmp6, &V);
	}
	if (dot3 < 0)
	{
		//truncate so that intersection is outside polygon
		tmp6.x = (int)((((long)tmp1.x) * dot1) / dot2);
		tmp6.y = (int)((((long)tmp1.y) * dot1) / dot2);
		v_add(A, &tmp6, &V);
	}
	if (dot3 == 0)
	{
		V = *C;
	}
	*interpt = V;
	return(INTERSECT + INTERSECTI);
}


//vectops.c vector arithmatic operations

//vector subtraction for 2 components
static void near v_subtract(AvdPoint *X, AvdPoint *Y, AvdPoint *Res)
{
	Res->x = X->x - Y->x;
	Res->y = X->y - Y->y;
}

//vector addition for 2 components
static void near v_add(AvdPoint *X, AvdPoint *Y, AvdPoint *Res)
{
	Res->x = X->x + Y->x;
	Res->y = X->y + Y->y;
}

//2 component dot product
static long near v_dot(AvdPoint *X, AvdPoint *Y)
{
	return (((long)X->x) * ((long)Y->x) + ((long)X->y) * ((long)Y->y));
}

//return the third component of a cross product
static int near v_cross3rd_comp(AvdPoint *X, AvdPoint *Y)
{
	long direction;
	direction = ((((long)X->x) * ((long)Y->y)) - (((long)X->y) * ((long)Y->x)));
	if (direction < 0)
		return(-1);
	if (direction > 0)
		return(1);
	return(0);
}

//return the magnitude (squared) of vector X
static long near v_sizesqrd(AvdPoint *X, AvdPoint *Y)
{
	long x, y;
	x = (((long)X->x) - ((long)Y->x)) * (((long)X->x) - ((long)Y->x));
	y = (((long)X->y) - ((long)Y->y)) * (((long)X->y) - ((long)Y->y));
	return (x + y);
}

