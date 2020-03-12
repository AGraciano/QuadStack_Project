/**
*	Implementation of the Reader interface to read objects from VTK files
*
*	@author Alejandro Graciano
*/

#ifndef VTK_GRID_READER_H
#define VTK_GRID_READER_H

#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include "core/voxelmodel.h"
#include <fstream>

using namespace std;
using glm::vec3;


namespace io {
	template<class T>
	class VTKGridReader {

	public:

		/**
		Constructor
		*/
		VTKGridReader();

		VoxelModel<T>* open(string filePath);

		/**
		Default destructor
		*/
		~VTKGridReader();
	};

	template<class T>
	VTKGridReader<T>::VTKGridReader() {
	}


	template<class T>
	VoxelModel<T>* VTKGridReader<T>::open(string filePath) {
		ifstream inputStream;
		inputStream.exceptions(ifstream::failbit | ifstream::badbit);

		int xDimension, yDimension, zDimension;
		double xSpacing, ySpacing, zSpacing;
		double xOrigin, yOrigin, zOrigin;

		string attribute, line, type;
		T *data;
		int nData;
		VoxelModel<T>* vm;

		try {

			inputStream.open(filePath);

			getline(inputStream, line); // Extract VTK header
			getline(inputStream, line); // Extract name
			getline(inputStream, line); // Extract file type
			getline(inputStream, line); // Extract dataset type

			inputStream >> line; // Extract dimension
			inputStream >> xDimension;	xDimension -= 1;
			inputStream >> yDimension;	yDimension -= 1;
			inputStream >> zDimension;	zDimension -= 1;


			inputStream >> line; // Extract spacing
			inputStream >> xSpacing;
			inputStream >> ySpacing;
			inputStream >> zSpacing;


			inputStream >> line; // Extract origin
			inputStream >> xOrigin;
			inputStream >> yOrigin;
			inputStream >> zOrigin;


			inputStream >> line; // Extract n data
			inputStream >> nData;

			inputStream >> line; // Extract attribute name
			inputStream >> attribute;
			inputStream >> type;

			getline(inputStream, line); // Extract table name
			getline(inputStream, line);

			data = new T[nData];
			for (int i = 0; i < nData; ++i) // Extract data
				inputStream >> data[i];

			vec3 spacing{ xSpacing, ySpacing, zSpacing };
			vec3 origin{ xOrigin, yOrigin, zOrigin };
			ivec3 dimension{ xDimension, yDimension, zDimension };

			vm = new VoxelModel<T>(dimension, spacing, origin, attribute, data);

		} catch (ifstream::failure &e) {
			cerr << "Exception reading file\n" << e.what();
		}

		return vm;
	}

	template<class T>
	VTKGridReader<T>::~VTKGridReader() {
	}

}


using IntVTKReader = io::VTKGridReader<int>;
using ShortVTKReader = io::VTKGridReader<short>;
using ByteVTKReader = io::VTKGridReader<char>;

#endif
