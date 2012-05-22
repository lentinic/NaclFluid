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

#include <algorithm>
#include <vector>
#include <list>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "Util.h"
#include "DistanceField.h"
#include "Fluid.h"

///////////////////////////////////////////////////////////////////////////////
//
// ---------------------------------- Fluid ----------------------------------- 
//
///////////////////////////////////////////////////////////////////////////////
Fluid::Fluid(int gwidth, int gheight)
:	Color(0xff0000ff),
	Density(3.5f),
	Stiffness(0.5f),
	Viscosity(0.f)
{
	Grid = new GridCell*[gheight];
	Grid[0] = new GridCell[gheight * gwidth];
	for (int i=1; i<gheight; i++)
		Grid[i] = Grid[i-1] + gwidth;
}
///////////////////////////////////////////////////////////////////////////////
Fluid::~Fluid()
{
	delete [] Grid[0];
	delete [] Grid;
	Grid = NULL;
}
///////////////////////////////////////////////////////////////////////////////
void Fluid::AddParticle(float x, float y, float vx, float vy)
{
	Particle p;
	p.x = x;
	p.y = y;
	p.vx = vx;
	p.vy = vy;
	Particles.push_back(p);
	Weights.push_back(CellWeight());
}
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
// --------------------------------- FluidSim --------------------------------- 
//
///////////////////////////////////////////////////////////////////////////////
FluidSim::FluidSim(int width, int height, float scale)
{
	Scale = scale;
	GWidth = (width / scale) + 1;
	GHeight = (height / scale) + 1;

	Grid = new GridCell*[GHeight];
	Grid[0] = new GridCell[GWidth * GHeight];
	for (int i=1; i<GHeight; i++)
		Grid[i] = Grid[i - 1] + GWidth;

	GridCoeff = 1.f;
	GravityX = 0.f;
	GravityY = (9.81f / scale) * (1.f / 900.f);

	SDF.Create(256, GWidth, GHeight);
	SDF.SubRect(2.f, 2.f, GWidth-4.f, GHeight-4.f);
	SDF.Blur();
}
///////////////////////////////////////////////////////////////////////////////
FluidSim::~FluidSim()
{
	delete [] Grid[0];
	delete [] Grid;
	Grid = NULL;

	for (unsigned i=0; i<Fluids.size(); i++)
		delete Fluids[i];
	Fluids.clear();
}
///////////////////////////////////////////////////////////////////////////////
void FluidSim::Update()
{
	// Clear all grid cells
	memset(Grid[0], 0, sizeof(GridCell) * GWidth * GHeight);
	for (int i=0, lim=Fluids.size(); i<lim; i++)
		memset(Fluids[i]->Grid[0], 0, sizeof(GridCell) * GWidth * GHeight);

	// Fill out grid initial grid information
	for (int i=0, lim=Fluids.size(); i<lim; i++)
		InitGrid(Fluids[i]);

	// Average grid velocity
	for (int y=0, ylim=GHeight; y<ylim; y++)
	{
		for (int x=0, xlim=GWidth; x<xlim; x++)
		{
			float m = Grid[y][x].m;
			if (m == 0.f)
				continue;
			Grid[y][x].vx /= m;
			Grid[y][x].vy /= m;
		}
	}
	
	// Compute particle acceleration and propagate to grid
	for (int i=0, lim=Fluids.size(); i<lim; i++)
		CalcAccel(Fluids[i]);

	// Average grid acceleration
	for (int y=0, ylim=GHeight; y<ylim; y++)
	{
		for (int x=0, xlim=GWidth; x<xlim; x++)
		{
			GridCell & cell = Grid[y][x];
			float m = cell.m;
			if (m == 0.f)
				continue;
			cell.ax /= m;
			cell.ay /= m;
		}
	}
	
	// Update fluid velocity fields
	// Update particle positions
	for (int i=0, lim=Fluids.size(); i<lim; i++)
	{
		CalcVelocity(Fluids[i]);
		UpdateParticles(Fluids[i]);
	}
}
///////////////////////////////////////////////////////////////////////////////
void FluidSim::InitGrid(Fluid * fluid)
{
	for (int i=0, lim=fluid->Particles.size(); i<lim; i++)
	{
		Particle p = fluid->Particles[i];

		int cx = std::min(GWidth-3, std::max(0, (int)(p.x - 0.5f)));
		int cy = std::min(GHeight-3, std::max(0, (int)(p.y - 0.5f)));

		float u = (float)cx - p.x;
		float v = (float)cy - p.y;

		CellWeight & weight = fluid->Weights[i];

		// Biquadratic interpolation weights along each axis
		weight.wx[0] = 0.5f * u * u + 1.5f * u + 1.125f;
		weight.gx[0] = u + 1.5f;
		u++;
		weight.wx[1] = -u * u + 0.75f;
		weight.gx[1] = -2.f * u;
		u++;
		weight.wx[2] = 0.5f * u * u - 1.5f * u + 1.125f;
		weight.gx[2] = u - 1.5f;

		weight.wy[0] = 0.5f * v * v + 1.5f * v + 1.125f;
		weight.gy[0] = v + 1.5f;
		v++;
		weight.wy[1] = -v * v + 0.75f;
		weight.gy[1] = -2.f * v;
		v++;
		weight.wy[2] = 0.5f * v * v - 1.5f * v + 1.125f;
		weight.gy[2] = v - 1.5f;

		for (int y=0; y<3; y++)
		{
			for (int x=0; x<3; x++)
			{
				float w = weight.wy[y] * weight.wx[x];

				GridCell & cell = Grid[cy + y][cx + x];
				cell.m += w;
				cell.vx += p.vx * w;
				cell.vy += p.vy * w;
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
void FluidSim::CalcAccel(Fluid * fluid)
{
	for (int i=0, lim=fluid->Particles.size(); i<lim; i++)
	{
		float fx = fluid->Particles[i].x;
		float fy = fluid->Particles[i].y;
		int cx = std::min(GWidth-3, std::max(0, (int)(fx - 0.5f)));
		int cy = std::min(GHeight-3, std::max(0, (int)(fy - 0.5f)));

		CellWeight & weight = fluid->Weights[i];

		// Determine interpolated mass and velocity derivatives
		float dudx = 0.f, dudy = 0.f;
		float dvdx = 0.f, dvdy = 0.f;
		float mass = 0.f;
		for (int y=0; y<3; y++)
		{
			for (int x=0; x<3; x++)
			{
				float w = weight.wx[x] * weight.wy[y];
				float dx = weight.gx[x] * weight.wy[y];
				float dy = weight.wx[x] * weight.gy[y];

				GridCell & cell = Grid[cy + y][cx + x];

				dudx += cell.vx * dx;
				dudy += cell.vx * dy;
				dvdx += cell.vy * dx;
				dvdy += cell.vy * dy;
				mass += cell.m * w;
			}
		}
		
		float pressure = (fluid->Stiffness / std::max(1.f, fluid->Density)) * 
			(mass - fluid->Density);

		// Add a bit of a pushing force near the collision boundaries
		float ax = 0.f, ay = 0.f;
		float d = SDF.SampleDistance(fx, fy);
		if (d < 3.f)
		{
			float dirx, diry;
			SDF.SampleGradient(fx, fy, &dirx, &diry);
			ax += dirx * (1.f - (d / 3.f));
			ay += diry * (1.f - (d / 3.f));
		}

		// Update grid acceleration values
		for (int y=0; y<3; y++)
		{
			for (int x=0; x<3; x++)
			{
				float w = weight.wx[x] * weight.wy[y];
				float dx = weight.gx[x] * weight.wy[y];
				float dy = weight.wx[x] * weight.gy[y];

				GridCell & cell = Grid[cy + y][cx + x];
				cell.ax += ax * w - dx * pressure - (dudx * dx + dudy * dy) * fluid->Viscosity * w;
				cell.ay += ay * w - dy * pressure - (dvdx * dx + dvdy * dy) * fluid->Viscosity * w;
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
void FluidSim::CalcVelocity(Fluid * fluid)
{	
	for (int i=0, lim=fluid->Particles.size(); i<lim; i++)
	{
		Particle & p = fluid->Particles[i];
		float fx = p.x;
		float fy = p.y;
		int cx = std::min(GWidth-3, std::max(0, (int)(fx - 0.5f)));
		int cy = std::min(GHeight-3, std::max(0, (int)(fy - 0.5f)));

		// Add grid acceleration to the particle velocities
		CellWeight & weight = fluid->Weights[i];
		for (int y=0; y<3; y++)
		{
			for (int x=0; x<3; x++)
			{
				GridCell & cell = Grid[cy + y][cx + x];
				float w = weight.wx[x] * weight.wy[y];
				p.vx += w * cell.ax;
				p.vy += w * cell.ay;
			}
		}

		p.vx += GravityX;
		p.vy += GravityY; 

		// Check new position and push away from distance field boundaries
		float nx = fx + p.vx;
		float ny = fy + p.vy;
		float d = SDF.SampleDistance(nx, ny);
		if (d < 1.f)
		{
			float dirx, diry;
			SDF.SampleGradient(nx, ny, &dirx, &diry);
			p.vx += (dirx) * (1.f - d) * (1.f + frand() * 0.01f);
			p.vy += (diry) * (1.f - d) * (1.f + frand() * 0.01f);
		}

		// Update fluid specific velocity grid
		for (int y=0; y<3; y++)
		{
			for (int x=0; x<3; x++)
			{
				float w = weight.wx[x] * weight.wy[y];
				GridCell & cell = fluid->Grid[cy+y][cx+x];
				cell.m += w;
				cell.vx += (w * p.vx);
				cell.vy += (w * p.vy);
			}
		}
	}

	// Average out the fluid velocity grid
	for (int y=0, ylim=GHeight; y<ylim; y++)
	{
		for (int x=0, xlim=GWidth; x<xlim; x++)
		{
			GridCell & cell = fluid->Grid[y][x];
			float m = cell.m;
			if (m == 0.f)
				continue;
			cell.vx /= m;
			cell.vy /= m;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
void FluidSim::UpdateParticles(Fluid * fluid)
{
	for (int i=0, lim=fluid->Particles.size(); i<lim; i++)
	{
		Particle & p = fluid->Particles[i];
	
		int cx = std::min(GWidth-3, std::max(0, (int)(p.x - 0.5f)));
		int cy = std::min(GHeight-3, std::max(0, (int)(p.y - 0.5f)));

		// Get interpolated velocity
		CellWeight & weight = fluid->Weights[i];
		float vx = 0.f, vy = 0.f;
		for (int y=0; y<3; y++)
		{
			for (int x=0; x<3; x++)
			{
				GridCell & cell = fluid->Grid[cy+y][cx+x];
				float w = weight.wx[x] * weight.wy[y];
				vx += w * cell.vx;
				vy += w * cell.vy;
			}
		}

		// Update particle position, velocity
		p.x += vx;
		p.y += vy;
		p.vx += GridCoeff * (vx - p.vx);
		p.vy += GridCoeff * (vy - p.vy);

		// Resolve collisions, clamp positions, update velocities based on this
		float x = p.x;
		float y = p.y;
			
		float d = SDF.SampleDistance(x, y);
		if (d < 0.f)
		{
			float dx, dy;
			SDF.SampleGradient(x, y, &dx, &dy);
			x -= dx;
			y -= dy;
		}

		x = std::min(std::max(x, 1.f), GWidth - 2.f);
		y = std::min(std::max(y, 1.f), GHeight - 2.f);
		p.x = x;
		p.y = y;
	}
}
///////////////////////////////////////////////////////////////////////////////
int FluidSim::ParticleCount() const
{
	int count = 0;
	for (unsigned i=0; i<Fluids.size(); i++)
		count += Fluids[i]->Particles.size();
	return count;
}
///////////////////////////////////////////////////////////////////////////////
