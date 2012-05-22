/*
Copyright (c) 2012 Chris Lentini
http://divergentcoder.com

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to use, 
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS 
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "DistanceField.h"
#include <math.h>
#include <float.h>
#include <string.h>

#define SQ2 1.4142135623730950488016887242097f

///////////////////////////////////////////////////////////////////////////////
//
// ------------------------------ DistanceField -------------------------------
//
///////////////////////////////////////////////////////////////////////////////
DistanceField::DistanceField()
:	pValues(NULL),
	fWidth(0.f),
	fHeight(0.f),
	nResolution(0)
{}
///////////////////////////////////////////////////////////////////////////////
DistanceField::~DistanceField()
{
	delete [] pValues;
}
///////////////////////////////////////////////////////////////////////////////
#define EMPTY 		10000.f
#define FILLED		0.f

void DistanceField::Create(int nresolution, float w, float h)
{
	// adding a 1 pixel border around the image
	nInternalRes = nresolution + 2;
	fWidth = w;
	fHeight = h;
	nResolution = nresolution;

	int count = nInternalRes * nInternalRes;
	
	pValues = new float[count];
	pFilled = new float[count];
	pEmpty = new float[count];


	int i = 0;
	for (int y=0; y<nInternalRes; y++)
	{
		for (int x=0; x<nInternalRes; x++, i++)
		{
			pFilled[i] = FILLED;
			pEmpty[i] = EMPTY;
		}
	}

	Propagate();
}
///////////////////////////////////////////////////////////////////////////////
void DistanceField::AddCircle(float x, float y, float r)
{
	for (int iy=0; iy<nResolution; iy++)
	{
		float fy = (iy / (float) nResolution) * fHeight;

		for (int ix=0; ix<nResolution; ix++)
		{
			float fx = (ix / (float) nResolution) * fWidth;

			float dx = fx - x;
			float dy = fy - y;
			float d = sqrt(dx*dx + dy*dy) - r;

			if (d < 0.f)
			{
				int i = ((iy + 1) * nInternalRes) + ix + 1;
				pFilled[i] = FILLED;
				pEmpty[i] = EMPTY;
			}
		}
	}

	Propagate();
}
///////////////////////////////////////////////////////////////////////////////
void DistanceField::SubCircle(float x, float y, float r)
{
	for (int iy=0; iy<nResolution; iy++)
	{
		float fy = (iy / (float) nResolution) * fHeight;

		for (int ix=0; ix<nResolution; ix++)
		{
			float fx = (ix / (float) nResolution) * fWidth;

			float dx = fx - x;
			float dy = fy - y;
			float d = sqrt(dx*dx + dy*dy) - r;

			if (d < 0.f)
			{
				int i = ((iy + 1) * nInternalRes) + ix + 1;
				pEmpty[i] = FILLED;
				pFilled[i] = EMPTY;
			}
		}
	}

	Propagate();
}
///////////////////////////////////////////////////////////////////////////////
void DistanceField::SubRect(float x, float y, float w, float h)
{
	for (int iy=0; iy<nResolution; iy++)
	{
		float fy = (iy / (float) nResolution) * fHeight;

		for (int ix=0; ix<nResolution; ix++)
		{
			float fx = (ix / (float) nResolution) * fWidth;

			if (fx > x && fx < (x + w) && fy > y && fy < (y + h))
			{
				int i = ((iy + 1) * nInternalRes) + ix + 1;
				pEmpty[i] = FILLED;
				pFilled[i] = EMPTY;
			}
		}
	}

	Propagate();
}
///////////////////////////////////////////////////////////////////////////////
float DistanceField::SampleDistance(int x, int y) const
{
	x++;
	y++;
	x = (x < 0) ? 0 : ((x >= nInternalRes) ? (nInternalRes-1) : x);
	y = (y < 0) ? 0 : ((y >= nInternalRes) ? (nInternalRes-1) : y);
	
	return pValues[y*nInternalRes+x] * (fWidth / nResolution);
}
///////////////////////////////////////////////////////////////////////////////
float DistanceField::SampleDistance(float x, float y) const
{
	x = (x / fWidth);
	y = (y / fHeight);
	return SampleDistanceN(x,y);
}
///////////////////////////////////////////////////////////////////////////////
float DistanceField::SampleDistanceN(float x, float y) const
{
	x = x * nResolution;
	y = y * nResolution;
	int ix = (int)x;
	int iy = (int)y;
	float dx = x - ix;
	float dy = y - iy;

	float d0, d1, d2, d3;

	d0 = SampleDistance(ix, iy);
	d1 = SampleDistance(ix + 1, iy);
	d2 = SampleDistance(ix, iy + 1);
	d3 = SampleDistance(ix + 1, iy + 1);

	d0 = d0 * (1.f - dx) + d1 * dx;
	d1 = d2 * (1.f - dx) + d3 * dx;
	return d0 * (1.f - dy) + d1 * dy;
}
///////////////////////////////////////////////////////////////////////////////
void DistanceField::SampleGradient(float x, float y, float * outx, float * outy) const
{
	float d0 = SampleDistance(x, y - (0.5f / nResolution));
	float d1 = SampleDistance(x - (0.5f / nResolution), y);
	float d2 = SampleDistance(x + (0.5f / nResolution), y);
	float d3 = SampleDistance(x, y + (0.5f / nResolution));

	*outx = (d2 - d1) * nResolution;
	*outy = (d3 - d0) * nResolution;
}
///////////////////////////////////////////////////////////////////////////////
float DistanceField::SampleNormal(float x, float y, float * outx, float * outy) const
{
	float gx, gy;
	SampleGradient(x,y,&gx,&gy);
	float len = sqrtf(gx*gx + gy*gy);
	*outx = gx / len;
	*outy = gy / len;
	return len;
}
///////////////////////////////////////////////////////////////////////////////
void DistanceField::Propagate()
{
	// Using 8SSDT Algorithm
	for (int i=0; i<nInternalRes; i++)
	{
		for (int j=0; j<nInternalRes; j++)
		{
			int id = i*nInternalRes+j;
			float d0, d1, d2, d3, d4;

			{
				d0 = GetDistance(pFilled,j-1,i-1);
				d1 = GetDistance(pFilled,j,i-1);
				d2 = GetDistance(pFilled,j+1,i-1);
				d3 = GetDistance(pFilled,j-1,i);
				d4 = GetDistance(pFilled,j,i);;

				d0 += SQ2;
				d1 += 1.f;
				d2 += SQ2;
				d3 += 1.f;

				d0 = d0 < d1 ? d0 : d1;
				d1 = d2 < d3 ? d2 : d3;
				d0 = d0 < d1 ? d0 : d1;
				d0 = d0 < d4 ? d0 : d4;
				pFilled[id] = d0;
			}
			{
				d0 = GetDistance(pEmpty,j-1,i-1);
				d1 = GetDistance(pEmpty,j,i-1);
				d2 = GetDistance(pEmpty,j+1,i-1);
				d3 = GetDistance(pEmpty,j-1,i);
				d4 = GetDistance(pEmpty,j,i);;

				d0 += SQ2;
				d1 += 1.f;
				d2 += SQ2;
				d3 += 1.f;

				d0 = d0 < d1 ? d0 : d1;
				d1 = d2 < d3 ? d2 : d3;
				d0 = d0 < d1 ? d0 : d1;
				d0 = d0 < d4 ? d0 : d4;
				pEmpty[id] = d0;
			}
		}
	}

	for (int i=nInternalRes-1; i>=0; i--)
	{
		for (int j=nInternalRes-1; j>=0; j--)
		{
			int id = i*nInternalRes+j;
			float d0, d1, d2, d3, d4;

			{
				d0 = GetDistance(pFilled,j-1,i+1);
				d1 = GetDistance(pFilled,j,i+1);
				d2 = GetDistance(pFilled,j+1,i+1);
				d3 = GetDistance(pFilled,j+1,i);
				d4 = GetDistance(pFilled,j,i);

				d0 += SQ2;
				d1 += 1.f;
				d2 += SQ2;
				d3 += 1.f;

				d0 = d0 < d1 ? d0 : d1;
				d1 = d2 < d3 ? d2 : d3;
				d0 = d0 < d1 ? d0 : d1;
				d0 = d0 < d4 ? d0 : d4;
				pFilled[id] = d0;
			}
			{
				d0 = GetDistance(pEmpty,j-1,i+1);
				d1 = GetDistance(pEmpty,j,i+1);
				d2 = GetDistance(pEmpty,j+1,i+1);
				d3 = GetDistance(pEmpty,j+1,i);
				d4 = GetDistance(pEmpty,j,i);
				

				d0 += SQ2;
				d1 += 1.f;
				d2 += SQ2;
				d3 += 1.f;

				d0 = d0 < d1 ? d0 : d1;
				d1 = d2 < d3 ? d2 : d3;
				d0 = d0 < d1 ? d0 : d1;
				d0 = d0 < d4 ? d0 : d4;
				pEmpty[id] = d0;
			}
		}
	}

	for (int i=0; i<nInternalRes; i++)
	{
		for (int j=0; j<nInternalRes; j++)
		{
			int id = i*nInternalRes+j;
			pValues[id] = pFilled[id] - pEmpty[id];
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
void DistanceField::Blur()
{
	for (int i=0; i<nInternalRes; i++)
	{
		for (int j=0; j<nInternalRes; j++)
		{
			float d[5];
			d[0] = GetDistance(pValues, j-2, i) * 2.f;
			d[1] = GetDistance(pValues, j-1, i) * 4.f;
			d[2] = GetDistance(pValues, j, i) * 8.f;
			d[3] = GetDistance(pValues, j+1, i) * 4.f;
			d[4] = GetDistance(pValues, j+2, i) * 2.f;

			pValues[i*nInternalRes+j] = (d[0] + d[1] + d[2] + d[3] + d[4]) / 20.f;
		}
	}

	for (int i=0; i<nInternalRes; i++)
	{
		for (int j=0; j<nInternalRes; j++)
		{
			float d[5];
			d[0] = GetDistance(pValues, j, i-2) * 2.f;
			d[1] = GetDistance(pValues, j, i-1) * 4.f;
			d[2] = GetDistance(pValues, j, i) * 8.f;
			d[3] = GetDistance(pValues, j, i+1) * 4.f;
			d[4] = GetDistance(pValues, j, i+2) * 2.f;

			pValues[i*nInternalRes+j] = (d[0] + d[1] + d[2] + d[3] + d[4]) / 20.f;
		}
	}	
}
///////////////////////////////////////////////////////////////////////////////
float DistanceField::GetDistance(float * src, int x, int y) const
{
	x = (x < 0) ? 0 : ((x >= nInternalRes) ? (nInternalRes-1) : x);
	y = (y < 0) ? 0 : ((y >= nInternalRes) ? (nInternalRes-1) : y);
	
	return src[y*nInternalRes+x];
}
///////////////////////////////////////////////////////////////////////////////
