#ifndef QUADSTACK_COMP_VIEW_H
#define QUADSTACK_COMP_VIEW_H

#include "core\quadstack.h"
#include "graphics\renderable.h"

#include <glm\glm.hpp>
#include <vector>

using std::vector;
using glm::vec3;
using glm::ivec4;


class QuadStackView : public graphics::Renderable {

	QuadStack *_quadstack;

	/** < Index for the raycasting subroutine */
	GLuint _raycastingPass;

	/** < Index for the shading subroutine */
	GLuint _shadingPass;

	/** < Index for the postprocessing subroutine */
	GLuint _postprocessingPass;

	/** < Index for the back-face render subroutine */
	GLuint _backfacePass;

	/**< Occlusion distance to render vector layer*/
	float _occlusionDistance;

	unsigned int _viewportHeight;

	unsigned int _viewportWidth;

	bool _newFBO;
	void getVertices(vector<vec3> &points, vector<int> &indices);

	const static unsigned MAX_LEVELS = 15;

	const static int UNKNOWN_INDEX = 999;

	const static unsigned NUMBER_OF_COLORS = 20;

	const static string SHADER_PATH;

	const static unsigned char ColorTable[NUMBER_OF_COLORS][4];

public:
	QuadStackView(GLFunctions *gl, QuadStack *quadstack, unsigned int width, unsigned int height) :
		Renderable(gl),
		_quadstack(quadstack),
		_viewportWidth(width),
		_viewportHeight(height) {

	};

	void render() override;
	ivec4 init();
	void setViewport(unsigned int width, unsigned int height);
	const vec3 getMinBB() { return _minBB; }
	const vec3 getMaxBB() { return _maxBB; }
	GLuint getRaycastingPassHandle() const { return _raycastingPass; }
	GLuint getShadingPassHandle() const { return _shadingPass; }

	~QuadStackView();
};

#endif

