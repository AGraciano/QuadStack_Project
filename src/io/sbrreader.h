/**
*	Implementation of the Reader interface to read objects from stack-based representation custom files
*
*	@class SBReader
*	@author Alejandro Graciano
*/

#ifndef SBR_READER_H
#define SBR_READER_H

#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include "core/stackbasedrep.h"

using std::ifstream;
using std::string;
using glm::vec2;
using glm::ivec2;


namespace io {

	template <class T>
	class SBRReader {

	public:

		/**
		Constructor
		*/
		SBRReader();

		StackBasedRep<T>* open(string filePath);

		/**
		Destructor
		*/
		~SBRReader();
	};

	

	template<class T>
	SBRReader<T>::SBRReader() {
	}

	template<class T>
	StackBasedRep<T>* SBRReader<T>::open(string filePath) {
		ifstream inputStream;
		inputStream.exceptions(ifstream::failbit | ifstream::badbit);
		ivec2 dimension;
		vec2 spacing;
		vec2 origin;
		double min, max;
		StackBasedRep<T> *sbr;

		string attribute, line;

		string stackDelimiter = ";";
		string intervalDelimiter = "$";
		string valueDelimiter = "|";
		string::size_type sz;

		try {
			inputStream.open(filePath);

			inputStream >> line; // Extract origin
			inputStream >> origin.x;
			inputStream >> origin.y;

			origin.x = originX / CM_FACTOR;
			origin.y = originY / CM_FACTOR;

			
			inputStream >> line; // Extract dimension
			inputStream >> dimensionX;
			inputStream >> dimensionY;

			
			inputStream >> line; // Extract spacing
			inputStream >> spacing.x;
			inputStream >> spacing.y;

			inputStream >> line; // Extract spacing
			inputStream >> min;
			inputStream >> max;

			sbr = new StratumSBRTerrain(min, max, "material", origin, spacing, dimension);

			unsigned int stackPos;
			unsigned int index1D = 0;

			while (!inputStream.eof()) {
				string tokenStack;
				inputStream >> tokenStack;
				ivec2 ind2D = ivec2{ index1D % dimension.x, index1D / dimension.x };

				auto& cellStack = sbrt->getStack(ind2D.x, ind2D.y);

				string::size_type intervalPos;

				while ((intervalPos = tokenStack.find(intervalDelimiter)) != string::npos) {
					auto tokenInterval = tokenStack.substr(0, intervalPos);

					auto valuePos = tokenInterval.find(valueDelimiter);
					auto tokenValue = tokenInterval.substr(0, valuePos);
					auto tokenHeight = tokenInterval.substr(valuePos + 1, tokenInterval.length());

					auto value = stoi(tokenValue);
					auto height = stod(tokenHeight, &sz);

					sbrt->setMaterial(value);

					cellStack.addInterval(value, height);
					tokenStack.erase(0, intervalPos + intervalDelimiter.length());
				}

				index1D++;
			}

			inputStream.close();

		} catch (ifstream::failure &e) {
			cerr << "Exception reading file " << e.what() << endl;
		}



		return sbr;
	}


	template<class T>
	SBRReader<T>::~SBRReader() {
	}


}

using IntSBRReader = io::SBRReader<int>;

#endif