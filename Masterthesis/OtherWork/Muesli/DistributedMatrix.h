// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef DISTRIBUTED_MATRIX_H
#define DISTRIBUTED_MATRIX_H

#include "mpi.h"

#include "Curry.h"
#include "DistributedDataStructure.h"
#include "Exception.h"
#include "Muesli.h"
#include <iostream>

namespace msl {

// ***************************************************************************************************
// * Distributed Matrix  with data parallel skeletons map, fold, zip, gather, permute, broadcast...  *
// ***************************************************************************************************

	template<class E>
	class DistributedMatrix: public DistributedDataStructure {

		// number of columns
		int m;
		// number of local columns
		int mLocal;
		// nLocal * mLocal;
		int localsize;
		// number of partitions per row
		int blocksInRow;
		// number of partitions per column
		int blocksInCol;

		// assumption: block division of matrix

		// X position of processor in data parallel group of processors
		int localColPosition;
		// Y position of processor in data parallel group of processors
		int localRowPosition;
		// position of processor in data parallel group of processors
		int localPosition;
		// first column index in local partition; assuming division mode: block
		int firstRow;
		// first row index in local partition; assuming division mode: block
		int firstCol;
		// index of first row in next partition
		int nextRow;
		// index of first column in next partition
		int nextCol;
		// array containing the mpi ids of the processes belonging to the distributed matrix
		int* ranks;
		// total number of mpi processes
		int np;

		// E a[nLocal][mLocal];
		E** a;

	private:

		// initializes distributed matrix (used in constructors)
		void init(int rows, int cols) { 
			if((Muesli::MSL_myExit == MSL_UNDEFINED) || (Muesli::MSL_myEntrance == MSL_UNDEFINED)) {
				throws(MissingInitializationException());
			}

			blocksInRow = cols;
			blocksInCol = rows;
			
			// for simplicity: assuming MSL_numOfLocalProcs is a power of 2
			if(rows * cols != Muesli::MSL_numOfLocalProcs) {
				throws(PartitioningImpossibleException());
			}

			// for simplicity: assuming rows divides n
			nLocal = n / rows;
			// for simplicity: assuming cols divides m
			mLocal = m / cols;
			localsize =  nLocal * mLocal;
			// blocks assigned row by row
			localColPosition = (Muesli::MSL_myId - Muesli::MSL_myEntrance) % cols;
			localRowPosition = (Muesli::MSL_myId - Muesli::MSL_myEntrance) / cols;
			localPosition = localRowPosition * cols + localColPosition;
			firstRow = localRowPosition * nLocal;
			firstCol = localColPosition * mLocal;
			nextRow = firstRow + nLocal;
			nextCol = firstCol + mLocal;
			a = new E*[nLocal];
			for(int i = 0; i < nLocal; i++) {
				a[i] = new E[mLocal];
			}
			//initialize number of processes
 			np = Muesli::MSL_numOfLocalProcs;
			//initialize ranks array
			ranks = new int[np];
            for(int i = 0; i < np; i++) {
                ranks[i] = i + Muesli::MSL_myEntrance;
            }
		}

	public:

		// CONSTRUCTORS

		DistributedMatrix(int n0, int m0, int rows, int cols):
		DistributedDataStructure(n0), m(m0) {
			init(rows, cols);
		}

		DistributedMatrix(int n0, int m0, E initial, int rows, int cols):
		DistributedDataStructure(n0), m(m0) {
			init(rows, cols);

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				for(int j = 0; j < mLocal; j++) {
					a[i][j] = initial;
				}
			}
		}

		// The following constructor is in fact scatter. Since non-distributed data structures
		// are replicated on every collaborating processor, there is no need for communication
		// here. E b[n][m] should be const E** b, but the compiler complains about const
		DistributedMatrix(int n0, int m0, E** b, int rows, int cols):
		DistributedDataStructure(n0), m(m0) {
			init(rows, cols);

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				for(int j = 0; j < mLocal; j++) {
					a[i][j] = b[i + firstRow][j + firstCol];
				}
			}
		}

		DistributedMatrix(int n0, int m0, E (*f)(int, int), int rows, int cols):
		DistributedDataStructure(n0), m(m0) {
			init(rows, cols);

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				for(int j = 0; j < mLocal; j++) {
					a[i][j] = f(i + firstRow, j + firstCol);
				}
			}
		}

		template<class F>
		DistributedMatrix(int n0, int m0, const Fct2<int, int, E, F>& f, int rows, int cols):
		DistributedDataStructure(n0), m(m0) {
			init(rows, cols);

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				for(int j = 0; j < mLocal; j++) {
				  a[i][j] = f(i + firstRow, j + firstCol);
				}
			}
		}

		/* Copy constructor.
		*/
		DistributedMatrix(const DistributedMatrix<E>& b):
		DistributedDataStructure(b.n), m(b.m) {
			init(b.blocksInCol, b.blocksInRow);

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				for(int j = 0; j < mLocal; j++) {
					a[i][j] = b.a[i][j];
				}
			}
		}

		// GETTER AND SETTER

		/* Only allowed for locally available elements; using global index
		*/
		inline E get(int i, int j) const {
			// old, but strange implementation without broadcast
			/*if(!isLocal(i, j)) {
				throws(NonLocalAccessException());
			}

			return a[i - firstRow][j - firstCol];*/

			// die ID des Prozesses, der das Element speichert
			int idSource;
			// die zu empfangende Nachricht
			E message;

			// Element mit dem globalen Index liegt lokal vor
			if(isLocal(i, j)) {
				// Nachricht kopieren
				message = a[i - firstRow][j - firstCol];
				// ID "bestimmen"
				idSource = Muesli::MSL_myId;
			}
			// Element mit dem globalen Index liegt nicht lokal vor
			else {
				// ID des Prozesses bestimmen, der das gesuchte Element speichert
				int x = i / nLocal;
				int y = j / mLocal;
				idSource = n / nLocal * x + y;
			}

			// Broadcast durchführen
			msl::broadcast(&message, ranks, np, idSource);
			// Ergebnis zurückgeben
			return message;
		}

		inline int getBlocksInCol() const {
			return blocksInCol;
		}

		inline int getBlocksInRow() const {
			return blocksInRow;
		}

		inline int getCols() const {
			return m;
		}

		inline int getFirstCol() const {
			return firstCol;
		}

		inline int getFirstRow() const {
			return firstRow;
		}

		/* i is global index; j is local index ;
		*/
		inline E getGlobalLocal(int i, int j) const {
			if((i < firstRow) || (i >= nextRow)) {
				throws(NonLocalAccessException());
			}

			// assuming 0 <= j < mLocal;
			return a[i - firstRow][j];
		}

		/* 
		* only applicable to locally available elements;
		* pass 'true' if given indexes are global. 
		*/
		inline E getLocal(int i, int j, bool globalIndex = false) const {
			if (globalIndex) return a[i-firstRow][j-firstCol];
			else return a[i][j];
		}

		/* 
		* *deprecated*
		* only applicable to locally available elements;
		* uses global indexes
		*/
        inline E getLocal(int i, int j, int dummy) const {
            return a[i-firstRow][j-firstCol];    
        } 

		inline int getLocalCols() const {
			return mLocal;
		}

		/* j is global index; i is local index ;
		*/
		inline E getLocalGlobal(int i, int j) const {
			if((j < firstCol) || (j >= nextCol)) {
				throws(NonLocalAccessException());
			}
		
			// assuming 0 <= i < nLocal;
			return a[i][j - firstCol];
		}

		inline int getLocalRows() const {
			return nLocal;
		}

		inline int getRows() const {
			return n;
		}

		/* using local index; assuming 0 <= i < nLocal; 0 <= j < mLocal
		*/
		inline bool isLocal(int i, int j) const {
			return (i >= firstRow) && (i < nextRow) && (j >= firstCol) && (j < nextCol);
		}
		
		/* only applicable to locally available elements,
		*  uses local indexes. 
		*/
		inline void setLocal(int i, int j, E v) {
			a[i][j] = v;
		}

		// VARIOUS

		/* Returns a copy of the distributed matrix by
		   means of the compiler-generated copy-constructor.
		*/
		inline DistributedMatrix<E> copy() const {
			return DistributedMatrix<E>(*this);
		}

		/* Alternatively use << (see below)
		*/
		inline void show() const {
			E** b = new E*[n];
			// output stream
			std::ostringstream s;

			for(int i = 0; i < n; i++) {
				b[i] = new E[m];
			}

			gather(b);

			if(Muesli::MSL_myId == Muesli::MSL_myEntrance) {
				for(int i = 0; i < n; i++) {
					s << "[";
					
					for(int j = 0; j < m; j++) {
						s << b[i][j];

						if(j < m - 1) {
							s << " ";
						}
					}
					
					s << "]" << std::endl;
				}
			}

			for(int i = 0; i < n; i++) {
				delete [] b[i];
			}

			delete [] b;

			// print array
			printf("%s", s.str().c_str());
		}

		// SKELETONS

		// SKELETONS / COMPUTATION

		// SKELETONS / COMPUTATION / COUNT

		template<typename F>
		int count(const Fct1<E, bool, F>& f) const {
			// set result to zero
			int result = 0;

			// iterate over all rows
			#pragma omp parallel for reduction(+:result)
			for(int i = 0; i < nLocal; i++) {
				// iterate over all columns
				for(int j = 0; j < mLocal; j++) {
					// result of applying given function to current value is true
					if(f(a[i][j])) {
						// increase counter
						result++;
					}
				}
			}
			
			// exchange results

    		int power = 1;
    		int neighbor;
    		int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);
    		int result2 = 0;

    		for(int i = 0; i < log2noprocs; i++) {
				neighbor = Muesli::MSL_myEntrance + (localPosition ^ power);
      			power *= 2;
				MSL_SendReceive(neighbor, &result, &result2);
				result += result2;
			}

			// return result
    		return result;
		}

		int count(bool (*f)(E)) const {
			return count(curry(f));
		}

		// SKELETONS / COMPUTATION / FOLD

		/* result on every collaborating processor
		*/
		template<class F>
		E fold(const Fct2<E, E, E, F>& f) const {
			int i, j;
			// assumption: f is associative and commutative
			// parallel prefix-like algorithm
			// step 1: local fold
			E result = a[0][0];

			for(i = 0; i < nLocal; i++) {
				for(j = 0; j < mLocal; j++) {
					if((i != 0) || (j != 0)) {
						result = f(result, a[i][j]);
					}
				}
			}

			// step 2: global folding
			int power = 1;
			int neighbor;
			E result2;
			int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);

			for(i = 0; i < log2noprocs; i++) {
				neighbor = Muesli::MSL_myEntrance + (localPosition ^ power);
				power *= 2;
				// important: this has to be synchronous!
				sendReceive(neighbor, &result, &result2, sizeof(E));
				result = f(result, result2);
			}

			return result;
		}

		inline E fold(E (*f)(E, E)) const {
			return fold(curry(f));
		}

		// SKELETONS / COMPUTATION / MAP

		/* requires no communication
		*/
		template<class R, class F>
		inline DistributedMatrix<R> map(const Fct1<E, R, F>& f) const {
			// causes implicit call of constructor
			DistributedMatrix<R> b(n, m, blocksInRow, blocksInCol);
			
			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				for(int j = 0; j < mLocal; j++) {
					b.setLocal(i, j, f(a[i][j]));
				}
			}
			
			return b;
		}

		template<class R>
		inline DistributedMatrix<R> map(R (*f)(E)) const {
			return map(curry(f));
		}

		// SKELETONS / COMPUTATION / MAP / MAP INDEX

		/* requires no communication
		*/
		template<class R, class F>
		inline DistributedMatrix<R> mapIndex(const Fct3<int, int, E, R, F>& f) const {
			DistributedMatrix<R> b(n, m, blocksInRow, blocksInCol);

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++)
				for(int j = 0; j < mLocal; j++)
					b.setLocal(i, j, f(i + firstRow, j + firstCol, a[i][j]));

			return b;
		}

		template<class R>
		inline DistributedMatrix<R> mapIndex(R (*f)(int, int, E)) const {
			return mapIndex(curry(f));
		}

		// SKELETONS / COMPUTATION / MAP / MAP INDEX IN PLACE

		/* requires no communication
		*/
		template<class F>
		inline void mapIndexInPlace(const Fct3<int, int, E, E, F>& f) {
			#pragma omp parallel for
			for(int i = 0; i < nLocal; ++i)
				for(int j = 0; j < mLocal; ++j)
					a[i][j] = f(i + firstRow, j + firstCol, a[i][j]);
		}

		inline void mapIndexInPlace(E (*f)(int, int, E)) {
			mapIndexInPlace(curry(f));
		}

		// SKELETONS / COMPUTATION / MAP / MAP IN PLACE

		template<class F>
		inline void mapInPlace(const Fct1<E, E, F>& f) {
			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++)
				for(int j = 0; j < mLocal; j++)
					a[i][j] = f(a[i][j]);
		}

		inline void mapInPlace(E (*f)(E)) {
			mapInPlace(curry(f));
		}

		// SKELETONS / COMPUTATION / MAP / MAP PARTITION IN PLACE

		template<class F>
		inline void mapPartitionInPlace(const Fct1<E**, void, F>& f) {
			f(a);
		}

		inline void mapPartitionInPlace(void (*f)(E**)) {
			f(a);
		}

		// SKELETONS / COMPUTATION / SCAN

		//   template<class F>
		//   void scan(const Fct2<E, E, E, F> f) const {... still  missing ...;}

		// SKELETONS / COMPUTATION / ZIP WITH

		/* requires no communication
		*/
		template<class E2, class R, class F>
		inline DistributedMatrix<R> zipWith(const DistributedMatrix<E2>& b,
		const Fct2<E, E2, R, F>& f) const {
			DistributedMatrix<R> c(n, m, blocksInRow, blocksInCol);

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				for(int j = 0; j < mLocal; j++) {
					c.setLocal(i, j, f(a[i][j], b.getLocal(i, j)));
				}
			}

			return c;
		}

		template<class E2, class R>
		inline DistributedMatrix<R> zipWith(const DistributedMatrix<E2>& b, R (*f)(E, E2)) const {
			return zipWith(b, curry(f));
		}

		// SKELETONS / COMPUTATION / ZIP WITH / ZIP WITH INDEX

		/* requires no communication
		*/
		template<class E2, class R, class F>
		inline DistributedMatrix<R> zipWithIndex(const DistributedMatrix<E2>& b,
		const Fct4<int, int, E, E2, R, F>& f) const {
			DistributedMatrix<R> c(n, m, blocksInRow, blocksInCol);

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				for(int j = 0; j < mLocal; j++) {
					c.setLocal(i, j, f(i + firstRow, j + firstCol, a[i][j], b.getLocal(i, j)));
				}
			}

			return c;
		}

		template<class E2, class R>
		inline DistributedMatrix<R> zipWithIndex(const DistributedMatrix<E2>& b,
		R (*f)(int, int, E, E2)) const {
			return zipWithIndex(b, curry(f));
		}

		// SKELETONS / COMPUTATION / ZIP WITH / ZIP WITH INDEX IN PLACE

		/* requires no communication
		*/
		template<class E2, class F>
		inline void zipWithIndexInPlace(const DistributedMatrix<E2>& b,
		const Fct4<int, int, E, E2,E, F> f) {
			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				for(int j = 0; j < mLocal; j++) {
					a[i][j] = f(i + firstRow, j + firstCol, a[i][j], b.getLocal(i, j));
				}
			}
		}

		template<class E2>
		inline void zipWithIndexInPlace(const DistributedMatrix<E2>& b, E (*f)(int, int, E, E2)) {
			zipWithIndexInPlace(b, curry(f));
		}

		// SKELETONS / COMPUTATION / ZIP WITH / ZIP WITH IN PLACE

		/* requires no communication
		*/
		template<class E2, class F>
		inline void zipWithInPlace(const DistributedMatrix<E2>& b, const Fct2<E, E2, E, F>& f) {
			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				for(int j = 0; j < mLocal; j++) {
					a[i][j] = f(a[i][j], b.getLocal(i, j));
				}
			}
		}

		template<class E2>
		inline void zipWithInPlace(const DistributedMatrix<E2>& b, E (*f)(E, E2)) {
			zipWithInPlace(b, curry(f));
		}

		// SKELETONS / COMMUNICATION

		// SKELETONS / COMMUNICATION / BROADCAST

		void broadcast(int row, int col) {
			int block = row / nLocal * (m / mLocal) + col / mLocal;
			
			if((block < 0) || (block >= Muesli::MSL_numOfLocalProcs)) {
				throws(IllegalPartitionException());
			}

			unsigned int i, j, neighbor;
			unsigned int power = 1;
			// 2^30 - 2
			unsigned int mask = 1073741822;
			MPI_Status stat;

			if(block == localPosition) {
				a[0][0] = a[row - firstRow][col - firstCol];
			}

			int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);

			for(i = 0; i < log2noprocs; i++) {
				if((localPosition & mask) == (block & mask)) {
					neighbor = Muesli::MSL_myEntrance + (localPosition ^ power);

					if((localPosition & power) == (block & power)) {
						syncSend(neighbor, &a[0][0], sizeof(E));
					}
					else {
						MSL_receive(neighbor, &a[0][0], sizeof(E), &stat);
					}
				}
				
				power *= 2;
				mask &= ~power;
			}

			for(i = 0; i < nLocal; i++) {
				for(j = 0; j < mLocal; j++) {
					a[i][j] = a[0][0];
				}
			}
		}

		// SKELETONS / COMMUNICATION / BROADCAST PARTITION

		void broadcastPartition(int blockRow, int blockCol) {
			if(blockRow < 0 || blockRow >= blocksInCol || blockCol < 0 || blockCol >= blocksInRow) {
				throws(IllegalPartitionException());
			}
			/*int block = blockRow * blocksInRow + blockCol;
			unsigned int neighbor;
			unsigned int power = 1;
			// 2^30 - 2
			unsigned int mask = 1073741822;
			MPI_Status stat;
			E* buf = new E[nLocal * mLocal];
			int bufCount;
			int i, k, l;
			int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);
		 
			for(i = 0; i < log2noprocs; i++) {
				if((localPosition & mask) == (block & mask)) {
					neighbor = Muesli::MSL_myEntrance + (localPosition ^ power);
						
					if((localPosition & power) == (block & power)) {
						bufCount = 0;

						for(k = 0; k < nLocal; k++) {
							for(l = 0; l < mLocal; l++) {
								buf[bufCount++] = a[k][l];
							}
						}

						syncSend(neighbor, buf, sizeof(E) * localsize);
					}
					else {
						msl::MSL_Receive(neighbor, buf, msl::MSLT_MYTAG, &stat, localsize);
						bufCount = 0;

						for(k = 0; k < nLocal; k++) {
							for(l = 0; l< mLocal; l++) {
								a[k][l] = buf[bufCount++];
							}
						}
					}
				}
				 
				power *= 2;
				mask &= ~power;
			}

			delete [] buf;*/
            E* buf = new E[nLocal * mLocal];
            int bufCount = 0;
            for(int k = 0; k < nLocal; k++) {
			    for(int l = 0; l < mLocal; l++) {
					buf[bufCount++] = a[k][l];
					}
				}
            int rootId = blockRow * blocksInRow + blockCol + Muesli::MSL_myEntrance;
            msl::broadcast(buf, ranks, np, rootId, localsize);
            bufCount = 0;
            for(int k = 0; k < nLocal; k++) {
			    for(int l = 0; l < mLocal; l++) {
					a[k][l] = buf[bufCount++];
					}
				}
            delete [] buf;
		}

		// possible additional skeletons: broadcastRow, broadcastCol

		// SKELETONS / COMMUNICATION / GATHER
		
		struct Buffer{
			int row;
			int col;
		};

		/* E b[n][m]
		*/
		void gather(E** b) const {
			int i, j, k, l;

			//#pragma omp parallel for
			for(i = 0; i < nLocal; i++) {
				for(j = 0; j < mLocal; j++) {
					b[i + firstRow][j + firstCol] = a[i][j];
				}
			}

			// prepare communication buffers and pointers to their components
			Buffer inbuf;
			Buffer outbuf;
			E* inmatrix  = new E[n * m];
			E* outmatrix = new E[n * m];
			int bufCounter;

			int power = 1;
			ProcessorNo neighbor;
			int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);
			// can be avoided by massive index calculations
			int* indexStack = new int[2 * n];
			indexStack[0] = firstRow;
			indexStack[1] = firstCol;
			int top = 1;
			int oldtop;

			for(i = 0; i < log2noprocs; i++) {
				neighbor = Muesli::MSL_myEntrance + (localPosition ^ power);
				power *= 2;
				oldtop = top;

				// synchronous!
				for(j = 0; j <= oldtop; j += 2) {
					// fill output buffer
					outbuf.row = indexStack[j];
					outbuf.col = indexStack[j + 1];
					bufCounter = 0;

					for(k = 0; k < nLocal; k++) {
						for(l = 0; l < mLocal; l++) {
							outmatrix[bufCounter++] = b[outbuf.row + k][outbuf.col + l];
						}
					}

					sendReceive(neighbor, &outbuf, &inbuf, 2 * sizeof(int));
					sendReceive(neighbor, outmatrix, inmatrix, localsize * sizeof(E));
					// fetch from input buffer
					indexStack[++top] = inbuf.row;
					indexStack[++top] = inbuf.col;
					bufCounter = 0;

					for(k = 0; k < nLocal; k++) {
						for(l = 0; l < mLocal; l++) {
							b[inbuf.row + k][inbuf.col + l] = inmatrix[bufCounter++];
						}
					}
				}
			}

			delete [] indexStack;
			delete [] inmatrix;
			delete [] outmatrix;
		}

		// SKELETONS / COMMUNICATION / PERMUTE PARTITION

		template<class F1, class F2>
        inline void permutePartition(const Fct2<int, int, int, F1>& newRow,
        const Fct2<int, int, int, F2>& newCol) {
            int i, j, receiver;
            MPI_Status stat[1];
            MPI_Request req[1];
            receiver = Muesli::MSL_myEntrance + newRow(localRowPosition, localColPosition) *
                    blocksInRow + newCol(localRowPosition, localColPosition);

            if (receiver < Muesli::MSL_myEntrance || receiver >= Muesli::MSL_myEntrance +
                    Muesli::MSL_numOfLocalProcs) {
                throws(IllegalPartitionException());
            }

            int sender = MSL_UNDEFINED;

            // determine sender
            for (i = 0; i < blocksInCol; i++) {
                for (j = 0; j < blocksInRow; j++) {
                    if (Muesli::MSL_myEntrance + newRow(i, j) * blocksInRow +
                            newCol(i, j) == Muesli::MSL_myId) {
                        if (sender == MSL_UNDEFINED) {
                            sender = Muesli::MSL_myEntrance + i * blocksInRow + j;
                        }                            // newRow and newCol don't produce a permutation
                        else {
                            throws(IllegalPermuteException());
                        }
                    }
                }
            }

            // newRow and newCol don't produce a permutation
            if (sender == MSL_UNDEFINED) {
                throws(IllegalPermuteException());
            }

            if (receiver != Muesli::MSL_myId) {
                E* b1 = new E[nLocal * mLocal];
                E* b2 = new E[nLocal * mLocal];
                int bufCount = 0;

                for (i = 0; i < nLocal; ++i) {
                    for (j = 0; j < mLocal; ++j) {
                        b1[bufCount++] = a[i][j];
                    }
                }

                MSL_ISend(receiver, &b1[0], req, stat, msl::MSLT_MYTAG, localsize);
                MSL_Receive(sender, &b2[0], msl::MSLT_MYTAG, stat, nLocal * mLocal);
				MPI_Wait(req, stat);
                bufCount = 0;

                for (i = 0; i < nLocal; ++i) {
                    for (j = 0; j < mLocal; ++j) {
                        a[i][j] = b2[bufCount++];
                    }
                }

                delete [] b1;
                delete [] b2;
            }
        }

		inline void permutePartition(int (*f)(int, int), int (*g)(int, int)) {
			permutePartition(curry(f), curry(g));
		}

		template<class F>
		inline void permutePartition(int (*f)(int, int), const Fct2<int, int, int, F>& g) {
			permutePartition(curry(f), g);
		}

		template<class F>
		inline void permutePartition(const Fct2<int, int, int, F>& f, int (*g)(int, int)) {
			permutePartition(f, curry(g));
		}

		// SKELETONS / COMMUNICATION / ROTATE

		// SKELETONS / COMMUNICATION / ROTATE / ROTATE COLUMNS

		template<class F>
		inline void rotateCols(const Fct1<int, int, F>& f) {
			permutePartition(curry((int (*)(const Fct1<int, int, F>&,
				int, int, int))auxRotateCols<F>)(f)(blocksInCol),
				curry((int (*)(int, int))proj2_2<int, int>));
		}

		inline void rotateCols(int (*f)(int)) {
			rotateCols(curry(f));
		}

		inline void rotateCols(int rows) {
			rotateCols(curry((int (*)(int, int))proj1_2<int, int>)(rows));
		}

		// SKELETONS / COMMUNICATION / ROTATE / ROTATE ROWS

		template<class F>
		inline void rotateRows(const Fct1<int, int, F>& f) {
			// Most C++ compilers need an explicit cast here :(
			// I have to check what the standard says ... (JS)
			permutePartition(curry((int (*)(int, int)) proj1_2<int, int>),
				curry((int (*)(const Fct1<int, int, F>&,
				int, int, int))auxRotateRows<F>)(f)(blocksInRow));
		}

		inline void rotateRows(int (*f)(int)) {
			rotateRows(curry(f));
		}

		inline void rotateRows(int cols) {
			rotateRows(curry((int (*)(int, int))proj1_2<int, int>)(cols));
		}

	};

	template<class E>
	inline std::ostream& operator<<(std::ostream& os, const msl::DistributedMatrix<E>& a) {
		int n = a.getRows();
		int m = a.getCols();
		E** b = new E*[n];

		for(int i = 0; i < n; i++) {
			b[i] = new E[m];
		}

		a.gather(b);
		
		if(Muesli::MSL_myId == Muesli::MSL_myEntrance) {
			for(int i = 0; i < n; i++) {
				os << "[";

				for(int j = 0; j < m; j++) {
					os << b[i][j];

					if(j < m - 1) {
						os << " ";
					}
					else {
						os << "]";
					}
				}

				if(i < n - 1) {
					os << std::endl;
				}
			}
		}

		for(int i = 0; i < n; i++) {
			delete [] b[i];
		}

		delete [] b;
		return os;
	}

}

#endif
