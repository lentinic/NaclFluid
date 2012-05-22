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

#ifndef HH_SDFC_DISTANCEFIELD_HH
#define HH_SDFC_DISTANCEFIELD_HH

class DistanceField
{	
public:
	DistanceField();
	~DistanceField();

	void 	Create(int nresolution, float w, float h);

	void	AddCircle(float x, float y, float r);
	void	SubCircle(float x, float y, float r);
	void	SubRect(float x, float y, float w, float h);
	
	float 	SampleDistance(int x, int y) const;
	float	SampleDistance(float x, float y) const;
	float	SampleDistanceN(float x, float y) const;
	void	SampleGradient(float x, float y, float * outx, float * outy) const;
	float	SampleNormal(float x, float y, float * outx, float * outy) const;

	void	Propagate();
	void 	Blur();

	int 	GetResolution() const { return nResolution; }
	
private:		
	DistanceField(const DistanceField &);
	DistanceField & operator = (const DistanceField &);

	float	GetDistance(float * src, int x, int y) const;

	float *		pValues;
	float *		pFilled;
	float *		pEmpty;

	float		fWidth;
	float		fHeight;
	int			nResolution;
	int			nInternalRes;
};

#endif // HH_SDFC_DISTANCEFIELD_HH
