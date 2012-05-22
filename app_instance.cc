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

#include "app_instance.h"
#include <stdio.h>
#include <string.h>
#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/Var.h>
#include <sstream>
#include "Util.h"
#include "Fluid.h"

#define PI 3.1415926535897932384626433832795f
#define GRID_SIZE 128
#define TANK_SIZE 64.f

///////////////////////////////////////////////////////////////////////////////
void FlushCallback(void * data, int32_t result)
{
	((AppInstance *) data)->FlushComplete();
}
///////////////////////////////////////////////////////////////////////////////

void AddMousePuff(float x, float y);
void SetDensity(int, float);
void SetViscosity(int, float);
void SetColor(int, int, int, int);

///////////////////////////////////////////////////////////////////////////////
//
// ------------------------------- AppInstance --------------------------------
//
///////////////////////////////////////////////////////////////////////////////
AppInstance::AppInstance(PP_Instance instance)
	:	pp::Instance(instance),
		context(NULL),
		pixels(NULL),
		nWidth(0),
		nHeight(0),
		bFlushIsPending(false),
		bRenderSurface(true),
		bRenderDistance(false),
		bRenderFiltered(true),
		bMouseDown(false),
		bOneDown(false),
		bTwoDown(false),
		sim(NULL),
		water(NULL),
		oil(NULL)

{
	RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE);
	RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);

	sim = new FluidSim(TANK_SIZE, TANK_SIZE, 0.5f);
	water = new Fluid(sim->GWidth, sim->GHeight);

	water->Density = 2.f;
	water->Viscosity = 0.f;
	water->Color = 0xff0000ff;

	oil = new Fluid(sim->GWidth, sim->GHeight);

	oil->Density = 1.f;
	oil->Viscosity = 4.f;
	oil->Color = 0xffffff00;

	// Collision environment
	sim->SDF.AddCircle(sim->GWidth/2.f, sim->GHeight/2.f, 32.f);
	sim->SDF.AddCircle(0.f, sim->GHeight, 32.f);
	sim->SDF.AddCircle(sim->GWidth, sim->GHeight, 32.f);
	sim->SDF.Blur();

	sim->Fluids.push_back(water);
	sim->Fluids.push_back(oil);
}
///////////////////////////////////////////////////////////////////////////////
AppInstance::~AppInstance()
{
	delete sim;
	DestroyContext();
}
///////////////////////////////////////////////////////////////////////////////
void AppInstance::Clear()
{
	for (unsigned i=0; i<sim->Fluids.size(); i++)
		sim->Fluids[i]->Particles.clear();
}
///////////////////////////////////////////////////////////////////////////////
void AppInstance::HandleMessage(const pp::Var & var_message)
{
	if (!var_message.is_string())
		return;

	std::string message = var_message.AsString();
	if (message.empty())
		return;

	std::stringstream stream(message);
	std::string cmd;
	stream>>cmd;

	if (cmd == "Paint")
	{
		Paint();
	}
	else if (cmd == "Clear")
	{
		Clear();
	}
	else if (cmd == "ToggleSurface")
	{
		bRenderSurface = !bRenderSurface;
	}
	else if (cmd == "ToggleDistance")
	{
		bRenderDistance = !bRenderDistance;
	}
	else if (cmd == "ToggleFiltering")
	{
		bRenderFiltered = !bRenderFiltered;
	}
	else if (cmd == "GridCoeff")
	{
		float value;
		stream>>value;
		if (stream.fail())
		{
			std::string msg("{ \"Log\": \"Error parsing 'GridCoeff' command\" }");
			PostMessage(pp::Var(msg));
			return;
		}
		sim->GridCoeff = value;
	}
	else if (cmd == "GravityX")
	{
		float value;
		stream>>value;
		if (stream.fail())
		{
			std::string msg("{ \"Log\": \"Error parsing 'GravityX' command\" }");
			PostMessage(pp::Var(msg));
			return;
		}
		sim->GravityX = (value / sim->Scale) * (1.f / 900.f);
	}
	else if (cmd == "GravityY")
	{
		float value;
		stream>>value;
		if (stream.fail())
		{
			std::string msg("{ \"Log\": \"Error parsing 'GravityY' command\" }");
			PostMessage(pp::Var(msg));
			return;
		}
		sim->GravityY = (value / sim->Scale) * (1.f / 900.f);
	}
	else if (cmd == "Density")
	{
		unsigned int id;
		float value;
		stream>>id>>value;
		if (stream.fail())
		{
			std::string msg("{ \"Log\": \"Error parsing 'Density' command\" }");
			PostMessage(pp::Var(msg));
			return;
		}
		if (id < 0 || id > sim->Fluids.size())
		{
			std::string msg("{ \"Log\": \"Error setting fluid density - fluid ID out of range.\" }");
			PostMessage(pp::Var(msg));
			return;
		}
		sim->Fluids[id]->Density = value;
	}
	else if (cmd == "Viscosity")
	{
		unsigned int id;
		float value;
		stream>>id>>value;
		if (stream.fail())
		{
			std::string msg("{ \"Log\": \"Error parsing 'Viscosity' command\" }");
			PostMessage(pp::Var(msg));
			return;
		}
		if (id < 0 || id > sim->Fluids.size())
		{
			std::string msg("{ \"Log\": \"Error setting fluid viscosity - fluid ID out of range.\" }");
			PostMessage(pp::Var(msg));
			return;
		}
		sim->Fluids[id]->Viscosity = value;
	}
	else if (cmd == "Color")
	{
		unsigned int id, r, g, b;
		stream>>id>>r>>g>>b;
		if (stream.fail())
		{
			std::string msg("{ \"Log\": \"Error parsing 'Color' command\" }");
			PostMessage(pp::Var(msg));
			return;
		}
		if (id < 0 || id > sim->Fluids.size())
		{
			std::string msg("{ \"Log\": \"Error setting fluid color - fluid ID out of range.\" }");
			PostMessage(pp::Var(msg));
			return;
		}
		sim->Fluids[id]->Color = (255 << 24) | (r << 16) | (g << 8) | b;
	}
	else
	{
		std::string msg("{ \"Log\": \"Unknown command encountered\" }");
		PostMessage(pp::Var(msg));
		return;
	}
}
///////////////////////////////////////////////////////////////////////////////
bool AppInstance::HandleInputEvent(const pp::InputEvent & event)
{
	int type = event.GetType();

	if (type == PP_INPUTEVENT_TYPE_KEYDOWN)
	{
		pp::KeyboardInputEvent key(event);
		if (key.GetKeyCode() == '1')
			bOneDown = true;
		else if (key.GetKeyCode() == '2')
			bTwoDown = true;
	}
	else if (type == PP_INPUTEVENT_TYPE_KEYUP)
	{
		pp::KeyboardInputEvent key(event);
		if (key.GetKeyCode() == '1')
			bOneDown = false;
		else if (key.GetKeyCode() == '2')
			bTwoDown = false;
	}

	if (type == PP_INPUTEVENT_TYPE_MOUSEDOWN)
	{
		pp::MouseInputEvent mouse(event);
	
		fMouseX = mouse.GetPosition().x() / (float) nWidth;
		fMouseY = mouse.GetPosition().y() / (float) nHeight;

		bMouseDown = true;
	}
	else if (type == PP_INPUTEVENT_TYPE_MOUSELEAVE)
	{
		bMouseDown = bOneDown = bTwoDown = false;
	}
	else if (type == PP_INPUTEVENT_TYPE_MOUSEUP)
	{
		bMouseDown = false;
	}
	else if (type == PP_INPUTEVENT_TYPE_MOUSEMOVE)
	{
		pp::MouseInputEvent mouse(event);
	
		fMouseX = mouse.GetPosition().x() / (float) nWidth;
		fMouseY = mouse.GetPosition().y() / (float) nHeight;
	}

	return true;
}
///////////////////////////////////////////////////////////////////////////////
void AppInstance::DidChangeView(const pp::Rect & position,
								const pp::Rect & clip)
{
	if (position.size().width() == nWidth &&
		position.size().height() == nHeight)
	{
		return;
	}

	nWidth = position.size().width();
	nHeight = position.size().height();

	DestroyContext();
	CreateContext(position.size());

	delete pixels;
	pixels = NULL;

	if (!context)
		return;

	pixels = new pp::ImageData(this, PP_IMAGEDATAFORMAT_BGRA_PREMUL,
		context->size(), false);
}
///////////////////////////////////////////////////////////////////////////////
void AppInstance::Paint()
{
	std::stringstream ss;

	int64_t start, end;
	start = GetTimeMS();
	UpdateSimulation();
	end = GetTimeMS();

	ss<<"{ \"Update\": \""<<((int)(end-start))<<"\" }";
	PostMessage(pp::Var(ss.str()));

	start = GetTimeMS();
	RenderSimulation();
	FlushPixelBuffer();
	end = GetTimeMS();

	ss.str("");
	ss<<"{ \"Render\": \""<<((int)(end-start))<<"\" }";
	PostMessage(pp::Var(ss.str()));

	ss.str("");
	ss<<"{ \"Count\": \""<<sim->ParticleCount()<<"\" }";
	PostMessage(pp::Var(ss.str()));
}
///////////////////////////////////////////////////////////////////////////////
void AppInstance::UpdateSimulation()
{
	if (bMouseDown)
	{
		float fx = fMouseX * sim->GWidth;
		float fy = fMouseY * sim->GHeight;

		for (unsigned i=0; i<sim->Fluids.size(); i++)
		{
			for (int j=0, lim=sim->Fluids[i]->Particles.size(); j<lim; j++)
			{
				float dx = sim->Fluids[i]->Particles[j].x - fx;
				float dy = sim->Fluids[i]->Particles[j].y - fy;
				float l2 = dx*dx + dy*dy;
				if (l2 < 64.f)
				{
					float l = sqrtf(l2);
					sim->Fluids[i]->Particles[j].vx += (0.5f - (l / 16.f)) * dx * (1.f + frand() * 0.1f);
					sim->Fluids[i]->Particles[j].vy += (0.5f - (l / 16.f)) * dy * (1.f + frand() * 0.1f);
				}
			}
		}
	}
	else if (bOneDown)
	{
		for (int i=0; i<32; i++)
		{
			float x = (fMouseX * sim->GWidth) + (frand() * 6.f) - 3.f;
			float y = (fMouseY * sim->GHeight) + (frand() * 6.f) - 3.f;
			if (sim->SDF.SampleDistance(x, y) <= 0.f)
				continue;

			water->AddParticle(x, y, 0.f, 0.f);
		}	
	}
	else if (bTwoDown)
	{
		for (int i=0; i<32; i++)
		{
			float x = (fMouseX * sim->GWidth) + (frand() * 6.f) - 3.f;
			float y = (fMouseY * sim->GHeight) + (frand() * 6.f) - 3.f;
			if (sim->SDF.SampleDistance(x, y) <= 0.f)
				continue;

			oil->AddParticle(x, y, 0.f, 0.f);
		}
	}

	sim->Update();
}
///////////////////////////////////////////////////////////////////////////////
void AppInstance::RenderSimulation()
{
	int32_t * buffer = (int32_t *) pixels->data();
	memset(buffer, 0, sizeof(int32_t) * nWidth * nHeight);
	
	if (bRenderDistance || bRenderSurface)
	{
		float dx = (1.f / nWidth);
		float dy = (1.f / nHeight);
		float fx = 0.f, fy = 0.f;

		for (int y=0; y<nHeight; y++, fy+=dy)
		{
			fx = 0.f;
			for (int x=0; x<nWidth; x++, fx+=dx)
			{
				float d;

				if (!bRenderFiltered)
				{
					int mx = (int)(fx * sim->SDF.GetResolution());
					int my = (int)(fy * sim->SDF.GetResolution());
					d = sim->SDF.SampleDistance(mx, my);	
				}
				else
				{
					d = sim->SDF.SampleDistanceN(fx, fy);	
				}

				int id = y*nWidth+x;
				if (d < 0.1f && d > -0.1f && bRenderSurface)
				{
					buffer[id] = 0xffff0000;
				}
				else if (bRenderDistance)
				{
					int v = (fabs(d) / 5.f) * 255.f;
					v = v > 255 ? 255 : v;
					buffer[id] = 0xff000000 | v << 16 | v << 8 | v;
				}	
			}
		}
	}
	
	for (int i=0, lim=water->Particles.size(); i<lim; i++)
	{
		int x0 = floor((water->Particles[i].x / sim->GWidth) * nWidth);
		int y0 = floor((water->Particles[i].y / sim->GHeight) * nHeight);

		float dx = (water->Particles[i].vx / sim->GWidth) * nWidth;
		float dy = (water->Particles[i].vy / sim->GHeight) * nHeight;
		float len = sqrtf(dx*dx + dy*dy);

		if (len < 0.5f)
		{
			DrawCircle(buffer, nWidth, nHeight, x0, y0, 1, water->Color);
			continue;
		}

		dx /= len;
		dy /= len;
		int x1 = floor(x0 - dx*std::min(len*4.f, 10.f));
		int y1 = floor(y0 - dy*std::min(len*4.f, 10.f));

		DrawLine(buffer, nWidth, nHeight, x0, y0, x1, y1, water->Color);
	}

	for (int i=0, lim=oil->Particles.size(); i<lim; i++)
	{
		int x0 = floor((oil->Particles[i].x / sim->GWidth) * nWidth);
		int y0 = floor((oil->Particles[i].y / sim->GHeight) * nHeight);

		float dx = (oil->Particles[i].vx / sim->GWidth) * nWidth;
		float dy = (oil->Particles[i].vy / sim->GHeight) * nHeight;
		float len = sqrtf(dx*dx + dy*dy);

		if (len < 0.5f)
		{
			DrawCircle(buffer, nWidth, nHeight, x0, y0, 1, oil->Color);
			continue;
		}

		dx /= len;
		dy /= len;
		int x1 = floor(x0 - dx*std::min(len*4.f, 10.f));
		int y1 = floor(y0 - dy*std::min(len*4.f, 10.f));

		DrawLine(buffer, nWidth, nHeight, x0, y0, x1, y1, oil->Color);
	}
}
///////////////////////////////////////////////////////////////////////////////
void AppInstance::FlushPixelBuffer()
{
	if (!context)
		return;
		
	context->PaintImageData(*pixels, pp::Point());

	if (bFlushIsPending)
		return;

	context->Flush(pp::CompletionCallback(&FlushCallback, this));
}
///////////////////////////////////////////////////////////////////////////////
void AppInstance::CreateContext(const pp::Size & size)
{
	context = new pp::Graphics2D(this, size, false);
	if (!BindGraphics(*context))
	{
		printf("Error:  could not bind device context\n");
	}
}
///////////////////////////////////////////////////////////////////////////////
void AppInstance::DestroyContext()
{
	delete context;
	context = NULL;
}
///////////////////////////////////////////////////////////////////////////////

