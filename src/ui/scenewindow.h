/**
*	Interface that encapsulates a graphic scene. The scene uses as windows toolkit Qt
*
*	@author Alejandro Graciano
*/

#ifndef SCENE_WINDOW_H
#define SCENE_WINDOW_H

#include <QStatusBar>
#include "openglwindow.h"
#include "core/quadstack.h"
#include "graphics/shaderprogram.h"
#include "graphics/orbitalcamera.h"
#include "graphics/renderable.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>


using glm::mat4;
/**
*	Main class for rendering the Graphics objects
*
*	@author Alejandro Graciano
*/

using glm::vec3;
using glm::ivec4;

using std::string;
using std::vector;
using namespace std::chrono;


/**
* This can be removed because the Qt enumeration is used
*/

enum class MouseAction {
	Release, Press, Nothing
};


class SceneWindow : public OpenGLWindow {

	Q_OBJECT
	
protected:

	QStatusBar *_statusBar;

	unsigned _frames;

	high_resolution_clock::time_point _lastTime;

	/** < Scene triangle mesh. Model to be rendered*/
	graphics::Renderable* _model;

	/** < Scene camera*/
	unique_ptr<graphics::OrbitalCamera> _camera;

	/** < Last action performed by the mouse */
	static MouseAction _lastAction;

	/** < Last action performed by the mouse */
	static Qt::MouseButton _buttonPressed;

	/** < Last mouse position coordinates */
	static double _lastXpos, _lastYpos;

	/** < Handle for Framebuffer Object for the raycasting pass*/
	GLuint _fboRaycasting;

	/** < Handle for Framebuffer Object for the normal pass*/
	GLuint _fboNormal;

	/** < Handle for full-screen quad buffer */
	GLuint _fsQuad;

	/** < Index for the raycasting subroutine */
	GLuint _raycastingPass;

	/** < Index for the shading subroutine */
	GLuint _shadingPass;

	/** < Index for the postprocessing subroutine */
	GLuint _postprocessingPass;

	/** < Index for the back-face render subroutine */
	GLuint _backfacePass;

	/**< Occlusion distance to render vector layer*/
	static float _occlusionDistance;

	/**< Flag for indicating the FBO calculation*/
	bool _resizeRenderBuffers;

	const char* const SHADER_PATH = "src/graphics/shaders/";
	const double BYTES_TO_MBYTES = 1048576.0;

	/**< FBO handlers*/
	GLuint _colorNormalTex;
	GLuint _normalTex;
	GLuint _colorTex;
	GLuint _depthTex;


	/**< Memory consumption of the data structures*/
	int _voxelBytes;
	int _octreeBytes;
	int _sbrBytes;
	ivec4 _qsBytes;

	QuadStack* initModel();
	void initVisualization(QuadStack* qs);
	void showStats();

public:
	SceneWindow(QStatusBar *statusBar, QWindow *parent = nullptr);
	~SceneWindow();

	void initialize() override;
	void render() override;

	void raycastingRender();
	void normalCalculationRender();

	void setupFBO();
	void resizeFBO();
	void setMatrices(const unsigned calculation);

protected:
	void mouseMoveEvent(QMouseEvent *ev) override;
	void mousePressEvent(QMouseEvent *ev) override;
	void mouseReleaseEvent(QMouseEvent *ev) override;
	void wheelEvent(QWheelEvent *ev) override;
	void resizeGL(int width, int height) override;


};

#endif

