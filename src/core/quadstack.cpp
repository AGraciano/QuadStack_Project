#include "quadstack.h"
#include "core/heightfieldcompressor.h"
#include <algorithm>
#include <sstream>

/**
	QuadStack::Node methods
*/

QuadStack::Node::Node(int level, ivec2 min, ivec2 max, ShortSBR *terrain) :
_nw(nullptr),
_ne(nullptr),
_sw(nullptr),
_se(nullptr),
_level(level),
_bb(min, max),
_compressed(false),
_terrain(terrain) {
}


QuadStack::Node::Node(const QuadStack::Node& other) :
_nw(other._nw),
_ne(other._ne),
_sw(other._sw),
_se(other._se),
_level(other._level),
_bb(other._bb),
_compressed(other._compressed),
_terrain(other._terrain) {
}

QuadStack::Node& QuadStack::Node::operator=(const QuadStack::Node& other) {
	if (&other != this) {
		_nw = other._nw;
		_ne = other._ne;
		_sw = other._sw;
		_se = other._se;
		_compressed = other._compressed;
		_level = other._level;
		_bb = other._bb;
		_terrain = other._terrain;
		_stack = other._stack;
	}

	return *this;
}

bool QuadStack::Node::lastStack() {
	return _bb.min.x == _bb.max.x - 1 && _bb.min.y == _bb.max.y - 1;
}

bool QuadStack::Node::divisible() {
	return (_bb.max.x - _bb.min.x) > 0 && (_bb.max.y - _bb.min.y) > 0;
}


bool QuadStack::Node::isLeaf() const {
	return _nw == nullptr;
}

int QuadStack::Node::treeHeight() {
	if (isLeaf())
		return 1;
	else {
		int nwHeight = _nw->treeHeight();
		int neHeight = _ne->treeHeight();
		int swHeight = _sw->treeHeight();
		int seHeight = _se->treeHeight();

		return std::max(std::max(nwHeight, neHeight), std::max(swHeight, seHeight)) + 1;
	}
}


unsigned QuadStack::Node::getMinLevel() {
		if (isLeaf())
		return _level;
	else {

		unsigned nwDeep = _nw->getMinLevel();
		unsigned swDeep = _sw->getMinLevel();
		unsigned neDeep = _ne->getMinLevel();
		unsigned seDeep = _se->getMinLevel();

		return std::min(nwDeep, std::min(swDeep, std::min(neDeep, seDeep)));
	}

}

bool QuadStack::Node::isInside(unsigned x, unsigned y) {
	return _bb.min.x <= x && _bb.max.x > x && _bb.min.y <= y && _bb.max.y > y;
}


bool QuadStack::Node::compress() {

	Stack<short>& firstStack = _terrain->getStack(_bb.min.x, _bb.min.y);
	std::vector<HeightField*> heights;

	vec2 origin;
	origin.x = _terrain->getOriginX() + _bb.min.x * _terrain->getResolution();
	origin.y = _terrain->getOriginY() + _bb.min.y * _terrain->getResolution();
	
	vec2 spacing;
	spacing.x = _terrain->getResolution();
	spacing.y = _terrain->getResolution();
	
	ivec2 dimension;
	dimension.x = _bb.max.x - _bb.min.x;
	dimension.y = _bb.max.y - _bb.min.y;
	float minHeight = _terrain->getMinHeight();
	float maxHeight = _terrain->getMaxHeight();
	float nullData = -999;

	// We introduce the height and materials of the first stack
	for (auto interval : firstStack.getIntervals()) {
		HeightField *map = new HeightField(origin, spacing, dimension, minHeight, maxHeight, nullData, nullptr);

		map->setData(interval._accumulatedHeight, 0, 0);

		heights.push_back(map);
	}

	// Check if every stack is equal to the first one
	for (int x = _bb.min.x; x < _bb.max.x; ++x) {
		for (int y = _bb.min.y; y < _bb.max.y; ++y) {
			unsigned relativeX = x - _bb.min.x;
			unsigned relativeY = y - _bb.min.y;
			Stack<short>& nextStack = _terrain->getStack(x, y);

			if (!(firstStack.compareAttributes(nextStack))) {

				// Reset the height maps introduced so far
				for (auto map : heights)
					delete map;

				return false;
			}

			int i = 0;
			for (auto interval : nextStack.getIntervals())
				heights[i++]->setData(interval._accumulatedHeight, relativeX, relativeY);

		}
	}

	// Save the intervals
	int i = 0;
	for (auto interval : firstStack.getIntervals()) {
		Interval newInterval(interval._attribute, heights[i++]);
		_stack.push_back(newInterval);

	}

	return true;
}

bool QuadStack::Node::unify() {

	vec2 origin;
	origin.x = _terrain->getOriginX() + _bb.min.x * _terrain->getResolution();
	origin.y = _terrain->getOriginY() + _bb.min.y * _terrain->getResolution();
	
	vec2 spacing;
	spacing.x = _terrain->getResolution();
	spacing.y = _terrain->getResolution();
	
	ivec2 dimension;
	dimension.x = _bb.max.x - _bb.min.x;
	dimension.y = _bb.max.y - _bb.min.y;
	float minHeight = _terrain->getMinHeight();
	float maxHeight = _terrain->getMaxHeight();
	float nullData = NULL_VALUE;

	if (!divisible())
		auto heightField = new HeightField(origin, spacing, dimension, minHeight, maxHeight, nullData, nullptr);
	
	auto& reference = _terrain->getStack(_bb.min.x, _bb.min.y);

	unsigned stackSize = reference.getIntervals().size();
	vector<HeightField*> heightFields(stackSize);

	for (int i = 0; i < stackSize; ++i)
		heightFields[i] = new HeightField(origin, spacing, dimension, minHeight, maxHeight, nullData, nullptr);

	for (int x = _bb.min.x; x < _bb.max.x; ++x) {
		for (int y = _bb.min.y; y < _bb.max.y; ++y) {
			auto& stack = _terrain->getStack(x, y);

			if (!stack.compareAttributes(reference)) {
				heightFields.clear();
				return false;
			} else {
				for (int i = 0; i < stackSize; ++i) {
					float height = stack.getAttributeAtIndex(i)._accumulatedHeight;
					heightFields[i]->setData(height, x - _bb.min.x, y - _bb.min.y);
				}
			}
		}
	}

	int i = 0;
	for (auto& interval : reference.getIntervals())
		_stack.push_back(Interval(interval._attribute, heightFields[i++]));


	return true;
}

unsigned QuadStack::Node::nonUIntervals(GStack& stack) {
	unsigned count = 0;
	for (auto i : stack)
	if (!i.isNull())
		count++;
	return count;
}



QuadStack::GStack QuadStack::Node::compact(Interval *s1, Interval *s2, Interval *s3, Interval *s4, vector<int> l, Cachemap& cache) {
	IntPair pair1(l[0], l[1]);
	IntPair pair2(l[2], l[3]);
	DoublePair dpair(pair1, pair2);
	auto it = cache.find(dpair);
	if (it != cache.end())
		return it->second;

	GStack r;

	if (l[0] > 0 && l[1] > 0 && l[2] > 0 && l[3] > 0) {
		bool lengthE1 = (l[0] > 1) == (l[1] > 1) && (l[2] > 1) == (l[3] > 1) && (l[0] > 1) == (l[3] > 1);
		bool lengthE2 = (l[0] > 2) == (l[1] > 2) && (l[2] > 2) == (l[3] > 2) && (l[0] > 2) == (l[3] > 2);

		bool equalB1 = *s1 == *s2 && !s1[0].isNull() && !s2[0].isNull();
		bool equalB2 = *s3 == *s4 && !s3[0].isNull() && !s4[0].isNull();
		bool equalB3 = *s1 == *s3 && !s1[0].isNull() && !s3[0].isNull();

		bool equalE1 = s1[l[0] - 1] == s2[l[1] - 1] && !s1[l[0] - 1].isNull() && !s2[l[1] - 1].isNull();
		bool equalE2 = s3[l[2] - 1] == s4[l[3] - 1] && !s3[l[2] - 1].isNull() && !s4[l[3] - 1].isNull();
		bool equalE3 = s1[l[0] - 1] == s3[l[2] - 1] && !s1[l[0] - 1].isNull() && !s3[l[2] - 1].isNull();

		Interval aux1, aux2;
		GStack r1;
		
		if (equalB1 && equalB2 && equalB3 && equalE1 && equalE2 && equalE3 && lengthE2) {
			aux1 = Interval(s1->getMaterial(), nullptr);
			aux2 = Interval(s1[l[0] - 1].getMaterial(), nullptr);

			vector<int> newL = { l[0] - 2, l[1] - 2, l[2] - 2, l[3] - 2 };
			r1 = compact(s1 + 1, s2 + 1, s3 + 1, s4 + 1, newL, cache);
			r.push_back(aux1);
			r.insert(r.end(), r1.begin(), r1.end());
			r.push_back(aux2);

		} else if (equalB1 && equalB2 && equalB3 && lengthE1) {
			aux1 = Interval(s1->getMaterial(), nullptr);

			vector<int> newL = { l[0] - 1, l[1] - 1, l[2] - 1, l[3] - 1 };
			r1 = compact(s1 + 1, s2 + 1, s3 + 1, s4 + 1, newL, cache);
			r.push_back(aux1);
			r.insert(r.end(), r1.begin(), r1.end());
		} else if (equalE1 && equalE2 && equalE3 && lengthE1) {
			aux2 = Interval(s1[l[0] - 1].getMaterial(), nullptr);

			vector<int> newL = { l[0] - 1, l[1] - 1, l[2] - 1, l[3] - 1 };
			r1 = compact(s1, s2, s3, s4, newL, cache);
			r.insert(r.end(), r1.begin(), r1.end());
			r.push_back(aux2);
		} else {

			GStack bestRet;
			r.push_back(Interval(NULL_VALUE, nullptr));
			int bestScore = 0;

			for (int c1 = 1; c1 < l[0]; ++c1) {
				for (int c2 = 1; c2 < l[1]; ++c2) {
					for (int c3 = 1; c3 < l[2]; ++c3) {
						for (int c4 = 1; c4 < l[3]; ++c4) {
							if (s1[c1] == s2[c2] &&
								s3[c3] == s4[c4] &&
								s1[c1] == s3[c3] &&
								!s1[c1].isNull() &&
								!s2[c2].isNull() &&
								!s3[c3].isNull() &&
								!s4[c4].isNull() &&
								std::min(std::min(l[0] - c1, l[1] - c2), std::min(l[2] - c3, l[3] - c4)) > bestScore) {

								vector<int> newL = { l[0] - c1, l[1] - c2, l[2] - c3, l[3] - c4 };

								GStack branchRet = compact(s1 + c1, s2 + c2, s3 + c3, s4 + c4, newL, cache);

								int branchScore = nonUIntervals(branchRet);
								if (branchScore > bestScore) {
									bestScore = branchScore;
									bestRet = branchRet;

								}
							}
						}
					}

				}
			}

			if (bestScore > 0) {
				r.insert(r.end(), bestRet.begin(), bestRet.end());
			}
		}

	}

	cache[DoublePair(IntPair(l[0], l[1]), IntPair(l[2], l[3]))] = r;
	return r;

}


QuadStack::GStack QuadStack::Node::shrink(GStack &newStack, HeightField::Quadrant quadrant) {
	std::vector<Interval*> aux;
	
	int lastUnknowns = 0;

	int newIndex = 0;
	int selfIndex = 0;
	int newCheck = newIndex; // Last wildcard
	int selfCheck = selfIndex; // Last included
	int stackSize = _stack.size();

	int newSize = newStack.size();
	while (!newStack[newSize - 1].isNull())
		newStack[newSize-- - 1].addQuadrant(&_stack[stackSize-- - 1], quadrant);


	while (selfIndex < stackSize) {
		auto& iNew = newStack[newIndex];
		auto& iSelf = _stack[selfIndex];


		if (iNew.isNull()) {
			iNew.addQuadrant(&iSelf, quadrant);

			newCheck = newIndex;
			newIndex = std::min(newIndex + 1, static_cast<int>(newSize - 1));
			selfCheck = ++selfIndex;

			aux.push_back(&iSelf);


		} else if (iNew == iSelf) {
			iNew.addQuadrant(&iSelf, quadrant);

			newIndex++;
			selfIndex++;
		} else {
			
			selfIndex = selfCheck;
			newIndex = newCheck;
		}

	}

	if (aux.empty()) {
		Interval last = _stack.back();
		last.setMaterial(Stack<int>::UNKNOWN_VALUE);
		_stack.clear();
		_stack.push_back(last);
	} else {
		GStack retStack;
		
		for (auto interval : aux)
			retStack.push_back(*interval);

		return retStack;
	}
}

void QuadStack::Node::promote() {

	if (!isLeaf()) {
		if (!_nw->isLeaf())
			_nw->promote();
		if (!_ne->isLeaf())
			_ne->promote();
		if (!_sw->isLeaf())
			_sw->promote();
		if (!_se->isLeaf())
			_se->promote();

		auto nw = _nw;
		auto ne = _ne;
		auto sw = _sw;
		auto se = _se;

		Interval *c1, *c2, *c3, *c4;
		int lengths[4];

		unsigned nwLength = nw->gstackSize();
		unsigned neLength = ne->gstackSize();
		unsigned swLength = sw->gstackSize();
		unsigned seLength = se->gstackSize();

		c1 = &nw->getGStack()[0];
		c2 = &ne->getGStack()[0];
		c3 = &sw->getGStack()[0];
		c4 = &se->getGStack()[0];
		lengths[0] = nwLength;
		lengths[1] = neLength;
		lengths[2] = swLength;
		lengths[3] = seLength;

		Cachemap cache;

		vector<int> l = { lengths[0], lengths[1], lengths[2], lengths[3] };
		_stack = compact(c1, c2, c3, c4, l, cache);
		if (_stack.empty())
			_stack.push_back(Interval(NULL_VALUE, nullptr));

		auto newNw = nw->shrink(_stack, HeightField::Quadrant::NW);
		auto newNe = ne->shrink(_stack, HeightField::Quadrant::NE);
		auto newSw = sw->shrink(_stack, HeightField::Quadrant::SW);
		auto newSe = se->shrink(_stack, HeightField::Quadrant::SE);

		for (auto &interval : _stack)
			interval.merge();

		nw->_stack = newNw;
		ne->_stack = newNe;
		sw->_stack = newSw;
		se->_stack = newSe;
	}

}

void QuadStack::Node::decompose() {

	if (!unify() && divisible()) {

		subdivide();

		_nw->decompose();
		_ne->decompose();
		_sw->decompose();
		_se->decompose();
	}
}

void QuadStack::Node::classify() {
	_compressed = compress();
	
	if (divisible() && !_compressed) {

		subdivide();

		_nw->classify();
		_ne->classify();
		_sw->classify();
		_se->classify();
	}
}


void QuadStack::Node::rearrangeHeightFields(std::vector<std::pair<Interval*, ivec2>> intervals, int& index) {
	for (auto& i : _stack) {

		if (i.isOwner()) {
			i.setHfIndex(index++);
			std::pair<Interval*, ivec2> p(&i, _bb.min);
			intervals.push_back(p);
		} else {

			bool cent = false;
			unsigned x = _bb.min.x;
			unsigned y = _bb.min.y;

			for (auto& entry : intervals) {


				Interval *ihf = entry.first;
				ivec2 min = entry.second;

				int rx = x - min.x;
				int ry = y - min.y;
				if (ihf->getHeight(rx, ry) == i.getHeight(0, 0)) {
					i.setHeightField(ihf->getHeightField());
					i.setHfIndex(ihf->getHfIndex());
					break;
				}
			}

		}
	}
	if (!isLeaf()) {
		if (!_nw->noCompression())
			_nw->rearrangeHeightFields(intervals, index);
		if (!_ne->noCompression())
			_ne->rearrangeHeightFields(intervals, index);
		if (!_sw->noCompression())
			_sw->rearrangeHeightFields(intervals, index);
		if (!_se->noCompression())
			_se->rearrangeHeightFields(intervals, index);
	}
}


std::string QuadStack::Node::print() {
	std::string output = "";

	output += "-------------------------------------------------\n";
	//output += "Level " + std::to_string(_level) + " -> (" + std::to_string(_bb.min.x) + ", " + std::to_string(_bb.min.y) + "), (" + std::to_string(_bb.max.x) +
	//	", " + std::to_string(_bb.max.y) + ")" + "\n";

	output += "Level " + std::to_string(_level) + "\n";
	output += "The material stack (" + std::to_string(_stack.size()) + ") is: \n";

	for (auto& i : _stack) {

		//output += name;
		output += std::to_string(i.getMaterial()) + " - " + std::to_string(i.isOwner()) + ", ";

		output += "\n";


		//if (i.isOwner()) {
		for (int x = 0; x < i.getDimensionX(); ++x) {
			for (int y = 0; y < i.getDimensionY(); ++y) {
				output += std::to_string(i.getHeight(x, y)) + " ";
			}
			output += "\n";

		}

		output += "\n";
		//}


		//const void * address = static_cast<const void*>(i.getHeightField());
		//std::stringstream ss;
		//ss << address;
		//std::string name = ss.str();

		//output += name;

	}


	output += "\n";


	if (!isLeaf()) {
		output += _nw->print();
		output += _ne->print();
		output += _sw->print();
		output += _se->print();
	}

	return output;
}

void QuadStack::Node::updateTerrain(ShortSBR *terrain) {
	_terrain = terrain;

	if (!isLeaf()) {
		_nw->updateTerrain(terrain);
		_ne->updateTerrain(terrain);
		_sw->updateTerrain(terrain);
		_se->updateTerrain(terrain);
	}
}

vec4 QuadStack::Node::memorySize() {
	vec4 accumulatedSize = { 0, 0, 0, 0 };
	if (!noCompression() && _stack.size() > 0) {
		accumulatedSize.x += sizeof(int)* 2;
		for (auto& interval : _stack) {
			accumulatedSize += sizeof(int);

			if (interval.isOwner()) {
				unsigned xSize = _bb.max.x - _bb.min.x;
				unsigned ySize = _bb.max.y - _bb.min.y;
				interval.getHeightField()->getHeightResolution();
				accumulatedSize.z += interval.getHeightField()->memorySizeCompressed();

			}
			accumulatedSize.x += sizeof(int); // pointer to height map
			accumulatedSize.w++;
		}

		if (!isLeaf()) {
			accumulatedSize.y += sizeof(Node*);

			accumulatedSize += _nw->memorySize();
			accumulatedSize += _ne->memorySize();
			accumulatedSize += _sw->memorySize();
			accumulatedSize += _se->memorySize();
		}
	}
	return accumulatedSize;

}



void QuadStack::Node::subdivide() {
	int halfX = (_bb.max.x + _bb.min.x) / 2;
	int halfY = (_bb.max.y + _bb.min.y) / 2;

	ivec2 minNW(_bb.min.x, halfY);
	ivec2 maxNW(halfX, _bb.max.y);

	ivec2 minNE(halfX, halfY);
	ivec2 maxNE(_bb.max);

	ivec2 minSW(_bb.min);
	ivec2 maxSW(halfX, halfY);

	ivec2 minSE(halfX, _bb.min.y);
	ivec2 maxSE(_bb.max.x, halfY);

	_nw = new QuadStack::Node(_level + 1, minNW, maxNW, _terrain);
	_ne = new QuadStack::Node(_level + 1, minNE, maxNE, _terrain);
	_sw = new QuadStack::Node(_level + 1, minSW, maxSW, _terrain);
	_se = new QuadStack::Node(_level + 1, minSE, maxSE, _terrain);

}


int QuadStack::Node::sample(int x, int y, float height, float fatherHeight) {

	Interval currentInterval;
	float currentHeight = fatherHeight;
	int currentMaterial;
	bool hasHeightMap = false;
	bool isOwner = false;


	if (_compressed) {
		auto stackSize = _stack.size();

		for (auto& interval : _stack) {

			int relativeX, relativeY;

			if (interval.isOwner()) {
				relativeX = x - _bb.min.x;
				relativeY = y - _bb.min.y;
			} else {
				auto relativeHM = interval.getRelativeCoordinates();
				relativeX = x - relativeHM.x;
				relativeY = y - relativeHM.y;
			}

			currentMaterial = interval.getMaterial();

			if (interval.hasHeightField()) {
				currentHeight = interval.getHeight(relativeX, relativeY);
			} else // last interval
				currentHeight = fatherHeight;


			if (currentHeight >= height) {
				if (currentMaterial == NULL_VALUE)
					break;
				else
					return currentMaterial;
			}
		}

	}


	if (isLeaf()) {
		if (_terrain) // For the recalculation of the terrain
			return _terrain->getStack(x, y).getAttribute(height);
		else
			return NULL_VALUE;
	}

	else if (_nw->isInside(x, y)) return _nw->sample(x, y, height, currentHeight);
	else if (_ne->isInside(x, y)) return _ne->sample(x, y, height, currentHeight);
	else if (_sw->isInside(x, y)) return _sw->sample(x, y, height, currentHeight);
	else return _se->sample(x, y, height, currentHeight);

}


// Iterator Methods

bool QuadStack::Iterator::next() {
	if (!_node->isLeaf()) {
		_list.push_back(_node->getNW());
		_list.push_back(_node->getNE());
		_list.push_back(_node->getSW());
		_list.push_back(_node->getSE());

	} else if (_list.empty())
		return false;

	_node = _list.front();
	_list.pop_front();
	return true;
}


/**
	QuadStack methods
*/


QuadStack::QuadStack(ShortSBR *terrain) :
_terrain(terrain),
_resolution(terrain->getHeightResolution()),
_root(new QuadStack::Node(0, ivec2(0, 0), ivec2(terrain->getDimension().x, terrain->getDimension().y), terrain)) {

	unsigned maxDimension = std::max(_terrain->getDimension().x, _terrain->getDimension().y);
	float logOf2 = log2(maxDimension);
	bool powerOf2 = ceil(logOf2) == floor(logOf2);
	_maxLevels = powerOf2 ? logOf2 : ceil(logOf2);
	_maxLevels++;
}

void QuadStack::classify() {
	_root->classify();
}


int QuadStack::sample(int x, int y, float height, float fatherHeight) {
	return _root->sample(x, y, height, fatherHeight);
}

void QuadStack::setTerrain(ShortSBR *terrain) {
	_terrain = terrain;
	_root->updateTerrain(terrain);
}

std::string QuadStack::print() {
	return _root->print();
}


vec4 QuadStack::memorySize() const {
	return _root->memorySize();
}


void QuadStack::compressHeightField(float resolution) {
	unsigned block = 8;
	Iterator it = iterator();
	do {
		auto *node = it.data();
		for (auto& interval : node->getGStack()) {
			if (interval.isOwner() && !node->noCompression()) {				
				auto hfPointer = interval.getHeightField();
				HeightFieldCompressor *compressor = new HeightFieldCompressor(hfPointer, block, block, resolution);
				
				hfPointer->setCompressor(compressor);
				compressor->compress();
			}
		}
	} while (it.next());

}

float QuadStack::getHeightResolution() {

	if (_resolution == 0) {
		_resolution = std::numeric_limits<float>::max();
		Iterator it = iterator();

		do {
			auto *node = it.data();
			for (auto& interval : node->getGStack()) {
				if (interval.isOwner() && !node->noCompression()) {
					float diff = interval.getHeightField()->getHeightResolution();
					if (diff < _resolution && diff != 0)
						_resolution = diff;
				}
			}

		} while (it.next());
	}

	if (_resolution == std::numeric_limits<float>::max())
		_resolution = 0;

	return _resolution;
}

QuadStack::~QuadStack() {}

std::ostream& operator<<(std::ostream& os, QuadStack &quadtree) {
	std::string outpuString = "";

	outpuString += "#Dimension \n";
	outpuString += std::to_string(quadtree.getTerrain()->getDimension().x);
	outpuString += " ";
	outpuString += std::to_string(quadtree.getTerrain()->getDimension().y);
	outpuString += "\n";

	outpuString += "#Tree \n";
	outpuString += quadtree.print();

	return os << outpuString;
}

void QuadStack::topDownPhase() {
	_root->decompose();
}

void QuadStack::bottomUpPhase() {
	_root->promote();
}

void QuadStack::rearrangeHeightField() {
	int index = 0;
	_root->rearrangeHeightFields(std::vector<std::pair<QuadStack::Interval*, ivec2>>(), index);
}

/**

QuadStack::Interval methods

*/

QuadStack::Interval::Interval(const QuadStack::Interval& other) :
_material(other._material),
_heightField(other._heightField),
_heightFieldOwner(other._heightFieldOwner),
_relative(other._relative),
_erase(other._erase),
_hfIndex(other._hfIndex) {


}

QuadStack::Interval& QuadStack::Interval::operator=(const QuadStack::Interval& other) {
	if (&other != this) {
		_material = other._material;
		_heightField = other._heightField;
		_heightFieldOwner = other._heightFieldOwner;
		_relative = other._relative;
		_erase = other._erase;
		_hfIndex = other._hfIndex;
	}

	return *this;
}

void QuadStack::Interval::addQuadrant(QuadStack::Interval *subInterval, HeightField::Quadrant quadrant) {

	if (_quadrants.find(quadrant) != _quadrants.end())
		_quadrants[quadrant]->_heightFieldOwner = true;


	subInterval->_heightFieldOwner = false;
	_quadrants[quadrant] = subInterval;
}

void QuadStack::Interval::merge() {

	if (_quadrants.size() == 4) {
		auto nw = _quadrants[HeightField::Quadrant::NW];
		auto ne = _quadrants[HeightField::Quadrant::NE];
		auto sw = _quadrants[HeightField::Quadrant::SW];
		auto se = _quadrants[HeightField::Quadrant::SE];

		vec2 origin;
		origin.x = sw->_heightField->getOriginX();
		origin.y = sw->_heightField->getOriginY();

		vec2 spacing;
		spacing.x = nw->_heightField->getResolution();
		spacing.y = nw->_heightField->getResolution();

		ivec2 dimension;
		dimension.x = nw->getDimensionX() + ne->getDimensionX();
		dimension.y = nw->getDimensionY() + sw->getDimensionY();
		float minHeight = nw->_heightField->getMinHeight();
		float maxHeight = nw->_heightField->getMaxHeight();
		float nullData = nw->_heightField->getNullData();

		_heightField = new HeightField(origin, spacing, dimension, minHeight, maxHeight, nullData, nullptr);

		for (int x = 0; x < nw->getDimensionX(); ++x) {
			for (int y = 0; y < nw->getDimensionY(); ++y) {
				float height = nw->_heightField->getData(x, y);
				_heightField->setData(height, x, sw->getDimensionY() + y);
			}
		}

		for (int x = 0; x < ne->getDimensionX(); ++x) {
			for (int y = 0; y < ne->getDimensionY(); ++y) {
				float height = ne->_heightField->getData(x, y);
				_heightField->setData(height, sw->getDimensionX() + x, sw->getDimensionY() + y);
			}
		}

		for (int x = 0; x < sw->getDimensionX(); ++x) {
			for (int y = 0; y < sw->getDimensionY(); ++y) {
				float height = sw->_heightField->getData(x, y);
				_heightField->setData(height, x, y);
			}
		}

		for (int x = 0; x < se->getDimensionX(); ++x) {
			for (int y = 0; y < se->getDimensionY(); ++y) {
				float height = se->_heightField->getData(x, y);
				_heightField->setData(height, sw->getDimensionX() + x, y);
			}
		}
	}
}