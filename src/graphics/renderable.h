#ifndef RENDERABLE_H
#define RENDERABLE_H

#include "glaccessible.h"
#include "shaderprogram.h"

#include <glm\glm.hpp>
#include <memory>
#include <vector>

using std::vector;
using glm::mat4;
using std::unique_ptr;

namespace graphics {
	class Renderable : public GLAccessible {

	protected:
		GLuint _vboHandle;
		GLuint _vaoHandle;
		GLuint _faces;
		mat4 _model;
		unique_ptr<graphics::Shader> _shaderProgram;
		vec3 _minBB;
		vec3 _maxBB;

	public:
		Renderable(GLFunctions* gl) :
			GLAccessible(gl),
			_vaoHandle(0),
			_vboHandle(0),
			_faces(0),
			_model(1.0),
			_shaderProgram(std::make_unique<graphics::Shader>(gl)) {
		};

		virtual void render() = 0;
		mat4 getModel() const { return _model; }
		void setModel(mat4 model) { _model = model; }
		void compileAndLinkShaders(const vector<string> &filePaths);
		GLuint getShaderHandle() const { return _shaderProgram->getHandle(); }
		void useProgram() const { _shaderProgram->use(); }
		graphics::Shader* getShader() { return _shaderProgram.get(); }
		const vec3 getMinBB() { return _minBB; }
		const vec3 getMaxBB() { return _maxBB; }

		virtual ~Renderable() {};
	};
}


#endif

