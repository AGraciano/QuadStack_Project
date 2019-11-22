#ifndef OCTREE_VOXEL_MODEL_H
#define OCTREE_VOXEL_MODEL_H

#include <memory.h>
#include "voxelmodel.h"

/**
Class that encapsulate an octree representation for compress a voxel model

@author Alejandro Graciano
*/

template<class T>
class Octree {

	/**
	Nested class that represents a QuadTree node

	@author Alejandro Graciano
	*/
	class Node {

		/** Node child */
		Node *_child0, *_child1, *_child2, *_child3, *_child4, *_child5, *_child6, *_child7;

		/** Node init point */
		unsigned int _initX, _initY, _initZ;

		/** Node bound point */
		unsigned int _boundX, _boundY, _boundZ;

		unsigned int _level; /*< Current octree level */

		std::vector<T> _representative; /*< Representative value that compress the node */

		VoxelModel<T> *_pVM; /*< Pointer to the actual voxel model */


		/**
		Return a percentage between 0.0 and 1.0 that measures the similarity among the voxels contained in the node
		*/
		float measureValues();

		/**
		Return a percentage between 0.0 and 1.0 that measures the similarity among the voxels contained in the node
		*/
		bool lastVoxel();

	public:

		/**
		Constructor
		*/
		Node(VoxelModel<T> *pVM, unsigned int initX, unsigned int initY, unsigned int initZ, unsigned int boundX, unsigned int boundY, unsigned int boundZ, unsigned int level);

		/**
		Send to classify the children
		*/
		void classify();

		/**
		Subdivide the node into eight children
		*/
		void subdivide();

		/**
		Subdivide the node into eight children
		*/
		bool isLeaf() { return _child0 == nullptr; }

		/**
		Check if the mode has representative
		*/
		bool hasRepresentative() { return _representative.size() > 0; }

		/**
		Check if a point in within the node
		*/
		bool isInside(unsigned int xIndex, unsigned int yIndex, unsigned int zIndex);

		/**
		Print the node in the standard output
		*/
		void print();

		/**
		Return recursively the value of the point
		*/
		T getValue(unsigned int xIndex, unsigned int yIndex, unsigned int zIndex);

		/**
		Return the memory size
		*/
		float memorySize();

		~Node();
	};

	/**
	Actual Voxel Model
	*/
	VoxelModel<T> *_pVM;

	/**
	Max number of levels
	*/
	unsigned int _maxLevels;

	/**
	Root node
	*/
	Node *_root;

public:

	Octree(VoxelModel<T> *pVM);

	/**
	Send to classify the children
	*/
	void classify();

	/**
	Print the node in the standard output
	*/
	void print();

	/**
	Return the memory size
	*/
	float memorySize();

	/**
	Return a Voxel Model with the original size
	*/
	VoxelModel<T> decompress();

	~Octree();

};

template<class T>
Octree<T>::Octree(VoxelModel<T> *pVM) :
	_pVM(pVM),
	_root(new Node(pVM, 0, 0, 0, pVM->getDimensionX(), pVM->getDimensionY(), pVM->getDimensionZ(), 0)) {

	unsigned maxDimension = std::max(pVM->getDimensionX(), std::max(pVM->getDimensionY(), pVM->getDimensionZ()));
	float logOf2 = log2(maxDimension);
	bool powerOf2 = ceil(logOf2) == floor(logOf2);
	_maxLevels = powerOf2 ? logOf2 : ceil(logOf2);
	_maxLevels++;

	classify();
}

template<class T>
void Octree<T>::classify() {

	_root->classify();
}

template<class T>
void Octree<T>::print() {
	std::cout << "--------------------------------" << std::endl;
	std::cout << "Max level " << _maxLevels << std::endl;
	_root->print();
}

template<class T>
float Octree<T>::memorySize() {

	return _root->memorySize();
}

template<class T>
VoxelModel<T> Octree<T>::decompress() {
	vec3 origin;
	origin.x = _pVM->getOriginX();
	origin.y = _pVM->getOriginY();
	origin.z = _pVM->getOriginZ();

	vec3 spacing;
	spacing.x = _pVM->getSpacingX();
	spacing.y = _pVM->getSpacingY();
	spacing.z = _pVM->getSpacingZ();

	auto boundX = _pVM->getBoundX();
	auto boundY = _pVM->getBoundY();
	auto boundZ = _pVM->getBoundZ();

	ivec3 dimension;
	dimension.x = (boundX - origin.x) / spacing.x;
	dimension.y = (boundY - origin.y) / spacing.y;
	dimension.z = (boundZ - origin.z) / spacing.z;

	auto attributeName = _pVM->getAttributeName();
	VoxelModel<T> returnVM(dimension, spacing, origin, attributeName);
	int *data = new int[dimensionX * dimensionY * dimensionZ];


	for (auto zIndex = 0; zIndex < dimension.z; ++zIndex) {
		for (auto yIndex = 0; yIndex < dimension.y; ++yIndex) {
			for (auto xIndex = 0; xIndex < dimension.x; ++xIndex) {
				int index = VoxelModel<T>::index1D(xIndex, yIndex, zIndex, dimensionX, dimensionY, dimensionZ);
				data[index] = _root->getValue(xIndex, yIndex, zIndex);
			}
		}
	}

	returnVM.setData(data);
	delete[] data;

	return returnVM;
}

template<class T>
Octree<T>::~Octree() {
	delete _root;
}


/**
Node class implementation
*/

template<class T>
Octree<T>::Node::Node(VoxelModel<T> *pVM, unsigned int initX, unsigned int initY, unsigned int initZ,
	unsigned int boundX, unsigned int boundY, unsigned int boundZ, unsigned int level) :
	_child0(nullptr),
	_child1(nullptr),
	_child2(nullptr),
	_child3(nullptr),
	_child4(nullptr),
	_child5(nullptr),
	_child6(nullptr),
	_child7(nullptr),
	_pVM(pVM),
	_initX(initX),
	_initY(initY),
	_initZ(initZ),
	_boundX(boundX),
	_boundY(boundY),
	_boundZ(boundZ),
	_level(level) {
}

template<class T>
bool Octree<T>::Node::isInside(unsigned int xIndex, unsigned int yIndex, unsigned int zIndex) {
	bool checkX = xIndex >= _initX && xIndex <= _boundX;
	bool checkY = yIndex >= _initY && yIndex <= _boundY;
	bool checkZ = zIndex >= _initZ && zIndex <= _boundZ;

	return checkX && checkY && checkZ;
}

template<class T>
T Octree<T>::Node::getValue(unsigned int xIndex, unsigned int yIndex, unsigned int zIndex) {

	if (isLeaf()) {
		if (hasRepresentative())
			return _representative;
		else
			return _pVM->getData(xIndex, yIndex, zIndex);
	} else {
		if (_child0->isInside(xIndex, yIndex, zIndex)) return _child0->getValue(xIndex, yIndex, zIndex);
		else if (_child1->isInside(xIndex, yIndex, zIndex)) return _child1->getValue(xIndex, yIndex, zIndex);
		else if (_child2->isInside(xIndex, yIndex, zIndex)) return _child2->getValue(xIndex, yIndex, zIndex);
		else if (_child3->isInside(xIndex, yIndex, zIndex)) return _child3->getValue(xIndex, yIndex, zIndex);
		else if (_child4->isInside(xIndex, yIndex, zIndex)) return _child4->getValue(xIndex, yIndex, zIndex);
		else if (_child5->isInside(xIndex, yIndex, zIndex)) return _child5->getValue(xIndex, yIndex, zIndex);
		else if (_child6->isInside(xIndex, yIndex, zIndex)) return _child6->getValue(xIndex, yIndex, zIndex);
		else return _child7->getValue(xIndex, yIndex, zIndex);
	}
}

template<class T>
bool Octree<T>::Node::lastVoxel() {
	return _boundX == _initX + 1 || _boundY == _initY + 1 || _boundZ == _initZ + 1;
}

template<class T>
float Octree<T>::Node::measureValues() {
	_representative.push_back(_pVM->getData(_initX, _initY, _initZ));
	bool first = true;
	for (int zIndex = _initZ; zIndex < _boundZ; ++zIndex) {
		for (int yIndex = _initY; yIndex < _boundY; ++yIndex) {
			for (int xIndex = _initX; xIndex < _boundX; ++xIndex) {
				if (!first) {
					T currentValue = _pVM->getData(xIndex, yIndex, zIndex);
					
					if (_representative[0] != currentValue)
						return 0.0;

				}

				first = false;
			}
		}
	}

	return 1.0;
}


template<class T>
void Octree<T>::Node::classify() {
	if (!lastVoxel()) {
		float similarity = measureValues();

		if (similarity < 1.0) { // End of the subdivision
			_representative.clear();

			subdivide();

			_child0->classify();
			_child1->classify();
			_child2->classify();
			_child3->classify();
			_child4->classify();
			_child5->classify();
			_child6->classify();
			_child7->classify();

		}

	}

}

template<class T>
void Octree<T>::Node::subdivide() {

	unsigned int halfX = (_boundX + _initX + 1) / 2;
	unsigned int halfY = (_boundY + _initY + 1) / 2;
	unsigned int halfZ = (_boundZ + _initZ + 1) / 2;

	unsigned int child0InitX = _initX;
	unsigned int child0InitY = _initY;
	unsigned int child0InitZ = _initZ;
	unsigned int child0BoundX = halfX;
	unsigned int child0BoundY = halfY;
	unsigned int child0BoundZ = halfZ;

	unsigned int child1InitX = halfX;
	unsigned int child1InitY = _initY;
	unsigned int child1InitZ = _initZ;
	unsigned int child1BoundX = _boundX;
	unsigned int child1BoundY = halfY;
	unsigned int child1BoundZ = halfZ;

	unsigned int child2InitX = _initX;
	unsigned int child2InitY = halfY;
	unsigned int child2InitZ = _initZ;
	unsigned int child2BoundX = halfX;
	unsigned int child2BoundY = _boundY;
	unsigned int child2BoundZ = halfZ;

	unsigned int child3InitX = halfX;
	unsigned int child3InitY = halfY;
	unsigned int child3InitZ = _initZ;
	unsigned int child3BoundX = _boundX;
	unsigned int child3BoundY = _boundY;
	unsigned int child3BoundZ = halfZ;

	unsigned int child4InitX = _initX;
	unsigned int child4InitY = _initY;
	unsigned int child4InitZ = halfZ;
	unsigned int child4BoundX = halfX;
	unsigned int child4BoundY = halfY;
	unsigned int child4BoundZ = _boundZ;

	unsigned int child5InitX = halfX;
	unsigned int child5InitY = _initY;
	unsigned int child5InitZ = halfZ;
	unsigned int child5BoundX = _boundX;
	unsigned int child5BoundY = halfY;
	unsigned int child5BoundZ = _boundZ;

	unsigned int child6InitX = _initX;
	unsigned int child6InitY = halfY;
	unsigned int child6InitZ = halfZ;
	unsigned int child6BoundX = halfX;
	unsigned int child6BoundY = _boundY;
	unsigned int child6BoundZ = _boundZ;

	unsigned int child7InitX = halfX;
	unsigned int child7InitY = halfY;
	unsigned int child7InitZ = halfZ;
	unsigned int child7BoundX = _boundX;
	unsigned int child7BoundY = _boundY;
	unsigned int child7BoundZ = _boundZ;

	_child0 = new Node(_pVM, child0InitX, child0InitY, child0InitZ, child0BoundX, child0BoundY, child0BoundZ, _level - 1);
	_child1 = new Node(_pVM, child1InitX, child1InitY, child1InitZ, child1BoundX, child1BoundY, child1BoundZ, _level - 1);
	_child2 = new Node(_pVM, child2InitX, child2InitY, child2InitZ, child2BoundX, child2BoundY, child2BoundZ, _level - 1);
	_child3 = new Node(_pVM, child3InitX, child3InitY, child3InitZ, child3BoundX, child3BoundY, child3BoundZ, _level - 1);
	_child4 = new Node(_pVM, child4InitX, child4InitY, child4InitZ, child4BoundX, child4BoundY, child4BoundZ, _level - 1);
	_child5 = new Node(_pVM, child5InitX, child5InitY, child5InitZ, child5BoundX, child5BoundY, child5BoundZ, _level - 1);
	_child6 = new Node(_pVM, child6InitX, child6InitY, child6InitZ, child6BoundX, child6BoundY, child6BoundZ, _level - 1);
	_child7 = new Node(_pVM, child7InitX, child7InitY, child7InitZ, child7BoundX, child7BoundY, child7BoundZ, _level - 1);

}

template<class T>
void Octree<T>::Node::print() {
	std::cout << "Node at level " << _level << std::endl;
	auto nCells = (_boundX - _initX) * (_boundY - _initY) * (_boundZ - _initZ);
	if (isLeaf()) {
		if (hasRepresentative())
			std::cout << "COMPRESS. This leaf encloses " << nCells << " cells" << std::endl;
		else
			std::cout << "Is leaf. This leaf does not enclose " << nCells << " cells" << std::endl;


	} else {
		_child0->print();
		_child1->print();
		_child2->print();
		_child3->print();
		_child4->print();
		_child5->print();
		_child6->print();
		_child7->print();
	}
}

template<class T>
float Octree<T>::Node::memorySize() {
	float acum = 0;

	if (!isLeaf()) {
		acum += sizeof(int) * 8;

		acum += _child0->memorySize();
		acum += _child1->memorySize();
		acum += _child2->memorySize();
		acum += _child3->memorySize();
		acum += _child4->memorySize();
		acum += _child5->memorySize();
		acum += _child6->memorySize();
		acum += _child7->memorySize();
	} else {
		if (hasRepresentative())
			acum += sizeof(T) / 2;
		else 
			acum += sizeof(T) / 2 * (_boundX - _initX) * (_boundY - _initY) * (_boundZ - _initZ);		
	}

	return acum;
}

template<class T>
Octree<T>::Node::~Node() {
	delete _child0;
	delete _child1;
	delete _child2;
	delete _child3;
	delete _child4;
	delete _child5;
	delete _child6;
	delete _child7;
}

using IntOctree = Octree<int>;
using ShortOctree = Octree<short>;
using ByteOctree = Octree<char>;

#endif

