//
// GraphicalUI.cpp
//
// Handles FLTK integration and other user interface tasks
//
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#ifndef COMMAND_LINE_ONLY

#include <FL/fl_ask.H>
#include "debuggingView.h"

#include "GraphicalUI.h"
#include "../RayTracer.h"

bool GraphicalUI::stopTrace = false;
bool GraphicalUI::doneTrace = true;

//------------------------------------- Help Functions --------------------------------------------
GraphicalUI* GraphicalUI::whoami(Fl_Menu_* o)	// from menu item back to UI itself
{
	return ( (GraphicalUI*)(o->parent()->user_data()) );
}

//--------------------------------- Callback Functions --------------------------------------------
void GraphicalUI::cb_load_scene(Fl_Menu_* o, void* v) 
{
	GraphicalUI* pUI=whoami(o);
	
	static char* lastFile = 0;
	char* newfile = fl_file_chooser("Open Scene?", "*.ray", NULL );

	if (newfile != NULL) {
		char buf[256];

		if (pUI->raytracer->loadScene(newfile)) {
			sprintf(buf, "Ray <%s>", newfile);
			stopTracing();	// terminate the previous rendering
		} else{
			sprintf(buf, "Ray <Not Loaded>");
		}

		pUI->m_mainWindow->label(buf);
		pUI->m_debuggingWindow->m_debuggingView->setDirty();

		if( lastFile != 0 && strcmp(newfile, lastFile) != 0 )
			pUI->m_debuggingWindow->m_debuggingView->resetCamera();

		pUI->m_debuggingWindow->redraw();
	}
}

void GraphicalUI::cb_save_image(Fl_Menu_* o, void* v) 
{
	GraphicalUI* pUI=whoami(o);
	
	char* savefile = fl_file_chooser("Save Image?", "*.bmp", "save.bmp" );
	if (savefile != NULL) {
		pUI->m_traceGlWindow->saveImage(savefile);
	}
}

void GraphicalUI::cb_exit(Fl_Menu_* o, void* v)
{
	GraphicalUI* pUI=whoami(o);

	// terminate the rendering
	stopTracing();

	pUI->m_traceGlWindow->hide();
	pUI->m_mainWindow->hide();
	pUI->m_debuggingWindow->hide();
}

void GraphicalUI::cb_exit2(Fl_Widget* o, void* v) 
{
	GraphicalUI* pUI=(GraphicalUI *)(o->user_data());
	
	// terminate the rendering
	stopTracing();

	pUI->m_traceGlWindow->hide();
	pUI->m_mainWindow->hide();
	pUI->m_debuggingWindow->hide();
}

void GraphicalUI::cb_about(Fl_Menu_* o, void* v) 
{
	fl_message("RayTracer Project, FLTK version for CS384g Fall 2005.");
}

void GraphicalUI::cb_sizeSlides(Fl_Widget* o, void* v)
{
	GraphicalUI* pUI=(GraphicalUI*)(o->user_data());

	// terminate the rendering so we don't get crashes
	stopTracing();

	pUI->m_nSize=int( ((Fl_Slider *)o)->value() ) ;
	int	height = (int)(pUI->m_nSize / pUI->raytracer->aspectRatio() + 0.5);
	pUI->m_traceGlWindow->resizeWindow( pUI->m_nSize, height );
	// Need to call traceSetup before trying to render
	pUI->raytracer->setReady(false);
}

void GraphicalUI::cb_depthSlides(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_nDepth=int( ((Fl_Slider *)o)->value() ) ;
}

void GraphicalUI::cb_jitterSamplingRadioButton(Fl_Widget* o, void* v)
{
    ((GraphicalUI*)(o->user_data()))->m_bJitter= true ;
    ((GraphicalUI*)(o->user_data()))->m_bUniform= false ;
}

void GraphicalUI::cb_uniformSamplingRadioButton(Fl_Widget* o, void* v)
{
    ((GraphicalUI*)(o->user_data()))->m_bJitter= false ;
    ((GraphicalUI*)(o->user_data()))->m_bUniform= true ;
}

void GraphicalUI::cb_sampleSizeSlides(Fl_Widget* o, void* v)
{
    ((GraphicalUI*)(o->user_data()))->m_nSampleSize=int( ((Fl_Slider *)o)->value() ) ;
}

void GraphicalUI::cb_debuggingDisplayCheckButton(Fl_Widget* o, void* v)
{
	GraphicalUI* pUI=(GraphicalUI*)(o->user_data());
	pUI->m_displayDebuggingInfo = (((Fl_Check_Button*)o)->value() == 1);
	if( pUI->m_displayDebuggingInfo )
		pUI->m_debuggingWindow->show();
	else
		pUI->m_debuggingWindow->hide();
}

void GraphicalUI::cb_heuristicCheckButton(Fl_Widget* o, void* v)
{
    GraphicalUI* pUI=(GraphicalUI*)(o->user_data());
    pUI->m_bSurfaceHeuristic = (((Fl_Check_Button*)o)->value() == 1);
}

void GraphicalUI::cb_adaptiveSamplingCheckButton(Fl_Widget* o, void* v)
{
    GraphicalUI* pUI=(GraphicalUI*)(o->user_data());
    pUI->m_bAdaptiveSampling = (((Fl_Check_Button*)o)->value() == 1);
}

void GraphicalUI::cb_accelerateCheckButton(Fl_Widget* o, void* v)
{
    GraphicalUI* pUI=(GraphicalUI*)(o->user_data());
    pUI->m_accelerate = (((Fl_Check_Button*)o)->value() == 1);
}

void GraphicalUI::cb_render(Fl_Widget* o, void* v)
{
	char buffer[256];

	GraphicalUI* pUI=((GraphicalUI*)(o->user_data()));
	
	if (pUI->raytracer->sceneLoaded()) {
		int width=pUI->getSize();
		int	height = (int)(width / pUI->raytracer->aspectRatio() + 0.5);
		pUI->m_traceGlWindow->resizeWindow( width, height );

		pUI->m_traceGlWindow->show();

		pUI->raytracer->traceSetup(width, height);
		
		// Save the window label
		const char *old_label = pUI->m_traceGlWindow->label();

		// start to render here	
		clock_t prev, now;
		prev=clock();
		
		pUI->m_traceGlWindow->refresh();
		Fl::check();
		Fl::flush();

		doneTrace = false;
		stopTrace = false;
		for (int y=0; y<height; y++) {
			for (int x=0; x<width; x++) {
				if (stopTrace) break;
				
				// current time
				now = clock();

				// check event every 1/2 second
				if (((double)(now-prev)/CLOCKS_PER_SEC)>0.5) {
					prev=now;

					// refresh
					pUI->m_traceGlWindow->refresh();
					Fl::check();

					if (Fl::damage()) {
						Fl::flush();
					}
                }
				pUI->raytracer->tracePixel( x, y );
				pUI->m_debuggingWindow->m_debuggingView->setDirty();
			}
			if (stopTrace) break;

			// refresh at end of row
			pUI->m_traceGlWindow->refresh();
			Fl::check();

			if (Fl::damage()) {
				Fl::flush();
			}

			// update the window label
			sprintf(buffer, "(%d%%) %s", (int)((double)y / (double)height * 100.0), old_label);
			pUI->m_traceGlWindow->label(buffer);
			
		}
		doneTrace=true;
		stopTrace=false;

		pUI->m_traceGlWindow->refresh();

		// Restore the window label
		pUI->m_traceGlWindow->label(old_label);		
	}
}

void GraphicalUI::cb_stop(Fl_Widget* o, void* v)
{
	stopTrace = true;
}

int GraphicalUI::run()
{
	Fl::visual(FL_DOUBLE|FL_INDEX);

	m_mainWindow->show();

	return Fl::run();
}

void GraphicalUI::alert( const string& msg )
{
	fl_alert( "%s", msg.c_str() );
}

void GraphicalUI::setRayTracer(RayTracer *tracer)
{
	TraceUI::setRayTracer( tracer );
	m_traceGlWindow->setRayTracer(tracer);
	m_debuggingWindow->m_debuggingView->setRayTracer(tracer);
}

// menu definition
Fl_Menu_Item GraphicalUI::menuitems[] = {
	{ "&File",		0, 0, 0, FL_SUBMENU },
		{ "&Load Scene...",	FL_ALT + 'l', (Fl_Callback *)GraphicalUI::cb_load_scene },
		{ "&Save Image...",	FL_ALT + 's', (Fl_Callback *)GraphicalUI::cb_save_image },
		{ "&Exit",			FL_ALT + 'e', (Fl_Callback *)GraphicalUI::cb_exit },
		{ 0 },

	{ "&Help",		0, 0, 0, FL_SUBMENU },
		{ "&About",	FL_ALT + 'a', (Fl_Callback *)GraphicalUI::cb_about },
		{ 0 },

	{ 0 }
};

void GraphicalUI::stopTracing()
{
	if( stopTrace ) return;			// Only one person can be waiting at a time

	stopTrace = true;

	// Wait for the trace to finish (simple synchronization)
	while( !doneTrace )	Fl::wait();
}

GraphicalUI::GraphicalUI() {
	// init.

    m_mainWindow = new Fl_Window(100, 40, 350, 320, "Ray <Not Loaded>");
		m_mainWindow->user_data((void*)(this));	// record self to be used by static callback functions
		// install menu bar
		m_menubar = new Fl_Menu_Bar(0, 0, 320, 25);
		m_menubar->menu(menuitems);

		// install depth slider
        m_nDepth = 2;
		m_depthSlider = new Fl_Value_Slider(10, 30, 180, 20, "Depth");
		m_depthSlider->user_data((void*)(this));	// record self to be used by static callback functions
		m_depthSlider->type(FL_HOR_NICE_SLIDER);
        m_depthSlider->labelfont(FL_COURIER);
        m_depthSlider->labelsize(12);
		m_depthSlider->minimum(0);
		m_depthSlider->maximum(10);
		m_depthSlider->step(1);
		m_depthSlider->value(m_nDepth);
		m_depthSlider->align(FL_ALIGN_RIGHT);
		m_depthSlider->callback(cb_depthSlides);

		// install size slider
		m_sizeSlider = new Fl_Value_Slider(10, 55, 180, 20, "Size");
		m_sizeSlider->user_data((void*)(this));	// record self to be used by static callback functions
		m_sizeSlider->type(FL_HOR_NICE_SLIDER);
        m_sizeSlider->labelfont(FL_COURIER);
        m_sizeSlider->labelsize(12);
		m_sizeSlider->minimum(64);
		m_sizeSlider->maximum(512);
		m_sizeSlider->step(1);
		m_sizeSlider->value(m_nSize);
		m_sizeSlider->align(FL_ALIGN_RIGHT);
		m_sizeSlider->callback(cb_sizeSlides);

        // install sample size slider
        m_nSampleSize = 1;
        m_sampleSizeSlider = new Fl_Value_Slider(10, 80, 180, 20, "Sampling Size");
        m_sampleSizeSlider->user_data((void*)(this));	// record self to be used by static callback functions
        m_sampleSizeSlider->type(FL_HOR_NICE_SLIDER);
        m_sampleSizeSlider->labelfont(FL_COURIER);
        m_sampleSizeSlider->labelsize(12);
        m_sampleSizeSlider->minimum(1);
        m_sampleSizeSlider->maximum(10);
        m_sampleSizeSlider->step(1);
        m_sampleSizeSlider->value(m_nSampleSize);
        m_sampleSizeSlider->align(FL_ALIGN_RIGHT);
        m_sampleSizeSlider->callback(cb_sampleSizeSlides);

        // turn uniform sampling
        m_bUniform = true;
        m_uniformSamplingButton = new Fl_Round_Button(0, 110, 180, 20, "Uniform Sampling");
        m_uniformSamplingButton->user_data((void*)(this));
        m_uniformSamplingButton->callback(cb_jitterSamplingRadioButton);
        m_uniformSamplingButton->type(FL_RADIO_BUTTON);
        m_uniformSamplingButton->setonly();

        // turn jitter sampling
        m_bJitter = false;
        m_jitterSamplingButton = new Fl_Round_Button(0, 140, 180, 20, "Jitter Sampling");
        m_jitterSamplingButton->user_data((void*)(this));
        m_jitterSamplingButton->callback(cb_uniformSamplingRadioButton);
        m_jitterSamplingButton->type(FL_RADIO_BUTTON);

        m_lineLabel = new Fl_Box (0,170,300,20,"Tree is built when scene is loaded!!");
        // set up heuristic checkbox
        m_bSurfaceHeuristic = true;
        m_surfaceHeuristicButton = new Fl_Check_Button(0, 200, 180, 20, "Surface or Median Heuristic? X for SAH");
        m_surfaceHeuristicButton->user_data((void*)(this));
        m_surfaceHeuristicButton->callback(cb_heuristicCheckButton);
        m_surfaceHeuristicButton->set();

        // set up acceleration checkbox
        m_accelerate = true;
        m_accelerateCheckButton = new Fl_Check_Button(0, 230, 180, 20, "Accelerate");
        m_accelerateCheckButton->user_data((void*)(this));
        m_accelerateCheckButton->callback(cb_accelerateCheckButton);
        m_accelerateCheckButton->set();

        // set up adaptive checkbox
        m_bAdaptiveSampling = true;
        m_adaptiveSamplingButton = new Fl_Check_Button(0, 260, 180, 20, "Adaptive Sampling");
        m_adaptiveSamplingButton->user_data((void*)(this));
        m_adaptiveSamplingButton->callback(cb_adaptiveSamplingCheckButton);
        m_adaptiveSamplingButton->set();

		// set up debugging display checkbox
        m_debuggingDisplayCheckButton = new Fl_Check_Button(0, 290, 180, 20, "Debugging display");
		m_debuggingDisplayCheckButton->user_data((void*)(this));
        m_debuggingDisplayCheckButton->callback(cb_debuggingDisplayCheckButton);
        m_debuggingDisplayCheckButton->value(m_displayDebuggingInfo);

		// set up "render" button
		m_renderButton = new Fl_Button(240, 27, 70, 25, "&Render");
		m_renderButton->user_data((void*)(this));
		m_renderButton->callback(cb_render);

		// set up "stop" button
		m_stopButton = new Fl_Button(240, 55, 70, 25, "&Stop");
		m_stopButton->user_data((void*)(this));
		m_stopButton->callback(cb_stop);

		m_mainWindow->callback(cb_exit2);
		m_mainWindow->when(FL_HIDE);
    m_mainWindow->end();

	// image view
	m_traceGlWindow = new TraceGLWindow(100, 150, m_nSize, m_nSize, "Rendered Image");
	m_traceGlWindow->end();
	m_traceGlWindow->resizable(m_traceGlWindow);

	// debugging view
	m_debuggingWindow = new DebuggingWindow();
}

#endif

