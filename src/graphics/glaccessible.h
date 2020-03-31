/**
*	Interface to be implemented for any object which will access to the
*	OpenGL API. The decision of this approach has lots of "phylosophical"
*	computer science and C++ issues. This solution is argumented in the
*	documentation.
*
*	@class GLAccesible
*	@author Alejandro Graciano
*/

#ifndef GL_ACCESSIBLE_H
#define GL_ACCESSIBLE_H

#include "glcore.h"
#include <memory>
#include <iostream>

using std::shared_ptr;
using std::unique_ptr;

class GLAccessible {

protected:
	GLFunctions *_gl;

	GLAccessible();
public:
	GLAccessible(GLFunctions* gl) : _gl(gl) {}
	static int checkForOpenGLError(const char * file, int line);

};

#endif