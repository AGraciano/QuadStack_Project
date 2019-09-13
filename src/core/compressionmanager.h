#ifndef COMPRESSION_MANAGER_H
#define COMPRESSION_MANAGER_H

#include "core/quadstack.h"
#include <memory>

using std::shared_ptr;

class CompressionManager {
private:
	QuadStack *_quadStack;
	ShortSBR *_sbr;

public:
	CompressionManager(ShortSBR *sbr) : _sbr(sbr), _quadStack(new QuadStack(sbr)) {};

	QuadStack* getQuadStack() { return _quadStack; }
	void execute();

	~CompressionManager() {};
};

#endif