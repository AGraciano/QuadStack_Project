#include "heightfield.h"
//#include "HeightFieldcompressor.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <set>
#include <fstream>
#include <chrono>


HeightField::HeightField()
	: _data(new float[0]) {
}

HeightField::HeightField(vec2 origin, vec2 spacing, ivec2 dimension, float minHeight, float maxHeight,
	float nullData, float *data, HeightFieldCompressor *compressor)
	: _origin(origin),
	_spacing(spacing),
	_dimension(dimension),
	_minHeight(minHeight),
	_maxHeight(maxHeight),
	_data(new float[dimension.x * dimension.y]),
	_nullData(nullData),
	_resolution(nullData),
	_compressor(compressor) {

	if (data)
		std::copy(data, data + (dimension.x * dimension.y), _data);


}

HeightField::HeightField(const HeightField& other) :
	_origin(other._origin),
	_spacing(other._spacing),
	_dimension(other._dimension),
	_minHeight(other._minHeight),
	_maxHeight(other._maxHeight),
	_data(new float[other._dimension.x * other._dimension.y]),
	_nullData(other._nullData),
	_resolution(other._resolution) {

	if (other._data)
		std::copy(other._data, other._data + (_dimension.x * _dimension.y), _data);
}

HeightField::HeightField(const HeightField& other, iaabb2 bb) {
	_dimension.x = bb.max.x - bb.min.x;
	_dimension.y = bb.max.y - bb.min.y;
	_spacing = other._spacing;
	_origin.x = other._origin.x + _dimension.x * _spacing.x;
	_origin.y = other._origin.y + _dimension.y * _spacing.y;

	_minHeight = std::numeric_limits<float>::max();
	_maxHeight = std::numeric_limits<float>::min();
	_data = new float[_dimension.x * _dimension.y];
	_nullData = other._nullData;
	_resolution = other._resolution;

	int index = 0;
	for (int x = bb.min.x; x < bb.max.x; ++x) {
		for (int y = bb.min.y; y < bb.max.y; ++y) {
			float height = other.getData(x, y);
			if (height < _minHeight) _minHeight = height;
			if (height > _maxHeight) _maxHeight = height;
			_data[index++] = height;
		}
	}
}

HeightField::HeightField(const HeightField& other, Quadrant q) {
	unsigned halfX = floor(other._dimension.x / 2.0);
	unsigned halfY = floor(other._dimension.y / 2.0);

	iaabb2 bb;
	switch (q) {
	case Quadrant::NW:
		bb.min = ivec2(0, halfY);
		bb.max = ivec2(halfX, other._dimension.y);
		break;
	case Quadrant::NE:
		bb.min = ivec2(halfX, halfY);
		bb.max = ivec2(other._dimension.x, other._dimension.y);
		break;
	case Quadrant::SW:
		bb.min = ivec2(0, 0);
		bb.max = ivec2(halfX, halfY);
		break;
	case Quadrant::SE:
		bb.min = ivec2(halfX, 0);
		bb.max = ivec2(other._dimension.x, halfY);
		break;
	}

	_dimension.x = bb.max.x - bb.min.x;
	_dimension.y = bb.max.y - bb.min.y;
	_spacing.x = other._spacing.x;
	_spacing.y = other._spacing.y;
	_origin.x = other._origin.x + bb.min.x * _spacing.x;
	_origin.y = other._origin.y + bb.min.y * _spacing.y;

	_minHeight = std::numeric_limits<float>::max();
	_maxHeight = std::numeric_limits<float>::min();
	_data = new float[_dimension.x * _dimension.y];
	_nullData = other._nullData;
	_resolution = _nullData;

	int index = 0;
	for (int x = bb.min.x; x < bb.max.x; ++x) {
		for (int y = bb.min.y; y < bb.max.y; ++y) {
			float height = other.getData(x, y);
			if (height < _minHeight) _minHeight = height;
			if (height > _maxHeight) _maxHeight = height;
			setData(height, x - bb.min.x, y - bb.min.y);
		}
	}
}



HeightField& HeightField::operator=(const HeightField& other) {
	if (&other != this) {
		if (_data)
			delete[] _data;

		_origin = other._origin;
		_spacing.x = other._spacing.x;
		_spacing.y = other._spacing.y;
		_dimension.x = other._dimension.x;
		_dimension.y = other._dimension.y;
		_minHeight = other._minHeight;
		_maxHeight = other._maxHeight;
		_data = new float[other._dimension.x * other._dimension.y];
		_nullData = other._nullData;

		if (other._data)
			std::copy(other._data, other._data + (_dimension.x * _dimension.y), _data);
	}

	return *this;
}

double HeightField::memorySize() const {
	return (sizeof(short)* _dimension.x * _dimension.y);
}

double HeightField::memorySizeCompressed() const {
	//if (_compressor)
	//	return _compressor->memorySize();
	return 0;
}

double HeightField::memorySizeCompressedDelta() const {
	std::set<float> deltas;
	//unsigned maxDifference = std::numeric_limits<unsigned>::min();

	for (int col = 1; col < _dimension.x; ++col)
		deltas.insert(getData(col, 0) - getData(col - 1, 0));

	for (int row = 1; row < _dimension.y; ++row)
		deltas.insert(getData(0, row) - getData(0, row - 1));

	for (int col = 1; col < _dimension.x; ++col) {
		for (int row = 1; row < _dimension.y; ++row) {

			float value = getData(col, row);

			//std::cout << "Dimension: " << _dimension.x << "x" << _dimension.y << std::endl;
			//std::cout << "Factor: " << factor << std::endl;
			//std::cout << "Dimq: " << dimqX << ", " << dimqY << std::endl;

			float value1 = getData(col - 1, row - 1);
			//std::cout << "(" << firstCol << ", " << firstRow << ") = " << value1 << std::endl;

			float value2 = getData(col, row - 1);
			//std::cout << "(" << firstCol << ", " << boundRow << ") = " << value2 << std::endl;

			float value3 = getData(col - 1, row);
			//std::cout << "(" << boundCol << ", " << boundRow << ") = " << value3 << std::endl;

			float accum = value1 + value2 - value3;

			deltas.insert(value - accum);

		}
		//std::cout << std::endl;	
	}

	unsigned bits = deltas.size() == 0 ? 0 : ceil(std::log2(deltas.size()));

	float compressed = bits * _dimension.x * _dimension.y + 32;
	std::cout << _dimension.x << " x " << _dimension.y << ", " << bits << ", " << compressed / 8 << std::endl;
	getchar();
	return compressed / 8;
}

float HeightField::getData(vec2 point) const {
	int col = std::ceil((point.x - _origin.x) / _spacing.x);
	int row = std::ceil((point.y - _origin.y) / _spacing.y);

	return getData(col, row);
}


float HeightField::getHeightResolution() {

	if (_resolution == _nullData) {
		_resolution = std::numeric_limits<float>::max();

		for (int i = 0; i < (_dimension.x * _dimension.y) - 1; ++i) {
			for (int j = i + 1; j < _dimension.x * _dimension.y; ++j) {
				float difference = abs(_data[i] - _data[j]);
				if (difference < _resolution && _data[i] != _data[j])
					_resolution = difference;
			}
		}
	}

	if (_resolution == std::numeric_limits<float>::max())
		_resolution = 0;

	return _resolution;
}

void HeightField::computePrediction() {
	using namespace std::chrono;

	//auto timestamp = duration_cast< seconds >(steady_clock::now().time_since_epoch()).count();
	//std::string filename = std::to_string(timestamp) + ".csv";
	//std::ofstream stream(filename, std::ofstream::app);

	float min = std::numeric_limits<float>::max();
	float max = std::numeric_limits<float>::min();
	int factor = 5;

	unsigned int dimqX = floor(_dimension.x / static_cast<float>(factor));
	unsigned int dimqY = floor(_dimension.y / static_cast<float>(factor));
	std::set<float> deltas;

	for (int col = 1; col < _dimension.x; ++col) {
		for (int row = 1; row < _dimension.y; ++row) {

			float value = getData(col, row);

			//std::cout << "Dimension: " << _dimension.x << "x" << _dimension.y << std::endl;
			//std::cout << "Factor: " << factor << std::endl;
			//std::cout << "Dimq: " << dimqX << ", " << dimqY << std::endl;

			float value1 = getData(col - 1, row - 1);
			//std::cout << "(" << firstCol << ", " << firstRow << ") = " << value1 << std::endl;

			float value2 = getData(col, row - 1);
			//std::cout << "(" << firstCol << ", " << boundRow << ") = " << value2 << std::endl;

			float value3 = getData(col - 1, row);
			//std::cout << "(" << boundCol << ", " << boundRow << ") = " << value3 << std::endl;

			float accum = value1 + value2 - value3;

			deltas.insert(value - accum);

		}
		//std::cout << std::endl;	
	}
	std::cout << "Dimensions : " << _dimension.x << " x " << _dimension.y << std::endl;
	std::cout << "Deltas: " << deltas.size() << std::endl;
	std::cout << "Bits: 32 * " << _dimension.x + _dimension.y - 1 << " + deltas: " << (32 * (_dimension.x + _dimension.y - 1) + deltas.size()) << std::endl;
	float compressed = 32 * (_dimension.x + _dimension.y - 1) + deltas.size();
	float real = 32 * _dimension.x * _dimension.y;
	std::cout << "Compression " << 100 - (compressed / real) * 100 << "%" << std::endl;
	getchar();
	//stream.close();
}

void HeightField::computeMipmap() {
	int maxMipmap = std::floor(std::log2(std::max(_dimension.x, _dimension.y))) + 1;


	_mipmap.push_back(new ivec2[_dimension.x * _dimension.y]);
	for (int i = 0; i < _dimension.x * _dimension.y; ++i)
		_mipmap[0][i] = ivec2(_data[i], _data[i]);

	for (int l = 1; l < maxMipmap; ++l) {

		_mipmap.push_back(new ivec2[std::max(_dimension.x >> l, 1) * std::max(_dimension.y >> l, 1)]);

		for (int x = 0; x < std::max(1, _dimension.x >> l); ++x) {
			float x0 = x << 1;
			float x1 = std::min(x0 + 1, (float)std::max(1, _dimension.x >> (l - 1)) - 1);

			for (int y = 0; y < std::max(1, _dimension.y >> l); ++y) {
				float y0 = y << 1;
				float y1 = std::min(y0 + 1, (float)std::max(1, _dimension.y >> (l - 1)) - 1);


				float max1 = _mipmap[l - 1][(int)x1 + (int)y0 * (_dimension.x >> (l - 1))].x;
				float max2 = _mipmap[l - 1][(int)x1 + (int)y1 * (_dimension.x >> (l - 1))].x;
				float max3 = _mipmap[l - 1][(int)x0 + (int)y0 * (_dimension.x >> (l - 1))].x;
				float max4 = _mipmap[l - 1][(int)x0 + (int)y1 * (_dimension.x >> (l - 1))].x;
				float finalMax = std::numeric_limits<float>::min();

				if (max1 != _nullData)
					finalMax = std::max(max1, finalMax);
				if (max2 != _nullData)
					finalMax = std::max(max2, finalMax);
				if (max3 != _nullData)
					finalMax = std::max(max3, finalMax);
				if (max4 != _nullData)
					finalMax = std::max(max4, finalMax);

				_mipmap[l][x + y * (_dimension.x >> l)].x = std::max(finalMax, std::numeric_limits<float>::min());

				float min1 = _mipmap[l - 1][(int)x1 + (int)y0 * (_dimension.x >> (l - 1))].y;
				float min2 = _mipmap[l - 1][(int)x1 + (int)y1 * (_dimension.x >> (l - 1))].y;
				float min3 = _mipmap[l - 1][(int)x0 + (int)y0 * (_dimension.x >> (l - 1))].y;
				float min4 = _mipmap[l - 1][(int)x0 + (int)y1 * (_dimension.x >> (l - 1))].y;
				float finalMin = std::numeric_limits<float>::max();

				if (min1 != _nullData)
					finalMin = std::min(min1, finalMin);
				if (min2 != _nullData)
					finalMin = std::min(min2, finalMin);
				if (min3 != _nullData)
					finalMin = std::min(min3, finalMin);
				if (min4 != _nullData)
					finalMin = std::min(min4, finalMin);

				_mipmap[l][x + y * (_dimension.x >> l)].y = std::min(finalMin, std::numeric_limits<float>::max());
			}

		}

	}
}


std::ostream& operator<<(std::ostream& os, HeightField &hm) {
	std::string outpuString = "";

	for (int col = 0; col < hm._dimension.x; ++col) {
		for (int row = 0; row < hm._dimension.y; ++row) {
			float value = hm.getData(col, row);
			outpuString += std::to_string(value);
			outpuString += " ";
		}
		outpuString += "\n";
	}


	return os << outpuString.substr(0, outpuString.size() - 2);
	//return os << outpuString;
}

HeightField::~HeightField() {
	if (_data)
		delete[] _data;

	if (_compressor)
		delete _compressor;
}


