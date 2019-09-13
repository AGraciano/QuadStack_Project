#ifndef HEIGHT_FIELD_H
#define HEIGHT_FIELD_H

#include <glm/glm.hpp>
#include "core/aabb.h"
#include <vector>

/**
Class that encapsulates a heightfield.

@author Alejandro Graciano
*/

using std::vector;
using glm::vec2;
using glm::ivec2;

class HeightFieldCompressor;

class HeightField {

	private:

		vec2 _origin; /** < Coordinates origin */

		vec2 _spacing; /** < Resolution of the cell */

		ivec2 _dimension; /** < Number of cells */

		float _minHeight; /** < Min height boundaries. Usefull for bounding box */

		float _maxHeight; /** < Max height boundaries. Usefull for bounding box */

		float *_data; /** < Data buffer */

		float _nullData; /** < Representation value of void data */

		float _resolution; /** < Resolution of the model in height */

		std::vector<ivec2*> _mipmap; /** < Mipmap levels */

		HeightFieldCompressor *_compressor;

	public:

		enum class Quadrant {
			NW, SW, NE, SE
		};

		HeightField();

		HeightField(vec2 origin, vec2 spacing, ivec2 dimension, float minHeight, float maxHeight,
			float nullData, float *data, HeightFieldCompressor *compressor = nullptr);

		HeightField(const HeightField& other);

		HeightField(const HeightField& other, iaabb2 bb);

		HeightField(const HeightField& other, Quadrant q);

		HeightField& operator=(const HeightField& other);

		// @{
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

		std::string getAttributeName() const { return "Height"; }

		unsigned int getDimensionX() const { return _dimension.x; }

		unsigned int getDimensionY() const { return _dimension.y; }

		float getSpacingX() const { return _spacing.x; }

		float getSpacingY() const { return _spacing.y; }

		float getNullData() const { return _nullData; }

		float getData(unsigned int col, unsigned int row) const { return _data[col + row * _dimension.x]; }

		ivec2 getData(unsigned int col, unsigned int row, unsigned int mipmap) const { return _mipmap[mipmap][col + row * (_dimension.x >> mipmap)]; }

		void computePrediction();

		float getData(vec2 point) const;

		vector<float> getVectorOfData() const { return vector<float>(_data, _data + (_dimension.x * _dimension.y)); }

		//@}

		float getHeightResolution();

		void setData(float height, unsigned int col, unsigned int row) { _data[col + row * _dimension.x] = height; }

		double memorySize() const;

		double memorySizeCompressed() const;

		double memorySizeCompressedDelta() const;

		void computeMipmap();

		bool isCompressed() { return !_compressor; }

		void setCompressor(HeightFieldCompressor *compressor) { _compressor = compressor; }

		friend std::ostream& operator<<(std::ostream& os, HeightField &hm);

		~HeightField();
};



#endif


