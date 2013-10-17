//
// rayUI.h
//
// The header file for the UI part
//

#ifndef __rayUI_h__
#define __rayUI_h__

// who the hell cares if my identifiers are longer than 255 characters:
#pragma warning(disable : 4786)

#include <math.h>
#include "../vecmath/vec.h"
#include "../vecmath/mat.h"

#include <string>

using std::string;

class RayTracer;

class TraceUI {
public:
	TraceUI()
		: m_nDepth(0), m_nSize(150), 
		m_displayDebuggingInfo( false ),
		raytracer( 0 )
	{ }

	virtual int		run() = 0;

	// Send an alert to the user in some manner
	virtual void		alert(const string& msg) = 0;

	// setters
	virtual void		setRayTracer( RayTracer* r )
		{ raytracer = r; }

	// accessors:
	int		getSize() const { return m_nSize; }
    int		getDepth() const { return m_nDepth; }
    bool    acceleration() const { return m_accelerate; }
    bool    jitter() const { return m_bJitter; }
    bool    uniform() const { return m_bUniform; }
    bool    useSurface() const { return m_bSurfaceHeuristic; }
    int     getSampleSize() const { return m_nSampleSize; }


protected:
	RayTracer*	raytracer;

	int			m_nSize;				// Size of the traced image
    int			m_nDepth;				// Max depth of recursion
    bool        m_accelerate;           // acceleration preprocess is on?
    int         m_nSampleSize;          // super sample size
    bool        m_bJitter;
    bool        m_bUniform;
    bool        m_bSurfaceHeuristic;

	// Determines whether or not to show debugging information
	// for individual rays.  Disabled by default for efficiency
    // reasons.
    bool		m_displayDebuggingInfo;

};

#endif
