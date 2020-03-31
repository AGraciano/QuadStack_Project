/**
*	The purpose of this class is to be used as base class for a window with a
*	OpenGL support.
*
*	@author Alejandro Graciano
*/

#ifndef OPEN_GL_WINDOW_H
#define OPEN_GL_WINDOW_H

#include <QtGui/QWindow>
#include <QMouseEvent>
#include <memory>
#include "graphics/glcore.h"
#include "graphics/shaderprogram.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

using std::vector;
using std::unique_ptr;
using glm::mat4;

class OpenGLWindow : public QWindow {
	Q_OBJECT

protected:

	unique_ptr<GLCore> _glCore;
	bool _animating;
	bool _initialized;

	/** < Shader object	*/
	unique_ptr<graphics::Shader> _shaderProgram;

	/** < List of path shaders */
	vector<string> _shadersPaths;

	/** < Matrices for visualization */
	mat4 _model, _viewport;

	/** < Constant for matrices calculation */
	const unsigned NO_CALCULATE_MATRICES = 0;

	/** < Constant for matrices calculation */
	const unsigned CALCULATE_MATRICES = 1;

	unsigned WIN_WIDTH;
	unsigned WIN_HEIGHT;

public:
	OpenGLWindow(QWindow *parent = 0);
	~OpenGLWindow();

	/**
	* Abstract method to be reimplemented for rendering
	*/
	virtual void render() = 0;

	virtual void initialize();

	/**
	* Compiles and links the shader list program
	*/
	void compileAndLinkShaders(graphics::Shader &program, const vector<string> &filePaths);

	/**
	* Set up of the view and model matrice
	*/
	virtual void setMatrices(unsigned calculation) = 0;

	void setSceneWidth(unsigned w) { WIN_WIDTH = w; }
	void setSceneHeight(unsigned h) { WIN_HEIGHT = h; }


	void setAnimating(bool animating);

	public slots:
	void renderLater();
	void renderNow();

protected:
	bool event(QEvent *event) override;

	void exposeEvent(QExposeEvent *event);

	virtual void resizeGL(int width, int height);
};

#endif

