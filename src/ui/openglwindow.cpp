#include "openglwindow.h"
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>
#include <QTimer>

#include <iostream>

using namespace std;

OpenGLWindow::OpenGLWindow(QWindow *parent)
: QWindow(parent), _animating(false), _initialized(false) {

	setSurfaceType(QWindow::OpenGLSurface);
	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setVersion(4, 5);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setSamples(1);
	format.setSwapInterval(0);
	setFormat(format);
	create();

	_glCore = std::make_unique<GLCore>(format);

	// This is to prevent the FPS limitation. Thanks a lot Qt!
	QTimer *timer = new QTimer();
	connect(timer, SIGNAL(timeout()), this, SLOT(renderNow()));
	timer->start(0);
}

void OpenGLWindow::initialize() {
	cout << "initialize window" << endl;
}


void OpenGLWindow::renderLater() {
	requestUpdate();
}

// This method will receive all the events associated with the current window
bool OpenGLWindow::event(QEvent *event) {

	switch (event->type()) {
	case QEvent::UpdateRequest:
		renderNow();
		return true;

	case QEvent::Resize:
		resizeGL(width(), height());
		return true;

	default:
		return QWindow::event(event);
	}
}

void OpenGLWindow::exposeEvent(QExposeEvent *event) {
	Q_UNUSED(event);

	if (isExposed())
		renderNow();
}

void OpenGLWindow::resizeGL(int width, int height) {
	if (!isExposed())
		return;

	_glCore->makeCurrent(this);

	auto gl = _glCore->getGL();

	WIN_WIDTH = width;
	WIN_HEIGHT = height;
	gl->glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);
}

void OpenGLWindow::renderNow() {
	if (!isExposed())
		return;

	_glCore->makeCurrent(this);


	if (!_initialized) {
		_glCore->getGL()->initializeOpenGLFunctions();
		_initialized = true;
		initialize();

	}

	render();

	_glCore->swapBuffers(this);

	if (_animating)
		renderLater();

}

void OpenGLWindow::setAnimating(bool animating) {
	_animating = animating;

	if (animating)
		renderLater();
}

void OpenGLWindow::compileAndLinkShaders(graphics::Shader &program, const vector<string> &filePaths) {
	try {
		for (auto path : filePaths)
			program.compileShader(path.c_str());

		program.link();
		program.validate();
		program.use();
	} catch (graphics::ShaderException & e) {
		std::cerr << e.what() << std::endl;
		getchar();
		exit(EXIT_FAILURE);
	}

}


OpenGLWindow::~OpenGLWindow() {
}
