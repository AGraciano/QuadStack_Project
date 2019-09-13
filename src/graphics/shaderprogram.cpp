#include "shaderprogram.h"

#include <fstream>
using std::ifstream;
using std::ios;


#include <sstream>
#include <sys/stat.h>

#include <iostream>
using namespace std;


namespace graphics {

	namespace GLSLShaderInfo {
		struct shader_file_extension {
			const char *ext;
			GLSLShader::GLSLShaderType type;
		};

		struct shader_file_extension extensions[] =
		{
			{ ".vs", GLSLShader::VERTEX },
			{ ".vert", GLSLShader::VERTEX },
			{ ".gs", GLSLShader::GEOMETRY },
			{ ".geom", GLSLShader::GEOMETRY },
			{ ".tcs", GLSLShader::TESS_CONTROL },
			{ ".tes", GLSLShader::TESS_EVALUATION },
			{ ".fs", GLSLShader::FRAGMENT },
			{ ".frag", GLSLShader::FRAGMENT },
			{ ".cs", GLSLShader::COMPUTE }
		};
	}

	Shader::Shader(GLFunctions* gl) : GLAccessible(gl), handle(0), linked(false) {}

	Shader::~Shader() {
		if (handle == 0) return;

		// Query the number of attached shaders
		GLint numShaders = 0;
		_gl->glGetProgramiv(handle, GL_ATTACHED_SHADERS, &numShaders);

		// Get the shader names
		GLuint * shaderNames = new GLuint[numShaders];
		_gl->glGetAttachedShaders(handle, numShaders, NULL, shaderNames);

		// Delete the shaders
		for (int i = 0; i < numShaders; i++)
			_gl->glDeleteShader(shaderNames[i]);

		// Delete the program
		_gl->glDeleteProgram(handle);

		delete[] shaderNames;
	}

	void Shader::compileShader(const char * fileName)
		throw(ShaderException) {
		int numExts = sizeof(GLSLShaderInfo::extensions) / sizeof(GLSLShaderInfo::shader_file_extension);

		// Check the file name's extension to determine the shader type
		string ext = getExtension(fileName);
		GLSLShader::GLSLShaderType type = GLSLShader::VERTEX;
		bool matchFound = false;
		for (int i = 0; i < numExts; i++) {
			if (ext == GLSLShaderInfo::extensions[i].ext) {
				matchFound = true;
				type = GLSLShaderInfo::extensions[i].type;
				break;
			}
		}

		// If we didn't find a match, throw an exception
		if (!matchFound) {
			string msg = "Unrecognized extension: " + ext;
			throw ShaderException(msg);
		}

		// Pass the discovered shader type along
		compileShader(fileName, type);


	}

	string Shader::getExtension(const char * name) {
		string nameStr(name);

		size_t loc = nameStr.find_last_of('.');
		if (loc != string::npos) {
			return nameStr.substr(loc, string::npos);
		}
		return "";
	}

	void Shader::compileShader(const char * fileName,
		GLSLShader::GLSLShaderType type)
		throw(ShaderException) {

		if (!fileExists(fileName)) {
			string message = string("Shader: ") + fileName + " not found.";
			throw ShaderException(message);
		}

		if (handle <= 0) {
			handle = _gl->glCreateProgram();
			if (handle == 0) {
				throw ShaderException("Unable to create shader program.");
			}
		}

		ifstream inFile(fileName, ios::in);
		if (!inFile) {
			string message = string("Unable to open: ") + fileName;
			throw ShaderException(message);
		}

		// Get file contents
		std::stringstream code;

		//code << "#version 430 core" << "\n" << "#define MAX_NUM 20" << inFile.rdbuf();
		code << inFile.rdbuf();
		inFile.close();

		compileShader(code.str(), type, fileName);

	}

	void Shader::compileShader(const string & source,
		GLSLShader::GLSLShaderType type,
		const char * fileName)
		throw(ShaderException) {


		if (handle <= 0) {
			handle = _gl->glCreateProgram();
			if (handle == 0) {
				throw ShaderException("Unable to create shader program.");
			}
		}

		GLuint shaderHandle = _gl->glCreateShader(type);
		const char * c_code = source.c_str();
		_gl->glShaderSource(shaderHandle, 1, &c_code, NULL);

		// Compile the shader
		_gl->glCompileShader(shaderHandle);

		// Check for errors
		int result;
		_gl->glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &result);

		if (GL_FALSE == result) {
			// Compile failed, get log
			int length = 0;
			string logString;
			_gl->glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &length);
			if (length > 0) {
				char * c_log = new char[length];
				int written = 0;
				_gl->glGetShaderInfoLog(shaderHandle, length, &written, c_log);
				logString = c_log;
				delete[] c_log;
			}
			string msg;
			if (fileName) {
				msg = string(fileName) + ": shader compliation failed\n";
			} else {
				msg = "Shader compilation failed.\n";
			}
			msg += logString;

			throw ShaderException(msg);

		} else {
			// Compile succeeded, attach shader
			_gl->glAttachShader(handle, shaderHandle);
		}

	}

	void Shader::link() throw(ShaderException) {
		if (linked) return;
		if (handle <= 0)
			throw ShaderException("Program has not been compiled.");

		_gl->glLinkProgram(handle);

		int status = 0;
		_gl->glGetProgramiv(handle, GL_LINK_STATUS, &status);
		if (GL_FALSE == status) {
			// Store log and return false
			int length = 0;
			string logString;

			_gl->glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length);

			if (length > 0) {
				char * c_log = new char[length];
				int written = 0;
				_gl->glGetProgramInfoLog(handle, length, &written, c_log);
				logString = c_log;
				delete[] c_log;
			}

			throw ShaderException(string("Program link failed:\n") + logString);
		} else {
			uniformLocations.clear();
			linked = true;
		}
	}

	void Shader::use() throw(ShaderException) {
		if (handle <= 0 || (!linked))
			throw ShaderException("Shader has not been linked");
		_gl->glUseProgram(handle);
	}

	int Shader::getHandle() {
		return handle;
	}

	bool Shader::isLinked() {
		return linked;
	}

	void Shader::bindAttribLocation(GLuint location, const char * name) {
		_gl->glBindAttribLocation(handle, location, name);
	}

	void Shader::bindFragDataLocation(GLuint location, const char * name) {
		_gl->glBindFragDataLocation(handle, location, name);
	}

	void Shader::bindTexture(GLuint unit, GLuint target, GLuint location) {
		_gl->glActiveTexture(unit);
		_gl->glBindTexture(target, location);
	}

	void Shader::setUniform(const char *name, float x, float y, float z) {
		GLint loc = getUniformLocation(name);
		_gl->glUniform3f(loc, x, y, z);
	}

	void Shader::setUniform(const char *name, const vec3 & v) {
		this->setUniform(name, v.x, v.y, v.z);
	}

	void Shader::setUniform(const char *name, const uvec3 & v) {
		GLint loc = getUniformLocation(name);
		_gl->glUniform3ui(loc, v.x, v.y, v.z);
	}

	void Shader::setUniform(const char *name, const vec4 & v) {
		GLint loc = getUniformLocation(name);
		_gl->glUniform4f(loc, v.x, v.y, v.z, v.w);
	}

	void Shader::setUniform(const char *name, const ivec2 & v) {
		GLint loc = getUniformLocation(name);
		_gl->glUniform2i(loc, v.x, v.y);
	}

	void Shader::setUniform(const char *name, const vec2 & v) {
		GLint loc = getUniformLocation(name);
		_gl->glUniform2f(loc, v.x, v.y);
	}

	void Shader::setUniform(const char *name, const mat4 & m) {
		GLint loc = getUniformLocation(name);
		_gl->glUniformMatrix4fv(loc, 1, GL_FALSE, &m[0][0]);
	}

	void Shader::setUniform(const char *name, const mat3 & m) {
		GLint loc = getUniformLocation(name);
		_gl->glUniformMatrix3fv(loc, 1, GL_FALSE, &m[0][0]);
	}

	void Shader::setUniform(const char *name, float val) {
		GLint loc = getUniformLocation(name);
		_gl->glUniform1f(loc, val);
	}

	void Shader::setUniform(const char *name, double val) {
		GLint loc = getUniformLocation(name);
		_gl->glUniform1d(loc, val);
	}

	void Shader::setUniform(const char *name, int val) {
		GLint loc = getUniformLocation(name);
		_gl->glUniform1i(loc, val);
	}

	void Shader::setUniform(const char *name, GLuint val) {
		GLint loc = getUniformLocation(name);
		_gl->glUniform1ui(loc, val);
	}

	void Shader::setUniform(const char *name, bool val) {
		int loc = getUniformLocation(name);
		_gl->glUniform1i(loc, val);
	}

	void Shader::printActiveUniforms() {
		GLint numUniforms = 0;
		_gl->glGetProgramInterfaceiv(handle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numUniforms);

		GLenum properties[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX };

		printf("Active uniforms:\n");
		for (int i = 0; i < numUniforms; ++i) {
			GLint results[4];
			_gl->glGetProgramResourceiv(handle, GL_UNIFORM, i, 4, properties, 4, NULL, results);

			if (results[3] != -1) continue;  // Skip uniforms in blocks 
			GLint nameBufSize = results[0] + 1;
			char * name = new char[nameBufSize];
			_gl->glGetProgramResourceName(handle, GL_UNIFORM, i, nameBufSize, NULL, name);
			printf("%-5d %s (%s)\n", results[2], name, getTypeString(results[1]));
			delete[] name;
		}
	}

	void Shader::printActiveUniformBlocks() {
		GLint numBlocks = 0;

		_gl->glGetProgramInterfaceiv(handle, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &numBlocks);
		GLenum blockProps[] = { GL_NUM_ACTIVE_VARIABLES, GL_NAME_LENGTH };
		GLenum blockIndex[] = { GL_ACTIVE_VARIABLES };
		GLenum props[] = { GL_NAME_LENGTH, GL_TYPE, GL_BLOCK_INDEX };

		for (int block = 0; block < numBlocks; ++block) {
			GLint blockInfo[2];
			_gl->glGetProgramResourceiv(handle, GL_UNIFORM_BLOCK, block, 2, blockProps, 2, NULL, blockInfo);
			GLint numUnis = blockInfo[0];

			char * blockName = new char[blockInfo[1] + 1];
			_gl->glGetProgramResourceName(handle, GL_UNIFORM_BLOCK, block, blockInfo[1] + 1, NULL, blockName);
			printf("Uniform block \"%s\":\n", blockName);
			delete[] blockName;

			GLint * unifIndexes = new GLint[numUnis];
			_gl->glGetProgramResourceiv(handle, GL_UNIFORM_BLOCK, block, 1, blockIndex, numUnis, NULL, unifIndexes);

			for (int unif = 0; unif < numUnis; ++unif) {
				GLint uniIndex = unifIndexes[unif];
				GLint results[3];
				_gl->glGetProgramResourceiv(handle, GL_UNIFORM, uniIndex, 3, props, 3, NULL, results);

				GLint nameBufSize = results[0] + 1;
				char * name = new char[nameBufSize];
				_gl->glGetProgramResourceName(handle, GL_UNIFORM, uniIndex, nameBufSize, NULL, name);
				printf("    %s (%s)\n", name, getTypeString(results[1]));
				delete[] name;
			}

			delete[] unifIndexes;
		}
	}

	void Shader::printActiveAttribs() {
		GLint numAttribs;

		_gl->glGetProgramInterfaceiv(handle, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &numAttribs);

		GLenum properties[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION };

		printf("Active attributes:\n");
		for (int i = 0; i < numAttribs; ++i) {
			GLint results[3];
			_gl->glGetProgramResourceiv(handle, GL_PROGRAM_INPUT, i, 3, properties, 3, NULL, results);

			GLint nameBufSize = results[0] + 1;
			char * name = new char[nameBufSize];
			_gl->glGetProgramResourceName(handle, GL_PROGRAM_INPUT, i, nameBufSize, NULL, name);
			printf("%-5d %s (%s)\n", results[2], name, getTypeString(results[1]));
			delete[] name;
		}
	}

	const char * Shader::getTypeString(GLenum type) {
		// There are many more types than are covered here, but
		// these are the most common in these examples.
		switch (type) {
		case GL_FLOAT:
			return "float";
		case GL_FLOAT_VEC2:
			return "vec2";
		case GL_FLOAT_VEC3:
			return "vec3";
		case GL_FLOAT_VEC4:
			return "vec4";
		case GL_DOUBLE:
			return "double";
		case GL_INT:
			return "int";
		case GL_UNSIGNED_INT:
			return "unsigned int";
		case GL_BOOL:
			return "bool";
		case GL_FLOAT_MAT2:
			return "mat2";
		case GL_FLOAT_MAT3:
			return "mat3";
		case GL_FLOAT_MAT4:
			return "mat4";
		default:
			return "?";
		}
	}

	void Shader::validate() throw(ShaderException) {
		if (!isLinked())
			throw ShaderException("Program is not linked");

		GLint status;
		_gl->glValidateProgram(handle);
		_gl->glGetProgramiv(handle, GL_VALIDATE_STATUS, &status);

		if (GL_FALSE == status) {
			// Store log and return false
			int length = 0;
			string logString;

			_gl->glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length);

			if (length > 0) {
				char * c_log = new char[length];
				int written = 0;
				_gl->glGetProgramInfoLog(handle, length, &written, c_log);
				logString = c_log;
				delete[] c_log;
			}

			throw ShaderException(string("Program failed to validate\n") + logString);

		}
	}

	int Shader::getUniformLocation(const char * name) {
		std::map<string, int>::iterator pos;
		pos = uniformLocations.find(name);

		if (pos == uniformLocations.end()) {
			uniformLocations[name] = _gl->glGetUniformLocation(handle, name);
		}

		return uniformLocations[name];
	}

	bool Shader::fileExists(const string & fileName) {
		struct stat info;
		int ret = -1;

		ret = stat(fileName.c_str(), &info);
		return 0 == ret;
	}

}
