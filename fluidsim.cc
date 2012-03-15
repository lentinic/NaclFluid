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
/* 
	Based off the paper:
	http://www.iro.umontreal.ca/labs/infographie/papers/Clavet-2005-PVFS/pvfs.pdf
*/
#include <algorithm>
#include <vector>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "Util.h"

void UpdateSimulation(float);
void ApplyViscosity(float);
void AdjustSprings(float);
void ApplySpringDisplacement(float);
void ApplyRelaxation(float);
void ResolveCollisions(float);

// Global Parameters
int nParticles;
float fGravity;
float fNearDistance;
float fViscosity[2];
float fYieldLength;
float fPressure;
float fPressureNear;
float fRestPressure;
float fCollisionRadius;
float fRestitution;
float fFriction;
float fAlpha;
float fSpring;

// Simulation state
Point * velocity = NULL;
Point * position = NULL;
Point * position_old = NULL;

#define RES 8
typedef std::vector<int> Cell;
Cell grid[RES][RES];

void GetGridCell(const Point & pt, int * outx, int * outy)
{
	int gx = (int) (pt.x * (float)RES);
	int gy = (int) (pt.y * (float)RES);
	*outx = std::max(0, std::min(gx, RES-1));
	*outy = std::max(0, std::min(gy, RES-1));
}

///////////////////////////////////////////////////////////////////////////////
void InitSimulation(int count)
{
	fGravity = -9.8f / 100.f;
	fRestitution = 0.4f;
	fFriction = 0.001f;

	fNearDistance = 1.f / 16.f;
	fViscosity[0] = 0.3f;
	fViscosity[1] = 0.0f;
	fYieldLength = 0.3f;
	fAlpha = 0.3f;
	fSpring = 0.3f;
	fPressure = 0.004f;
	fPressureNear = 0.01f;
	fRestPressure = 10.f;
	fCollisionRadius = 16.f / 100.f;

	velocity = (Point *) malloc(sizeof(Point) * count);
	position = (Point *) malloc(sizeof(Point) * count);
	position_old = (Point *) malloc(sizeof(Point) * count);

	for (int i=0; i<count; i++)
	{
		position[i].x = (rand() & 4095) / 4096.f;
		position[i].y = 1.f - ((rand() & 4095) / 32768.f);

		int gx, gy;
		GetGridCell(position[i], &gx, &gy);
		grid[gx][gy].push_back(i);
			
		velocity[i].y = 0.f;
		velocity[i].x = (1.f - ((rand() & 4095) / 2048.f)) / 10.f;
	}

	nParticles = count;
}
///////////////////////////////////////////////////////////////////////////////
void ShutdownSimulation()
{
	free(velocity);
	free(position);
	free(position_old);

	velocity = NULL;
	position = NULL;
	position_old = NULL;
}
///////////////////////////////////////////////////////////////////////////////
void DrawCircle(int32_t * pixels, int xres, int yres, int x, int y, int r)
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
				pixels[(i * xres) + j] = 0xff0000ff;
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
void RenderSimulation(int32_t * pixels, int x_res, int y_res)
{
	memset(pixels, 0, sizeof(int32_t) * x_res * y_res);
	for (int i=0; i<nParticles; i++)
	{
		int x = int(position[i].x * x_res);
		x = std::max(0, std::min(x, x_res - 1));
		int y = int((1.f - position[i].y) * y_res);
		y = std::max(0, std::min(y, y_res - 1));
		
		DrawCircle(pixels, x_res, y_res, x, y, 8);
	}
}
///////////////////////////////////////////////////////////////////////////////
void UpdateSimulation(float dt)
{
	for (int i=0; i<nParticles; i++)
	{
		velocity[i].y = (velocity[i].y + dt * fGravity);	
	}

	ApplyViscosity(dt);

	for (int i=0; i<nParticles; i++)
	{
		// save position
		position_old[i].x = position[i].x;
		position_old[i].y = position[i].y;
		// predicted new position
		position[i].x = position[i].x + velocity[i].x * dt;
		position[i].y = position[i].y + velocity[i].y * dt;
	}

	ApplyRelaxation(dt);

	// TODO: Don't be dumb by rebuilding the grid every update...
	for (int i=0; i<RES; i++)
	{
		for (int j=0; j<RES; j++)
		{
			grid[i][j].clear();
		}
	}

	// update velocity, grid cells
	for (int i=0; i<nParticles; i++)
	{
		velocity[i].x = (position[i].x - position_old[i].x) / dt;
		velocity[i].y = (position[i].y - position_old[i].y) / dt;

		int nx, ny;
		GetGridCell(position[i], &nx, &ny);
		grid[nx][ny].push_back(i);
	}

	ResolveCollisions(dt);
}
///////////////////////////////////////////////////////////////////////////////
void ApplyViscosity(float dt)
{
	for (int j=0; j<nParticles; j++)
	{
		int gx, gy;
		GetGridCell(position[j], &gx, &gy);
		int ulx = std::max(0, gx - 1);
		int uly = std::max(0, gy - 1);
		int lrx = std::min(RES-1, gx + 1);
		int lry = std::min(RES-1, gy + 1);

		for (int x=ulx; x<=lrx; x++)
		{
			for (int y=uly; y<=lry; y++)
			{
				Cell & cell = grid[x][y];
				for (unsigned i=0; i<cell.size(); i++)
				{
					int id = cell[i];
					if (j == id)
						continue;

					Point dir;
					Sub(position[id], position[j], &dir);
					float d = Length(dir);
					float q = d / fNearDistance;

					if (q < 1.f)
					{
						dir.x = dir.x / d;
						dir.y = dir.y / d;

						Point diff_vel;
						Sub(velocity[id], velocity[j], &diff_vel);
						float u = Dot(dir, diff_vel);
						if (u > 0.f)
						{
							float coeff = dt * (1.f - q) * (fViscosity[0] * u + fViscosity[1] * u * u);
							Point impulse;
							impulse.x = dir.x * coeff * 0.5f;
							impulse.y = dir.y * coeff * 0.5f;
							velocity[id].x -= impulse.x;
							velocity[id].y -= impulse.y;
							velocity[j].x += impulse.x;
							velocity[j].y += impulse.y;
						}
					}
				}
			}	
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
void ApplyRelaxation(float dt)
{
	for (int i=0; i<nParticles; i++)
	{
		float p = 0.f;
		float p_near = 0.f;

		int gx, gy;
		GetGridCell(position[i], &gx, &gy);
		int ulx = std::max(0, gx - 1);
		int uly = std::max(0, gy - 1);
		int lrx = std::min(RES-1, gx + 1);
		int lry = std::min(RES-1, gy + 1);

		for (int x=ulx; x<=lrx; x++)
		{
			for (int y=uly; y<=lry; y++)
			{
				Cell & cell = grid[x][y];
				for (unsigned j=0; j<cell.size(); j++)
				{
					int id = cell[j];
					if (i == id)
						continue;

					Point dir;
					Sub(position[id], position[i], &dir);
					float q = Length(dir) / fNearDistance;
					if (q < 1.f)
					{
						float co = 1.f - q;
						p += co * co;
						p_near += co * co * co;
					} 
				}
			}
		}

		p = fPressure * (p - fRestPressure);
		p_near = fPressureNear * p_near;

		Point delta;
		delta.x = delta.y = 0.f;
		for (int x=ulx; x<=lrx; x++)
		{
			for (int y=uly; y<=lry; y++)
			{
				Cell & cell = grid[x][y];
				for (unsigned j=0; j<cell.size(); j++)
				{
					int id = cell[j];
					if (i == id)
						continue;

					Point dir;
					Sub(position[id], position[i], &dir);
					float d = Length(dir);
					float q = d / fNearDistance;
					dir.x /= d;
					dir.y /= d;
					if (q < 1.f)
					{
						Point D;
						float co = 1.f - q;
						co = dt * dt * (p * co + p_near * co * co);
						D.x = co * dir.x * 0.5f;
						D.y = co * dir.y * 0.5f;
						position[id].x += D.x;
						position[id].y += D.y;
						delta.x -= D.x;
						delta.y -= D.y;
					}	
				}
			}
		}
		position[i].x += delta.x;
		position[i].y += delta.y;
	}
}
///////////////////////////////////////////////////////////////////////////////
void ResolveCollisions(float dt)
{
	for (int i=0; i<nParticles; i++)
	{
		float vx = velocity[i].x;
		float vy = velocity[i].y;

		if (position[i].x >= 1.f || position[i].x <= 0.f)
		{
			float col_dt = ((vx > 0 ? 1.f : 0.f) - position_old[i].x) / vx;
			Point cpt;
			cpt.x = position_old[i].x + vx * col_dt;
			cpt.y = position_old[i].y + vy * col_dt;
			
			velocity[i].x = -vx * fRestitution;
			velocity[i].y *= fFriction;

			position[i].x = cpt.x - (vx * (dt - col_dt) * fRestitution);
			position[i].y = cpt.y + (vy * (dt - col_dt) * fFriction);
		}
		
		if (position[i].y >= 1.f || position[i].y <= 0.f)
		{
			float col_dt = ((vy > 0 ? 1.f : 0.f) - position_old[i].y) / vy;
			Point cpt;
			cpt.x = position_old[i].x + vx * col_dt;
			cpt.y = position_old[i].y + vy * col_dt;

			velocity[i].x *= fFriction;
			velocity[i].y = -vy * fRestitution;

			position[i].x = cpt.x + (vx * (dt - col_dt) * fFriction);
			position[i].y = cpt.y - (vy * (dt - col_dt) * fRestitution);
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
void AddMousePuff(const Point & pt)
{
	int gx, gy;
	GetGridCell(pt, &gx, &gy);
	int ulx = std::max(0, gx - 1);
	int uly = std::max(0, gy - 1);
	int lrx = std::min(RES-1, gx + 1);
	int lry = std::min(RES-1, gy + 1);

	for (int x=ulx; x<=lrx; x++)
	{
		for (int y=uly; y<=lry; y++)
		{
			Cell & cell = grid[x][y];
			for (unsigned j=0; j<cell.size(); j++)
			{
				int id = cell[j];
				Point dir;
				Sub(position[id], pt, &dir);
				float d = Length(dir);
				dir.x /= d;
				dir.y /= d;
				float strength = 9.8f / (1.f + (d * d) * 100000.f);

				velocity[id].x += dir.x * strength;
				velocity[id].y += dir.y * strength;
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
