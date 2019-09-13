#include "compressionmanager.h"


void CompressionManager::execute() {
	_quadStack->topDownPhase();
	_quadStack->bottomUpPhase();
	_quadStack->rearrangeHeightField();
	_quadStack->compressHeightField();
}