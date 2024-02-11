// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef SHORTEST_PATHS_H
#define SHORTEST_PATHS_H

#include <string>

#include "MatrixMarketDistributedSparseMatrixReader.h"
#include "Utility.h"

namespace msl {

	namespace sp {

		/* Returns length of a single edge
		*/
		template<typename T>
		inline T edgeLength(T length) {
			return length;
		}

		/* Returns length of a single edge multiplied by 2
		*/
		template<typename T>
		inline T edgeLengthTwice(T length) {
			return length * 2;
		}

		/* Runtime data for shorterDistance()
		*/
		template<typename T = double>
		struct ShorterDistanceRuntimeData {
			T* distances;
			bool distancesChangedLocally;
		};

		template<typename T>
		inline T shortestDistance(T a, T b) {
			T negativeInfinity = utl::getNegativeInfinity<T>();

			if(a == negativeInfinity)
				return b < 0 ? b : 0;
			else if(b == negativeInfinity)
				return a < 0 ? a : 0;
			else
				return a > b ? b : a;
		}

		/*
		*/
		template<typename T>
		inline T getPredecessor(T a, T b, int source, int destination) {
			if(destination >= 0 && source >= 0) {
				if(b > 0) {
					return source;
				}
				else {
					return -1;
				}
			}
			else {
				return a >= 0 ? a : b;
			}
		}

		/* Returns 1 if source is the predecessor of destination in the shortest path, else 0.
		*/
		template<typename T, typename T2>
		inline int markPredecessor(T2* distances, T currentMark, T2 distance, int source,
			int destination) {
			T negativeInfinity = utl::getNegativeInfinity<T>();

			// distance of 0 would have been marked as NEGATIVE_INFINITY by shorterDistance()
			if(distance == distances[destination] || (distances[destination] == 0 &&
				distance == negativeInfinity)) {
				return 1;
			}
			else {
				return 0;
			}
		}


		template<typename T>
		inline T markShorterDistance(ShorterDistanceRuntimeData<T>* rtData, T (*c)(T),
		T distanceMark, T currentEdgeLength, int source, int destination) {
			T infinity = utl::getPositiveInfinity<T>();
			T negativeInfinity = utl::getNegativeInfinity<T>();

			// if current edge length is not zero
			if(currentEdgeLength != 0) {
				T cost = c(currentEdgeLength);
				T currentWeightedDistance;

				if(rtData->distances[source] != infinity) {
					currentWeightedDistance = rtData->distances[source] +
						(cost == negativeInfinity ? 0 : cost);
				}
				// adding a positive value to INFINITY in case of T representing natural
				// numbers would result in a negative value due to an overflow and thus
				// mislead the algorithm
				else {
					currentWeightedDistance = infinity;
				}

				if(rtData->distances[destination] > currentWeightedDistance) {
					rtData->distancesChangedLocally = true;
					rtData->distances[destination] = currentWeightedDistance;

					// update current distance mark to the current value and return it
					return currentWeightedDistance == 0 ? negativeInfinity : currentWeightedDistance;
				}
			}

			// keep current distance mark
			return distanceMark;
		}

		/* Uses the Bellman and Ford algorithm to calculate shortest
		   distances between the given "vertex" an all other vertexes.
		*/
		template<typename T>
		bool getShortestPathsSwap(const DistributedSparseMatrix<T>* adjacencyMatrix,
		T (*c)(T), int vertex, T* distances, int* pathTree, int* negativeCircleVertex) {
			// determine number of columns
			int columnCount = adjacencyMatrix->getColumnCount();
			// runtime data for shorterDistances()
			ShorterDistanceRuntimeData<T> rtData;
			// array storing extensions
			rtData.distances = new T[columnCount];
			// indicator for changed distance to at least one vertex
			bool distancesChanged;
			// indicator for early algorithm termination
			bool earlyTermination = false;
			// indicator for success or failure due to circles of negative length
			bool success = true;
			// positive infinity
			T infinity = utl::getPositiveInfinity<T>();

			DistributedSparseMatrix<T>* shorterDistanceMarker = new DistributedSparseMatrix<T>(
				adjacencyMatrix->getRowCount(), adjacencyMatrix->getColumnCount(),
				adjacencyMatrix->getR(), adjacencyMatrix->getC());
			DistributedSparseMatrix<int>* shortestPathPredecessorMarker =
				new DistributedSparseMatrix<int>(adjacencyMatrix->getRowCount(),
				adjacencyMatrix->getColumnCount(), adjacencyMatrix->getR(), adjacencyMatrix->getC(), 0);

			DistributedSparseMatrix<T>* swapShorterDistanceMarker;
			DistributedSparseMatrix<int>* swapShortestPathPredecessorMarker;

			// initialize distances with positive infinity and pathTree
			// with -1, indicating invalid vertex numbers
			for(int i = 0; i < columnCount; i++) {
				distances[i] = infinity;
				pathTree[i] = -1;
			}

			// distance from the given vertex to itself is 0
			distances[vertex] = 0;

			utl::printv("calculating distances to all other vertexes.\n");

			// calculate distances to all the other (columnCount - 1) vertexes
			// and prepare path calculation
			for(int i = 1; i < columnCount; i++) {
				// copy current distances to rtData.distances
				memcpy(rtData.distances, distances, sizeof(T) * columnCount);
				// reset distances change indicators
				rtData.distancesChangedLocally = false;
				distancesChanged = false;

				// find shorter distances
				swapShorterDistanceMarker = shorterDistanceMarker;
				shorterDistanceMarker = shorterDistanceMarker->zipIndex(*adjacencyMatrix,
					curry(markShorterDistance<double>)(&rtData, c));
				delete swapShorterDistanceMarker;
				distances = shorterDistanceMarker->foldColumns(shortestDistance, distances);

				// if a distance has changed locally
				if(rtData.distancesChangedLocally) {
					distancesChanged = true;
				}
				// there might be a change to other distances issued by another participating proess
				else {
					// iterate over all distance values
					for(int j = 0; j < columnCount; j++) {
						// if a shorter distance has been found for at least one vertex
						if(distances[j] < rtData.distances[j]) {
							// remember this
							distancesChanged = true;

							break;
						}
					}
				}

				// DEBUG
				// disables early termination
				distancesChanged = true;
				// DEBUG

				// if nothing changed within this iteration, all
				// distances are optimized and we can stop here
				if(!distancesChanged) {
					// earlyTermination is only set true, if stop
					// condition for the loop is not fulfilled
					earlyTermination = (i < columnCount);
					
					if(earlyTermination) {
						utl::printv("getShortestPaths()::earlyTermination from iteration #%i\n", i);
					}
					
					break;
				}
			}

			// by default, assume no negative circle
			*negativeCircleVertex = -1;

			// if a changed distance was detected within the last iteration
			// without earlyTermination, there might be a negative circle
			if(!earlyTermination && distancesChanged) {
				// array holding an unchanged backup of the current distances
				T* oldDistances = new T[columnCount];
				// copy current distances to distancesBackup
				memcpy(oldDistances, distances, sizeof(T) * columnCount);
				// copy current distances to rtData.distances
				memcpy(rtData.distances, distances, sizeof(T) * columnCount);

				// try to find shorter distances
				swapShorterDistanceMarker = shorterDistanceMarker;
				shorterDistanceMarker = shorterDistanceMarker->zipIndex(*adjacencyMatrix,
					curry(markShorterDistance<double>)(&rtData, c));
				delete swapShorterDistanceMarker;

				distances = shorterDistanceMarker->foldColumns(shortestDistance, distances);

				// iterate over all distances
				for(int j = 0; j < columnCount; j++) {
					// if a shorter distance has been found for vertex j
					if(distances[j] < oldDistances[j]) {
						// distance to vertex j is influenced by a negative circle
						*negativeCircleVertex = j;
						success = false;
						break;
					}
				}
				delete [] oldDistances;
			}

			// mark the predecessor of each vertex for building the tree of shortest paths
			swapShortestPathPredecessorMarker = shortestPathPredecessorMarker;
			shortestPathPredecessorMarker =
				shortestPathPredecessorMarker->zipIndex(*shorterDistanceMarker,
				curry(markPredecessor<int, double>)(distances));
			delete swapShortestPathPredecessorMarker;

			// build the tree of shortest paths
			pathTree = shortestPathPredecessorMarker->foldColumnsIndex(getPredecessor, pathTree);

			delete [] rtData.distances;
			delete shortestPathPredecessorMarker;
			delete shorterDistanceMarker;

			return success;
		}

		/* Uses the Bellman and Ford algorithm to calculate shortest
		   distances between the given "vertex" an all other vertexes.
		*/
		template<typename T>
		bool getShortestPathsNoSwap(const DistributedSparseMatrix<T>* adjacencyMatrix,
		T (*c)(T), int vertex, T* distances, int* pathTree, int* negativeCircleVertex) {
			// determine number of columns
			int columnCount = adjacencyMatrix->getColumnCount();
			// runtime data for shorterDistances()
			ShorterDistanceRuntimeData<T> rtData;
			// array storing extensions
			rtData.distances = new T[columnCount];
			// indicator for changed distance to at least one vertex
			bool distancesChanged;
			// indicator for early algorithm termination
			bool earlyTermination = false;
			// indicator for success or failure due to circles of negative length
			bool success = true;
			// positive infinity
			T infinity = utl::getPositiveInfinity<T>();

			DistributedSparseMatrix<T>* shorterDistanceMarker = new DistributedSparseMatrix<T>(
				adjacencyMatrix->getRowCount(), adjacencyMatrix->getColumnCount(),
				adjacencyMatrix->getR(), adjacencyMatrix->getC(), 0);
			DistributedSparseMatrix<int>* shortestPathPredecessorMarker =
				new DistributedSparseMatrix<int>(adjacencyMatrix->getRowCount(),
				adjacencyMatrix->getColumnCount(), adjacencyMatrix->getR(), adjacencyMatrix->getC(), 0);

			// iterate over all vertexes
			for(int i = 0; i < columnCount; i++) {
				// initialize distances with positive infinity
				distances[i] = infinity;
				// initialize pathTree with -1 indicating invalid vertex numbers
				pathTree[i] = -1;
			}

			// distance from the given vertex to itself is 0
			distances[vertex] = 0;

			// calculate distances to all the other (columnCount - 1)
			// vertexes and prepare path calculation
			for(int i = 1; i < columnCount; i++) {
				// copy current distances to rtData.distances
				memcpy(rtData.distances, distances, sizeof(T) * columnCount);
				// reset distances change indicators
				rtData.distancesChangedLocally = false;
				distancesChanged = false;

				// find shorter distances
				shorterDistanceMarker->zipIndexInPlace(*adjacencyMatrix,
					curry(markShorterDistance<double>)(&rtData, c));
				// exchange distances
				distances = shorterDistanceMarker->foldColumns(shortestDistance, distances);
				
				// if a distance has changed locally
				if(rtData.distancesChangedLocally) {
					distancesChanged = true;
				}
				// there might be a change to other distances issued by another participating process
				else {
					// iterate over all distance values
					for(int j = 0; j < columnCount; j++) {
						// if a shorter distance has been found for at least one vertex
						if(distances[j] < rtData.distances[j]) {
							// remember this
							distancesChanged = true;
							// we do not need to look at other distances
							break;
						}
					}
				}

				// if nothing changed within this iteration, all
				// distances are optimized and we can stop here
				if(!distancesChanged) {
					// earlyTermination is only set true, if stop
					// condition for the loop is not fulfilled
					earlyTermination = (i < columnCount);
					
					if(earlyTermination) {
						utl::printv("getShortestPaths()::earlyTermination from iteration #%i\n", i);
					}
					
					break;
				}
			}

			// by default, assume no negative circle
			*negativeCircleVertex = -1;

			// if a changed distance was detected within the last iteration
			// without earlyTermination, there might be a negative circle
			if(!earlyTermination && distancesChanged) {
				// array holding an unchanged backup of the current distances
				T* oldDistances = new T[columnCount];
				// copy current distances to distancesBackup
				memcpy(oldDistances, distances, sizeof(T) * columnCount);
				// copy current distances to rtData.distances
				memcpy(rtData.distances, distances, sizeof(T) * columnCount);

				// try to find shorter distances
				shorterDistanceMarker->zipIndexInPlace(*adjacencyMatrix,
					curry(markShorterDistance<double>)(&rtData, c));
				// exchange distances
				distances = shorterDistanceMarker->foldColumns(shortestDistance, distances);

				// iterate over all distances
				for(int j = 0; j < columnCount; j++) {
					// if a shorter distance has been found for vertex j
					if(distances[j] < oldDistances[j]) {
						// distance to vertex j is influenced by a negative circle
						*negativeCircleVertex = j;
						success = false;
						break;
					}
				}

				// delete array of old distances
				delete [] oldDistances;
			}

			// mark the predecessor of each vertex for building the tree of shortest paths
			shortestPathPredecessorMarker->zipIndexInPlace(*shorterDistanceMarker,
				curry(markPredecessor<int, double>)(distances));
			// build the tree of shortest paths
			pathTree = shortestPathPredecessorMarker->foldColumnsIndex(getPredecessor, pathTree);

			delete [] rtData.distances;
			delete shortestPathPredecessorMarker;
			delete shorterDistanceMarker;

			return success;
		}

		/* Checks the given adjacency matrix for negative circles. Note that this method
		   does not require a starting vertex.
		*/
		template<typename T>
		bool checkForNegativeCircle(const DistributedSparseMatrix<T>* adjacencyMatrix,
		T (*c)(T), int* pathTree, int* negativeCircleVertex) {
			T* distances = new T[adjacencyMatrix->getColumnCount() + 1];
			//int vertex = 1;
			int numColumns = adjacencyMatrix->getColumnCount();
			// negative infinity
			T GENERIC_NEGATIVE_INFINITY = utl::getNegativeInfinity<T>();
			
			DistributedSparseMatrix<T>* extendedAdjacencyMatrix =
				transformations::extendMatrix(adjacencyMatrix, 1, numColumns, 1, numColumns);
			extendedAdjacencyMatrix->mapInPlace(c);
			
			for(int j = 1; j < extendedAdjacencyMatrix->getColumnCount(); j++) {
				extendedAdjacencyMatrix->setElement(GENERIC_NEGATIVE_INFINITY, numColumns, j);
			}

			bool negativeCircle = !getShortestPaths(extendedAdjacencyMatrix,
				edgeLength<T>, numColumns, distances, pathTree, negativeCircleVertex);

			delete [] distances;
			
			return negativeCircle && *negativeCircleVertex >= 0;
		}
		
		/* Measure performance of the full bellman and ford
		   algorithm calculating distances and shortest paths
		*/
		void computeShortestPaths() {
			MatrixMarketDistributedSparseMatrixReader reader(FILENAME);
			DistributedSparseMatrix<double>* a =
				reader.readMatrixMarketDistributedSparseMatrix<double>(10, 10, false, true);

			double* result = new double[a->getColumnCount()];
			int* pathTree = new int[a->getColumnCount()];
			int vertex = 1;
			int negativeCircleVertex;

			// perform the measurement
			sp::getShortestPathsNoSwap(a, sp::edgeLength<double>, vertex, result, pathTree,
				&negativeCircleVertex);
		}

	}

}

#endif
