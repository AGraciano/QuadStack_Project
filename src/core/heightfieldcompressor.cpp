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

				//std::cout << "MaxDiff: " << maxDiff << ", offset: " << _offset << ", bits: " << bits << std::endl;
				//float pack = 0;
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

						//if (i == aux.size() - 1) {
						//	leftBits = 32 - accum;
						//	_data[_data.size() - 1] <<= leftBits;
						//}

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

	//getchar();
	//Append header
	//vector<glm::ivec4> buffer;

	//for (int i = 0; i < _baseValues.size(); ++i) {
	//	int bits = _bits.size() > 0 ? _bits[i] : 0;
	//	int pointers = _pointers.size() > 0 ? _pointers[i] : 0;
	//	buffer.push_back(glm::ivec4(_baseValues[i], bits, pointers, 0));
	//}
	//for (int i = 0; i < _data.size(); ++i) {
	//	if (i % 4 == 0)
	//		buffer.push_back(glm::ivec4(0, 0, 0, 0));
	//	
	//	buffer[buffer.size() - 1][i % 4] = _data[i];

	//}

	//// pointers, bits, base, data
	//for (int row = 0; row < rows; ++row) {
	//	for (int col = 0; col< cols; ++col) {

	//		unsigned blockRows = rows / _blockRow;
	//		unsigned blockCols = cols / _blockCol;

	//		MortonCurve mcBase(blockRows, blockCols);
	//		int index = mc.computeMortonCode(col, row);
	//		int indexBlock = mcBase.computeMortonCode(col / _blockCol, row / _blockRow);
	//		int indexData = blockRows * blockCols;

	//		float decompress;

	//		if (rows <= 1 || cols <= 1) {
	//			decompress = buffer[indexBlock].x;
	//		} else {

	//			glm::ivec4 header = buffer[indexBlock];
	//			int base = header.x;
	//			int bits = header.y;
	//			int pointer = header.z;

	//			unsigned unpacked = 0;
	//			if (bits > 0) {


	//				//int accumBits = 0;
	//				//for (int i = 0; i < indexBase; ++i)
	//				//	accum += _bits[i];
	//				unsigned valueInBlock = index % (_blockCol * _blockRow);
	//				int startBit = pointer + valueInBlock * bits;
	//				int endBit = startBit + bits;


	//				if (startBit % 32 > endBit % 32 && endBit % 32 > 0) { // splitted value
	//					int element = startBit / 32;
	//					int pack = buffer[indexData + element / 4][element % 4];
	//					unsigned nBits = 32 - (startBit % 32);
	//					unpacked = extractBits(pack, 0, nBits);
	//					unpacked <<= (endBit % 32);
	//					element = endBit / 32;
	//					pack = buffer[indexData + element / 4][element % 4];
	//					nBits = bits - nBits;
	//					int unpacked1 = extractBits(pack, 32 - nBits, nBits);
	//					unpacked |= unpacked1;
	//				} else {
	//					int element = startBit / 32;
	//					unsigned pack = buffer[indexData + element / 4][element % 4];
	//					//unpacked = extractBits(pack, 31 - startBit % 32, bits);
	//					unpacked = (((1 << bits) - 1) & (pack >> (32 - (endBit % 32))));
	//				}

	//				//decompress = unpacked + base / _offset;
	//			}

	//			//decompress = unpacked + base / _offset;
	//			decompress = unpacked * _offset + base;
	//		}

	//		std::cout << decompress << " ";
	//	}
	//	std::cout << std::endl;
	//}
	//getchar();
}

void HeightFieldCompressor::compressPow2() {
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
	_data.push_back(0);
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

				//std::cout << "MaxDiff: " << maxDiff << ", offset: " << _offset << ", bits: " << bits << std::endl;
				//float pack = 0;
				if (bits > 0) {
					for (int i = 0; i < aux.size(); ++i) {
						unsigned scale = aux[i] / _offset;
						unsigned leftBits = 32 - accum;

						int discardedBits = bits;

						if (leftBits < bits) {
							accum = 0;
							leftBits = 32;
							_data.push_back(0);
							discardedBits = leftBits;
						}

						unsigned shift = leftBits < bits ? 0 : (leftBits - bits);
						unsigned shifted = scale << shift;

						accum = accum + bits;
						bitPointer += discardedBits;
						_data[_data.size() - 1] |= shifted;

					}
				}
				aux.clear();
				currentBlock.clear();
				baseBlock = std::numeric_limits<float>::max();

			}
		}

	}

	//std::cout << cols << " x " << rows << std::endl;
	//getchar();
	// Append header
	//vector<glm::ivec4> buffer;

	//for (int i = 0; i < _baseValues.size(); ++i) {
	//	int bits = _bits.size() > 0 ? _bits[i] : 0;
	//	int pointers = _pointers.size() > 0 ? _pointers[i] : 0;
	//	buffer.push_back(glm::ivec4(_baseValues[i], bits, pointers, 0));
	//}
	//for (int i = 0; i < _data.size(); ++i) {
	//	if (i % 4 == 0)
	//		buffer.push_back(glm::ivec4(0, 0, 0, 0));

	//	buffer[buffer.size() - 1][i % 4] = _data[i];

	//}

	//// pointers, bits, base, data
	//for (int row = 0; row < rows; ++row) {
	//	for (int col = 0; col < cols; ++col) {

	//		//if (row == 2 && col == 7)
	//		//	std::cout << "stop" << std::endl;
	//		unsigned blockRows = rows / _blockRow;
	//		unsigned blockCols = cols / _blockCol;

	//		MortonCurve mcBase(blockRows, blockCols);
	//		int index = mc.computeMortonCode(col, row);
	//		int indexBlock = mcBase.computeMortonCode(col / _blockCol, row / _blockRow);
	//		int indexData = blockRows * blockCols;

	//		float decompress;

	//		if (rows <= 1 || cols <= 1) {
	//			decompress = buffer[index].x;
	//		} else {

	//			glm::ivec4 header = buffer[indexBlock];
	//			int base = header.x;
	//			int bits = header.y;
	//			int pointer = header.z;

	//			unsigned unpacked = 0;
	//			if (bits > 0) {


	//				//int accumBits = 0;
	//				//for (int i = 0; i < indexBase; ++i)
	//				//	accum += _bits[i];
	//				unsigned elementsInWord = 32 / bits;
	//				unsigned valueInBlock = index % (_blockCol * _blockRow);
	//				unsigned wordBehind = valueInBlock / elementsInWord;
	//				unsigned word = valueInBlock % elementsInWord;
	//				int startBit = pointer + wordBehind * 32 + word * bits;
	//				int endBit = startBit + bits;


	//				int element = startBit / 32;
	//				unsigned pack = buffer[indexData + element / 4][element % 4];
	//				unpacked = (((1 << bits) - 1) & (pack >> (32 - (endBit % 32))));

	//				//decompress = unpacked + base / _offset;
	//			}

	//			decompress = unpacked * _offset + base;
	//		}

	//		std::cout << decompress << " ";
	//	}
	//	std::cout << std::endl;
	//}
	//getchar();
}

int HeightFieldCompressor::extractBits(int value, int firstBit, int nBits) {
	return (((1 << nBits) - 1) & (value >> firstBit));
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
