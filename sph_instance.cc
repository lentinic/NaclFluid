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

#include "sph_instance.h"
#include "Util.h"
#include <stdio.h>
#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/Var.h>

///////////////////////////////////////////////////////////////////////////////
void FlushCallback(void * data, int32_t result)
{
	((SPHInstance *) data)->FlushComplete();
}
///////////////////////////////////////////////////////////////////////////////

extern void InitSimulation(int);
extern void ShutdownSimulation();
extern void UpdateSimulation(float);
extern void RenderSimulation(int32_t *, int, int);
extern void AddMousePuff(const Point &);

///////////////////////////////////////////////////////////////////////////////
//
// ------------------------------- SPHInstance --------------------------------
//
///////////////////////////////////////////////////////////////////////////////
SPHInstance::SPHInstance(PP_Instance instance)
	:	pp::Instance(instance),
		context(NULL),
		pixels(NULL),
		nWidth(0),
		nHeight(0),
		bFlushIsPending(false)
{
	RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE);
	InitSimulation(1024);
}
///////////////////////////////////////////////////////////////////////////////
SPHInstance::~SPHInstance()
{
	ShutdownSimulation();
	DestroyContext();
}
///////////////////////////////////////////////////////////////////////////////
void SPHInstance::HandleMessage(const pp::Var & var_message)
{
	if (!var_message.is_string())
		return;

	std::string message = var_message.AsString();
	if (message == "paint")
	{
		Paint();
	}
}
///////////////////////////////////////////////////////////////////////////////
bool SPHInstance::HandleInputEvent(const pp::InputEvent & event)
{
	if (event.GetType() == PP_INPUTEVENT_TYPE_MOUSEMOVE)
	{
		pp::MouseInputEvent mouse(event);
		Point pos;
		pos.x = mouse.GetPosition().x() / (float) nWidth;
		pos.y = 1.f - mouse.GetPosition().y() / (float) nHeight;
		AddMousePuff(pos);
	}
	return true;
}
///////////////////////////////////////////////////////////////////////////////
void SPHInstance::DidChangeView(const pp::Rect & position,
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
void SPHInstance::Paint()
{
	int64_t start, end;
	start = GetTimeMicro();
	UpdateSimulation(1.f / 30.f);
	end = GetTimeMicro();

	char tmp[255];
	sprintf(tmp, "Time: %d ms", (int)(end - start));
	PostMessage(pp::Var(tmp));

	RenderSimulation((int32_t *) pixels->data(), nWidth, nHeight);

	FlushPixelBuffer();
}
///////////////////////////////////////////////////////////////////////////////
void SPHInstance::FlushPixelBuffer()
{
	if (!context)
		return;
		
	context->PaintImageData(*pixels, pp::Point());

	if (bFlushIsPending)
		return;

	context->Flush(pp::CompletionCallback(&FlushCallback, this));
}
///////////////////////////////////////////////////////////////////////////////
void SPHInstance::CreateContext(const pp::Size & size)
{
	context = new pp::Graphics2D(this, size, false);
	if (!BindGraphics(*context))
	{
		printf("Error:  could not bind device context\n");
	}
}
///////////////////////////////////////////////////////////////////////////////
void SPHInstance::DestroyContext()
{
	delete context;
	context = NULL;
}
///////////////////////////////////////////////////////////////////////////////

