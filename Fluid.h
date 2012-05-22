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

#ifndef HH_MPM_FLUID_HH
#define HH_MPM_FLUID_HH

#include <vector>
#include "DistanceField.h"

struct GridCell
{	
	float m;		// mass
	float vx;		// x-axis velocity
	float vy;		// y-axis velocity
	float ax;		// x-axis acceleration
	float ay;		// y-axis acceleration
};

// Cell weights for performing quadratic interpolation
struct CellWeight
{
	float wx[3];	// x-axis weight
	float wy[3];	// y-axis weight
	float gx[3];	// x-axis gradient
	float gy[3];	// y-axis gradient
};

struct Particle
{
	float x;		// x-axis position
	float y;		// y-axis position
	float vx;		// x-axis velocity
	float vy;		// y-axis velocity
};

class Fluid
{
public:
	Fluid(int gwidth, int gheight);
	~Fluid();

	void AddParticle(float x, float y, float vx, float vy);

	int 						Color;

	std::vector<Particle>		Particles;
	std::vector<CellWeight> 	Weights;

	float						Density;
	float						Stiffness;
	float						Viscosity;
	GridCell **					Grid;

private:
	Fluid(const Fluid &);
	Fluid & operator = (const Fluid &);
};

class FluidSim
{
public:
	FluidSim(int width, int height, float scale);
	~FluidSim();

	void Update();
	void InitGrid(Fluid * fluid);
	void CalcAccel(Fluid * fluid);
	void CalcVelocity(Fluid * fluid);
	void UpdateParticles(Fluid * fluid);
	
	int ParticleCount() const;

	DistanceField				SDF;
	GridCell **					Grid;
	std::vector<Fluid *>		Fluids;
	float 						GridCoeff;
	float						GravityX;
	float						GravityY;
	float						Scale;
	int							GWidth;
	int							GHeight;

private:
	FluidSim(const FluidSim &);
	FluidSim & operator = (const FluidSim &);
};

#endif // HH_MPM_FLUID_HH
