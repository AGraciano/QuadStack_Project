#include "scenewindow.h"

#include "graphics/quadstackview.h"
#include "io/vtkgridreader.h"
#include "core/stackbasedrep.h"
#include "core/octree.h"
#include "core/compressionmanager.h"
#include <chrono>

using std::cout;
using std::endl;

MouseAction SceneWindow::_lastAction = MouseAction::Nothing;
Qt::MouseButton SceneWindow::_buttonPressed = Qt::NoButton;

double SceneWindow::_lastXpos = 0;
double SceneWindow::_lastYpos = 0;



SceneWindow::SceneWindow(QStatusBar *statusBar, QWindow *parent) :
OpenGLWindow(parent),
_statusBar(statusBar),
_camera(new graphics::OrbitalCamera()),
_resizeRenderBuffers(false){
}

QuadStack* SceneWindow::initModel() {
	ShortVTKReader reader;
	
	std::cout << "Reading..." << std::endl;
	ShortVM *vm = reader.open("data/sample_terrain.vtk");

	unsigned nRows = vm->getDimensionX();
	unsigned nCols = vm->getDimensionY();

	if (nRows & (nRows - 1) != 0 || nCols & (nCols - 1) != 0) {
		std::cout << "Dataset with a non power-of-two columns or rows." << std::endl;
		exit(1);
	}

	std::cout << "Octree construction..." << std::endl;
	auto start = std::chrono::high_resolution_clock::now();
	ShortOctree *octree = new ShortOctree(vm);
	auto stop = std::chrono::high_resolution_clock::now();
	auto durationOctree = std::chrono::duration_cast<std::chrono::seconds>(stop - start).count();
	std::cout << "Construction time: " << durationOctree << " s" << std::endl;

	std::cout << "SBR construction..." << std::endl;
	start = std::chrono::high_resolution_clock::now();
	ShortSBR *sbr = new ShortSBR(*vm);
	stop = std::chrono::high_resolution_clock::now();
	auto durationSBR = std::chrono::duration_cast<std::chrono::seconds>(stop - start).count();
	std::cout << "Construction time: " << durationSBR << " s" << std::endl;

	std::cout << "QuadStack construction..." << std::endl;
	start = std::chrono::high_resolution_clock::now();
	CompressionManager cm(sbr);
	cm.execute();
	stop = std::chrono::high_resolution_clock::now();
	auto durationQS = std::chrono::duration_cast<std::chrono::seconds>(stop - start).count();
	std::cout << "Construction time: " << durationQS << " s" << std::endl;

	_voxelBytes = vm->memorySize();
	_sbrBytes = sbr->memorySize();
	_octreeBytes = octree->memorySize();
	//showStats();
	//getchar();
	delete vm;
	delete octree;
	return cm.getQuadStack();
}

void SceneWindow::initVisualization(QuadStack* qs) {
	GLFunctions* gl = _glCore->getGL();

	graphics::Renderable *r = new QuadStackView(gl, qs, WIN_WIDTH, WIN_HEIGHT);

	std::cout << "GPU storage and Mipmap calculation..." << std::endl;
	auto start = std::chrono::high_resolution_clock::now();
	_qsBytes = dynamic_cast<QuadStackView*>(r)->init();
	auto stop = std::chrono::high_resolution_clock::now();
	auto durationGPU = std::chrono::duration_cast<std::chrono::seconds>(stop - start).count();
	std::cout << "Construction time: " << durationGPU << " s" << std::endl;

	_model = r;

	setupFBO();
}

void SceneWindow::initialize() {
	auto gl = _glCore->getGL();
	WIN_WIDTH = width();
	WIN_HEIGHT = height();

	QuadStack *qs = initModel();
	
	gl->glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	float aspect = static_cast<float>(WIN_WIDTH) / WIN_HEIGHT;
	_camera.reset(new graphics::OrbitalCamera(3.14 / 5, 3.1416 / -2, -1.2, 0, 0, 45.0, aspect));

	_frames = 0;
	
	gl->glEnable(GL_BLEND);
	gl->glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA); //This is what we are implementing in shader
	gl->glEnable(GL_CULL_FACE);
	gl->glEnable(GL_DEPTH_TEST);
	gl->glEnable(GL_MULTISAMPLE);
	
	initVisualization(qs);
	showStats();
	//getchar();
}

void SceneWindow::showStats() {
	std::cout << std::endl;
	std::cout << "-------------------------------" << std::endl;
	std::cout << "Voxel model size: " << _voxelBytes / BYTES_TO_MBYTES << " MB" << std::endl;
	std::cout << "Octree size: " << _octreeBytes / BYTES_TO_MBYTES << " MB" << std::endl;
	std::cout << "SBR size: " << _sbrBytes / BYTES_TO_MBYTES << " MB" << std::endl;
	std::cout << "QS attributes size: " << _qsBytes.x / BYTES_TO_MBYTES << " MB" << std::endl;
	std::cout << "QS heightfields raw size: " << _qsBytes.w / BYTES_TO_MBYTES << " MB" << std::endl;
	std::cout << "QS heightfields size: " << _qsBytes.y / BYTES_TO_MBYTES << " MB" << std::endl;
	std::cout << "QS total size: " << (_qsBytes.x + _qsBytes.y) / BYTES_TO_MBYTES << " MB" << std::endl;
	std::cout << "Mipmap size: " << (_qsBytes.z) / BYTES_TO_MBYTES << " MB" << std::endl;
	std::cout << "QS total size with mipmap: " << (_qsBytes.x + _qsBytes.y + _qsBytes.z) / BYTES_TO_MBYTES << "MB" << std::endl;
	std::cout << "-------------------------------" << std::endl;
}

void SceneWindow::setMatrices(const unsigned calculation) {
	mat4 model = _model->getModel();
	mat4 view = _camera->getViewMatrix();
	mat4 mv = view * model;
	mat4 projection = _camera->getProjection();
	mat4 mvp = projection * mv;
	mat4 inversemvp = glm::inverse(mvp);
	mat4 vm = glm::inverse(mv);
	vec3 camPos(vm[3]);
	vec3 lookup(vm[2]);

	if (calculation != CALCULATE_MATRICES) {
		view = mat4(1.0f);
		mv = mat4(1.0f);
		projection = mat4(1.0f);
	}

	bool downwards = lookup.y > 0;

	_model->getShader()->setUniform("downwards", downwards);
	_model->getShader()->setUniform("mvpMatrix", mvp);
	_model->getShader()->setUniform("inversemvpMatrix", inversemvp);
	_model->getShader()->setUniform("cameraPosition", camPos);
}

void SceneWindow::resizeFBO() {
	_resizeRenderBuffers = false;
	auto gl = _glCore->getGL();

	_model->getShader()->bindTexture(GL_TEXTURE4, GL_TEXTURE_2D, _depthTex);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIN_WIDTH, WIN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

	_model->getShader()->bindTexture(GL_TEXTURE5, GL_TEXTURE_2D, _colorTex);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIN_WIDTH, WIN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

	_model->getShader()->bindTexture(GL_TEXTURE6, GL_TEXTURE_2D, _normalTex);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIN_WIDTH, WIN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

	_model->getShader()->bindTexture(GL_TEXTURE7, GL_TEXTURE_2D, _colorNormalTex);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIN_WIDTH, WIN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

	GLAccessible::checkForOpenGLError(__FILE__, __LINE__);
}


void SceneWindow::setupFBO() {
	auto gl = _glCore->getGL();
	// Generate the fbos
	gl->glGenFramebuffers(1, &_fboRaycasting);

	// Set the fbo for the rayscating pass
	gl->glBindFramebuffer(GL_FRAMEBUFFER, _fboRaycasting);
	
	// Create the texture for the depth
	gl->glGenTextures(1, &_depthTex);
	_model->getShader()->bindTexture(GL_TEXTURE4, GL_TEXTURE_2D, _depthTex);
	
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIN_WIDTH, WIN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	// Create the texture for the color
	gl->glGenTextures(1, &_colorTex);
	_model->getShader()->bindTexture(GL_TEXTURE5, GL_TEXTURE_2D, _colorTex);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIN_WIDTH, WIN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	gl->glGenTextures(1, &_normalTex);
	_model->getShader()->bindTexture(GL_TEXTURE6, GL_TEXTURE_2D, _normalTex);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIN_WIDTH, WIN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	// Attach the textures to the framebuffer
	gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTex, 0);
	gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _depthTex, 0);
	gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, _normalTex, 0);
	
	// Set the targets for the fragment output variables
	GLuint drawBuffers2[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	gl->glDrawBuffers(3, drawBuffers2);

	// Generate the fbos
	gl->glGenFramebuffers(1, &_fboNormal);

	// Set the fbo for the rayscating pass
	gl->glBindFramebuffer(GL_FRAMEBUFFER, _fboNormal);

	// Create the texture for the final color
	//GLuint _colorNormalTex;
	gl->glGenTextures(1, &_colorNormalTex);
	_model->getShader()->bindTexture(GL_TEXTURE7, GL_TEXTURE_2D, _colorNormalTex);
	//gl->glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, WIN_WIDTH, WIN_HEIGHT);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIN_WIDTH, WIN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	// Attach the textures to the framebuffer
	gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, _colorNormalTex, 0);

	// Set the targets for the fragment output variables
	GLuint drawBuffers3[] = { GL_COLOR_ATTACHMENT3 };
	gl->glDrawBuffers(1, drawBuffers3);

	// Unbind the framebuffer and revert to default framebuffer
	gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);


}


void SceneWindow::render() {
	auto gl = _glCore->getGL();
	gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	if (_model == nullptr)
		return;

	gl->glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);
	
	auto currentTime = std::chrono::high_resolution_clock::now();
	_frames++;
	if (duration_cast<std::chrono::seconds>(currentTime - _lastTime).count() >= 1.0) { // If last cout was more than 1 sec ago
		QString fpsLabel = QString::number(_frames) + " fps";
		_statusBar->showMessage(fpsLabel);

		_frames = 0;
		_lastTime = currentTime;
	}


	if (_resizeRenderBuffers)
		resizeFBO();
	
	try {

		raycastingRender();
		normalCalculationRender();
		
	} catch (std::out_of_range &e) {
		std::cerr << e.what() << std::endl;
	}
}

void SceneWindow::raycastingRender() {
	auto gl = _glCore->getGL();
	gl->glBindFramebuffer(GL_FRAMEBUFFER, _fboRaycasting);
	//gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
	gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	GLuint raycastingPass = dynamic_cast<QuadStackView*>(_model)->getRaycastingPassHandle();
	gl->glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &raycastingPass);

	//calcFPS(1.0, "Current FPS: ");
	//glEnable(GL_BLEND);
	
	setMatrices(CALCULATE_MATRICES);
	_model->render();
	
	gl->glFinish();
}

void SceneWindow::normalCalculationRender() {
	auto gl = _glCore->getGL();
	gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
	gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLuint shadingPass = dynamic_cast<QuadStackView*>(_model)->getShadingPassHandle();
	gl->glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &shadingPass);

	setMatrices(CALCULATE_MATRICES);

	_model->render();

	GLAccessible::checkForOpenGLError(__FILE__, __LINE__);
}


void SceneWindow::mouseMoveEvent(QMouseEvent *ev) {
	double dx = (ev->y() - _lastYpos) / 70;
	double dy = (ev->x() - _lastXpos) / 70;

	if (_buttonPressed == Qt::MouseButton::LeftButton && _lastAction == MouseAction::Press)
		_camera->rotate(dx, dy);

	if (_buttonPressed == Qt::MouseButton::RightButton && _lastAction == MouseAction::Press)
		_camera->pan(dy / 10, -dx / 10);

	_lastXpos = ev->x();
	_lastYpos = ev->y();
}

void SceneWindow::mousePressEvent(QMouseEvent *ev) {
	_lastAction = MouseAction::Press;
	_buttonPressed = ev->button();
}

void SceneWindow::mouseReleaseEvent(QMouseEvent *ev) {
	_lastAction = MouseAction::Release;
	_buttonPressed = ev->button();
}

void SceneWindow::wheelEvent(QWheelEvent *ev) {
	_camera->zoom(ev->angleDelta().y() * 0.0005);
}

void SceneWindow::resizeGL(int width, int height) {
	OpenGLWindow::resizeGL(width, height);

	_camera->setAspectRatio(static_cast<float> (width) / static_cast<float>(height));
	_resizeRenderBuffers = true;
}


SceneWindow::~SceneWindow() {}
