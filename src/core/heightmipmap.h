/**
*	Class that computes min/max mipmap given a heightfield
*
*	@class HeightMipMap
*	@author Alejandro Graciano
*/

#ifndef HEIGHT_MIP_MAP_H
#define HEIGHT_MIP_MAP_H

#include "core/heightfield.h"
#include <glm/glm.hpp>
#include <vector>

using std::vector;

enum class MipmapMode {
	MIN, MAX
};

class HeightMipmap {
	HeightField *_heightField;
	MipmapMode _mode;
	std::vector<HeightField*> _mipmap;
	std::vector<HeightField*> _difference;


public:
	HeightMipmap(HeightField *heightField, MipmapMode mode) : _heightField(heightField), _mode(mode) {};
	void computeMipmap();
	float getData(unsigned int col, unsigned int row, unsigned int mipmap) const { return _mipmap[mipmap]->getData(col, row); }
	HeightField* getHeightField(int index) { return _mipmap[index]; }
	~HeightMipmap();
};

#endif

