#ifndef GL_CORE_H
#define GL_CORE_H

#include <QtGui/QOpenGLFunctions_4_5_Core>
#include <memory>

using GLFunctions = QOpenGLFunctions_4_5_Core;
using std::unique_ptr;

class GLCore {

private:
	unique_ptr<QOpenGLContext> _context;
	GLFunctions* _funcs;

public:
	GLCore();
	GLCore(const QSurfaceFormat &format);
	GLCore(const GLCore& other);
	~GLCore();

	GLFunctions* getGL() const { return _funcs; }
	bool makeCurrent(QSurface *surface)const { return _context->makeCurrent(surface); }
	void swapBuffers(QSurface *surface) { _context->swapBuffers(surface); }
};




#endif 