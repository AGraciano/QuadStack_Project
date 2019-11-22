#ifndef QUAD_STACK_H
#define QUAD_STACK_H

#include "core/stackbasedrep.h"
#include "core/heightfield.h"
#include <map>
#include <list>
#include <memory>

using glm::vec4;


/**
Class that encapsulates a quadstack representation.

@author Alejandro Graciano
*/


class QuadStack {

	private:
		static const int NULL_VALUE = -1;


		/**
			Class that encapsulates each interval within a stack.
		*/
		class Interval {
			int _material; /*< Part of the sequence of materials */

			HeightField *_heightField; /*< Layer of consecutive heights with a same material */

			int _hfIndex; /*< Index for the heightfield pool */

			bool _heightFieldOwner; /*< Flag that indicates if the height map must be sampled in this interval */

			glm::vec2 _relative; /*< Relative position */

			bool _erase;

			std::map<HeightField::Quadrant, Interval*> _quadrants;

		public:
			Interval() : _erase(false){};

			Interval(int material, HeightField *HeightField) : _material(material), _heightField(HeightField), _heightFieldOwner(true), _relative(), _erase(false), _hfIndex(0) {}

			Interval(const Interval& other);

			Interval& operator=(const Interval& other);

			bool operator==(const Interval& other) { return _material == other._material; }
			
			bool operator==(Interval& other) { return _material == other._material; }

			bool isNull() const { return _material == NULL_VALUE; }

			float getHeight(unsigned int col, unsigned int row) const { return _heightField->getData(col, row); }

			int getMaterial() const { return _material; }

			void setMaterial(int material) { _material = material; }

			void setHfIndex(int index) { _hfIndex = index; }

			int getHfIndex() { return _hfIndex; }

			void setHeightField(HeightField *HeightField) { _heightFieldOwner = false; _heightField = HeightField; }

			bool hasHeightField() const { return _heightField != nullptr; }

			HeightField* getHeightField() { return _heightField; }

			unsigned int getDimensionX() const { if (hasHeightField()) return _heightField->getDimensionX(); return 0; }

			unsigned int getDimensionY() const { if (hasHeightField()) return _heightField->getDimensionY(); return 0; }

			glm::vec2 getRelativeCoordinates() const { return _relative; }

			bool isOwner() { return _heightFieldOwner; }

			void merge();

			void addQuadrant(Interval *subInterval, HeightField::Quadrant quadrant);

			~Interval() {}
		};


		/**
			Class alias
		*/
		using GStack = std::vector<Interval>;
		using IntPair = std::pair<int, int>;
		using DoublePair = std::pair < IntPair, IntPair>;
		using Cachemap = std::map<DoublePair, GStack>;

		class Node {
			friend class Iterator;

			GStack _stack; /*< Stack in the node (with height)*/

			Node *_nw, *_ne, *_sw, *_se; /*< Node children */

			int _level; /*< Current quadtree level */

			iaabb2 _bb; /*< Node bounding box */

			ShortSBR *_terrain; /*< Pointer to the actual terrain */
			
			bool _compressed; /*< Flag indicating if the node represents compressed data*/

			/**
			Private methods
			*/

			void subdivide();

			GStack compact(Interval *s1, Interval *s2, Interval *s3, Interval *s4, vector<int> l, Cachemap& cache);

			static unsigned nonUIntervals(GStack& stack);

		public:

			Node(int level = 0, ivec2 min = { 0, 0 }, ivec2 max = { 0, 0 }, ShortSBR *terrain = nullptr);

			Node(const Node& other);

			Node& operator=(const Node& other);

			void classify();

			vec4 memorySize();

			unsigned gstackSize() { return _stack.size(); }

			std::vector<Interval>& getGStack() { return _stack; };

			Interval getInterval(unsigned index) { return _stack[index]; }

			bool isLeaf() const;

			bool isInside(unsigned x, unsigned y);

			Node* getNW() { return _nw; }
			Node* getSW() { return _sw; }
			Node* getNE() { return _ne; }
			Node* getSE() { return _se; }

			iaabb2 getBoundingBox() { return _bb; }

			unsigned getMinLevel();

			bool noCompression() { return _bb.max.x - _bb.min.x < 1 || _bb.max.y - _bb.min.y < 1; }

			unsigned getLevel() { return _level; }

			int treeHeight();
			
			/**
			Try the compression of the material of the whole node and return true in afirmative case.
			False otherwise.
			*/
			bool compress();

			
			/**
			Sample the data structure at a specific point
			*/
			int sample(int x, int y, float height, float fatherHeight);

			/**
			Traverse the tree in order to update the terrain pointer
			*/
			void updateTerrain(ShortSBR *terrain);
			
			void rearrangeHeightFields(std::vector<std::pair<Interval*, ivec2>> intervals, int& index);
			
			/**
			Auxiliar method for printing the tree structure
			*/
			std::string print();

			void decompose();

			void promote();

			bool unify();

			bool isCompressed() { return _compressed; }

			GStack shrink(GStack& newStack, HeightField::Quadrant quadrant);

			bool divisible();

			bool lastStack();

		};

		
		ShortSBR *_terrain;
		unsigned _maxLevels;
		Node *_root;
		bool _compressed;
		float  _resolution;


	public:

		/*
		Auxiliary class to iterate in a level order
		*/

		class Iterator {
			Node *_node;
			std::list<Node*> _list;

		public:
			Iterator(Node *node) : _node(node) {}
			bool end() { return _node->isLeaf(); }
			bool next();
			Node* data() { return _node; };
		};
		

		QuadStack(ShortSBR *terrain);

		void classify();

		std::string print();

		int sample(int x, int y, float height, float fatherHeight);

		bool isCompressed() { return _compressed; }

		ShortSBR* getTerrain() { return _terrain; }

		void setTerrain(ShortSBR *terrain);

		friend std::ostream& operator<<(std::ostream& os, QuadStack &quadtree);

		Node* getRoot() { return _root; }

		unsigned getHfRows() { return _terrain->getDimension().x; }

		unsigned getHfCols() { return _terrain->getDimension().y; }

		Iterator iterator() { return Iterator(_root); }

		float getMinHeight() const { return _terrain->getMinHeight(); }

		float getMaxHeight() const { return _terrain->getMaxHeight(); }

		float getHeightResolution();

		unsigned getMaxLevels() { return _maxLevels; }

		std::string getAttributeName() const { return std::string("Attribute"); }

		vec4 memorySize() const;

		void topDownPhase();

		void bottomUpPhase();

		void rearrangeHeightField();

		void compressHeightField(float resolution);

		int treeHeight() { return _root->treeHeight(); }

		~QuadStack();
};

#endif

