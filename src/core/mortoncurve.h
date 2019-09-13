#ifndef MORTON_CURVE_H
#define MORTON_CURVE_H

#include <vector>
#include <glm\vec2.hpp>

using glm::ivec2;

/**
Class that encapsulate a Morton space-filling curve.
Translated (col,row) into 1d index. It only works for 16 bit col and 15 bit row!

@author Alejandro Graciano
*/

class MortonCurve {

	class Division {
		ivec2 _init, _dimension;
		Division *_child0, *_child1;

	public:

		Division(ivec2 init, ivec2 dimension);

		void subdivide();

		bool isPowerOfTwoSquare();

		bool isLeaf() { return (_dimension.x == 1 && _dimension.y == 1) || isPowerOfTwoSquare(); }

		unsigned int computeMortonCode(unsigned int col, unsigned int row);

		ivec2 decomputeMortonCode(unsigned int index);

		bool contains(unsigned int col, unsigned int row) {
			return col >= _init.x && col < _init.x + _dimension.x &&
				row >= _init.y && row < _init.y + _dimension.y;
		}

		~Division();


	};

	static unsigned int spreadBits(unsigned int x);

	static unsigned int unSpreadBits(unsigned int x);

	static bool isPowerOfTwo(unsigned int x);

	unsigned int _rows;
	unsigned int _cols;
	Division *_root;

public:
	MortonCurve(unsigned int rows, unsigned int cols);

	unsigned int computeMortonCode(unsigned int col, unsigned int row);

	ivec2 decomputeMortonCode(unsigned int index);

	static unsigned int nextPowerOf2(unsigned int number);

	static unsigned int lastPowerOf2(unsigned int number);

	~MortonCurve();
};

#endif
