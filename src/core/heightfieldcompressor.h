/**
*	Class that computes a lossless random access compression algorithm based on
*	block encoding for heightfields
*
*	@class HeightFieldCompressor
*	@author Alejandro Graciano
*/

#ifndef HEIGHT_MAP_COMPRESSOR_H
#define HEIGHT_MAP_COMPRESSOR_H

#include "core/heightfield.h"
#include <memory>
#include <vector>
#include <bitset>

class HeightFieldCompressor {
	HeightField *_HeightField;
	unsigned _blockRow;
	unsigned _blockCol;
	unsigned _wordSize;
	float _offset;
	std::vector<float> _baseValues;
	std::vector<unsigned> _data;
	std::vector<int> _pointers;
	std::vector<int> _bits;
	std::vector<bool> _bitData;
	unsigned _indexBase;
	unsigned _indexPointers;
	unsigned _indexData;
	unsigned _indexBits;
	unsigned _blockSize;

	/**
	* Method that extracts nBits of a buffer
	*/
	int extractBits(int buffer, int firstBit, int nBits);

public:

	HeightFieldCompressor(HeightField *HeightField, unsigned blockCol, unsigned blockRow, float offset) :
		_HeightField(HeightField),
		_blockRow(blockRow <= _HeightField->getDimensionX() ? blockRow : _HeightField->getDimensionX()),
		_blockCol(blockCol <= _HeightField->getDimensionY() ? blockCol : _HeightField->getDimensionY()),
		_offset(offset) {
	};

	/**
	* Actual compression algorithm
	*/
	void compress();

	//@{
	/** Getter and setter methods */
	std::vector<unsigned> getData() { return _data; }
	unsigned getData(int index) { return _data[index]; }
	int getBit(int index) { return _bits[index]; }
	unsigned getPointers(int index) { return _pointers[index]; }
	float getBaseValue(int index) { return _baseValues[index]; }
	void setBaseValue(int index, float value) { _baseValues[index] = value; }
	std::vector<int> getEncodingBits() { return _bits; }
	std::vector<float> getBaseValues() { return _baseValues; }
	std::vector<int> getPointer() { return _pointers; }
	bool compressed() const { return _bits.size() > 0; }
	unsigned blockSize() const { return _baseValues.size(); }
	double memorySize() const;
	//@}

	~HeightFieldCompressor();
};


#endif
