#include "quadstackview.h"
#include "../core/heightmipmap.h"
#include "../core/heightfieldcompressor.h"
#include <glm/glm.hpp>
#include <map>
#include <fstream>
#include <chrono>

using std::map;
using glm::uvec2;
using glm::uvec3;
using glm::vec4;
using glm::uvec4;
using glm::ivec4;


const string QuadStackView::SHADER_PATH = "src/graphics/shaders/";

//const unsigned char QuadStackView::ColorTable[NUMBER_OF_COLORS][4] = {
//	{ 0x9e, 0xca, 0xe1, 0xff },
//	{ 0xff, 0xed, 0xa0, 0x00 },
//	{ 0x54, 0xca, 0xb2, 0x00 },
//	{ 0xd2, 0xd2, 0xd2, 0xff },
//	{ 0xbe, 0xff, 0x0c, 0xff },
//	{ 0xaa, 0xc4, 0xf2, 0xff },
//	{ 0x6a, 0x8f, 0xc4, 0xff },
//	{ 0xae, 0xe9, 0xf7, 0xff },
//	{ 0x76, 0x3d, 0x01, 0x00 },
//	{ 0xff, 0xfc, 0x89, 0xff },
//	{ 0xf6, 0xf0, 0x34, 0xff },
//	{ 0xaa, 0xaa, 0xaa, 0xff },
//	{ 0x59, 0x38, 0x07, 0xff },
//	{ 0x94, 0xf4, 0x91, 0xff },
//	{ 0xf0, 0x94, 0x67, 0x00 },
//	{ 0xd3, 0x7e, 0x53, 0xff },
//	{ 0x79, 0x79, 0x79, 0xff },
//	{ 0xaa, 0xae, 0x6e, 0xff }, 
//	{ 0x35, 0x9b, 0xcb, 0xff },
//	{ 0xe1, 0xd5, 0x37, 0xff }
//};

const unsigned char QuadStackView::ColorTable[NUMBER_OF_COLORS][4] = {
	{ 0x9e, 0xca, 0xe1, 0xff },
	{ 0xff, 0xed, 0xa0, 0xff },
	{ 0x54, 0xca, 0xb2, 0xff },
	{ 0xd2, 0xd2, 0xd2, 0xff },
	{ 0xbe, 0xff, 0x0c, 0xff },
	{ 0xaa, 0xc4, 0xf2, 0xff },
	{ 0x6a, 0x8f, 0xc4, 0xff },
	{ 0xae, 0xe9, 0xf7, 0xff },
	{ 0x76, 0x3d, 0x01, 0xff },
	{ 0xff, 0xfc, 0x89, 0xff },
	{ 0xf6, 0xf0, 0x34, 0xff },
	{ 0xaa, 0xaa, 0xaa, 0xff },
	{ 0x59, 0x38, 0x07, 0xff },
	{ 0x94, 0xf4, 0x91, 0xff },
	{ 0xf0, 0x94, 0x67, 0xff },
	{ 0xd3, 0x7e, 0x53, 0xff },
	{ 0x79, 0x79, 0x79, 0xff },
	{ 0xaa, 0xae, 0x6e, 0xff },
	{ 0x35, 0x9b, 0xcb, 0xff },
	{ 0xe1, 0xd5, 0x37, 0xff }
};


void QuadStackView::render() {
	_gl->glBindVertexArray(_vaoHandle);
	_gl->glDrawElements(GL_TRIANGLES, 3 * _faces, GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));
}

void QuadStackView::setViewport(unsigned int width, unsigned int height) {
	_viewportWidth = width;
	_viewportHeight = height;

	_shaderProgram->setUniform("viewportSize", vec2(_viewportWidth, _viewportHeight));
}

ivec4 QuadStackView::init() {
	vector<vec3> points;
	vector<int> indices;

	_minBB = vec3(std::numeric_limits<float>::max());
	_maxBB = vec3(std::numeric_limits<float>::min());

	getVertices(points, indices);

	for (auto point : points) {
		if (point.x > _maxBB.x) _maxBB.x = point.x;
		else if (point.x < _minBB.x) _minBB.x = point.x;

		if (point.y > _maxBB.y) _maxBB.y = point.y;
		else if (point.y < _minBB.y) _minBB.y = point.y;

		if (point.z > _maxBB.z) _maxBB.z = point.z;
		else if (point.z < _minBB.z) _minBB.z = point.z;
	}

	GLuint nVerts = GLuint(points.size());
	_faces = GLuint(indices.size() / 3);

	_gl->glGenVertexArrays(1, &_vaoHandle);
	_gl->glBindVertexArray(_vaoHandle);

	unsigned int handle[2];
	_gl->glGenBuffers(2, handle);

	_gl->glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
	_gl->glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(vec3), &points[0], GL_STATIC_DRAW);
	_gl->glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)NULL + (0)));
	_gl->glEnableVertexAttribArray(0);  // Vertex position


	_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle[1]);
	_gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * _faces * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	_gl->glBindVertexArray(0);

	vector<string> shaderPaths;
	shaderPaths.push_back(string(SHADER_PATH) + "quadstackRendering.vert");
	shaderPaths.push_back(string(SHADER_PATH) + "quadstackRendering.frag");

	compileAndLinkShaders(shaderPaths);

	GLuint programHandle = _shaderProgram->getHandle();

	GLuint colorTexture, treeTexture, lutTexture, hfMetaTexture, hfTextures[MAX_LEVELS], boundsTexture;

	_gl->glGenTextures(1, &colorTexture);
	_gl->glGenTextures(1, &treeTexture);
	_gl->glGenTextures(1, &lutTexture);
	_gl->glGenTextures(MAX_LEVELS, hfTextures);
	_gl->glGenTextures(1, &boundsTexture);
	GLAccessible::checkForOpenGLError(__FILE__, __LINE__);

	// Transfer Function
	_shaderProgram->bindTexture(GL_TEXTURE0, GL_TEXTURE_1D, colorTexture);
	_gl->glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	_gl->glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	_gl->glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, NUMBER_OF_COLORS, 0, GL_RGBA, GL_UNSIGNED_BYTE, ColorTable);
	_shaderProgram->setUniform("colors", 0);
	GLAccessible::checkForOpenGLError(__FILE__, __LINE__);


	// Tree index
	unsigned levels = _quadstack->getMaxLevels();
	vector<uvec3> treeNodes;
	vector<vec4> nodeBounds;
	vector<ivec3> lutData;
	map<int, ivec2> hfMetadata; // pointer, index

	unsigned intervalsIndex = 0;
	unsigned hfIndex = 0;
	unsigned pointerIndex = 1;
	unsigned hfRows = _quadstack->getHfRows();
	unsigned hfCols = _quadstack->getHfCols();
	int maxMipmap = std::floor(std::log2(std::max(hfRows, hfCols))) + 1;

	auto worldMinHeight = _minBB.y;
	auto worldMaxHeight = _maxBB.y;

	auto actualMaxHeight = vec2(_quadstack->getMaxHeight(), _quadstack->getMaxHeight());
	auto actualMinHeight = vec2(_quadstack->getMinHeight(), _quadstack->getMinHeight());

	// root insertion
	QuadStack::Iterator it = _quadstack->iterator();
	uvec3 root;
	unsigned nodesSize = 0;
	map<int, std::pair<int, float>> filtering;

	unsigned currentLevel = 0;

	vector<int> hfDataMin, hfDataMax;
	vector<ivec2> hfPointers(levels);
	vector<ivec2> mmPointers;

	int hfPointer = 0;
	float resolution = _quadstack->getHeightResolution();
	
	unsigned hDimension = (actualMaxHeight.x - actualMinHeight.x) / resolution;
	float worldResolution = hDimension == 0 ? hDimension : (worldMaxHeight - worldMinHeight) / hDimension;
	int blockSizeX = 8;
	int blockSizeY = 8;

	float gpuSizeHf = 0;
	float gpuSizeRawHf = 0;
	float gpuSizeRawMM = 0;
	float gpuSizeQs = 0;
	float gpuSizeMM = 0;

	float gpuSizeHf1 = 0;
	float gpuSizeRawHf1 = 0;
	float gpuSizeRawMM1 = 0;
	float gpuSizeQs1 = 0;
	float gpuSizeMM1 = 0;

	auto start = std::chrono::system_clock::now();
	int intervalCount = 0;

	do {
		auto node = it.data();
		uvec3 treeNode{ 0, 0, 0 };

		auto stack = node->getGStack();
		treeNode.x = stack.size();
		treeNode.y = intervalsIndex;
		intervalsIndex += stack.size();

		int minRow = node->getBoundingBox().min.x;
		int minCol = node->getBoundingBox().min.y;

		for (auto& interval : stack) {
			int index, levelIndex;
			float minHeight = std::numeric_limits<float>::max();
			float maxHeight = std::numeric_limits<float>::lowest();

			int blockMipmapX = blockSizeX;
			int blockMipmapY = blockSizeY;
			if (interval.isOwner() && !node->noCompression()) {
				currentLevel = node->getLevel();

				gpuSizeRawHf1 += interval.getHeightField()->memorySize();

				HeightMipmap maxMipmap(interval.getHeightField(), MipmapMode::MAX);
				HeightMipmap minMipmap(interval.getHeightField(), MipmapMode::MIN);

				maxMipmap.computeMipmap();
				minMipmap.computeMipmap();

				// Heightfield insertion
				int nRow = interval.getDimensionX();
				int nCol = interval.getDimensionY();

				int realNRow = ceil(hfRows / pow(2.0, currentLevel));
				int realNCol = ceil(hfCols / pow(2.0, currentLevel));
				int mipmapLevels = std::floor(std::log2(std::max(realNRow, realNCol))) + 1;

				int slice = hfPointer;

				gpuSizeRawHf += sizeof(short)* nRow * nCol;

				for (int mipIndex = 0; mipIndex < mipmapLevels; ++mipIndex) {
					hfPointers[mipIndex].x = hfDataMax.size();
					hfPointers[mipIndex].y = hfDataMin.size();

					unsigned row = std::max(nRow >> mipIndex, 1);
					unsigned col = std::max(nCol >> mipIndex, 1);

					if (blockMipmapX > row)
						blockMipmapX = std::max(blockMipmapX >> 1, 1);

					if (blockMipmapY > col)
						blockMipmapY = std::max(blockMipmapY >> 1, 1);

					int blockRows = row / blockMipmapX;
					int blockCols = col / blockMipmapY;

					auto hfMin = minMipmap.getHeightField(mipIndex);
					auto hfMax = maxMipmap.getHeightField(mipIndex);
					HeightFieldCompressor compressorMin(hfMin, blockMipmapY, blockMipmapX, resolution);
					HeightFieldCompressor compressorMax(hfMax, blockMipmapY, blockMipmapX, resolution);
					compressorMin.compress();
					compressorMax.compress();

					if (mipIndex > 0) {
						gpuSizeMM1 += compressorMin.memorySize();
						gpuSizeMM1 += compressorMax.memorySize();
						gpuSizeMM1 += sizeof(int)* 2; // pointers
					} else {
						gpuSizeHf1 += compressorMax.memorySize();
					}
					
					// Max mipmap
					int nBlocks = compressorMax.blockSize();
					bool compressed = compressorMax.compressed();
					for (int i = 0; i < nBlocks; ++i) {
						int base = compressorMax.getBaseValue(i);
						int bits = compressed ? compressorMax.getBit(i) : 0;
						int pointer = compressed ? compressorMax.getPointers(i) : 0;

						hfDataMax.push_back(base);
						hfDataMax.push_back(bits);
						hfDataMax.push_back(pointer);
						hfDataMax.push_back(0);
					}
									   
					auto vectorData = compressorMax.getData();
					hfDataMax.insert(hfDataMax.end(), vectorData.begin(), vectorData.end());

					// We count the 0 max mipmap as original hf 
					if (mipIndex == 0)
						gpuSizeHf += (nBlocks * 5 / 8) + (nBlocks + vectorData.size()) * sizeof(int);
					else {
						gpuSizeMM += (nBlocks * 5 / 8) + (nBlocks * 2 + vectorData.size()) * sizeof(int);
						gpuSizeRawMM += (sizeof(short) * row * col) * 2;

					}

					int size = 4 - hfDataMax.size() % 4;
					for (int i = 0; size != 4 && i < size; ++i)
						hfDataMax.push_back(0);

					// Min mipmap
					nBlocks = compressorMin.blockSize();
					compressed = compressorMin.compressed();
					for (int i = 0; i < nBlocks; ++i) {
						int base = compressorMin.getBaseValue(i);
						int bits = compressed ? compressorMin.getBit(i) : 0;
						int pointer = compressed ? compressorMin.getPointers(i) : 0;

						hfDataMin.push_back(base);
						hfDataMin.push_back(bits);
						hfDataMin.push_back(pointer);
						hfDataMin.push_back(0);
					}


					vectorData = compressorMin.getData();
					hfDataMin.insert(hfDataMin.end(), vectorData.begin(), vectorData.end());

					if (mipIndex > 0)
						gpuSizeMM += (nBlocks * 5 / 8) + (nBlocks * 2 + vectorData.size()) * sizeof(int);


					size = 4 - hfDataMin.size() % 4;
					for (int i = 0; size != 4 && i < size; ++i)
						hfDataMin.push_back(0);

				}

				index = hfPointer;

				for (int i = 0; i < mipmapLevels; ++i)
					mmPointers.push_back(hfPointers[i] / 4);

				hfPointer += mipmapLevels;

				levelIndex = currentLevel;
				hfMetadata[interval.getHfIndex()] = ivec2(slice, currentLevel);
			} else {
				ivec2 indices = hfMetadata[interval.getHfIndex()];
				index = indices.x;
				levelIndex = indices.y;

			}
			gpuSizeQs1 += sizeof(short) / 2 + sizeof (int) + 4 / 8;
			ivec3 intervalData{ interval.getMaterial(), index, levelIndex };
			lutData.push_back(intervalData);
		}

		if (!node->isLeaf()) {
			treeNode.z = pointerIndex;
			pointerIndex += 4;
		}

		gpuSizeQs1 += 6 / 8 + sizeof(int)* 2;
		treeNodes.push_back(treeNode);
		nodesSize++;
		
	} while (it.next());

	GLsizeiptr bufferSize;

	GLuint heightfieldsMaxSSBO, heightfieldsMinSSBO, lutSSBO, treeSSBO, pointersSSBO;
	_gl->glGenBuffers(1, &heightfieldsMaxSSBO);
	_gl->glGenBuffers(1, &heightfieldsMinSSBO);
	_gl->glGenBuffers(1, &treeSSBO);
	_gl->glGenBuffers(1, &lutSSBO);
	_gl->glGenBuffers(1, &pointersSSBO);

	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

	bufferSize = sizeof(int)* hfDataMax.size();
	_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, heightfieldsMaxSSBO);
	_gl->glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, hfDataMax.data(), GL_STATIC_DRAW);
	_gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, heightfieldsMaxSSBO);
	_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	
	bufferSize = sizeof(int)* hfDataMin.size();
	_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, heightfieldsMinSSBO);
	_gl->glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, hfDataMin.data(), GL_STATIC_DRAW);
	_gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, heightfieldsMinSSBO);
	_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Using 4 components instead of two for SSBO layout
	bufferSize = sizeof(ivec4)* lutData.size();
	_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, lutSSBO);
	_gl->glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);
	_gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lutSSBO);
	gpuSizeQs += (sizeof(int)+sizeof(byte)) * lutData.size();
	
	ivec4 *intervals = (ivec4 *)_gl->glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, bufMask);
	for (int i = 0; i < lutData.size(); ++i) {
		intervals[i].x = lutData[i].x;
		intervals[i].y = lutData[i].y;
		intervals[i].z = lutData[i].z;
	}
	
	_gl->glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	bufferSize = sizeof(uvec4)* treeNodes.size();
	_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, treeSSBO);
	_gl->glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);
	_gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, treeSSBO);
	gpuSizeQs += sizeof(ivec2)* treeNodes.size();

	uvec4 *nodes = (uvec4 *)_gl->glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, bufMask);
	for (int i = 0; i < treeNodes.size(); ++i) {
		nodes[i].x = treeNodes[i].x;
		nodes[i].y = treeNodes[i].y;
		nodes[i].z = treeNodes[i].z;
	}

	_gl->glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	bufferSize = sizeof(ivec2)* mmPointers.size();
	_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointersSSBO);
	_gl->glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, mmPointers.data(), GL_STATIC_DRAW);
	_gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, pointersSSBO);
	//gpuSizeMM1 += bufferSize;

	_gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	_gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	_shaderProgram->setUniform("maxStacks", _quadstack->getTerrain()->getMaxStack());
	_shaderProgram->setUniform("mipmapLevels", maxMipmap);
	_shaderProgram->setUniform("unknownIndex", UNKNOWN_INDEX);
	_shaderProgram->setUniform("numberColors", NUMBER_OF_COLORS);
	_shaderProgram->setUniform("maxLevels", levels);
	_shaderProgram->setUniform("ambientColor", vec3(0.2));
	_shaderProgram->setUniform("viewportSize", vec2(_viewportWidth, _viewportHeight));
	_shaderProgram->setUniform("lightPosition", vec3(10.0f, 10.0f, 10.0f));
	_shaderProgram->setUniform("resolution", worldResolution);
	_shaderProgram->setUniform("actualMinHeight", actualMinHeight.x);
	_shaderProgram->setUniform("actualMaxHeight", actualMaxHeight.x);
	_shaderProgram->setUniform("worldMinHeight", worldMinHeight);
	_shaderProgram->setUniform("worldMaxHeight", worldMaxHeight);
	_shaderProgram->setUniform("blockDim", ivec2(blockSizeX, blockSizeY));

	// Set up the subroutine indexes
	_raycastingPass = _gl->glGetSubroutineIndex(programHandle, GL_FRAGMENT_SHADER, "raycastingPass");
	_shadingPass = _gl->glGetSubroutineIndex(programHandle, GL_FRAGMENT_SHADER, "shadingPass");

	// Texture setup
	_shaderProgram->setUniform("totalRows", hfRows);
	_shaderProgram->setUniform("totalCols", hfCols);


	_shaderProgram->setUniform("minBB", _minBB);
	_shaderProgram->setUniform("maxBB", _maxBB);

	auto size = (_maxBB.x - _minBB.x) / (hfCols * 6.0);

	_shaderProgram->setUniform("stepSize", vec3(size));
	
	GLfloat normals[6 * 3];

	vec3 n0 = normalize(vec3(_minBB - vec3(_maxBB.x, _minBB.y, _minBB.z)));
	vec3 n1 = normalize(vec3(_maxBB - vec3(_minBB.x, _maxBB.y, _maxBB.z)));
	vec3 n2 = normalize(vec3(_minBB - vec3(_minBB.x, _maxBB.y, _minBB.z)));
	vec3 n3 = normalize(vec3(_maxBB - vec3(_maxBB.x, _minBB.y, _maxBB.z)));
	vec3 n4 = normalize(vec3(_minBB - vec3(_minBB.x, _minBB.y, _maxBB.z)));
	vec3 n5 = normalize(vec3(_maxBB - vec3(_maxBB.x, _maxBB.y, _minBB.z)));

	normals[0] = n0.x; normals[1] = n0.y; normals[2] = n0.z;
	normals[3] = n1.x; normals[4] = n1.y; normals[5] = n1.z;
	normals[6] = n2.x; normals[7] = n2.y; normals[8] = n2.z;
	normals[9] = n3.x; normals[10] = n3.y; normals[11] = n3.z;
	normals[12] = n4.x; normals[13] = n4.y; normals[14] = n4.z;
	normals[15] = n5.x; normals[16] = n5.y; normals[17] = n5.z;

	_gl->glUniform3fv(_gl->glGetUniformLocation(_shaderProgram->getHandle(), "normals"), 6, normals);

	GLAccessible::checkForOpenGLError(__FILE__, __LINE__);

	return ivec4{ gpuSizeQs1, gpuSizeHf1, gpuSizeMM1, gpuSizeRawHf };
}

void QuadStackView::getVertices(vector<vec3> &points, vector<int> &indices) {
	float worldDistanceX, worldDistanceY, worldDistanceZ;
	float maxDistance;

	auto distanceX = abs(_quadstack->getTerrain()->getBoundX() - _quadstack->getTerrain()->getOriginX());
	auto distanceY = abs(_quadstack->getTerrain()->getBoundY() - _quadstack->getTerrain()->getOriginY());
	auto distanceZ = abs(_quadstack->getTerrain()->getMaxHeight() - _quadstack->getTerrain()->getMinHeight());

	if (distanceX > distanceY && distanceX > distanceZ)
		maxDistance = distanceX;
	else if (distanceY > distanceZ)
		maxDistance = distanceY;
	else
		maxDistance = distanceZ;

	float heightScale = 1;
	float fMaxDistance = static_cast<float>(maxDistance);
	worldDistanceX = distanceX / fMaxDistance;
	worldDistanceY = distanceY / fMaxDistance;
	worldDistanceZ = distanceZ / fMaxDistance * heightScale;

	auto halfWorldDistanceX = worldDistanceX / 2;
	auto halfWorldDistanceY = worldDistanceY / 2;
	auto halfWorldDistanceZ = worldDistanceZ / 2;

	points.push_back(vec3(-halfWorldDistanceX, -halfWorldDistanceZ, halfWorldDistanceY));
	points.push_back(vec3(halfWorldDistanceX, -halfWorldDistanceZ, halfWorldDistanceY));
	points.push_back(vec3(-halfWorldDistanceX, halfWorldDistanceZ, halfWorldDistanceY));
	points.push_back(vec3(halfWorldDistanceX, halfWorldDistanceZ, halfWorldDistanceY));
	points.push_back(vec3(-halfWorldDistanceX, -halfWorldDistanceZ, -halfWorldDistanceY));
	points.push_back(vec3(halfWorldDistanceX, -halfWorldDistanceZ, -halfWorldDistanceY));
	points.push_back(vec3(-halfWorldDistanceX, halfWorldDistanceZ, -halfWorldDistanceY));
	points.push_back(vec3(halfWorldDistanceX, halfWorldDistanceZ, -halfWorldDistanceY));

	indices = { 0, 1, 2, 1, 3, 2, 0, 4, 1, 4, 5, 1, 0, 6, 4, 0, 2, 6, 1, 5, 3, 7, 3, 5, 3, 7, 6, 2, 3, 6, 6, 5, 4, 7, 5, 6 };

}

QuadStackView::~QuadStackView() {}
