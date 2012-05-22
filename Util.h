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

#ifndef HH_SDFC_UTIL_HH
#define HH_SDFC_UTIL_HH
#include <math.h>
#include <sys/time.h>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
inline int64_t GetTimeMS() 
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (int64_t)(t.tv_sec) * 1000 + (t.tv_usec / 1000);
}
///////////////////////////////////////////////////////////////////////////////
inline float frand()
{
	return rand() / (float)RAND_MAX;
}
///////////////////////////////////////////////////////////////////////////////
inline void DrawCircle(int32_t * pixels, int xres, int yres, int x, int y, 
	int r, int rgb = 0xff0000ff)
{
	int ulx = std::max(x - r, 0);
	int uly = std::max(y - r, 0);
	int lrx = std::min(x + r, xres - 1);
	int lry = std::min(y + r, yres - 1);
	for (int i=uly; i<lry; i++)
	{
		for (int j=ulx; j<lrx; j++)
		{
			int d2 = ((x - j) * (x - j)) + ((y - i) * (y - i));
			if (d2 <= (r * r))
			{
				pixels[(i * xres) + j] = rgb;
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
inline void DrawLine(int32_t * pixels, int xres, int yres, int x0, int y0, 
	int x1, int y1, int rgb = 0xff0000ff)
{
    int dx = (x1 - x0);
    char ix = (dx > 0) - (dx < 0);
    dx = std::abs(dx) << 1;

    int dy = y1 - y0;
    char iy = (dy > 0) - (dy < 0);
    dy = std::abs(dy) << 1;

    if ((y0 >= 0 && y0 < yres) && (x0 >= 0 && x0 < xres))
	    pixels[(y0*xres)+x0] = rgb;

    if (dx >= dy)
    {
        // error may go below zero
        int error = dy - (dx >> 1);

        while (x0 != x1)
        {
            if (error >= 0)
            {
                if (error || (ix > 0))
                {
                    y0 += iy;
                    error -= dx;
                }
                // else do nothing
            }
            // else do nothing
 
            x0 += ix;
            error += dy;
 
            if ((y0 >= 0 && y0 < yres) && (x0 >= 0 && x0 < xres))
	    		pixels[(y0*xres)+x0] = rgb;
        }
    }
    else
    {
        // error may go below zero
        int error = dx - (dy >> 1);

        while (y0 != y1)
        {
            if (error >= 0)
            {
                if (error || (iy > 0))
                {
                    x0 += ix;
                    error -= dy;
                }
                // else do nothing
            }
            // else do nothing
 
            y0 += iy;
            error += dx;
 
            if ((y0 >= 0 && y0 < yres) && (x0 >= 0 && x0 < xres))
	    		pixels[(y0*xres)+x0] = rgb;
        }
    }
}
///////////////////////////////////////////////////////////////////////////////

#endif // HH_SDFC_UTIL_HH
