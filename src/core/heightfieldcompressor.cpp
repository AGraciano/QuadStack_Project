#include "heightfieldcompressor.h"
#include "core/mortoncurve.h"

#include <iostream>
#include <glm/vec2.hpp>

using glm::vec2;


void HeightFieldCompressor::compress() {
	unsigned rows = _HeightField->getDimensionX();
	unsigned cols = _HeightField->getDimensionY();
	unsigned blockSize = _blockRow * _blockCol;

	MortonCurve mc(rows, cols);
	unsigned size = rows * cols;

	vector<float> currentBlock;
	float baseBlock = std::numeric_limits<float>::max();

	vector<float> aux;

	unsigned accum = 0;
	unsigned bitPointer = 0;
	for (int index = 0; index < size; ++index) {

		vec2 mortonIndex = mc.decomputeMortonCode(index);
		float value = _HeightField->getData(mortonIndex.x, mortonIndex.y);
		baseBlock = value < baseBlock ? value : baseBlock;
		
		if (rows <= 1 || cols <= 1)
			_baseValues.push_back(value);
		else {
			currentBlock.push_back(value);

			if ((index + 1) % blockSize == 0) { // End of block

				_pointers.push_back(bitPointer);
				_baseValues.push_back(baseBlock);
				float maxDiff = std::numeric_limits<float>::min();
				for (auto height : currentBlock) {
					float diff = height - baseBlock;

					maxDiff = diff > maxDiff ? diff : maxDiff;
					aux.push_back(diff);
				}

				int bits = _offset == 0 ? 0 : ceil(std::log2(maxDiff / _offset + 1));
				_bits.push_back(bits);
				int valuesInPack = bits == 0 ? 0 : 32 / bits;

				if (bits > 0) {
					for (int i = 0; i < aux.size(); ++i) {
						unsigned scale = aux[i] / _offset;
						unsigned leftBits = 32 - accum;
						int currentBits = bits;

						if (leftBits < bits && leftBits > 0) { // split shifted
							int splitBits = leftBits;
							currentBits = bits - splitBits;
							unsigned splitted = extractBits(scale, currentBits, splitBits);
							scale = extractBits(scale, 0, currentBits);
							_data[_data.size() - 1] <<= leftBits;
							_data[_data.size() - 1] |= splitted;

							accum = (accum + splitBits) % 32;
						}

						if (accum == 0) {
							_data.push_back(0);
						}

						accum = (accum + currentBits) % 32;
						bitPointer += bits;
						_data[_data.size() - 1] <<= currentBits;
						_data[_data.size() - 1] |= scale;

					}
				}
				aux.clear();
				currentBlock.clear();
				baseBlock = std::numeric_limits<float>::max();

			}
		}

	}

	if (!_data.empty())
		_data[_data.size() - 1] <<= (32 - (accum % 32));

}

int HeightFieldCompressor::extractBits(int buffer, int firstBit, int nBits) {
	return (((1 << nBits) - 1) & (buffer >> firstBit));
}

double HeightFieldCompressor::memorySize() const {
	double memory = 0;
	memory += _data.size() * sizeof(int);
	memory += _bits.size() * 5 / 8;
	memory += _pointers.size() * sizeof(int);
	memory += _baseValues.size() * sizeof(short);

	return memory;
}



HeightFieldCompressor::~HeightFieldCompressor() {
}
