#ifndef VTK_GRID_READER_H
#define VTK_GRID_READER_H

#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include "core/voxelmodel.h"
#include <fstream>

using namespace std;
using glm::vec3;

/**
Implementation of the Reader interface to read objects from VTK files

@author Alejandro Graciano
*/


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

			//VoxelModel<T> *aux = new VoxelModel<T>(dimension, spacing, origin, attribute, data);


			//ivec2 init = { 0, 0 };
			//ivec2 newDim = { 1024, 512 };
			//delete[] data;
			//data = new T[newDim.x * newDim.y * zDimension];
			//int cont = 0;
			//for (int k = 0; k < zDimension; ++k) {
			//	//for (int d = 0; d < 3; ++d) {
			//		for (int i = init.x; i < init.x + newDim.x; ++i) {
			//			for (int j = init.y; j < init.y + newDim.y; ++j) {
			//				data[cont++] = aux->getData(i, j, k);
			//			}
			//		}
			//	//}
			//}

			//dimension = ivec3{ newDim.x, newDim.y, zDimension };
			//vm = new VoxelModel<T>(dimension, spacing, origin, attribute, data);





			//inputStream.close();


			//std::ofstream os;

			//os.open("data/terrain.vtk");
			//auto nData = newDim.x * newDim.y * zDimension;
			//os << "# vtk DataFile Version 3.0\n";
			//os << "dataset" << endl;
			//os << "ASCII" << endl;
			//os << "DATASET STRUCTURED_POINTS\n";
			//os << "DIMENSIONS " << newDim.x + 1 << " " << newDim.y + 1 << " " << zDimension + 1 << endl;
			//os << "SPACING " << spacing.x << " " << spacing.y << " " << spacing.z << endl;
			//os << "ORIGIN " << origin.x << " " << origin.y << " " << origin.z << endl;
			//os << "CELL_DATA" << " " << nData << endl;
			//os << "SCALARS " << attribute << " " << typeid(data[0]).name() << endl;
			//os << "LOOKUP_TABLE default" << endl;

			//for (auto i = 0; i < nData; ++i)
			//	os << data[i] << " ";
			//os << endl;
			//os.close();

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
