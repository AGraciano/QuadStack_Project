#include "renderable.h"

using graphics::Renderable;

void Renderable::compileAndLinkShaders(const vector<string> &filePaths) {
	try {
		for (auto path : filePaths)
			_shaderProgram->compileShader(path.c_str());

		_shaderProgram->link();
		_shaderProgram->use();
	} catch (graphics::ShaderException & e) {
		std::cerr << e.what() << std::endl;
		getchar();
		exit(EXIT_FAILURE);
	}

}