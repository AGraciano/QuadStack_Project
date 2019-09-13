#include"glcore.h"
#include <iostream>

using namespace std;
GLCore::GLCore() {
}

GLCore::GLCore(const QSurfaceFormat &format) {
	_context.reset(new QOpenGLContext);
	_context->setFormat(format);
	_context->create();

	_funcs = _context->versionFunctions<GLFunctions>();
	if (!_funcs) {
		qWarning("Could not obtain OpenGL versions object");
		exit(1);
	}

}

GLCore::~GLCore() {

	_context.reset();
}