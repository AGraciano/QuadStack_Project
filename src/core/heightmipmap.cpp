#include "heightmipmap.h"
#include <algorithm>
#include <iostream>

using glm::ivec2;

void HeightMipmap::computeMipmap() {
	auto dimension = ivec2{ _heightField->getDimensionX(), _heightField->getDimensionY() };
	auto origin = vec2{ _heightField->getOriginX(), _heightField->getOriginY() };
	auto spacing = vec2{ _heightField->getSpacingX(), _heightField->getSpacingY() };
	auto minHeight = _heightField->getMinHeight();
	auto maxHeight = _heightField->getMaxHeight();
	auto nullData = _heightField->getNullData();

	int maxMipmap = std::floor(std::log2(std::max(dimension.x, dimension.y))) + 1;

	_mipmap.push_back(_heightField);

	for (int l = 1; l < maxMipmap; ++l) {
		ivec2 mipDimension;
		mipDimension.x = std::max(dimension.x >> l, 1);
		mipDimension.y = std::max(dimension.y >> l, 1);

		HeightField *mp = new HeightField(origin, spacing, mipDimension, minHeight, maxHeight, nullData, nullptr);

		_difference.push_back(new HeightField(origin, spacing, mipDimension, minHeight, maxHeight, nullData, nullptr));

		_mipmap.push_back(mp);

		for (int x = 0; x < std::max(1, dimension.x >> l); ++x) {
			float x0 = x << 1;
			float x1 = std::min(x0 + 1, (float)std::max(1, dimension.x >> (l - 1)) - 1);

			for (int y = 0; y < std::max(1, dimension.y >> l); ++y) {
				float y0 = y << 1;
				float y1 = std::min(y0 + 1, (float)std::max(1, dimension.y >> (l - 1)) - 1);


				float mip1 = _mipmap[l - 1]->getVectorOfData()[(int)x1 + (int)y0 * (dimension.x >> (l - 1))];
				float mip2 = _mipmap[l - 1]->getVectorOfData()[(int)x1 + (int)y1 * (dimension.x >> (l - 1))];
				float mip3 = _mipmap[l - 1]->getVectorOfData()[(int)x0 + (int)y0 * (dimension.x >> (l - 1))];
				float mip4 = _mipmap[l - 1]->getVectorOfData()[(int)x0 + (int)y1 * (dimension.x >> (l - 1))];
				float finalMip;
				if (_mode == MipmapMode::MAX) {
					finalMip = std::numeric_limits<float>::min();

					if (mip1 != _heightField->getNullData())
						finalMip = std::max(mip1, finalMip);
					if (mip2 != _heightField->getNullData())
						finalMip = std::max(mip2, finalMip);
					if (mip3 != _heightField->getNullData())
						finalMip = std::max(mip3, finalMip);
					if (mip4 != _heightField->getNullData())
						finalMip = std::max(mip4, finalMip);

					//_mipmap[l]->getVectorOfData()[x + y * (dimension.x >> l)] = std::max(finalMip, std::numeric_limits<float>::min());
					_mipmap[l]->setData(std::max(finalMip, std::numeric_limits<float>::min()), x, y);
				} else {
					finalMip = std::numeric_limits<float>::max();


					if (mip1 != _heightField->getNullData())
						finalMip = std::min(mip1, finalMip);
					if (mip2 != _heightField->getNullData())
						finalMip = std::min(mip2, finalMip);
					if (mip3 != _heightField->getNullData())
						finalMip = std::min(mip3, finalMip);
					if (mip4 != _heightField->getNullData())
						finalMip = std::min(mip4, finalMip);

					//_mipmap[l]->getVectorOfData()[x + y * (dimension.x >> l)] = std::min(finalMip, std::numeric_limits<float>::max());
					_mipmap[l]->setData(std::min(finalMip, std::numeric_limits<float>::max()), x, y);
				}
				int diff = 1;
				if (mip1 == finalMip && mip2 == finalMip && mip3 == finalMip && mip4 == finalMip)
					diff = 0;

				_difference[_difference.size() - 1]->setData(diff, x, y);
			}

		}

	}

}

HeightMipmap::~HeightMipmap() {
	for (int i = 1; i < _mipmap.size(); ++i)
		delete _mipmap[i];
}
