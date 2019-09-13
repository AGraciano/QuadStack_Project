#include "MortonCurve.h"
#include <algorithm>
#include <iostream>

using namespace std;


MortonCurve::MortonCurve(unsigned int rows, unsigned int cols) :
_rows(rows),
_cols(cols),
_root(new Division(ivec2(0, 0), ivec2(cols, rows))) {

	_root->subdivide();
}



unsigned int MortonCurve::nextPowerOf2(unsigned int number) {

	unsigned int returnNumber = number - 1;
	returnNumber |= returnNumber >> 1;
	returnNumber |= returnNumber >> 2;
	returnNumber |= returnNumber >> 4;
	returnNumber |= returnNumber >> 8;
	returnNumber |= returnNumber >> 16;

	return ++returnNumber;
}

unsigned int MortonCurve::lastPowerOf2(unsigned int number) {

	unsigned int returnNumber = number;
	returnNumber = returnNumber | (returnNumber >> 1);
	returnNumber = returnNumber | (returnNumber >> 2);
	returnNumber = returnNumber | (returnNumber >> 4);
	returnNumber = returnNumber | (returnNumber >> 8);
	returnNumber = returnNumber | (returnNumber >> 16);

	return returnNumber - (returnNumber >> 1);
}

unsigned int MortonCurve::computeMortonCode(unsigned int col, unsigned int row) {
	return spreadBits(col) | (spreadBits(row) << 1);
	//return _root->computeMortonCode(col, row);
}

ivec2 MortonCurve::decomputeMortonCode(unsigned int index) {
	return ivec2(unSpreadBits(index), unSpreadBits(index >> 1));
	//return _root->decomputeMortonCode(index);
}

unsigned int MortonCurve::spreadBits(unsigned int x) {
	unsigned int r = x;
	r = (r | (r << 8)) & 0x00FF00FF;
	r = (r | (r << 4)) & 0x0F0F0F0F;
	r = (r | (r << 2)) & 0x33333333;
	r = (r | (r << 1)) & 0x55555555;

	return r;
}

unsigned int MortonCurve::unSpreadBits(unsigned int x) {
	unsigned int r = x & 0x55555555;
	r = (r ^ (r >> 1)) & 0x33333333;
	r = (r ^ (r >> 2)) & 0x0F0F0F0F;
	r = (r ^ (r >> 4)) & 0x00FF00FF;
	r = (r ^ (r >> 8)) & 0x0000FFFF;

	return r;
}

bool MortonCurve::isPowerOfTwo(unsigned int x) {
	while (((x % 2) == 0) && x > 1) /* While x is even and > 1 */
		x /= 2;
	return (x == 1);
}

MortonCurve::~MortonCurve() {
	delete _root;
}


/**
Division node operations
*/

MortonCurve::Division::Division(ivec2 init, ivec2 dimension) :
_init(init),
_dimension(dimension),
_child0(nullptr),
_child1(nullptr) {
}

void MortonCurve::Division::subdivide() {
	if (!isLeaf()) {
		unsigned int powerOf2Block = min(lastPowerOf2(_dimension.x), lastPowerOf2(_dimension.y));
		unsigned int xDifference = _dimension.x - powerOf2Block;
		unsigned int yDifference = _dimension.y - powerOf2Block;

		if (isPowerOfTwo(_dimension.y) && _dimension.x >= _dimension.y) { // left-right
			_child0 = new Division(_init, ivec2(_dimension.y, _dimension.y));
			_child1 = new Division(ivec2(_init.x + _dimension.y, _init.y), ivec2(_dimension.x - _dimension.y, _dimension.y));
		} else if (_dimension.y > 1) { // up-down
			_child0 = new Division(_init, ivec2(_dimension.x, powerOf2Block));
			_child1 = new Division(ivec2(_init.x, _init.y + powerOf2Block), ivec2(_dimension.x, _dimension.y - powerOf2Block));
		} else { // left-right
			_child0 = new Division(_init, ivec2(1, 1));
			_child1 = new Division(ivec2(_init.x + 1, _init.y), ivec2(_dimension.x - 1, _dimension.y));
		}

		//cout << "child0 ->(" << _child0->_init.x << ", " << _child0->_init.y << ")" << "->(" << _child0->_dimension.x << ", " << _child0->_dimension.y << ")" << endl;
		//cout << "child1 ->(" << _child1->_init.x << ", " << _child1->_init.y << ")" << "->(" << _child1->_dimension.x << ", " << _child1->_dimension.y << ")" << endl;
		_child0->subdivide();
		_child1->subdivide();
	}


}

unsigned int MortonCurve::Division::computeMortonCode(unsigned int col, unsigned int row) {
	Division* _child;
	unsigned int xIndex = col - _init.x;
	unsigned int yIndex = row - _init.y;
	unsigned int acum = 0;

	if (isLeaf()) {
		return spreadBits(xIndex) | (spreadBits(yIndex) << 1);
	}

	if (_child0->contains(col, row)) {
		_child = _child0;
	} else if (_child1 != nullptr && _child1->contains(col, row)) {
		acum += _child0->_dimension.x * _child0->_dimension.y;
		_child = _child1;
	}

	////cout << "is in a child. Adding " << acum << " to a local index" << endl;
	return acum + _child->computeMortonCode(col, row);
}

ivec2 MortonCurve::Division::decomputeMortonCode(unsigned int index) {
	Division* _child;

	unsigned int acum = 0;

	if (isLeaf()) {
		return ivec2(unSpreadBits(index), unSpreadBits(index >> 1));
	}
	//
	//	if (_child0->contains(col, row)){
	//		_child = _child0;
	//	}
	//	else if (_child1 != nullptr && _child1->contains(col, row)) {
	//		acum += _child0->_dimension.x * _child0->_dimension.y;
	//		_child = _child1;
	//	}
	//
	//	////cout << "is in a child. Adding " << acum << " to a local index" << endl;
	//	return acum + _child->computeMortonCode(col, row);
}

bool MortonCurve::Division::isPowerOfTwoSquare() {

	return _dimension.x == _dimension.y && isPowerOfTwo(_dimension.x) && isPowerOfTwo(_dimension.y);
}

MortonCurve::Division::~Division() {
	delete _child0;
	delete _child1;
}
