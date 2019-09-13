#ifndef VOXEL_MODEL_H
#define VOXEL_MODEL_H

#include <iostream>
#include <string>
#include <glm/glm.hpp>

/**
	Class that encapsulates a voxel model.
	It uses a template for the type of attribute in cell; therefore, the class
	must be implemented in header file.

	@author Alejandro Graciano
*/

using glm::ivec3;
using glm::vec3;	

template <class T>
class VoxelModel {

private:

	ivec3 _dimension;

	vec3 _spacing;

	vec3 _origin;

	int _nData;

	std::string _attributeName;

	T *_data;

	
	/**
	@brief Copy constructor disabled
	*/
	VoxelModel(const VoxelModel<T>& vm);

	

public:

	// @{
	/**
	Constructor
	*/
	VoxelModel();

	VoxelModel(VoxelModel<T>&& vm);

	/**
	Naive constructor. If not data parameter has been given, it is populated
	with a random binary value (ocuppied or not)
	*/
	VoxelModel(ivec3 dimension, vec3 spacing, vec3 origin, std::string attribute, T *data);

	VoxelModel(ivec3 dimension, vec3 spacing, vec3 origin, std::string attribute);
	// @}

	// @{
	/**
	Setter methods
	*/
	void setDimension(const ivec3 dimension) {
		_dimension = dimension;
		_nData = dimension.x * dimension.y * dimension.z;
	}

	void setSpacing(const vec3 spacing) { _spacing = spacing; }

	void setOrigin(const vec3 origin) { _origin = origin; }

	void setAttributeName(const std::string attributeName) { _attributeName = attributeName; }

	void setData(const T* data);

	void setData(const T* data, const ivec3 dimension);

	// @}

	// @{
	/**
	Getter methods
	*/
	float getMinHeight() const { return static_cast<float>(_origin.z); }

	float getMaxHeight() const { return static_cast<float>(_origin.z + _spacing.z * _dimension.z); }

	float getOriginX() const { return _origin.x; };

	float getOriginY() const { return _origin.y; };

	float getOriginZ() const { return _origin.z; };

	float getBoundX() const { return _origin.x + _spacing.x * _dimension.x; };

	float getBoundY() const { return _origin.y + _spacing.y * _dimension.y; };

	float getBoundZ() const { return _origin.z + _spacing.z * _dimension.z; };

	float getResolution() const { return _spacing.x; };

	std::string getAttributeName() const { return _attributeName; }

	unsigned int getDimensionX() const { return _dimension.x; }

	unsigned int getDimensionY() const { return _dimension.y; }

	unsigned int getDimensionZ() const { return _dimension.z; }

	float getSpacingX() const { return _spacing.x; }

	float getSpacingY() const { return _spacing.y; }

	float getSpacingZ() const { return _spacing.z; }

	const T* getData() const { return _data; }

	T getData(unsigned int x, unsigned int y, unsigned int z) const { return _data[index1D(x, y, z, _dimension.x, _dimension.y, _dimension.z)]; }

	int getNData() const { return _nData; }

	// @}


	double memorySize() const;

	/**
	@brief Computes a one dimensional index from a three dimensional index
	*/
	static unsigned index1D(unsigned x, unsigned y, unsigned z, unsigned dimensionX, unsigned dimensionY, unsigned dimensionZ);


	/**
	Default destructor
	*/
	~VoxelModel();
};


template<class T>
VoxelModel<T>::VoxelModel()
	: _data(new T[0]) {
}


template<class T>
VoxelModel<T>::VoxelModel(VoxelModel<T>&& vm)
	: _dimension.x(vm._dimension.x),
	_dimension.y(vm._dimension.y),
	_dimension.z(vm._dimension.z),
	_spacing(vm._spacing),
	_origin(vm._origin),
	_attributeName(move(vm._attributeName)),
	_nData(vm._nData),
	_data(&vm._data[0]) {

	vm._data = nullptr;
}

template<class T>
VoxelModel<T>::VoxelModel(const VoxelModel<T>& vm)
	: _dimension.x(vm._dimension.x),
	_dimension.y(vm._dimension.y),
	_dimension.z(vm._dimension.z),
	_spacing(vm._spacing),
	_origin(vm._origin),
	_attributeName(vm._attributeName),
	_nData(vm._nData),
	_data(new T[_nData]) {

	memcpy(_data, vm._data, _nData *sizeof(T));

}

template<class T>
VoxelModel<T>::VoxelModel(ivec3 dimension, vec3 spacing, vec3 origin, std::string attribute)
	: _dimension(dimension),
	_spacing(spacing),
	_origin(origin),
	_attributeName(attribute),
	_nData(_dimension.x * _dimension.y * _dimension.z),
	_data(new T[_nData]) {

}

template<class T>
VoxelModel<T>::VoxelModel(ivec3 dimension, vec3 spacing, vec3 origin, std::string attribute, T *data)
: _dimension(dimension),
	_spacing(spacing),
	_origin(origin),
	_attributeName(attribute),
	_nData(_dimension.x * _dimension.y * _dimension.z),
	_data(new T[_nData]) {

	memcpy(_data, data, _nData *sizeof(T));

}

template<class T>
void VoxelModel<T>::setData(const T* data, const ivec3 dimension) {
	_dimension.x = dimension.x;
	_dimension.y = dimension.y;
	_dimension.z = dimension.z;
	_nData = dimension.x * dimension.y * dimension.z;

	setData(data);
}

template<class T>
void VoxelModel<T>::setData(const T* data) {
	delete[] _data;
	_data = new T[_nData];
	memcpy(_data, data, _nData * sizeof(T));
}

template<class T>
double VoxelModel<T>::memorySize() const {
	return (sizeof(T) * _nData);
}

template<class T>
VoxelModel<T>::~VoxelModel() {
	delete[] _data;
}

template<class T>
unsigned VoxelModel<T>::index1D(unsigned x, unsigned y, unsigned z, unsigned dimensionX, unsigned dimensionY, unsigned dimensionZ) {
	auto index = (z * dimensionX * dimensionY) + (y * dimensionX) + x;
	//auto index = y + dimensionY * (x + dimensionX * z);
	//auto index = x + dimensionY * (y + dimensionZ * z);
	auto nData = dimensionX * dimensionY * dimensionZ;

	if (index >= nData)
		throw std::out_of_range("Bad index access");

	return index;
}

using IntVM = VoxelModel<int>;
using ShortVM = VoxelModel<short>;


#endif
