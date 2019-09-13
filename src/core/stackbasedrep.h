#ifndef STACK_BASED_REP_H
#define STACK_BASED_REP_H

#include "core/stack.h"
#include "core/voxelmodel.h"
 
#include <cmath>
#include <iostream>
#include <stdexcept>
/**
	Class that encapsulates an Stack-Based Representation Terrain an its operations.
	It uses a template for the type of attribute in cell.

	Data structure explained in:
	Benes, B., & Forsbach, R. (2001). Layered data representation for visual simulation of terrain erosion.
	In Proceedings Spring Conference on Computer Graphics. http://doi.org/10.1109/SCCG.2001.945341

	@author Alejandro Graciano

*/

using glm::vec2;
using glm::vec3;
using glm::ivec3;
using glm::ivec2;

template<class T>
class StackBasedRep {

	private:
		vec2 _origin; /** < Coordinates origin */

		vec2 _spacing; /** < Resolution of the cell */

		ivec2 _dimension; /** < Number of cells on each direction */

		float _minHeight, _maxHeight; /** < Height boundaries */

		Stack<T> *_stacks; /** < Buffer of interval stacks */

		std::string _attributeName; /** < Label for attribute denomination */

		const Stack<T>& getStack(unsigned int col, unsigned int row) const;

		/**
		Copy constructor disabled
		*/
		StackBasedRep(const StackBasedRep<T>& other);

		/**
		Copy assignment operator disabled
		*/
		StackBasedRep<T>& operator=(const StackBasedRep<T>& other);

	public:	

		StackBasedRep();

		/**
		Move constructor
		*/
		StackBasedRep(StackBasedRep<T>&& other);

		StackBasedRep(float minHeight, float maxHeight, std::string attribute, vec2 origin, vec2 spacing, ivec2 dimension);

		StackBasedRep(vec2 origin, vec2 spacing, ivec2 dimension);

		StackBasedRep(const VoxelModel<T>& vm);

		/**
		Move assignment operator
		*/
		StackBasedRep<T>& operator=(StackBasedRep<T>&& other);

		/**
		Getter methods
		*/

		float getMinHeight() const { return _minHeight; }

		float getMaxHeight() const { return _maxHeight; }
		
		float getOriginX() const { return _origin.x; }

		float getOriginY() const { return _origin.y; }

		float getBoundX() const { return _origin.x + _spacing.x * _dimension.x; }

		float getBoundY() const { return _origin.y + _spacing.y * _dimension.y; }

		float getResolution() const { return _spacing.x; }

		std::string getAttributeName() const { return _attributeName; }

		ivec2 getDimension() const { return _dimension; }

		Stack<T>& getStack(unsigned int col, unsigned int row);

		unsigned getMaxStack();

		void setMinHeight(float minHeigh) { _minHeight = minHeigh; }

		void setMaxHeight(float maxHeigh) { _maxHeight = maxHeigh; }

		void setNonEmptyMaxHeight(float nonEmptyMaxHeigh) { _nonEmptyMaxHeight = nonEmptyMaxHeigh; }

		void setOriginX(float originX) { _origin._x = originX; };

		void setOriginY(float originY) { _origin._y = originY; };

		void setResolution(float resolution) { _spacingX = _spacingY = resolution; };

		void setAttributeName(std::string name) { _attributeName = name; }

		void setMaterial(T &material) { _materials.insert(material); }

		// @}

		/**
		Begin to iterate
		*/
		Stack<T>* begin() { return _stacks; }

		/**
		End to iterate
		*/
		Stack<T>* end() { return _stacks + _dimension.x * _dimension.y; }

		double memorySize() const; /** < Memory consumption returned in bytes*/

		/**
		Return a copy of the specified stack
		*/
		Stack<T> copy(unsigned int col, unsigned int row);

		/**
		Overloading of the << operator. Prints the data structure state
		*/
		template<class T> // Needed for a friend method
		friend std::ostream& operator<<(std::ostream& os, StackBasedRep<T> &sbr);
		
};


template<class T>
StackBasedRep<T>::StackBasedRep()
: _stacks(new Stack<T>[0]),
_attributeName("") {

}

template<class T>
StackBasedRep<T>::StackBasedRep(StackBasedRep<T>&& other)
:_stacks(&other._stacks[0]),
_origin(other._origin),
_spacing(other._spacing),
_dimension(other._dimension),
_minHeight(other._minHeight),
_maxHeight(other._maxHeight),
_attributeName(move(other._attributeName)) {

	other._stacks = nullptr;
}

template<class T>
StackBasedRep<T>::StackBasedRep(float minHeight, float maxHeight, std::string attribute, vec2 origin, vec2 spacing, ivec2 dimension)
: _origin(origin),
_spacing(spacing),
_dimension(dimension),
_minHeight(minHeight),
_maxHeight(maxHeight),
_attributeName(attribute),
_stacks(new Stack<T>[dimension.x * dimension.y]) {
}

template<class T>
StackBasedRep<T>::StackBasedRep(vec2 origin, vec2 spacing, ivec2 dimension)
: _origin(origin),
_spacing(spacing),
_dimension(dimension),
_attributeName(""),
_stacks(new Stack<T>[dimension.x * dimension.y]) {
}

template<class T>
StackBasedRep<T>::StackBasedRep(const VoxelModel<T>& vm) :
_origin(vm.getOriginX(), vm.getOriginY()),
_spacing(vm.getResolution(), vm.getResolution()),
_dimension(vm.getDimensionX(), vm.getDimensionY()),
_minHeight(vm.getMinHeight()),
_maxHeight(vm.getMaxHeight()),
_attributeName(vm.getAttributeName()),
_stacks(new Stack<T>[vm.getDimensionX() * vm.getDimensionY()]) {

	const T *data = vm.getData();

	auto spacingZ = vm.getSpacingZ();
	int dimensionZ = round((_maxHeight - _minHeight) / spacingZ);

	try {
		for (auto xValue = 0; xValue < _dimension.x; ++xValue) {
			for (auto yValue = 0; yValue < _dimension.y; ++yValue) {
				auto& stack = getStack(xValue, yValue);
				//stack.reserveIntervals(dimensionZ);

				for (auto zValue = 0; zValue < dimensionZ; ++zValue) {
					auto currentHeight = (zValue + 1) * spacingZ + _minHeight;
					int index1D = yValue + _dimension.y * (xValue + _dimension.x * zValue);

					T attribute = data[index1D];

					stack.addInterval(attribute, currentHeight);
				}
			}
		}
	} catch (std::exception& e) {
		std::cerr << "Exception converting into a SBRT: " << e.what();
	}
}

template<class T>
StackBasedRep<T>& StackBasedRep<T>::operator=(StackBasedRep<T>&& other) {
	delete[] _stacks;

	_stacks = std::move(other._stacks);
	_origin = other._origin;
	_spacing.x = other._spacing.y;
	_spacing = other._spacing;
	_dimension = other._dimension;
	_minHeight = other._minHeight;
	_maxHeight = other._maxHeight;
	_attributeName = move(other._attributeName);

	return *this;
}

template<class T>
const Stack<T>& StackBasedRep<T>::getStack(unsigned int col, unsigned int row) const {

	if (col > _dimension.x || row > _dimension.y)
		throw std::out_of_range("Stack out of range");

	return _stacks[col + row * _dimension.x];

}

template<class T>
Stack<T>& StackBasedRep<T>::getStack(unsigned int col, unsigned int row) {
	if (col > _dimension.x || row > _dimension.y)
		throw std::out_of_range("Stack out of range");

	return const_cast<Stack<T>&>(static_cast<const StackBasedRep&>(*this).getStack(col, row));
}

template<class T>
double StackBasedRep<T>::memorySize() const {
	double accumulatedSize = 0;

	for (auto yIndex = 0; yIndex < _dimension.y; ++yIndex) {
		for (auto xIndex = 0; xIndex < _dimension.x; ++xIndex) {
			auto& stack = getStack(xIndex, yIndex);
			accumulatedSize += stack.memorySize();
		}
	}

	return accumulatedSize;
}

template<class T>
unsigned StackBasedRep<T>::getMaxStack() {
	unsigned maxStack = 0;

	for (auto x = 0; x < _dimension.x; ++x) {
		for (auto y = 0; y < _dimension.y; ++y) {
			Stack<T>&  stack = getStack(x, y);
			if (maxStack < stack.getIntervals().size())
				maxStack = stack.getIntervals().size();
		}
	}

	return maxStack;
}

template<class T>
std::ostream& operator<<(std::ostream& os, StackBasedRep<T> &sbr) {
	std::string outpuString = "";

	outpuString += "Origin ";
	outpuString += std::to_string(sbr._origin.x);
	outpuString += " ";
	outpuString += std::to_string(sbr._origin.y);
	outpuString += "\n";

	outpuString += "Dimension ";
	outpuString += std::to_string(sbr._dimension.x);
	outpuString += " ";
	outpuString += std::to_string(sbr._dimension.y);
	outpuString += "\n";

	outpuString += "Spacing ";
	outpuString += std::to_string(sbr._spacing.x);
	outpuString += " ";
	outpuString += std::to_string(sbr._spacing.y);
	outpuString += "\n";

	outpuString += "Height ";
	outpuString += std::to_string(sbr._minHeight);
	outpuString += " ";
	outpuString += std::to_string(sbr._maxHeight);
	outpuString += "\n";

	for (auto yIndex = 0; yIndex < sbr._dimension.y; ++yIndex) {
		for (auto xIndex = 0; xIndex < sbr._dimension.x; ++xIndex) {
			auto& stack = sbr.getStack(xIndex, yIndex);
			auto& intervals = stack.getIntervals();

			if (intervals.size() == 0) {
				outpuString += std::to_string(Stack<T>::UNKNOWN_VALUE);
				outpuString += "|";
				outpuString += std::to_string(sbr._maxHeight);
				outpuString += "$";
			}
			for (const Interval<T> &interval : intervals) {
				outpuString += std::to_string(interval._attribute);
				outpuString += "|";
				outpuString += std::to_string(interval._accumulatedHeight);
				outpuString += "$";
			}
			//outpuString += ";";
			outpuString += "\n";
		}
	}


	return os << outpuString.substr(0, outpuString.size() - 2);
}

using IntSBR = StackBasedRep<int>;
using ShortSBR = StackBasedRep<short>;

#endif