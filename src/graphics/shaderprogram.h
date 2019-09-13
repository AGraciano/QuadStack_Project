#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

#include "glaccessible.h"

#include <string>
using std::string;
#include <map>

#include <glm/glm.hpp>
using glm::vec2;
using glm::ivec2;
using glm::vec3;
using glm::uvec3;
using glm::vec4;
using glm::mat4;
using glm::mat3;

#include <stdexcept>

/**
Class that encapsulate a GLSL program.

@author Alejandro Graciano. Based on David Wolff code
*/

namespace graphics {

	class ShaderException : public std::runtime_error {
	public:
		ShaderException(const string & msg) :
			std::runtime_error(msg) {}
	};

	namespace GLSLShader {
		enum GLSLShaderType {
			VERTEX = GL_VERTEX_SHADER,
			FRAGMENT = GL_FRAGMENT_SHADER,
			GEOMETRY = GL_GEOMETRY_SHADER,
			TESS_CONTROL = GL_TESS_CONTROL_SHADER,
			TESS_EVALUATION = GL_TESS_EVALUATION_SHADER,
			COMPUTE = GL_COMPUTE_SHADER
		};
	};

	class Shader : public GLAccessible {

	private:
		int  handle;
		bool linked;
		std::map<string, int> uniformLocations;

		GLint  getUniformLocation(const char * name);
		bool fileExists(const string & fileName);
		string getExtension(const char * fileName);

		// Make these private in order to make the object non-copyable
		Shader(const Shader & other) {}
		Shader & operator=(const Shader &other) { return *this; }

	public:
		Shader();
		Shader::Shader(GLFunctions* gl);
		~Shader();

		void   compileShader(const char *fileName) throw (ShaderException);
		void   compileShader(const char * fileName, GLSLShader::GLSLShaderType type) throw (ShaderException);
		void   compileShader(const string & source, GLSLShader::GLSLShaderType type,
			const char *fileName = NULL) throw (ShaderException);

		void   link() throw (ShaderException);
		void   validate() throw(ShaderException);
		void   use() throw (ShaderException);

		int    getHandle();
		bool   isLinked();

		void   bindAttribLocation(GLuint location, const char * name);
		void   bindFragDataLocation(GLuint location, const char * name);
		void   bindTexture(GLuint unit, GLuint target, GLuint location);

		void   setUniform(const char *name, float x, float y, float z);
		void   setUniform(const char *name, const vec2 & v);
		void   setUniform(const char *name, const ivec2 & v);
		void   setUniform(const char *name, const vec3 & v);
		void   setUniform(const char *name, const uvec3 & v);
		void   setUniform(const char *name, const vec4 & v);
		void   setUniform(const char *name, const mat4 & m);
		void   setUniform(const char *name, const mat3 & m);
		void   setUniform(const char *name, float val);
		void   setUniform(const char *name, double val);
		void   setUniform(const char *name, int val);
		void   setUniform(const char *name, bool val);
		void   setUniform(const char *name, GLuint val);

		void   printActiveUniforms();
		void   printActiveUniformBlocks();
		void   printActiveAttribs();

		const char * getTypeString(GLenum type);
	};

	
}
#endif

