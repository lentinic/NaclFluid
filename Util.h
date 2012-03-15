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

#ifndef HH_SPH_UTIL_HH
#define HH_SPH_UTIL_HH
#include <math.h>
#include <sys/time.h>

///////////////////////////////////////////////////////////////////////////////
struct Point
{
	float x;
	float y;
};
///////////////////////////////////////////////////////////////////////////////
inline void Sub(const Point & a, const Point & b, Point * out)
{
	out->x = a.x - b.x;
	out->y = a.y - b.y;
}
///////////////////////////////////////////////////////////////////////////////
inline float Length(const Point & pt)
{
	return sqrt(pt.x * pt.x + pt.y * pt.y);
}
///////////////////////////////////////////////////////////////////////////////
inline float Length2(const Point & pt)
{
	return pt.x * pt.x + pt.y * pt.y;
}
///////////////////////////////////////////////////////////////////////////////
inline float Dot(const Point & a, const Point & b)
{
	return a.x * b.x + a.y * b.y;
}
///////////////////////////////////////////////////////////////////////////////
inline int64_t GetTimeMicro() 
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (int64_t)(t.tv_sec) * 1000 + (t.tv_usec / 1000);
}

#endif // HH_SPH_UTIL_HH
