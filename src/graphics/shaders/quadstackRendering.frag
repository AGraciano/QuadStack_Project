#version 450 core

layout( location = 0 ) out vec4 fragColor;
layout( location = 1 ) out vec4 fragDepth;
layout( location = 2 ) out vec4 fragNormal;
layout( location = 3 ) out vec4 fragNormalColor;


subroutine void renderPassType();
subroutine uniform renderPassType renderPass;

layout( binding=2 ) buffer hfshaderSSBOMax {
	ivec4 heightsMax[];
};

layout( binding=6 ) buffer hfshaderSSBOMin {
	ivec4 heightsMin[];
};

layout ( binding=3 ) buffer lutSSBO {
	ivec4 intervals[];
};

layout ( binding=4 ) buffer treeSSBO {
	uvec4 nodes[];
};

layout ( binding=5 ) buffer mipmapSSBO {
	ivec2 mmPointers[];
};

// Vertex shader inputs
layout(location = 4) in vec3 entryPoint;
//layout(location = 5) in vec3 rayDir;

// Some constants
const int MAX_SAMPLES = 600;
const float EPSILON = 0.0001;
const float DELTA = 0.001; // for gradient calculation
const int UNKNOWN = -1;
const int LOWEST = -2;
const int NW = 0;
const int NE = 1;
const int SW = 2;
const int SE = 3;
const int MAX_LEVELS = 15;
const int BOUNDS = 2;
const int VALUE = 0;
const int SLICE = 1;
const int MAX = 0;
const int MIN = 1;
const int HIT_HF = 0;
const int HIT_BOUND = 2; 
const int MAX_STACKS = 50;
const int COARSE_STEP = 0;
const int FINE_STEP = 1;
const int FIRST_LEVEL = -2;

const int EVALUATED = 0;
const int OUT = 1;
const int CHILDREN = 2;
const int DEBUG = 3;

const vec4 RED = vec4(1,0,0,1);
const vec4 GREEN = vec4(0,1,0,1);
const vec4 BLUE = vec4(0,0,1,1);
const vec4 YELLOW = vec4(1,1,0,1);
const vec4 BLACK = vec4(0,0,0,1);
const vec3 XAXIS = vec3(1,0,0);
const vec3 YAXIS = vec3(0,1,0);
const vec3 ZAXIS = vec3(0,0,1);

bool DOWNWARDS;

// structs
struct RayContext {
	vec3 position;
	ivec2 projectedIndex;
	float value;
	float upperHeight;
	float lowerHeight;
};

struct LevelStack{
	uint index;
	ivec2 minC;
	ivec2 maxC;
	vec2 minB;
	vec2 maxB;
	//vec2 height;
	ivec3 lowerInterval;
	ivec3 upperInterval;
};

uniform int spp;
// Parameter uniforms
uniform vec3 cameraPosition;
uniform vec3 stepSize;
uniform vec3 mipmapSize;
uniform vec3 minBB;
uniform vec3 maxBB;

// QuadStack parameters
uniform uvec3 root;
uniform uint totalRows;
uniform uint totalCols;
uniform int treeLevels;
uniform int mipmapLevels;
uniform float resolution;
uniform float actualMinHeight;
uniform float actualMaxHeight;
uniform float worldMinHeight;
uniform float worldMaxHeight;
uniform ivec2 blockDim;

// Texture uniforms
uniform int mipmapIndex[MAX_LEVELS];
uniform sampler1D colors;
uniform usamplerBuffer tree;
uniform isamplerBuffer lut;
uniform samplerBuffer heightfields[MAX_LEVELS];

layout( binding=4 ) uniform sampler2D depthTex;
layout( binding=5 ) uniform sampler2D colorTex;
layout( binding=6 ) uniform sampler2D normalTex;

uniform uint numberColors;
uniform int unknownIndex;
uniform mat4 inversemvpMatrix;
uniform mat4 mvpMatrix;
uniform vec2 viewportSize;
uniform vec3 lookAt;
uniform vec3 rightVector;
uniform vec3 upVector; 
uniform uint maxStacks; 

// Light parameters
uniform vec3 ambientColor;
uniform vec3 lightPosition;

uniform vec4 crossPlane; // defined in view-space
uniform bool clippingMode;
uniform float occlusionDistance;
uniform bool downwards;

// Proxy geometry normals
uniform vec3 normals[6];

ivec2 savedCoords[MAX_LEVELS];
float xOffset = (maxBB.x - minBB.x) / totalRows;
float yOffset = (maxBB.z - minBB.z) / totalCols;

float opacityThreshold = 0.01;

// Ray marching parameters
vec3 geomDir = normalize(entryPoint - cameraPosition);
vec3 epsilonVector = vec3(DELTA, DELTA, DELTA);
vec3 invDir = 1.0/geomDir;
vec3 dirStep = geomDir * stepSize;
vec3 blockStep = geomDir * vec3(xOffset);
vec3 mipmapStep[4] = vec3[](dirStep, geomDir * mipmapSize * 2, geomDir * mipmapSize * 4, geomDir * mipmapSize * 8);
const float MAX_STEPS = 100;
float invTotalRows = 1.0 / float(totalRows);
float invTotalCols = 1.0 / float(totalCols);
int mipmap = 0;
bool firstMatch = true;

vec3 setUpDataPos(vec3 geomDir, out vec3 normal){

	vec3 entry = entryPoint + geomDir * EPSILON;
	float distanceEntry = distance(entry, cameraPosition);

	if (geomDir.y > 0) // Upwards
		DOWNWARDS = false;

	return cameraPosition + geomDir * distanceEntry;
}

bool isEqual(float a, float b) {
	return a <= b + EPSILON && a >= b - EPSILON; 
}

bool isEqual(vec3 a, vec3 b) {
	return isEqual(a.x, b.x) && isEqual(a.y, b.y) && isEqual(a.z, b.z); 
}

bool inside(ivec2 pos, ivec2 minPoint, ivec2 maxPoint) {
	return all(lessThanEqual(minPoint, pos)) && all(lessThan(pos, maxPoint));
}

bool inside(ivec2 pos) {
	return all(lessThanEqual(ivec2(0,0), pos)) && all(lessThan(pos, ivec2(totalRows, totalCols)));
}

bool inside(vec2 pos, vec2 minPoint, vec2 maxPoint) {
	return dot(sign(pos - minPoint), sign(maxPoint - pos)) >= 2.0;
}

float insideBox(vec2 v, vec2 bottomLeft, vec2 topRight) {
	vec2 s = step(bottomLeft, v) - step(topRight, v);
	return s.x * s.y;
}

bool onBounds(vec3 pos, vec3 minPoint, vec3 maxPoint) {
	bool minP = any(equal(minPoint, pos));
	bool maxP = any(equal(maxPoint, pos));

	return minP || maxP;
}

bool insideInclusive(vec3 pos, vec3 minPoint, vec3 maxPoint) {
	return dot(sign(pos - minPoint), sign(maxPoint - pos)) >= 0.0;
}

bool insideExclusive(vec3 pos, vec3 minPoint, vec3 maxPoint) {
	return dot(sign(pos - minPoint), sign(maxPoint - pos)) >= 3.0;
}

bool inside(vec3 pos) {
	return insideExclusive(pos, minBB, maxBB);
}

vec4 transferFunction(uint index) {
	return texelFetch(colors, int(index % numberColors), 0);
}

bool earlyTermination(vec3 dataPos) {
	bool stop = !inside(dataPos);
	//if (clippingMode)
	//	stop = stop && sidePlane(dataPos) < 0;

	return stop;
}


ivec2 getTexelFromProjection(vec2 projection, float xOffset, float zOffset, int mipmap) {
	uint coordX = uint((projection.x - minBB.x) / xOffset); 
	uint coordY = uint((projection.y - minBB.z) / zOffset);
	coordX = min(max(0, coordX), (totalRows >> mipmap) - 1);
	coordY = min(max(0, coordY), (totalCols >> mipmap) - 1);

	return ivec2(coordX, coordY);
}


ivec2 getTexelFromProjectionNoBounds(vec2 projection, float xOffset, float zOffset) {
	int coordX = int(floor((projection.x - minBB.x) / xOffset)); 
	int coordY = int(floor((projection.y - minBB.z) / zOffset));

	return ivec2(coordX, coordY);
}

ivec2 getTexelFromProjection(vec2 projection, float xOffset, float zOffset) {
	uint coordX = uint((projection.x - minBB.x) / xOffset); 
	uint coordY = uint((projection.y - minBB.z) / zOffset);
	coordX = min(max(0, coordX), totalRows - 1);
	coordY = min(max(0, coordY), totalCols - 1);

	return ivec2(coordX, coordY);
}

bool isRenderable(float value, out vec4 color) {
	color =  transferFunction(int(value));
	float alpha = color.a;

	return alpha > opacityThreshold && !isEqual(value, float(unknownIndex)) && !isEqual(value, float(LOWEST));
}



ivec3 getInterval(int intervalIndex) {
	return intervals[intervalIndex].xyz; 
}

int spreadBits(int x) {
	int r = x;
	r = (r | (r << 8)) & 0x00FF00FF;
	r = (r | (r << 4)) & 0x0F0F0F0F;
	r = (r | (r << 2)) & 0x33333333;
	r = (r | (r << 1)) & 0x55555555;

	return r;
}

int unSpreadBits(int x) {
	int r = x & 0x55555555;
	r = (r ^ (r >> 1)) & 0x33333333;
	r = (r ^ (r >> 2)) & 0x0F0F0F0F;
	r = (r ^ (r >> 4)) & 0x00FF00FF;
	r = (r ^ (r >> 8)) & 0x0000FFFF;

	return r;
}

int computeMortonCode(ivec2 pos) {
	return spreadBits(pos.x) | (spreadBits(pos.y) << 1);
}

ivec2 decomputeMortonCode(int index) {
	return ivec2(unSpreadBits(index), unSpreadBits(index >> 1));
}

int extractBits(int value, int firstBit, int nBits) {
	return (((1 << nBits) - 1) & (value >>firstBit));
}

int getValue(ivec4 vector, int index) {
	
	if (index == 0)
		return vector.x;
	else if (index == 1)
		return vector.y;
	else if (index == 2)
		return vector.z;
	else
		return vector.w;
}


float evaluateInterval(int pointer, ivec2 coords, ivec2 minCoords, ivec2 dimension, int mipmap, int mipValue) {
	vec2 nodeCoords = vec2(coords - minCoords) / vec2(dimension);
	ivec2 mipmapDim = dimension;
	ivec2 mipmapBlock;
	if (blockDim.x == 0 || blockDim.y == 0)
		return -1;
	mipmapBlock.x = min(mipmapDim.x, blockDim.x);
	mipmapBlock.y = min(mipmapDim.y, blockDim.y);
	
	// Optimize this loop
	for (int i=0; i < mipmap; ++i) {
		mipmapDim = ivec2(max(mipmapDim.x >> 1, 1), max(mipmapDim.y >> 1, 1));

		if (mipmapBlock.x > mipmapDim.x)
			mipmapBlock.x = max(mipmapBlock.x >> 1, 1);

		if (mipmapBlock.y > mipmapDim.y)
			mipmapBlock.y = max(mipmapBlock.y >> 1, 1);
	}

	if (mipmapDim.x <= 1 || mipmapDim.y <= 1) {
		mipmapBlock = ivec2(1, 1);
		
	}

	ivec2 mipCoords = ivec2(floor(nodeCoords * vec2(mipmapDim)));
	
	int blockRows = mipmapDim.x / mipmapBlock.x;
	int blockCols = mipmapDim.y / mipmapBlock.y;

	int index = computeMortonCode(mipCoords);
	int blockIndex = computeMortonCode(ivec2(mipCoords.x / mipmapBlock.x, mipCoords.y / mipmapBlock.y));
	int dataIndex = blockRows * blockCols;

	int hfPointer = mmPointers[pointer + mipmap][mipValue];
	ivec4 header;
	if (mipValue == MAX) 
		header = heightsMax[hfPointer + blockIndex];
	else
		header = heightsMin[hfPointer + blockIndex];

	int base = header.x;
	int bits = header.y;
	int pointers = header.z;

	float heightPercentage = (base - actualMinHeight) / (actualMaxHeight - actualMinHeight);
	float worldBase = heightPercentage * (worldMaxHeight - worldMinHeight) + worldMinHeight;

	if (bits == 0)
		return worldBase;

	int valueInBlock = index % (mipmapBlock.x * mipmapBlock.y);
	int startBit = pointers + valueInBlock * bits;
	int endBit = startBit + bits;
	float sampled;

	uint unpacked = 0;

	int element = startBit / 32;
	int pack;
	if (mipValue == MAX)
		pack = getValue(heightsMax[hfPointer + dataIndex + element / 4] ,element % 4);	
	else
		pack = getValue(heightsMin[hfPointer + dataIndex + element / 4] ,element % 4);	

	if (startBit % 32 > endBit % 32 && endBit % 32 > 0) { // splitted value
		int nBits = 32 - (startBit % 32);
		unpacked = extractBits(pack, 0, nBits);
		unpacked <<= (endBit % 32);
		element = endBit / 32;
		
		if (mipValue == MAX)
			pack = getValue(heightsMax[hfPointer + dataIndex + element / 4] ,element % 4);
		else
			pack = getValue(heightsMin[hfPointer + dataIndex + element / 4] ,element % 4);

		nBits = bits - nBits;
		uint unpacked1 = extractBits(pack, 32 - nBits, nBits);
		unpacked |= unpacked1;
	} else {
		unpacked = (((1 << bits) - 1) & (pack >> (32 - (endBit % 32))));
	}

	sampled = unpacked * resolution + worldBase;
	return sampled; // return max and min values
}

bool between(float m, float m0, float m1) {
	return m0 < m && m1 >= m;
}

bool intersectBox(vec3 orig, vec3 dir, vec3 boxMax, vec3 boxMin, out float tnear, out float tfar) {
	vec3 tmin = (boxMin - orig) * invDir;
	vec3 tmax = (boxMax - orig) * invDir;
	
	vec3 realMin = min(tmin, tmax);
	vec3 realMax = max(tmin, tmax);

	tfar = min(min(realMax.x, realMax.y), realMax.z);
	tnear = max(max(realMin.x, realMin.y), realMin.z);

	return (tfar >= tnear) && tfar > 0;
	//return tfar > 0;
}

bool insideVec2(vec2 n, vec2 nmin, vec2 nmax) {
	bvec2 b0 = bvec2(greaterThanEqual(n, nmin));
	bvec2 b1 = bvec2(lessThan(n, nmax));
	return all(b0) && all(b1);
}

bool insideVec3(vec3 n, vec3 nmin, vec3 nmax) {
	bvec2 b0 = bvec2(greaterThanEqual(n, nmin));
	bvec2 b1 = bvec2(lessThanEqual(n, nmax));
	return all(b0) && all(b1);
}

ivec2 getCoords(vec3 pos) {
	float projectedX = pos.x; // OpenGL textures are upsidedown 
	float projectedZ = pos.z;
	// Project to the indices texture the current position and the next
	return getTexelFromProjection(vec2(projectedX, projectedZ), xOffset, yOffset);
}

ivec2 getCoordsNoBounds(vec3 pos) {
	float projectedX = pos.x; // OpenGL textures are upsidedown 
	float projectedZ = pos.z;
	// Project to the indices texture the current position and the next
	return getTexelFromProjectionNoBounds(vec2(projectedX, projectedZ), xOffset, yOffset);
}

ivec2 getCoords(vec3 pos, int mipmap) {
	float projectedX = pos.x; // OpenGL textures are upsidedown 
	float projectedZ = pos.z;
	// Project to the indices texture the current position and the next
	return getTexelFromProjection(vec2(projectedX, projectedZ), xOffset, yOffset, mipmap);
}

bool insideIvec2(ivec2 n, ivec2 n0, ivec2 n1) {
	bool bn0 = n0.x <= n.x && n0.y <= n.y;
	bool bn1 = n1.x > n.x && n1.y > n.y;
	return bn0 && bn1;
}

bool insideIvec2(ivec2 n) {
	bool bn0 = n.x >= 0 && n.y >= 0;
	bool bn1 = n.x < totalRows && n.y < totalCols;
	return bn0 && bn1;
}

float rayBoxIntersect ( vec3 rpos, vec3 rdir, vec3 vmin, vec3 vmax ) {
   float t[10];
   t[1] = (vmin.x - rpos.x) * invDir.x;
   t[2] = (vmax.x - rpos.x) * invDir.x;
   t[3] = (vmin.y - rpos.y) * invDir.y;
   t[4] = (vmax.y - rpos.y) * invDir.y;
   t[5] = (vmin.z - rpos.z) * invDir.z;
   t[6] = (vmax.z - rpos.z) * invDir.z;
   t[7] = max(max(min(t[1], t[2]), min(t[3], t[4])), min(t[5], t[6]));
   t[8] = min(min(max(t[1], t[2]), max(t[3], t[4])), max(t[5], t[6]));
   t[9] = (t[8] < 0 || t[7] > t[8]) ? -1 : t[7];
   return t[9];
}

bool isLeaf(uvec4 node) {
	return node.z == 0; 
}

bool hasData(ivec2 minB, ivec2 maxB) {
	return maxB.x > minB.x && maxB.y > minB.y;
}

bool hasData(vec2 minB, vec2 maxB) {
	return maxB.x > minB.x && maxB.y > minB.y;
}

bool minLOD(vec2 sampled, int mipmap) {
	return isEqual(sampled[MIN], sampled[MAX]) || mipmap == 0;
}

vec4 getQuadrant(ivec2 coords, ivec2 minCoords, ivec2 maxCoords, vec2 minBounds, vec2 maxBounds, int factor) {
	int rows = maxCoords.x - minCoords.x;
	int cols = maxCoords.y - minCoords.y;
	ivec2 dimensions = ivec2(max(rows >> factor, 1), max(cols >> factor, 1));
	vec2 nodeCoords = vec2(coords - minCoords) / vec2(maxCoords - minCoords);
	ivec2 indices = ivec2(floor(nodeCoords * vec2(dimensions)));

	vec2 spacing = vec2(maxBounds - minBounds) / vec2(dimensions);

	vec2 retMin = minBounds + vec2(indices) * spacing;
	vec2 retMax = retMin + spacing;

	return vec4(retMin, retMax);
}

vec4 getQuadrant(vec2 pos, vec2 minBounds, vec2 maxBounds, int factor) {
	vec2 dimensions = vec2(totalRows >> factor, totalCols >> factor);
	vec2 spacing = (maxBounds - minBounds) / dimensions;
	vec2 indices = (pos - minBounds) / spacing;

	vec2 retMin = minBounds + ivec2(indices) * spacing;
	vec2 retMax = retMin + spacing;

	return vec4(retMin, retMax);
}

void getQuadrant(vec2 pos, int factor, out vec4 bounds, out ivec4 coords) {
	vec2 dimensions = vec2(totalRows >> factor, totalCols >> factor);
	vec2 spacing = (maxBB.xz - minBB.xy) / dimensions;
	vec2 indices = (pos - minBB.xy) / spacing;

	vec2 retMin = minBB.xy + ivec2(indices) * spacing;
	vec2 retMax = retMin + spacing;

	bounds = vec4(retMin, retMax);
	coords = ivec4(indices, indices + dimensions);
}

vec4 getQuadrant(vec2 pos, int factor) {
	vec2 dimensions = vec2(totalRows >> factor, totalCols >> factor);
	vec2 spacing = (maxBB.xz - minBB.xy) / dimensions;
	vec2 indices = (pos - minBB.xy) / spacing;

	vec2 retMin = minBB.xy + ivec2(indices) * spacing;
	vec2 retMax = retMin + spacing;

	return vec4(retMin, retMax);
}

vec2 sampleInterval(LevelStack stack[MAX_LEVELS], vec3 position, ivec3 interval, int mipmap) {
	ivec2 coords = getCoordsNoBounds(position);
	int pointer = int(interval.y);
	int level = int(interval.z);
	ivec2 minCoords = stack[level].minC;
	ivec2 dimension = stack[level].maxC - stack[level].minC;
	vec2 sampleValue;
	sampleValue.x = evaluateInterval(pointer, coords, minCoords, dimension, mipmap, MAX); 
	sampleValue.y = evaluateInterval(pointer, coords, minCoords, dimension, mipmap, MIN); 
	return sampleValue;
}

float sampleIntervalMin(LevelStack stack[MAX_LEVELS], vec3 position, ivec3 interval, int mipmap) {
	ivec2 coords = getCoordsNoBounds(position);
	int pointer = int(interval.y);
	int level = int(interval.z);
	ivec2 minCoords = stack[level].minC;
	ivec2 dimension = stack[level].maxC - stack[level].minC;
	vec2 sampleValue;
	return evaluateInterval(pointer, coords, minCoords, dimension, mipmap, MIN);
}

float sampleIntervalMax(LevelStack stack[MAX_LEVELS], vec3 position, ivec3 interval, int mipmap) {
	ivec2 coords = getCoordsNoBounds(position);
	int pointer = int(interval.y);
	int level = int(interval.z);
	ivec2 minCoords = stack[level].minC;
	ivec2 dimension = stack[level].maxC - stack[level].minC;
	vec2 sampleValue;
	return evaluateInterval(pointer, coords, minCoords, dimension, mipmap, MAX);
}

vec4 castRayInterval(vec3 position, vec4 bounds, float lowerBounds, float upperBounds) {
	vec3 minBBNode = vec3(bounds.x, lowerBounds, bounds.y);
	vec3 maxBBNode = vec3(bounds.z, upperBounds, bounds.w);
	float tnear, tfar;
		
	bool intersects = intersectBox(position, geomDir, maxBBNode, minBBNode, tnear, tfar);

	if (intersects)
		position += geomDir * (tfar + DELTA);
	else // For corners and edges
		position += geomDir * DELTA;


	return vec4(position, intersects);
}

void nextLevel(vec2 pos, ivec4 coords, vec4 bounds, out ivec4 outCoords, out vec4 outBounds, out int child) {
	ivec2 halfCoord = coords.xy + ((coords.zw - coords.xy) / 2);
	vec2 halfBounds = bounds.xy + ((bounds.zw - bounds.xy) / 2);

	// Calculate the children
	ivec2 childrenMin[4];
	childrenMin[NW] = ivec2(coords.x, halfCoord.y); //nw
	childrenMin[NE] = halfCoord; //ne
	childrenMin[SW] = coords.xy; //sw
	childrenMin[SE] = ivec2(halfCoord.x, coords.y); //se

	vec2 childrenMinBounds[4];
	childrenMinBounds[NW] = vec2(bounds.x, halfBounds.y); //nw
	childrenMinBounds[NE] = halfBounds; //ne
	childrenMinBounds[SW] = bounds.xy; //sw
	childrenMinBounds[SE] = vec2(halfBounds.x, bounds.y); //se

	ivec2 childrenMax[4];
	childrenMax[NW] = ivec2(halfCoord.x, coords.w); //nw
	childrenMax[NE] = coords.zw; //ne
	childrenMax[SW] = halfCoord; //sw
	childrenMax[SE] = ivec2(coords.z, halfCoord.y); //se
		
	vec2 childrenMaxBounds[4];
	childrenMaxBounds[NW] = vec2(halfBounds.x, bounds.w); //nw
	childrenMaxBounds[NE] = bounds.zw; //ne
	childrenMaxBounds[SW] = halfBounds; //sw
	childrenMaxBounds[SE] = vec2(bounds.z, halfBounds.y); //se


	if (coords.z - coords.x == 1) {
		childrenMin[NW] = childrenMin[NE];
		childrenMin[SW] = childrenMin[SE];
		childrenMinBounds[NW] = childrenMinBounds[NE];
		childrenMinBounds[SW] = childrenMinBounds[SE];
	}

	if (coords.w - coords.y == 1) {
		childrenMin[NW] = childrenMin[SW];
		childrenMin[NE] = childrenMin[SE];
		childrenMinBounds[NW] = childrenMinBounds[SW];
		childrenMinBounds[NE] = childrenMinBounds[SE];
	}
		
	if (inside(pos, childrenMinBounds[NW], childrenMaxBounds[NW]))
		child = 0;
	else if (inside(pos, childrenMinBounds[NE], childrenMaxBounds[NE]))
		child = 1;
	else if (inside(pos, childrenMinBounds[SW], childrenMaxBounds[SW]))
		child = 2;
	else
		child = 3;

	outCoords = ivec4(childrenMin[child], childrenMax[child]);
	outBounds = vec4(childrenMinBounds[child], childrenMaxBounds[child]);


}

bool isInNode(vec3 pos, vec4 nodeBounds, ivec3 lowerInterval, ivec3 upperInterval, LevelStack stack[MAX_LEVELS]) {
	if (!inside(pos.xz, nodeBounds.xy, nodeBounds.zw))
		return false;

	float lowerSampled = minBB.y;
	if (lowerInterval.x != FIRST_LEVEL)
		lowerSampled = sampleIntervalMax(stack, pos, lowerInterval, 0);

	if (pos.y < lowerSampled)
		return false;

	float upperSampled = maxBB.y;
	if (upperInterval.x != FIRST_LEVEL)
		upperSampled = sampleIntervalMax(stack, pos, upperInterval, 0);

	if (pos.y > upperSampled)
		return false;

	return true;	
}

vec4 accumulateColor(vec4 colorIn, vec4 colorNew, float samplingDist) {
	vec4 colorOut;

	// Opacity correction
	float alphaCorr = 1.0 - pow((1.0 - colorNew.a), stepSize.x / samplingDist);
	colorOut.xyz = colorIn.xyz + colorNew.xyz * alphaCorr * (1.0 - colorIn.a); 
	colorOut.a = colorIn.a + alphaCorr * (1.0 - colorIn.a);
	
	return colorOut; 
}

int evaluateNode(int collisions, LevelStack stack[MAX_LEVELS], uint stackIndex, uvec2 node, vec3 pos, int maxMipmap, vec4 accumColor, out ivec3 outLowerInterval, out ivec3 outUpperInterval, out vec3 outPos, out vec4 color) {
	vec4 nodeBounds = vec4(stack[stackIndex].minB, stack[stackIndex].maxB);
	ivec4 nodeCoords = ivec4(stack[stackIndex].minC, stack[stackIndex].maxC);
	ivec3 lowerInterval = stack[stackIndex].lowerInterval; 
	ivec3 upperInterval = stack[stackIndex].upperInterval; 
	
	outLowerInterval = lowerInterval;

	uint size = int(node.x);
	int firstInterval = int(node.y);

	outPos = pos;

	int i = 0;
	int lastIndex = -1;
	ivec2 coords = getCoordsNoBounds(outPos);
	outUpperInterval = getInterval(firstInterval + int(size) - 1);
	
	int mipmap = maxMipmap;
	int minimumMipmap = 0;
	bool emptySpaceSkip = false;
	float lastSample = minBB.y;
	float fatherLower = minBB.y;
	float fatherUpper = maxBB.y;
	if (lowerInterval[VALUE] != FIRST_LEVEL)
		fatherLower = sampleIntervalMax(stack, outPos, lowerInterval, minimumMipmap);


	ivec3 first = getInterval(firstInterval);
	float firstSampling = sampleIntervalMax(stack, outPos, first, 0);

	ivec3 second = getInterval(firstInterval + 1);
	float secondSampling = sampleIntervalMax(stack, outPos, second, 0);


	while(i < size) {
		collisions++;
		
		// 1. Begin with the static sampling
		ivec3 interval = getInterval(firstInterval + i);

		float mipSampling = sampleIntervalMax(stack, outPos, interval, minimumMipmap);

		if (fatherLower > lastSample && fatherLower < mipSampling && lowerInterval[VALUE] != FIRST_LEVEL)
			outLowerInterval = lowerInterval;

		
		// 2. If success check the type of collision
		// 2.1 If "normal value", return it
		// 2.2 If wildcard, return CHILD
		// 2.3 If empty space, continue the inspection

		if (outPos.y <= mipSampling + EPSILON) {
			vec4 newColor;
			bool isChild = int(interval[VALUE]) == UNKNOWN;
			bool isRender = isRenderable(int(interval[VALUE]), newColor);



			if (isChild) {
				outUpperInterval = interval;
				return CHILDREN;
			}



			// Equation for acumulation. For now, just sample the TF
			if (isRender) {
				color = newColor;
				
				return EVALUATED;
			}  
			
			collisions++;
			float lastHeight;
				
			if (i == 0 && lowerInterval[VALUE] == FIRST_LEVEL )
				lastHeight = minBB.y;
			else
				lastHeight = sampleIntervalMax(stack, outPos, outLowerInterval, mipmap);
				
			float nextHeight = maxBB.y;

			vec4 bounds = getQuadrant(coords, nodeCoords.xy, nodeCoords.zw, nodeBounds.xy, nodeBounds.zw, mipmap);
			if (outPos.y >= lastHeight && outPos.y <= nextHeight) {

				vec4 newPos = castRayInterval(outPos, bounds, lastHeight, nextHeight);
				outPos = newPos.xyz;
				fatherUpper = sampleIntervalMax(stack, outPos, upperInterval, 0);

				coords = getCoordsNoBounds(outPos);

			} else {

				if (mipmap == minimumMipmap) {
					i = 0;
					outLowerInterval = lowerInterval;
					mipmap = maxMipmap;

				} else {
					mipmap = max(mipmap - 1, minimumMipmap);
				}

			}

				
			if (!isInNode(outPos, nodeBounds, lowerInterval, upperInterval, stack))
				return OUT;
			

		} else {
			i++;
			outLowerInterval = interval;
			lastSample = mipSampling;	
			
		} 

	}
	
	return OUT;
}



vec4 traverseQuadStack(vec3 position, out vec3 outPosition) {
	
	uint nodeIndex = 0;
	ivec2 minCoord = ivec2(0, 0);
	ivec2 maxCoord = ivec2(totalRows, totalCols);
	vec2 minBounds = minBB.xz;
	vec2 maxBounds = maxBB.xz;
	vec2 minHeight = vec2(minBB.y, minBB.y);
	uvec4 node;
	int minSlice=-1;
	int maxMipmap = mipmapLevels - 1;
	vec4 color = vec4(0);
	int collisions = 0;
	outPosition = position;
	
	// stack of levels
	int stackIndex = 0;
	LevelStack sl[MAX_LEVELS];
	sl[stackIndex].minC = ivec2(0, 0);
	sl[stackIndex].maxC = ivec2(totalRows, totalCols);
	sl[stackIndex].minB = minBB.xz;
	sl[stackIndex].maxB = maxBB.xz;
	sl[stackIndex].lowerInterval = ivec3(FIRST_LEVEL, 0, 0);
	sl[stackIndex].upperInterval = ivec3(FIRST_LEVEL, 0, 0);
	sl[stackIndex].index = 0;


	bool cent = false;
	int cont = 0;
	for(int i=0; i < MAX_SAMPLES; ++i) {
		cont++;
		collisions++;
		//node = texelFetch(tree, int(sl[stackIndex].index));
		node.xyz = nodes[sl[stackIndex].index].xyz;

		ivec3 outLower;
		ivec3 outUpper;
		int state = evaluateNode(collisions, sl, stackIndex, node.xy, outPosition, max(maxMipmap - stackIndex, 0), color, outLower, outUpper, outPosition, color);

		
		if (!inside(outPosition) || (state == EVALUATED && color.a >= 0.9) || state == DEBUG )
			return color;
		
		if (state == OUT) {
			bool ins;
			do {
				stackIndex--;
				vec4 bounds = vec4(sl[stackIndex].minB, sl[stackIndex].maxB);
				ivec3 lower = sl[stackIndex].lowerInterval;
				ivec3 upper = sl[stackIndex].upperInterval;
				ins = isInNode(outPosition, bounds, lower, upper, sl);
			} while(stackIndex > 0 && !ins);
			//stackIndex = 0;
			continue;


		} 

		// Calculate coordinates
		ivec4 currentCoords = ivec4(sl[stackIndex].minC, sl[stackIndex].maxC);
		vec4 currentBounds = vec4(sl[stackIndex].minB, sl[stackIndex].maxB);
		ivec4 nextCoords;
		vec4 nextBounds;
		int child;

		nextLevel(outPosition.xz, currentCoords, currentBounds, nextCoords, nextBounds, child);

		
		stackIndex++;
		sl[stackIndex].index = node.z + child;
		sl[stackIndex].minC = nextCoords.xy;
		sl[stackIndex].maxC = nextCoords.zw;
		sl[stackIndex].minB = nextBounds.xy;
		sl[stackIndex].maxB = nextBounds.zw;
		sl[stackIndex].lowerInterval = outLower;
		sl[stackIndex].upperInterval = outUpper;

	}
	
	// Debug enum. This part should never be reached
	return RED;
}

float getDepthValue(vec3 point) {
    vec4 projected = mvpMatrix * vec4(point, 1.0);
    float depth = ((projected.z / projected.w) + 1.0) * 0.5;
	
    return depth;
}

vec3 getWorldPoint(vec2 screenCoords, float depth) {
    vec4 clipSpaceLocation;

    clipSpaceLocation.xy = (screenCoords.xy / viewportSize.xy) * 2.0 - 1.0;
    clipSpaceLocation.z = depth * 2.0f - 1.0f;
    clipSpaceLocation.w = 1.0f;

    vec4 homogenousLocation = inversemvpMatrix * clipSpaceLocation;
    vec3 location = vec3(homogenousLocation.xyz / homogenousLocation.w);

    return location;
}

ivec2 getScreenCoords(vec3 point) {
    vec4 clipSpacePos = mvpMatrix * vec4(point, 1.0);
    
    vec3 ndcSpacePos = vec3(clipSpacePos.xyz / clipSpacePos.w);

    ivec2 windowSpacePos = ivec2(((ndcSpacePos.xy + 1.0) * 0.5) * viewportSize);

    return windowSpacePos;
}

vec3 deferredVoxelNormal(vec3 point3D) {
	vec3 sum = vec3(0.0, 0.0, 0.0);
	vec3 offset = vec3(xOffset);

	point3D += offset * 0.5;
	vec3 voxelCenter;
	voxelCenter.xz = vec2(floor(point3D.xz / offset.xz) * offset.xz + offset.xz / 2.0 );
	voxelCenter.y = point3D.y;
	
	for (int xIndex = -2; xIndex<3; xIndex++) {
		for (int yIndex = -2; yIndex<3; yIndex++) {
			for (int zIndex = -2; zIndex<3; zIndex++) {
				vec3 nextPos = voxelCenter + xOffset * vec3(xIndex, yIndex, zIndex);
				ivec2 screenCoords = getScreenCoords(nextPos);
				float realDepth = texelFetch(depthTex, screenCoords, 0).r;
				float supposedDepth = getDepthValue(nextPos);
				
				if (supposedDepth < realDepth) {
					ivec3 destination = ivec3(xIndex, yIndex, zIndex);
					sum += vec3(xIndex, yIndex, zIndex);
				}

			}
		}
	}

	return normalize(sum);
}

vec3 getDiffuseColor(vec3 kd, vec3 G, vec3 L) {
	float GdotL = max(dot(G, L), 0.0);
	return kd * GdotL;
}

vec3 getAmbientColor(vec3 ka) {
	return ka * ambientColor;
}

subroutine (renderPassType)
void shadingPass() {

	ivec2 pixel = ivec2(gl_FragCoord.xy);
	
	vec4 colorTexel= texelFetch(colorTex, pixel, 0).rgba;	
	vec4 normalTexel = texelFetch(normalTex, pixel, 0).rgba;
	vec4 depthTexel = texelFetch(depthTex, pixel, 0).rgba;	
	
	if (depthTexel.a < 0.9) {
		discard;
	}

	vec3 shadedColor = vec3(0.0);
	vec3 point3D = getWorldPoint(pixel, depthTexel.r);
	vec3 normal = normalTexel.rgb;
	if (normalTexel.a < 1.0)
		normal = deferredVoxelNormal(point3D);

	shadedColor += getDiffuseColor(colorTexel.rgb, normal, -geomDir);
	shadedColor += getAmbientColor(colorTexel.rgb);	

	fragColor = vec4(shadedColor, colorTexel.a);	
}

subroutine (renderPassType)
void raycastingPass() {
	RayContext ray;	

	vec3 normal;
	vec3 startPoint = setUpDataPos(geomDir, normal);

	vec3 lastPosition = startPoint;
	ray.position = startPoint;
	vec3 lastEmptyPos = ray.position;
	vec4 color = vec4(0.0);
	vec3 outPosition = vec3(0.0);
	fragColor = traverseQuadStack(ray.position, outPosition);

	// Fill deferred buffers
	float depth = getDepthValue(outPosition);
	if (fragColor.a > 0.00)
		fragDepth = vec4(vec3(depth), 1.0);
	else
		fragDepth = vec4(vec3(1.0,0.0,0.0), 0.5);

	if (isEqual(outPosition, startPoint))
		fragNormal = vec4(normal, 1.0);
	else {
		fragNormal = vec4(0.0,0.0,0.0,0.5);
	}
}


void main() {
	renderPass();
}