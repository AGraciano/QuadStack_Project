#ifndef STACK_H
#define STACK_H

#include <vector>

using std::vector;

/**
	Struct that defines an interval
*/
template<typename T>
struct Interval {
	float _accumulatedHeight; /**< Height in which the stack ends */
	T _attribute; /**< Attribute contained in the stack */

};

/**
	Class that encapsulates a cell of the Stack-based terrain.
	It uses a template for the type of attribute in cell.

	@author Alejandro Graciano
*/

template<class T>
class Stack {

private:

	/**
	Interval stack
	*/
	vector<Interval<T>> _stack;

	/**
	Copy constructor disabled
	*/
	Stack(Stack<T>& other);

	/**
	Copy assignment operator disabled
	*/
	Stack<T>& operator=(Stack<T>& other);

public:

	/**
		Static consts
	*/
	static const int UNKNOWN_VALUE = 999;


	/**
	Default constructor
	*/
	Stack();


	/**
	Constructor with fixed size
	*/
	Stack(short size);


	/**
	Move constructor
	*/
	Stack(Stack<T>&& other);


	/**
	Move assignment operator
	*/
	Stack<T>& operator=(Stack<T>&& other);


	/**
	Return the total height of stack
	*/
	float totalHeight();


	/**
	Method to add an interval to the stack from a height
	*/
	void addInterval(T material, float currentHeight);


	/**
	Getter method. Returns the attribute to a specific height
	*/
	T getAttribute(float height);


	/**
	Getter method. Returns the intervals
	*/
	vector<Interval<T>>& getIntervals() { return _stack; }

	/**
	Method that returns the memory size in MBytes of the structure.
	*/
	double memorySize() const;


	/**
	Reserves memory for nIntervals
	*/
	void reserveIntervals(short nIntervals);


	/**
	Eliminates the memory unneeded
	*/
	void compactIntervals();


	/**
	Sorts intervals by height
	*/
	void sortIntervals();


	/**
	Empty stacks checking
	*/
	bool empty();


	/**
	Check if the stack only contains a unknown interval
	*/
	bool unknown();


	/**
	Computes the similarity between two stacks
	*/
	Stack<T> compare(const Stack<T>& other, float& similarity, float error);


	/**
	Returns the material at the top
	*/
	T topMaterial();

	/**
	Returns an interval given its index within the stack
	*/
	Interval<T>& getAttributeAtIndex(unsigned int index) { return _stack[index]; }

	/**
	Remove all the intervals within the stack
	*/
	void clear() { _stack.clear(); }

	/**
	Compare operator
	*/
	bool operator==(const Stack<T>& other) const;

	/**
	Comparison of the attributes
	*/
	bool compareAttributes(const Stack<T>& other) const;

	/**
	Destructor
	*/
	~Stack();

};


template<class T>
Stack<T>::Stack() {
}

template<class T>
Stack<T>::Stack(Stack<T>&& other) :
_stack(other._stack) {

	other._stack = nullptr;
}

template<class T>
Stack<T>::Stack(short size) :
_stack(new vector<Interval<T>>(size)) {
}


template<class T>
Stack<T>& Stack<T>::operator=(Stack<T>&& other) {
	delete _stack;

	_stack = other._stack;
	other._stack = nullptr;

	return *this;
}

//template<class T>
//void Stack<T>::addInterval(T material, float currentHeight) {
//	if (!_stack.empty() && _stack.back()->_attribute == material) {
//		_stack.back()->_accumulatedHeight = currentHeight;
//	} else {
//		Interval<T> newInterval = { currentHeight, material };
//		_stack.push_back(newInterval);
//	}
//}

template<class T>
void Stack<T>::addInterval(T material, float currentHeight) {

	if (_stack.empty() || currentHeight >= _stack.back()._accumulatedHeight) {
		if (!_stack.empty() && _stack.back()._attribute == material) {
			_stack.back()._accumulatedHeight = currentHeight;
		} else {
			Interval<T> newInterval = { currentHeight, material };
			_stack.push_back(newInterval);
		}
	} else {
		vector<Interval<T>> auxStack;
		while (currentHeight < _stack.back()._accumulatedHeight) {
			auxStack.push_back(_stack.back());
			_stack.pop_back();
		}
		addInterval(material, currentHeight);
		for (int i = 0; i < auxStack.size(); ++i) {
			_stack.push_back(auxStack.back());
			auxStack.pop_back();
		}
	}
}

template<class T>
T Stack<T>::getAttribute(float height) {

	for (Interval<T>& i : _stack)
	if (i._accumulatedHeight >= height)
		return i._attribute;

	return UNKNOWN_VALUE;
}

template<class T>
double Stack<T>::memorySize() const {
	return sizeof(Interval<T>) * _stack.size();
	//return (sizeof(short)+sizeof(short) )* _stack.size();
}

template<class T>
void Stack<T>::reserveIntervals(short nIntervals) {
	if (_stack.empty())
		_stack.clear();

	_stack.resize(nIntervals);

}

template<class T>
void Stack<T>::compactIntervals() {
	_stack.shrink_to_fit();
}

template<class T>
void Stack<T>::sortIntervals() {

	sort(_stack.begin(), _stack.end(),
		[](const Interval<T> & a, const Interval<T> & b) -> bool {
		return a._accumulatedHeight < b._accumulatedHeight;
	});

}

template<class T>
float Stack<T>::totalHeight() {

	auto size = _stack.size();
	if (_stack.back()->_attribute == UNKNOWN_VALUE)
		return (*_stack)[size - 2]._accumulatedHeight;

	return _stack.back()->_accumulatedHeight;
}


template<class T>
Stack<T> Stack<T>::compare(const Stack<T>& other, float& similarity, float error) {
	Stack<T> representative;
	similarity = 0;
	auto& myIntervals = getIntervals();
	auto& otherIntervals = other.getIntervals();

	if (myIntervals.size() != otherIntervals.size())
		return Stack<T>();

	for (auto& interval : intervals) {
		auto myInterval = myIntervals[i];
		auto otherInterval = otherIntervals[i];

		if (myInterval._attribute == otherInterval._attribute) {
			float myRelativeHeight, otherRelativeHeight;
			if (i == 0) {
				myRelativeHeight = myInterval._accumulatedHeight;
				otherRelativeHeight = otherInterval._accumulatedHeight;
			} else {
				myRelativeHeight = myInterval._accumulatedHeight - myIntervals[i - 1]._accumulatedHeight;
				otherRelativeHeight = otherInterval._accumulatedHeight - otherIntervals[i - 1]._accumulatedHeight;
			}
			float upperHeight, lowerHeight;

			if (otherInterval._accumulatedHeight > myInterval._accumulatedHeight) {
				upperHeight = otherInterval._accumulatedHeight;
				lowerHeight = myInterval._accumulatedHeight;
			} else {
				lowerHeight = otherInterval._accumulatedHeight;
				upperHeight = myInterval._accumulatedHeight;
			}

			float averageHeight = (upperHeight + lowerHeight) / 2;
			representative.addInterval(myInterval._attribute, averageHeight);


			similarity += max(abs(myRelativeHeight - otherRelativeHeight) - error, 0.0f);
		} else {
			similarity = 0;
			return Stack<T>();
		}
	}

	float halfTotalHeight = (totalHeight() + other.totalHeight()) / 2;
	float norm = min(similarity / halfTotalHeight, 1.0f);

	similarity = 1.0 - norm;

	return representative;

}

template<class T>
bool Stack<T>::compareAttributes(const Stack<T>& other) const {
	short size1 = _stack.size();
	short size2 = other._stack.size();

	if (size1 != size2)
		return false;

	for (int i = 0; i < size1; ++i) {
		auto& interval1 = _stack[i];
		auto& interval2 = other._stack[i];

		if (interval1._attribute != interval2._attribute)
			return false;
	}


	return true;
}

template<class T>
bool Stack<T>::operator==(const Stack<T>& other) const {
	auto size1 = _stack.size();
	auto size2 = other._stack.size();

	if (size1 != size2)	return false;

	for (int i = 0; i < size1; ++i) {
		auto& interval1 = stack[i];
		auto& interval2 = other._stack[i];

		if (interval1._accumulatedHeight != interval2._accumulatedHeight ||
			interval1._attribute != interval2._attribute)
			return false;
	}

	return true;
}

template<class T>
T Stack<T>::topMaterial() {
	return (*_stack)[_stack.size() - 1]._attribute;
}

template<class T>
bool Stack<T>::empty() {
	return _stack.empty();
}

template<class T>
bool Stack<T>::unknown() {
	return (_stack.size() == 1 && _stack.begin()->_attribute == UNKNOWN_VALUE);
}

template<class T>
Stack<T>::~Stack() {
}





#endif