/**
*	Implementation of the Reader interface to read objects voxel binary files
*	
*	@class BinaryVoxelReader
*	@author Alejandro Graciano
*/

#ifndef BINARY_VOXEL_READER_H
#define BINARY_VOXEL_READER_H

#include <cstdio>
#include <string>
#include <glm/glm.hpp>
#include "core/voxelmodel.h"

using namespace std;
using glm::vec3;
using glm::ivec3;


namespace io {

	struct Header {
		int bytesVoxel;
		ivec3 dimension;
		vec3 spacing;
		vec3 origin;
	};

	template<class T>
	class BinaryVoxelReader {

	public:

		/**
		Constructor
		*/
		BinaryVoxelReader();

		VoxelModel<T>* open(string filePath);

		/**
		Default destructor
		*/
		~BinaryVoxelReader();
	};

	template<class T>
	BinaryVoxelReader<T>::BinaryVoxelReader() {
	}


	template<class T>
	VoxelModel<T>* BinaryVoxelReader<T>::open(string filePath) {
		ifstream inputStream;
		inputStream.exceptions(ifstream::failbit | ifstream::badbit);

		char headerBuf[40];
		Header *header;
		VoxelModel<T>* vm;

		try {

			inputStream.open(filePath, ios::binary);
			inputStream.read(headerBuf, 40);
			header = (Header*)headerBuf;

			// Read actual voxel data
			int nData = header->dimension.x * header->dimension.y * header->dimension.z;
			vector<T> data(nData);
			inputStream.read((char*)&data[0], nData * header->bytesVoxel);

			vm = new VoxelModel<T>(header->dimension, header->spacing, header->origin, "material", &data[0]);


			inputStream.close();

		} catch (ifstream::failure &e) {
			cerr << "Exception reading file\n" << e.what();
		}

		return vm;
	}

	template<class T>
	BinaryVoxelReader<T>::~BinaryVoxelReader() {
	}

}


using IntBinaryReader = io::BinaryVoxelReader<int>;
using ShortBinaryReader = io::BinaryVoxelReader<short>;

#endif
