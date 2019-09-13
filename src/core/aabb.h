#ifndef AABB_H
#define AABB_H

#include <glm\glm.hpp>

using glm::ivec3;
using glm::vec3;
using glm::ivec2;
using glm::vec2;


/**
Simple class that represents an Axis Aligned Bounding Box

@author Alejandro Graciano
*/

template<class T>
class AABB {

public:
	T min, max;

	AABB() {}
	AABB(T p1, T p2) : min(p1), max(p2) {}
	AABB(const AABB& other) : min(other.min), max(other.max) {}
	AABB& operator=(const AABB& other) { max = other.max; min = other.min; return *this; }
	bool operator==(const AABB& other) { return min == other.min && max == other.max; }

	unsigned getSpacingX() { return max.x - min.x; }
	unsigned getSpacingY() { return max.y - min.y; }
	unsigned getSpacingZ() { return max.z - min.z; }

	~AABB() {};
};


using iaabb2 = AABB<ivec2>;
using faabb2 = AABB<vec2>;

#endif