// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef DISTRIBUTED_SPARSE_MATRIX_H
#define DISTRIBUTED_SPARSE_MATRIX_H

#include "mpi.h"

#include "BlockDistribution.h"
#include "BsrIndex.h"
#include "CrsSubmatrix.h"
#include "DistributedDataStructure.h"
#include "Distribution.h"
#include "MatrixIndex.h"
#include "Muesli.h"
#include "OAL.h"
#include "RoundRobinDistribution.h"
#include "RowProxy.h"
#include "Submatrix.h"
#include "Utility.h"

#include <cstdarg>
#include <cstdio>
#include <iomanip>
#include <fstream>
#include <limits>
#include <map>
#include <math.h>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace msl {

	/* The ids of the submatrices are assigned in row-major order.
	 */
	template<typename T = double>
	class DistributedSparseMatrix : public DistributedDataStructure {

	private:

		// ATTRIBUTES

		// total number of mpi processes
		int np;
		// number of columns
		int m;
		// number of rows of each submatrix
		int r;
		// number of columns of each submatrix
		int c;
		// number of decimal places to print
		int precision;
		// array containing the mpi ids of the processes belonging to the distributed sparse matrix
		int* ranks;
		// map to store pointers to submatrices; the submatrix id is used as the key
		std::map<int, Submatrix<T>*> submatrices;
		// pointer to distribution-object
		Distribution* distribution;
		// pointer to row proxy object
		RowProxy<T>* rowProxy;
		// pointer to submatrix prototype
		Submatrix<T>* submatrix;
		// zero element
		T zero;

		// CONSTANTS

		// FUNCTIONS

		/* Returns true, if all ctor-arguments are in a valid range, i.e.
		   n >= r > 0 and m >= c > 0. Otherwise, the function returns false.
		*/
		bool argumentsAreValid(int n, int m, int r, int c) const {
			// test arguments for validity
			if(n > 0 && m > 0 && r > 0 && c > 0 && n >= r && m >= c) {
				// return true
				return true;
			}

			// return false
			return false;
		}

		/* Returns the number of columns of the submatrix with the given id.
		*/
		int getColumnCount(int idSubmatrix) const {
			int big = getSubmatrixCountWithBigColumns();
			int small = getSubmatrixCountWithSmallColumns();

			if(idSubmatrix % (big + small) < big) {
				return getColumnCountInBigSubmatrix();
			}
			else {
				return getColumnCountInSmallSubmatrix();
			}
		}

		/* Returns the number of columns which are stored in a big submatrix.
		*/
		int getColumnCountInBigSubmatrix() const {
			return c;
		}

		/* Returns the number of columns which are stored in a small submatrix.
		   Returns 0, if the number of columns which are stored in a big submatrix
		   are equal to the number of columns which are stored in a small submatrix.
		*/
		int getColumnCountInSmallSubmatrix() const {
			return m % c;
		}

		/* Determines the starting column index of the submatrix with the given column.
		*/
		int getColumnIndexStart(int idSubmatrix) const {
			int small = getSubmatrixCountWithSmallColumns();
			int big = getSubmatrixCountWithBigColumns();
			int id = idSubmatrix % (big + small);

			if(id < big) {
				return id * getColumnCountInBigSubmatrix();
			}
			else {
				return big * getColumnCountInBigSubmatrix() +
					(id - big) * getColumnCountInSmallSubmatrix();
			}
		}

		/** Retrieves the element with the given indexes from the given submatrix vector.
			This function can both be used for global or local accesses to matrix elements.
			If the given boolean flag is true, the function broadcasts the element to all
			other processes, thus implementing a global access. Otherwise, the element is
			not broadcasted, thus implementing a local access.

			- int rowIndex: The row index of the element to retrieve. Note that an
			  IndexOutOfBoundExcpetion is thrown, if the given row index is < 0 or >= n.
			- int columnIndex: The column index of the element to retrieve. Note that an
			  IndexOutOfBoundsException is thrown, if the given column index is < 0 or
			  >= m.
			- bool bcast: If set to true, the process which stores the element with
			  the given indexes broadcasts the element to all other processes (global
			  access). If set to false, only the process which stores the element locally
			  will return the indexed element (local access). Note that this parameter
			  cannot be set to true by default, since in this case the compiler could not
			  differentiate between the same public signature.
		*/
		T getElement(int rowIndex, int columnIndex, bool bcast) const {
			// id of the process
			int idProcess;
			// id of the current submatrix
			int idSubmatrix;
			// bsr index of the element
			BsrIndex bsrIndex;
			// pointer to submatrix storing the desired element
			Submatrix<T>* smx;
			// element to get
			T result = zero;

			// at least one of the given indexes is out of bounds
			if(rowIndex < 0 || rowIndex >= n || columnIndex < 0 || columnIndex >= m) {
				// throw exception
				throw IndexOutOfBoundsException();
			}

			// determine the index of the submatrix which should store the element
			getIndexBsr(rowIndex, columnIndex, bsrIndex);
			// determine id of the submatrix
			idSubmatrix = bsrIndex.getId();
			// determine the id of the process which should store the submatrix
			idProcess = distribution->getIdProcess(idSubmatrix);

			// submatrix should be stored locally
			if(id == idProcess) {
				// determine the index of the local values array which stores the submatrix
				smx = getSubmatrix(idSubmatrix);

				// element should be stored locally, but corresponding submatrix does not exist -> 0
				if(smx == NULL) {
					// return 0
					result = zero;
				}
				// submatrix exists
				else {
					// retrieve element from the stored submatrix
					result = smx->getElement(bsrIndex.getRowIndex(), bsrIndex.getColumnIndex());
				}
			}

			// global access rather than local one
			if(bcast) {
				// broadcast element
				broadcast(&result, idProcess + Muesli::MSL_myEntrance);
				//MPI_Bcast(&result, size, MPI_BYTE, idProcess, MSL_COMM_DSM);
			}

			// return result
			return result;
		}

		/* Translates the given global row and column index into the
		   local row and column index and the id of the submatrix.
		*/
		void getIndexBsr(int rowIndex, int columnIndex, BsrIndex& index) const {
			int submatrixCountWithBigRows = getSubmatrixCountWithBigRows();
			int submatrixCountWithBigColumns = getSubmatrixCountWithBigColumns();
			int columnCountInBigSubmatrix = getColumnCountInBigSubmatrix();
			int columnCountInSmallSubmatrix = getColumnCountInSmallSubmatrix();
			int rowCountInBigSubmatrix = getRowCountInBigSubmatrix();
			int rowCountInSmallSubmatrix = getRowCountInSmallSubmatrix();

			int column = columnIndex + 1;
			int columnBlockBased;
			int idSubmatrix;
			int localColumn;
			int localRow;
			int row = rowIndex + 1;
			int rowBlockBased;
			int tmp;

			tmp = submatrixCountWithBigColumns * columnCountInBigSubmatrix;

			// column belongs to a small block
			if(column > tmp) {
				columnBlockBased = (int)ceil((double)(column - tmp) / columnCountInSmallSubmatrix) +
					submatrixCountWithBigColumns;
				localColumn = column - submatrixCountWithBigColumns * columnCountInBigSubmatrix -
					(columnBlockBased - submatrixCountWithBigColumns - 1) * columnCountInSmallSubmatrix;
			}
			// column belongs to a big block
			else {
				columnBlockBased = (int)ceil((double)column / columnCountInBigSubmatrix);
				localColumn = column - (columnBlockBased - 1) * columnCountInBigSubmatrix;
			}

			tmp = submatrixCountWithBigRows * rowCountInBigSubmatrix;

			// row belongs to a small block
			if(row > tmp) {
				rowBlockBased = (int)ceil((double)(row - tmp) / rowCountInSmallSubmatrix) +
					submatrixCountWithBigRows;
				localRow = row - submatrixCountWithBigRows * rowCountInBigSubmatrix -
					(rowBlockBased - submatrixCountWithBigRows - 1) * rowCountInSmallSubmatrix;
			}
			// row belongs to a big block
			else {
				rowBlockBased = (int)ceil((double)row / rowCountInBigSubmatrix);
				localRow = row - (rowBlockBased - 1) * rowCountInBigSubmatrix;
			}

			// compute the zero-based id of the block
			idSubmatrix = (rowBlockBased - 1) * getSubmatrixCountPerRow() + columnBlockBased - 1;

			// fill the BsrBlockIndex with values: row, column and id
			// - 1 because localRow and localColumn respectively are
			// not zero-based indices, but merely absolute values
			index.setColumnIndex(localColumn - 1);
			index.setRowIndex(localRow - 1);
			index.setId(idSubmatrix);
		}

		/* Return the maximum number of submatrices to store globally.
		*/
		int getMaxSubmatrixCount() const {
			return getSubmatrixCountPerRow() * getSubmatrixCountPerColumn();
		}

		/* Returns the number of rows of the submatrix with the given id.
		*/
		int getRowCount(int idSubmatrix) const {
			int big = getSubmatrixCountWithBigColumns();
			int small = getSubmatrixCountWithSmallColumns();

			if(idSubmatrix / (big + small) < getSubmatrixCountWithBigRows()) {
				return getRowCountInBigSubmatrix();
			}
			else {
				return getRowCountInSmallSubmatrix();
			}
		}

		/* Returns the number of rows which are stored in a big submatrix.
		*/
		int getRowCountInBigSubmatrix() const {
			return r;
		}

		/* Returns the number of rows which are stored in a small submatrix.
		   Return 0, if the number of rows which are stored in a big submatrix
		   is equal to the number of rows which are stored in a small submatrix.
		*/
		int getRowCountInSmallSubmatrix() const {
			return n % r;
		}

		/* Determines the starting row index of the submatrix
		   with the given id.
		*/
		int getRowIndexStart(int idSubmatrix) const {
			// row index of the submatrix with the given id
			int rowIndex = idSubmatrix / getSubmatrixCountPerRow();

			// return row index start
			return rowIndex * getRowCountInBigSubmatrix();
		}

		/* Returns the number of submatrices per column.
		*/
		int getSubmatrixCountPerColumn() const {
			return getSubmatrixCountWithBigRows() + getSubmatrixCountWithSmallRows();
		}

		/* Returns the number of submatrices per row.
		*/
		int getSubmatrixCountPerRow() const {
			return getSubmatrixCountWithBigColumns() + getSubmatrixCountWithSmallColumns();
		}

		/* Returns the number of submatrices which store the large number of columns.
		*/
		int getSubmatrixCountWithBigColumns() const {
			return m / c;
		}

		/* Returns the number of submatrices which store the large number of rows.
		*/
		int getSubmatrixCountWithBigRows() const {
			return n / r;
		}

		/* Returns the number of submatrices which store the small number of columns.
		   Return 0, if the number of submatrices which store the small number of
		   columns is equal to the number of submatrices which store the large number
		   of columns.
		*/
		int getSubmatrixCountWithSmallColumns() const {
			if(m % c == 0) {
				return 0;
			}
			else {
				return 1;
			}
		}

		/* Returns the number of submatrices which store the small number of rows.
		   Return 0, if the number of submatrices which store the small number of
		   rows is equal to the number of submatrices which store the large number
		   of rows.
		*/
		int getSubmatrixCountWithSmallRows() const {
			if(n % r == 0) {
				return 0;
			}
			else {
				return 1;
			}
		}

		/* Initializes the MPI environment, the distribution
		   object and the array which stores the submatrices.
		*/
		void init(const Distribution& d, const Submatrix<T>& s) {
			// init id
			id = Muesli::MSL_myId - Muesli::MSL_myEntrance;
			// init np
			np = Muesli::MSL_numOfLocalProcs;

			// initialize ranks array
			ranks = new int[np];
			
			// fill ranks array with mpi ids
			for(int i = 0; i < np; i++) {
				// store id
				ranks[i] = i + Muesli::MSL_myEntrance;
			}

			// set precision
			precision = 4;
			// clone distribution object
			distribution = d.clone();
			// initialize distribution object
			distribution->initialize(np, n, m, r, c, getMaxSubmatrixCount());
			// clone submatrix object
			submatrix = s.clone();
			// set zero element
			submatrix->setZero(zero);
			// initialize row proxy object
			rowProxy = new RowProxy<T>(*this);
		}

		/* Determines, whether the process stores any elements of the row/column
		   with the given index and is therefore affected when rotating it. If a
		   row has to be rotated, the second parameter must be -1. Otherwise, a
		   column has to be rotated.
		*/
		bool performRotation(int rowIndex, int colIndex) const {
			// id of the first submatrix in the row/column
			int idSubmatrixStart;
			// id of the last submatrix in the row/column
			int idSubmatrixEnd;
			// current submatrix id is increased by this amount
			int step;
			// bsr index of the first submatrix in the row/column
			BsrIndex bsr;

			// rotate row
			if(colIndex == -1) {
				// calculate bsr index of the first submatrix in the given row
				getIndexBsr(rowIndex, 0, bsr);
				// determine id of the first submatrix in the given row
				idSubmatrixStart = bsr.getId();
				// calculate bsr index of the last submatrix in the given row
				getIndexBsr(rowIndex, m - 1, bsr);
				// determine id of the last submatrix in the given row
				idSubmatrixEnd = bsr.getId();
				// set step size
				step = 1;
			}
			// rotate column
			else {
				// calculate bsr index of the first submatrix in the given column
				getIndexBsr(0, colIndex, bsr);
				// determine id of the first submatrix in the given column
				idSubmatrixStart = bsr.getId();
				// calculate bsr index of the last submatrix in the given column
				getIndexBsr(n - 1, colIndex, bsr);
				// determine id of the last submatrix in the given column
				idSubmatrixEnd = bsr.getId();
				// set step size
				step = getSubmatrixCountPerRow();
			}

			// iterate over all submatrices of the given row/colum
			for(int idSubmatrix = idSubmatrixStart; idSubmatrix <= idSubmatrixEnd; idSubmatrix += step) {
				// current submatrix should be stored locally
				if(distribution->isStoredLocally(id, idSubmatrix)) {
					return true;
				}
			}
			
			// no submatrix should be stored locally
			return false;
		}

		/* Removes the submatrix with the given index from the
		   map of locally stored submatrices.
		*/
		void removeSubmatrix(int idSubmatrix) {
			// delete submatrix with the given id from the submatrix map
			submatrices.erase(idSubmatrix);
		}

		/* Removes the given submatrix from the map of locally
		   stored submatrices.
		*/
		void removeSubmatrix(Submatrix<T>* s) {
			// delete submatrix with the given id from the submatrix map
			removeSubmatrix(s->getId());
		}

		/* Private function for rotating rows or columns to the left/right
		   or top/down. It is invoked by the public functions rotateRow()
		   and rotateColumn(). The direction is derived from the given
		   arguments rowIndex and colIndex. Only one of these parameters
		   is allowed to be greater than or equal to 0, the other parameter
		   MUST be -1. If rowIndex == -1, a column must be rotated, other-
		   wise a row must be rotated. The third parameter, steps, is used
		   to pass the number of steps to rotate. If steps < 0, a row /
		   column is rotated to the left / up. If steps > 0, a row / column
		   is rotated to the right / down.

		   The fourth parameter, submatrices, is used to specify the vector
		   of submatrices whose row / column is to be rotated. This is
		   necessary for the matrix multiplication algorithm from cannon.
		   
		   The rotation is performed as follows: Each process iterates over
		   the elements of the row / column to be rotated and determines its
		   role for the current element which is one of the following three
		   roles: The process can either store the current element locally,
		   such that it must send the element to another process and execute
		   a MPI_Send() operation. Or it can be the process which must store
		   the current element and therefore execute a MPI_Recv() operation.
		   Last but not least, a process can be not involved with the current
		   element, such that it must not execute any MPI operation.
		   
		   By doing so, the elements are exchanged sequentially which seems
		   to be slower than exchanging them by MPI_Allgather(). However,
		   test showed that this is only the case for a small number of
		   processes. If np is large, the combination of MPI_Send() and
		   MPI_Recv() clearly outperforms the MPI_Allgather() alternative.
		*/
		void rotate(int rowIndex, int colIndex, int steps) {	
			// true, if a row must be rotated
			bool rotateRow;
			// id of the current submatrix
			int idSubmatrix;
			// id of the sending process
			int idSender;
			// id of the receiving process
			int idReceiver;
			// either n or m
			int x;
			// pointer to current submatrix
			Submatrix<T>* smx;
			// the current value to be sent
			T value;
			// row / column to be rotated; stores the values in sorted order
			T* rowCol;
			// bsr index
			BsrIndex bsr;
			// MPI status
			MPI_Status status;

			// a column must be rotated
			if(rowIndex == -1) {
				// set flag
				rotateRow = false;
				// store number of rows
				x = n;
				// cut column index
				colIndex %= m;
			}
			// a row must be rotated
			else {
				// set flag
				rotateRow = true;
				// store number of columns
				x = m;
				// cut row index
				rowIndex %= n;
			}

			// cut steps
			steps %= x;

			// rotation is actually necessary
			if(steps != 0 && performRotation(rowIndex, colIndex)) {
				// allocate space for the row/column
				rowCol = new T[x];

				// trick: transform rotation to the left to rotation to the right
				// and an upward rotation to a downward rotation, respectively
				if(steps < 0) {
					// add number of submatrices per row/column
					steps += x;
				}

				// copy locally available values to the right position in the row/column array

				// ATTENTION: THIS APPROACH IS QUITE INEFFICIENT, SINCE THE BSRINDEX IS
				// COMPUTED REPEATEDLY FOR THE SAME SUBMATRIX. A MORE EFFICIENT APPROACH
				// WOULD BE TO ITERATE OVER THE LOCALLY AVAILABLE COLUMNS. THIS WOULD MAKE
				// IT NECESSARY TO KNOW ABOUT THE LOCALLY STORED ROWS (NLOCAL) AND COLUMNS
				// (MLOCAL) RESPECTIVELY.

				// iterate over all elements of the given row
				for(int rowColIndex = 0; rowColIndex < x; rowColIndex++) {
					// determine id of the process which stores the current column
					if(rotateRow) {
						getIndexBsr(rowIndex, rowColIndex, bsr);
					}
					else {
						getIndexBsr(rowColIndex, colIndex, bsr);
					}

					// store id of the submatrix
					idSubmatrix = bsr.getId();

					// submatrix is stored locally
					if(distribution->isStoredLocally(id, idSubmatrix)) {
						// determine local index of the current submatrix
						smx = getSubmatrix(idSubmatrix);

						// submatrix is empty
						if(smx == NULL) {
							// store 0
							value = zero;
						}
						// submatrix is not empty
						else {
							// store current element
							value = smx->getElement(bsr.getRowIndex(), bsr.getColumnIndex());
						}

						// store current value in temporary array
						rowCol[rowColIndex] = value;
					}
				}

				// exchange locally available elements

				// iterate over all elements of the given row
				for(int rowColIndex = 0; rowColIndex < x; rowColIndex++) {
					// retrieve id of the submatrix which stores the current element
					if(rotateRow) {
						getIndexBsr(rowIndex, rowColIndex, bsr);
					}
					else {
						getIndexBsr(rowColIndex, colIndex, bsr);
					}

					// get id of the sending process
					idSender = distribution->getIdProcess(bsr.getId());
					
					// retrieve id of the submatrix which will store the current element
					if(rotateRow) {
						getIndexBsr(rowIndex, (rowColIndex + steps) % x, bsr);
					}
					else {
						getIndexBsr((rowColIndex + steps) % x, colIndex, bsr);
					}

					// get id of the receiving process
					idReceiver = distribution->getIdProcess(bsr.getId());

					// current element is stored locally
					if(id == idSender) {
						// retrieve element
						value = rowCol[rowColIndex];

						// sending process = receiving process -> no communication necessary
						if(id == idReceiver) {
							// store element
							if(rotateRow) {
								setElement(value, rowIndex, (rowColIndex + steps) % x);
							}
							else {
								setElement(value, (rowColIndex + steps) % x, colIndex);
							}
						}
						// sending process != receiving process -> communication necessary
						else {
							// send message
							MSL_Send(idReceiver + Muesli::MSL_myEntrance, &value, MSLT_ROTATE);
							//MPI_Send(&value, size, MPI_BYTE, idReceiver, 0, MPI_COMM_WORLD);
						}
					}
					// current element is not stored locally
					else {
						// sending and receiving process are not the same
						if(idSender != idReceiver) {
							// this process is the current receiver process
							if(id == idReceiver) {
								// receive message
								MSL_Receive(idSender + Muesli::MSL_myEntrance, &value, MSLT_ROTATE, &status);
								//MPI_Recv(&value, size, MPI_BYTE, idSender, 0, MPI_COMM_WORLD, &status);
								
								// store value
								if(rotateRow) {
									setElement(value, rowIndex, (rowColIndex + steps) % x);
								}
								else {
									setElement(value, (rowColIndex + steps) % x, colIndex);
								}
							}
						}
					}
				}
				
				// delete temporary array
				delete [] rowCol;
			}
		}

		/* Swaps the dimensions of the distributed sparse matrix. Needed
		   when transposing the matrix.
		*/
		void swapDimensions() {
			int tmp = n;

			n = m;
			m = tmp;
		}

		// FUNCTIONS / COMMUNICATION

		// The following methods are simply convenient wrappers for their
		// more complex counterparts in Muesli.h. They pass the ids array
		// and the number of processes used per default.
		
		// FUNCTIONS / COMMUNICATION / ALL GATHER

		/* Convenience wrapper for the MSL_Allgather operation (see below).
		*/
		template<typename T2>
		void allgather(T2* sendbuf, T2* recvbuf, int count = 1) const {
			// call msl::allgather in Muesli.h
			msl::allgather(sendbuf, recvbuf, ranks, np, count);
		}

		// FUNCTIONS / COMMUNICATION / ALLREDUCE

		/* Convenience wrapper for the msl::allreduce operation (see below).
		*/
		template<typename T2>
		void allreduce(T2* sendbuf, T2* recvbuf, T2 (*f)(T2, T2), int count = 1) const {
			// curry given function f and call allreduce 
			allreduce(sendbuf, recvbuf, curry(f), count);
		}

		/* Convenience wrapper for the msl::allreduce operation (see Muesli.h).
		*/
		template<typename T2, typename F>
		void allreduce(T2* sendbuf, T2* recvbuf, Fct2<T2, T2, T2, F> f, int count = 1) const {
			// call msl::allreduce in Muesli.h
			msl::allreduce(sendbuf, recvbuf, ranks, np, f, count);
		}

		/* Convenience wrapper for the MSL_AllreduceIndex operation (see below).
		*/
		template<typename T2>
		void allreduceIndex(T2* sendbuf, T2* recvbuf, T2 (*f)(T2, T2, int, int), int count = 1) const {
			// curry given function f and call allreduceIndex 
			allreduceIndex(sendbuf, recvbuf, curry(f), count);
		}

		/* Convenience wrapper for the MSL_AllreduceIndex operation (see Muesli.h).
		*/
		template<typename T2, typename F>
		void allreduceIndex(T2* sendbuf, T2* recvbuf, Fct4<T2, T2, int, int, T2, F>f, int count = 1) const {
			// call MSL_AllreduceIndex in Muesli.h
			msl::MSL_AllreduceIndex(sendbuf, recvbuf, ranks, np, f, count);
		}

		// FUNCTIONS / COMMUNICATION / BROADCAST

		/* Convenience wrapper for the MSL_Broadcast operation (see Muesli.h).
		*/
		template<typename T2>
		void broadcast(T2* sendbuf, int idRoot, int count = 1) const {
			// call MSL_Broadcast in Muesli.h
			msl::broadcast(sendbuf, ranks, np, idRoot, count);
		}

		// FUNCTIONS / AUXILIARY

		/* Function used to determine the number of non-zero elements
		   of the distributed sparse matrix (cf. getElementCount).
		*/
		template<typename E>
		static inline E add(E x, E y) {
			return x + y;
		}

		/* This function is used when multiplying a vector with the
		   distributed sparse matrix.
		*/
		template<typename E>
		static inline E mlt(E x, E y) {
			return x * y;
		}

	protected:

	public:

		// CONSTANTS

		// METHODS

		/* Just for debugging purposes. Prints all parameters (and submatrices).
		*/
		void debug(bool printSubmatrices = false) const {
			std::ostringstream stream;

			stream << "### debugging distributed sparse matrix" << std::endl;

			// attributes
			stream << "id = " << id << std::endl;
			stream << "np = " << np << std::endl;
			stream << "n  = " << n  << std::endl;
			stream << "m  = " << m  << std::endl;
			stream << "r  = " << r  << std::endl;
			stream << "c  = " << c  << std::endl;
			stream << "precision         = " << precision << std::endl;
			stream << "submatrixCount    = " << submatrices.size() << std::endl;
			stream << "maxSubmatrixCount = " << getMaxSubmatrixCount() << std::endl;
			stream << "ranks = [";

			// iterate over all ranks
			for(int i = 0; i < np; i++) {
				stream << ranks[i];

				if(i < np - 1) {
					stream << " ";
				}
				else {
					stream << "]" << std::endl;
				}
			}

			// submatrices
			if(printSubmatrices) {
				int ns = getSubmatrixCount();
				Submatrix<T>** smxs = new Submatrix<T>*[ns];
				getSubmatrices(smxs);

				for(int i = 0; i < ns; i++) {
					stream << smxs[i]->toString().c_str();
				}

				delete [] smxs;

				stream << std::endl;
			}

			printf("%s", stream.str().c_str());
		}

		/* Adds the given submatrix to the array of locally stored
		   submatrices. The array of submatrices is kept in sorted
		   order, i.e. submatrices are sorted according to their id.
		*/
		void addSubmatrix(Submatrix<T>* s) {
			// insert given submatrix
			submatrices.insert(std::make_pair(s->getId(), s));
		}

		/* Removes all submatrices from the distributed sparse matrix.
		*/
		void deleteSubmatrices() {
			// determine number of submatrices
			int ns = getSubmatrixCount();
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;

			// allocate space for submatrix pointers
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for
			// iterate over all submatrices
			for(int i = 0; i < ns; i++) {
				// delete submatrix
				delete smxs[i];
			}

			// remove all elements from the submatrix map
			submatrices.clear();
		}

		void combine(const T* const vector, T* const result, T (*f)(T, T), T (*g)(T, T)) const {
			combine(vector, result, curry(f), curry(g));
		}

		template<typename F>
		void combine(const T* const vector, T* const result, Fct2<T, T, T, F>f, T (*g)(T, T)) const {
			combine(vector, result, f, curry(g));
		}

		template<typename F>
		void combine(const T* const vector, T* const result, T (*f)(T, T), Fct2<T, T, T, F>g) const {
			combine(vector, result, curry(f), g);
		}

		/* Combines the given vector with the distributed sparse matrix
		   by means of the given functions f and g. If the user provides
		   a multipliation function for f and an addition function for g,
		   combine then calculates the matrix-vector product. In order to
		   perform this operation correctly the following conditions must
		   be fulfilled:

		   1. the length of the given vector must be equal to m
		   2. the length of the resulting vector must be equal to n

		   - const T* const vector: The vector to combine with the matrix.
			 Must at least be of size m. If it is smaller, the program
			 will exit. If it is larger, surplus elements are ignored.
		   - T* const result: The vector where the result of the combine
			 operation is stored. Must at least be of size n. If it is
			 smaller, the program will exit. If it is larger, surplus
			 elements are ignored.
		   - T (*f)(T, T): The function used to combine an element from
			 the matrix with an element from the given vector.
		   - T (*g)(T, T): The function used to combine / fold the results
			 of f.

		   Note that the function void multiply(const T* const vector, T*
		   const result) const can be used to compute the ordinary matrix-
		   vector-product.
		*/
		template<typename F>
		void combine(const T* const vector, T* const result, Fct2<T, T, T, F> f,
			Fct2<T, T, T, F> g) const {
			// global column index of the current value
			int colIndexGlobal;
			// global row index of the current value
			int rowIndexGlobal;
			// id of the thread
			int idThread;
			// number of elements of the current submatrix
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// maximum number of OpenMP threads
			int nt = oal::getMaxThreads();
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointer to all locally stored submatrices
			Submatrix<T>** smxs;
			// current value
			T value;
			// temporary result arrays for each thread
			T** localResults = new T*[nt];

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel private(idThread)
			{
				// determine thread id
				idThread = oal::getThreadNum();
				
				#pragma omp for
				// initialize result with 0
				for(int i = 0; i < n; i++) {
					result[i] = 0;
				}

				#pragma omp for
				// iterate over all threads
				for(int i = 0; i < nt; i++) {
					// alloate space for each thread
					localResults[i] = new T[n];
					// copy result to local thread array
					memcpy(localResults[i], result, sizeof(T) * n);
				}

				#pragma omp for private(colIndexGlobal, ne, rowIndexGlobal, smx, value)
				// iterate over all locally stored submatrices
				for(int i = 0; i < ns; i++) {
					// store pointer to current submatrix
					smx = smxs[i];
					// determine number of elements
					ne = smx->getElementCountLocal();

					// iterate over all locally available elements of the current submatrix
					for(int j = 0; j < ne; j++) {
						// determine current element
						value = smx->getElementLocal(j);

						// value is non-zero
						if(value != 0) {
							// determine global column index
							colIndexGlobal = smx->getColumnIndexGlobal(j);

							// corresponding value in the given vector is non-zero
							if(vector[colIndexGlobal] != 0) {
								// determine global row index
								rowIndexGlobal = smx->getRowIndexGlobal(j);
								// combine values
								localResults[idThread][rowIndexGlobal] =
									g(localResults[idThread][rowIndexGlobal],
									f(value, vector[colIndexGlobal]));
							}
						}
					}
				}

				// iterate over all rows
				#pragma omp for
				for(int i = 0; i < n; i++) {
					// iterate over all threads
					for(int j = 0; j < nt; j++) {
						// fold result using given function g
						result[i] = g(result[i], localResults[j][i]);
					}
				}

				// iterate over all threads
				#pragma omp for
				for(int i = 0; i < nt; i++) {
					// delete temporary array of each thread
					delete [] localResults[i];
				}
			}

			// delete temporary result array
			delete [] localResults;
			// delete pointers to submatrices
			delete [] smxs;
			// exchange result among all collaborating processes
			allreduce(result, result, g, n);
		}

		void combineCritical(const T* const x, T* const b, T (*f)(T, T), T (*g)(T, T)) const {
			combineCritical(x, b, curry(f), curry(g));
		}

		template<typename F>
		void combineCritical(const T* const vector, T* const result, Fct2<T, T, T, F> f,
			Fct2<T, T, T, F> g) const {
			// global column index of the current value
			int colIndexGlobal;
			// global row index of the current value
			int rowIndexGlobal;
			// number of elements of the current submatrix
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointer to all locally stored submatrices
			Submatrix<T>** smxs;
			// current value
			T value;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel
			{
				#pragma omp for
				// initialize result with 0
				for(int i = 0; i < n; i++) {
					result[i] = 0;
				}

				#pragma omp for private(colIndexGlobal, ne, rowIndexGlobal, smx, value)
				// iterate over all locally stored submatrices
				for(int i = 0; i < ns; i++) {
					// store pointer to current submatrix
					smx = smxs[i];
					// determine number of elements
					ne = smx->getElementCountLocal();

					// iterate over all locally available elements of the current submatrix
					for(int j = 0; j < ne; j++) {
						// determine current element
						value = smx->getElementLocal(j);

						// value is non-zero
						if(value != 0) {
							// determine global column index
							colIndexGlobal = smx->getColumnIndexGlobal(j);

							// corresponding value in the given vector is non-zero
							if(vector[colIndexGlobal] != 0) {
								// determine global row index
								rowIndexGlobal = smx->getRowIndexGlobal(j);
								// combine values
								#pragma omp critical(combine)
								result[rowIndexGlobal] = g(result[rowIndexGlobal],
									f(value, vector[colIndexGlobal]));
							}
						}
					}
				}
			}

			// delete pointers to submatrices
			delete [] smxs;
			// exchange result among all collaborating processes
			allreduce(result, result, g, n);
		}

		double mlt(double a, double b) {
			return a * b;
		}

		double add(double a, double b) {
			return a + b;
		}

		void multiplyCritical(const T* const x, T* const b) const {
			combineCritical(x, b, mlt, add);
		}

		/* The function compares the given matrix with the one on which
		   the function has been invoced element-wise. It returns true,
		   if all elements are equal and false, if at least one element
		   is not equal to its counterpart.
		*/
		bool equals(const DistributedSparseMatrix<T>& matrix) const {
			// rows and columns must be identical
			if(n == matrix.n && m == matrix.m) {
				// iterate over all rows
				for(int i = 0; i < n; i++) {
					// iterate over all columns
					for(int j = 0; j < m; j++) {
						// current elements are not equal
						if(getElement(i, j) != matrix.getElement(i, j)) {
							// return false
							return false;
						}
					}
				}

				// return true
				return true;
			}
			// given matrix has different row and/or column count
			else {
				// return false
				return false;
			}
		}

		/* Returns the paramter c, i.e. the number of columns of each submatrix.
		*/
		int getC() const {
			return c;
		}

		/* Copies the column with the given index to the given array. Afterwards,
		   all processes will possess the same result array stored in the given
		   array 'column'.

		   - int columnIndex: The index of the column to copy to the given array.
			 Note that an IndexOutOfBoundException is thrown, if the given column
			 index is < 0 or >= m.
		   - T* column: The array to copy the column with the given index to.
			 Must be of length n. If it is larger, surplus elements will be left
			 unchanged.
		*/
		void getColumn(int columnIndex, T* column) const {
			// starting column index of the current submatrix
			int colIndexStart;
			// number of locally stored rows of the current submatrix
			int nLocal;
			// starting row index of the current submatrix
			int rowIndexStart;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// current element
			T value;
			// receive buffer for all local columns
			T* results = new T[n * np];

			// given column index is out of bounds
			if(columnIndex < 0 || columnIndex >= m) {
				throw IndexOutOfBoundsException();
			}

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel
			{
				// iterate over all elements of the given array
				#pragma omp for
				for(int i = 0; i < n; i++) {
					// set current value to 0
					column[i] = 0;
				}

				#pragma omp for private(colIndexStart, nLocal, rowIndexStart, smx)
				// iterate over all submatrices; O(#submatrices)
				for(int i = 0; i < ns; i++) {
					// store pointer to current submatrix
					smx = smxs[i];

					// current submatrix stores the given column
					if(smx->columnIsLocal(columnIndex)) {
						// determine number of locally stored rows
						nLocal = smx->getLocalN();
						// determine starting column index of the current submatrix
						colIndexStart = smx->getColumnIndexStart();
						// determine starting row index of the current submatrix
						rowIndexStart = smx->getRowIndexStart();

						// iterate over all locally stored rows of the current submatrix
						for(int rowIndex = 0; rowIndex < nLocal; rowIndex++) {
							// copy element; O(log n)
							column[rowIndexStart + rowIndex] =
								smx->getElement(rowIndex, columnIndex - colIndexStart);
						}
					}
				}
			}

			// exchange local results
			allgather(column, results, n);

			// iterate over all elements
			for(int i = 0; i < n; i++) {
				// iterate over all processes
				for(int j = 0; j < np; j++) {
					// determine current value
					value = results[i + j * n];

					// current value is non-zero
					if(value != 0) {
						// store current value in result vector
						column[i] = value;
						// advance to next element
						break;
					}
				}
			}

			// delete pointers to submatrices
			delete [] smxs;
		}

		/* Returns the number of columns of the global matrix.
		*/
		int getColumnCount() const {
			return m;
		}

		/* Returns the element with the given (global) row and column index.
		*/
		T getElement(int rowIndex, int columnIndex) const {
			return getElement(rowIndex, columnIndex, true);
		}

		// GET ELEMENT COUNT

		/* Returns the number of elements of the sparse matrix which match the
		   criterion defined by the given function f. Only if f returns true
		   for the current value, the value is counted. Note that only non-zero
		   elements are passed to f.
		*/
		template<typename F>
		int getElementCount(Fct1<T, bool, F> f) const {
			// global result
			int globalResult;
			// element count of all locally stored submatrices
			int localResult = 0;
			// number of elements of the current submatrix
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for reduction(+:localResult) private(ne, smx)
			// iterate over all locally stored submatrices
			for(int i = 0; i < ns; i++) {
				// store pointer to current submatrix
				smx = smxs[i];
				// determine number of elements
				ne = smx->getElementCountLocal();

				// iterate over all elements
				for(int j = 0; j < ne; j++) {
					// value is non-zero and matches the criterion
					if(f(smx->getElementLocal(j)) == true) {
						// increase counter
						localResult++;
					}
				}
			}

			// exchange partial results with other processes
			allreduce<int>(&localResult, &globalResult, add);
			// delete submatrix pointers
			delete [] smxs;
			// return result
			return globalResult;
		}

		int getElementCount(bool (*f)(T)) const {
			return getElementCount(curry(f));
		}

		template<typename F>
		int getElementCountIndex(Fct3<T, int, int, bool, F> f) const {
			// global column index of the current element
			int colIndexGlobal;
			// global result
			int globalResult;
			// number of elements of the current submatrix
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// element count of all locally stored submatrices
			int localResult = 0;
			// global row index of the current element
			int rowIndexGlobal;
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// value of the current element
			T value;
		
			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for reduction(+:localResult) private(colIndexGlobal, ne, rowIndexGlobal, smx, value)
			// iterate over all locally stored submatrices
			for(int i = 0; i < ns; i++) {
				// store pointer to current submatrix
				smx = smxs[i];
				// determine number of elements
				ne = smx->getElementCountLocal();

				// iterate over all locally available elements
				for(int j = 0; j < ne; j++) {
					// determine the value of the current element
					value = smx->getElementLocal(j);
					// determine global column index
					colIndexGlobal = smx->getColumnIndexGlobal(j);
					// determine global row index
					rowIndexGlobal = smx->getRowIndexGlobal(j);

					// value is non-zero and matches the criterion
					if(value != zero && f(value, rowIndexGlobal, colIndexGlobal) == true) {
						// increase counter
						localResult++;
					}
				}
			}

			// exchange partial results with other processes
			allreduce<int>(&localResult, &globalResult, add);
			// delete submatrix pointers
			delete [] smxs;
			// return result
			return globalResult;
		}

		int getElementCountIndex(bool (*f)(T, int, int)) const {
			return getElementCountIndex(curry(f));
		}

		/* The method counts all non-zero elements of all locally
		   stored submatrices.
		*/
		int getElementCountLocal() const {
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// number of non-zero elements
			int result = 0;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
		
			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for reduction(+:result)
			// iterate over all submatrices
			for(int i = 0; i < ns; i++) {
				// determine number of non-zero elements
				result += smxs[i]->getElementCount();
			}

			// delete submatrix pointers
			delete [] smxs;
			// return result
			return result;
		}

		int getElementCountRow(bool (*f)(T), int rowIndex) const {
			return getElementCountRow(curry(f), rowIndex);
		}

		template<typename F>
		int getElementCountRow(Fct1<T, bool, F> f, int rowIndex) const {
			// global result
			int globalResult;
			// element count of all locally stored submatrices
			int localResult = 0;
			// number of elements of the current submatrix
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// value of the current element
			T value;
		
			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for reduction(+:localResult) private(ne, smx, value)
			// iterate over all locally stored submatrices
			for(int i = 0; i < ns; i++) {
				// store pointer to current submatrix
				smx = smxs[i];

				// current submatrix stores element with the given row index
				if(smx->rowIsLocal(rowIndex)) {
					// determine number of elements
					ne = smx->getElementCountLocal();

					// iterate over all elements
					for(int j = 0; j < ne; j++) {
						// current element has the right row index
						if(rowIndex == smx->getRowIndexGlobal(j)) {
							// determine current value
							value = smx->getElementLocal(j);

							// value is non-zero and matches the criterion
							if(value != zero && f(value) == true) {
								// increase counter
								localResult++;
							}
						}
					}
				}
			}

			// exchange partial results with other processes
			allreduce<int>(&localResult, &globalResult, add);
			// delete submatrix pointers
			delete [] smxs;
			// return result
			return globalResult;
		}

		/* Returns the element with the given (global) row and column index.
		   Note that this function will not broadcast the element but can
		   rather be used to access local elements of the sparse matrix. This
		   is only safe if the corresponding distribution objects are equal.
		*/
		T getElementLocal(int rowIndex, int columnIndex) const {
			return getElement(rowIndex, columnIndex, false);
		}

		/* Returns the parameter r, i.e. the number of rows of each
		   submatrix.
		*/
		int getR() const {
			return r;
		}

		/* Copies the row with the given index to the given array.
		   Obviously, the given array must be at least of length m.
		   If the given array is smaller, the program will exit. If
		   the given array is larger, surplus elements will be left
		   unchanged. Note that the given row index cannot be out
		   of bounds, since it is always set to abs(rowIndex % n).
		*/
		void getRow(int rowIndex, T* row) const {
			// starting column index of the current submatrix
			int colIndexStart;
			// number of locally stored columns of the current submatrix
			int mLocal;
			// starting row index of the current submatrix
			int rowIndexStart;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// current element
			T value;
			// receive buffer for all local rows
			T* results = new T[np * m];

			// given row index is out of bounds
			if(rowIndex < 0 || rowIndex >= n) {
				throw IndexOutOfBoundsException();
			}
		
			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel
			{
				// iterate over all elements of the given array
				for(int i = 0; i < m; i++) {
					// set current value to 0
					row[i] = 0;
				}

				#pragma omp for private(colIndexStart, mLocal, rowIndexStart, smx)
				// iterate over all submatrices; O(#submatrices)
				for(int i = 0; i < ns; i++) {
					// store pointer to current submatrix
					smx = smxs[i];

					// current submatrix stores the given row
					if(smx->rowIsLocal(rowIndex)) {
						// determine number of locally stored columns
						mLocal = smx->getLocalM();
						// determine starting row index of the current submatrix
						rowIndexStart = smx->getRowIndexStart();
						// determine starting column index of the current submatrix
						colIndexStart = smx->getColumnIndexStart();

						// iterate over all locally stored columns of the current submatrix
						for(int colIndex = 0; colIndex < mLocal; colIndex++) {
							// copy element; O(log n)
							row[colIndexStart + colIndex] = smx->getElement(rowIndex - rowIndexStart, colIndex);
						}
					}
				}
			}

			// exchange local results
			allgather(row, results, m);

			// iterate over all elements
			for(int i = 0; i < m; i++) {
				// iterate over all processes
				for(int j = 0; j < np; j++) {
					// determine current value
					value = results[i + j * m];

					// current value is non-zero
					if(value != zero) {
						// store current value in result vector
						row[i] = value;
						// advance to next element
						break;
					}
				}
			}

			// delete submatrix pointers
			delete [] smxs;
		}

		/* Returns the number of rows of the global matrix.
		*/
		int getRowCount() const {
			return n;
		}

		/* Fills the given array of submatrices with pointers
		   to all locally stored submatrices. By doing so, I
		   can circumvent the fact that elements from a std::map
		   cannot be iterated by an OpenMP parallelized loop,
		   because the loop is not conformant. The approach is
		   inspired by C. Terboven and D. an Mey: OpenMP and C++.
		*/
		void getSubmatrices(Submatrix<T>** smxs) const {
			// index for the given array of submatrices
			int i = 0;
			// iterator for all locally stored submatrices
			typename std::map<int, Submatrix<T>*>::const_iterator iter;

			// iterate over all locally stored submatrices
			for(iter = this->submatrices.begin(); iter != this->submatrices.end(); iter++) {
				// store pointer to current submatrix
				smxs[i++] = iter->second;
			}
		}

		/* Returns a pointer to the submatrix with the given id.
		   If none such submatrix exists, i.e. if no submatrix with
		   the given id is currently stored locally, NULL is
		   returned.

		   NOTE: The index operator cannot be used to access the
		   desired element, since it is not const. However, the
		   find method is declared const, although cppreference
		   claims it is not.
		*/
		Submatrix<T>* getSubmatrix(int idSubmatrix) const {
			// iterator pointing to the searched submatrix
			typename std::map<int, Submatrix<T>*>::const_iterator iter = submatrices.find(idSubmatrix);

			return iter == submatrices.end() ? NULL : iter->second;
		}

		/* Returns the total number of submatrices currently
		   stored in the vector.
		*/
		int getSubmatrixCount() const {
			// return number of locally stored submatrices
			return (int)submatrices.size();
		}

		/* True, if the submatrix with the given ID is
		   currently stored locally. False otherwise.
		*/
		bool isStoredLocally(int idSubmatrix) const {
			// number of submatrices with the given id is zero
			if(submatrices.count(idSubmatrix) == 0) {
				// return false
				return false;
			}
			// number of submatrices with the given id is one
			else {
				// return true
				return true;
			}
		}

		/* Multiplies the given vector with the distributed sparse
		   matrix. In order to perform this operation correctly the
		   following conditions must be fulfilled:

		   1. the length of the given vector must be equal to m
		   2. the length of the resulting vector must be equal to n
		*/
		void multiply(const T* const vector, T* const result) const {
			combine(vector, result, mlt, add);
		}

		/* Overwrites the index operator []. The result of this operation
		   is a reference to the RowProxy object constructed during the
		   init method. The trick is, that this object also overwrites
		   the index operator and simultaneously stores the row index of
		   the element to access and a pointer to the distributed sparse
		   matrix. By doing so, the second index operator when using an
		   access such as dsm[i][j] actually calls the index operator of
		   the RowProxy object which in turn calls the getElement method
		   of the distributed sparse matrix object. Note that a call such
		   as dsm[i] only return a reference to the RowProxy object, NOT
		   the row itself!

		   IMPORTANT: Currently, the overwriting of the index operator only
		   supports the access of elements, not the assignment of new values.
		   This means, you can read elements from the matrix, for example
		   int x = dsm[0][0], but you cannot write element to the matrix
		   like dsm[0][0] = x; This is due to the fact that I currently do
		   not know how to implement this :-) The primer claims that the
		   index operator should return a reference to the value such that
		   it can be used as both an lvalue and an rvalue. However, since
		   the elements are distributed across all processes, only one
		   process acutally stores the desired element, all other processes
		   should do nothing. This behaviour is of course implemented in the
		   method setElement which I would like to call in case the index
		   operator is used as an lvalue, but I don't know how to distinguish
		   between these two modes in the method :-(
		*/
		const RowProxy<T>& operator[](int rowIndex) {
			// store the given row index in the proxy object
			rowProxy->setRowIndex(rowIndex);
			// return a reference to the proxy object
			return *rowProxy;
		}

		/* Re-compresses the distributed sparse matrix by re-compressing
		   all locally stored submatrices. Afterwards, empty submatrices
		   are removed.
		*/
		void pack() {
			// compress all submatrices
			packSubmatrices();
			// remove empty submatrices
			removeEmptySubmatrices();
		}

		/* Re-compresses the distributed sparse matrix by re-compressing
		   all locally stored submatrices.
		*/
		void packSubmatrices() {
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for
			// iterate over all locally stored submatrices
			for(int i = 0; i < ns; i++) {
				// pack current submatrix
				smxs[i]->pack();
			}

			// delete submatrix pointers
			delete [] smxs;
		}

		/* Returns a string representation of the distributed sparse matrix.
		   The elements are outputted with a precision predefined in the
		   constant	precision and surrounded by "[" and "]". All values are
		   separated by " ".
		*/
		void print() const {
			print(0, 0, n, m);
		}

		/* Prints the whole matrix to the file with the given name.
		*/
		void print(const std::string* const filename) const {
			print(0, 0, n, m, filename);
		}

		/* Prints a string representation of a part of the matrix. The part is indicated by
		   a starting element indexed by the given row index and column index, respectively.
		   Furthermore, the number of rows and columns to print from this starting element
		   must be passed. Note that only the process with id 0 prints the output! As an
		   additional gimmick, the matrix can also be printed to a file by passing the
		   optional argument 'filename'.
		*/
		void print(int rowIndexStart, int columnIndexStart, int rows, int columns,
			const std::string* const filename = NULL) const {
			// get the string representation of distributed sparse matrix
			std::string tmp = toString(rowIndexStart, columnIndexStart, rows, columns);

			// only process with id = 0 outputs the string
			if(id == 0) {
				// output matrix to console
				if(filename == NULL) {
					// output the string
					printf("%s", tmp.c_str());
				}
				// output matrix to given file
				else {
					// create output file stream
					std::ofstream s(filename->c_str());
					// write matrix to stream
					s << tmp;
					// close stream
					s.close();
				}
			}
		}

		/* Removes ell empty submatrices from the submatrices vector.

		   The loop iterates from the end of the vector to its beginning.
		   Otherwise, an additional index would be required which keeps
		   track of the current vector index, since throughout the loop
		   submatrices may be removed from the vector resulting in false
		   indexes.
		*/
		void removeEmptySubmatrices() {
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for private(smx)
			// iterate over all locally stored submatrices; O(#submatrices)
			for(int i = 0; i < ns; i++) {
				// store pointer to current submatrix
				smx = smxs[i];

				// submatrix is empty after applying the given function f
				if(smx->isEmpty()) {
					// remove current submatrix; O(1)
					removeSubmatrix(smx);
				}
			}

			// delete submatrix pointers
			delete [] smxs;
		}

		/* Rotates the column with the given index by the given
		   amount (steps). If steps < 0, the column is rotated
		   upwards. Otherwise, the column is rotated downwards.
		   Note that the given column index is always set to
		   |columnIndex % m|, such that it can never be out of
		   range.
		*/
		void rotateColumn(int columnIndex, int steps) {
			// given column index is out of bounds
			if(columnIndex < 0 || columnIndex >= m) {
				// throw exception
				throw IndexOutOfBoundsException();
			}
			
			// rotate
			rotate(-1, columnIndex, steps);
		}

		/* Rotates all columns of the distributed sparse matrix by means
		   of the given function f. f is supposed to calculate the number
		   of steps for a given column index where negative values indicate
		   an upward rotation and positive values a downward rotation. If
		   zero is returned, no rotation will take place at all.
		*/
		void rotateColumns(int (*f)(int)) {
			rotateColumns(curry(f));
		}

		template<typename F>
		void rotateColumns(Fct1<int, int, F> f) {
			// iterate over all columns
			for(int colIndex = 0; colIndex < m; colIndex++) {
				// rotate current column
				rotate(-1, colIndex, f(colIndex));
			}
		}

		/* Rotates the row with the given index by the given
		   amount (steps). If steps < 0, the row is rotated
		   to the left. Otherwise, the row is rotated to the
		   right. Note that the given row index is always set
		   to |rowIndex % n|, such that it can never be out
		   of range.
		*/
		void rotateRow(int rowIndex, int steps) {
			// given row index is out of bounds
			if(rowIndex < 0 || rowIndex >= n) {
				// throw exception
				throw IndexOutOfBoundsException();
			}

			// rotate
			rotate(rowIndex % n, -1, steps);
		}

		/* Rotates all rows of the distributed sparse matrix by means
		   of the given function f. f is supposed to calculate the number
		   of steps for a given row index where negative values indicate
		   a rotation to the right and positive values a rotation to the
		   left. If zero is returned, no rotation will take place at all.
		*/
		void rotateRows(int (*f)(int)) {
			rotateRows(curry(f));
		}

		template<typename F>
		void rotateRows(Fct1<int, int, F> f) {
			// iterate over all rows
			for(int rowIndex = 0; rowIndex < n; rowIndex++) {
				// rotate current row
				rotate(rowIndex, -1, f(rowIndex));
			}
		}

		/* Sets the element with the given indexes to the given
		   value. If the given global row and/or column index is
		   out of bounds, an IndexOutOfBoundException is thrown.
		*/
		void setElement(T value, int rowIndex, int columnIndex) {
			// id of the process
			int idProcess;
			// id of the submatrix
			int idSubmatrix;
			// index of the starting row
			int rowIndexStart;
			// index of the starting column
			int colIndexStart;
			// index for a submatrix
			BsrIndex bsrIndex;
			// new submatrix to create
			Submatrix<T>* smx;

			// at least one of the given indexes is out of bounds
			if(rowIndex < 0 || rowIndex >= n || columnIndex < 0 || columnIndex >= m) {
				// throw exception
				throw IndexOutOfBoundsException();
			}

			// determine submatrix which should store the element
			getIndexBsr(rowIndex, columnIndex, bsrIndex);
			// get index of the submatrix
			idSubmatrix = bsrIndex.getId();
			// determine id of the process which should store the submatrix
			idProcess = distribution->getIdProcess(bsrIndex.getId());

			// element is stored locally
			if(idProcess == id) {
				// get pointer to desired submatrix; O(log n)
				smx = getSubmatrix(bsrIndex.getId());

				// submatrix does not exist yet
				if(smx == NULL) {
					// create matrix if value != 0
					if(value != zero) {
						// determine global row index
						rowIndexStart = getRowIndexStart(idSubmatrix);
						// determine global column index
						colIndexStart = getColumnIndexStart(idSubmatrix);
						
						// create new submatrix
						smx = submatrix->clone();
						// initialize submatrix
						smx->initialize(idSubmatrix, getRowCount(idSubmatrix),
							getColumnCount(idSubmatrix), rowIndexStart, colIndexStart,
							value, bsrIndex.getRowIndex(), bsrIndex.getColumnIndex());
						// add submatrix to locally stored submatrices; O(log n)
						addSubmatrix(smx);
					}
				}
				// submatrix already exists
				else {
					// set value of submatrix
					smx->setElement(value, bsrIndex.getRowIndex(), bsrIndex.getColumnIndex());
				}
			}
		}

		/* Sets the number of decimal places outputted when printing
		   the matrix.
		*/
		void setPrecision(int value) {
			// set precision to the given value
			precision = value;
		}

		/* Returns a string representation of the whole matrix.
		*/
		std::string toString() const {
			return toString(0, 0, n, m);
		}

		/* Returns a string representation of a part of the the distributed sparse
		   matrix which is dimensioned by the given parameters. All rows between
		   rowIndexStart and rowIndexStart + rows - 1 and all columns between
		   columnIndexStart and columnIndexStart + columns - 1 are printed.

		   NOTE 1: The function will throw an IndexOutOfBoundsException, if the
		   given parameters indicate a range of the distributed matrix which is out
		   of bounds. The parameters rows and columns are taken as absolute values,
		   such that the given rowIndexStart and columnIndexStart always point to
		   the upper left corner of the submatrix to print.

		   NOTE 2: This function must not be called by a single process of a process
		   group, since this will result in starvation due to the numerous getElement
		   calls. Admittedly, this more Java-like approach introduces an additional
		   point of failure, but I like Java :-)

		   NOTE 3: This method, in particular the outer for-loop, cannot be enhanced
		   by OpenMP directives, since the method call getElement must be made by all
		   participating processes. If only one thread makes the call, the function
		   starves.
		*/
		std::string toString(int rowIndexStart, int columnIndexStart, int rows, int columns) const {
			// output stream
			std::ostringstream stream;
			// set precision
			stream << std::fixed << std::setprecision(precision);
			// set number of columns to absolute value
			columns = abs(columns);
			// set number of rows to absolute value
			rows = abs(rows);

			// given indexes create a rectangle which is out of bounds
			if(rowIndexStart >= 0 && columnIndexStart >= 0 && rowIndexStart + rows <= n &&
				columnIndexStart + columns <= m) {
				// iterate over all relevant rows
				for(int rowIndex = rowIndexStart; rowIndex < rowIndexStart + rows; rowIndex++) {
					stream << "[";

					// iterate over all relevant columns
					for(int colIndex = 0; colIndex < columnIndexStart + columns; colIndex++) {
						// input current element to output stream
						stream << getElement(rowIndex, colIndex);

						// end of row is not reached yet
						if(colIndex < columnIndexStart + columns - 1) {
							stream << " ";
						}
					}

					// end of row is reached
					stream << "]" << std::endl;
				}

				// return result string
				return stream.str();
			}
			// given indices are out of bounds
			else {
				// throw exception
				throw IndexOutOfBoundsException();
			}
		}

		// CONSTRUCTORS

		/* Default constructor. Creates an empty distributed sparse matrix
		   with the given parameters.
		*/
		DistributedSparseMatrix(int rows, int columns, int r, int c, T zero,
		const Distribution& d = RoundRobinDistribution(), const Submatrix<T>& s = CrsSubmatrix<T>()):
		DistributedDataStructure(rows), m(columns), r(r), c(c), zero(zero) {
			// init distributed sparse matrix
			init(d, s);
		}

		/* Enhanced constructor. Creates a distributed sparse matrix with
		   the given parameters and initializes it with the given matrix.
		   Note that the given matrix is const!
		*/
		DistributedSparseMatrix(int rows, int columns, int r, int c, T zero,
			const T* const * const matrix, const Distribution& d = RoundRobinDistribution(),
			const Submatrix<T>& s = CrsSubmatrix<T>()):
		DistributedDataStructure(rows), m(columns), r(r), c(c), zero(zero) {
			int submatrixCountPerColumn;
			int submatrixCountPerRow;
			int submatrixCountWithBigColumns;
			int submatrixCountWithBigRows;
			int columnCountInBigSubmatrix;
			int columnCountInSmallSubmatrix;
			int rowCountInBigSubmatrix;
			int rowCountInSmallSubmatrix;
			int columnIndex = 0;

			int columnSize;
			int idSubmatrix;
			int rowIndex = 0;
			int rowSize;

			// init distributed sparse matrix
			init(d, s);
			
			submatrixCountPerColumn = getSubmatrixCountPerColumn();
			submatrixCountPerRow = getSubmatrixCountPerRow();
			submatrixCountWithBigColumns = getSubmatrixCountWithBigColumns();
			submatrixCountWithBigRows = getSubmatrixCountWithBigRows();
			columnCountInBigSubmatrix = getColumnCountInBigSubmatrix();
			columnCountInSmallSubmatrix = getColumnCountInSmallSubmatrix();
			rowCountInBigSubmatrix = getRowCountInBigSubmatrix();
			rowCountInSmallSubmatrix = getRowCountInSmallSubmatrix();

			#pragma omp parallel for private(columnSize, idSubmatrix, rowIndex, rowSize) firstprivate(columnIndex)
			// iterate over all possible submatrices (row-wise); O(rows / r)
			for(int row = 0; row < submatrixCountPerRow; row++) {
				// a submatrix with the large number of rows is created
				if(row < submatrixCountWithBigRows) {
					// set row size
					rowSize = rowCountInBigSubmatrix;
					// compute row index
					rowIndex = row * rowCountInBigSubmatrix;
				}
				// a submatrix with the small number of rows is created
				else {
					// set row size
					rowSize = rowCountInSmallSubmatrix;
					// compute row index
					rowIndex = submatrixCountWithBigRows * rowCountInBigSubmatrix +
						(row - submatrixCountWithBigRows) * rowCountInSmallSubmatrix;
				}

				// iterate over all possible submatrices (column-wise); O(colmns / c)
				for(int column = 0; column < submatrixCountPerColumn; column++) {
					// a submatrix with the large number of columns is created
					if(column < submatrixCountWithBigColumns) {
						// set column size
						columnSize = columnCountInBigSubmatrix;
					}
					// a submatrix with the small number of columns is created
					else {
						// set column size
						columnSize = columnCountInSmallSubmatrix;
					}

					// compute id of the current submatrix
					idSubmatrix = row * submatrixCountPerRow + column;

					// current submatrix must be stored locally
					if(distribution->isStoredLocally(id, idSubmatrix)) {
						// create a new submatrix
						Submatrix<T>* smx = submatrix->clone();
						// initialize submatrix
						smx->initialize(idSubmatrix, rowSize, columnSize, rowIndex, columnIndex,
							matrix, true);

						// submatrix is empty
						if(smx->getElementCount() == 0) {
							// delete submatrix
							delete smx;
						}
						// submatrix is not empty
						else {
							// THIS PRAGMA DESTROYS THE WHOLE SPEEDUP, IT MUST BE REMOVED
							// AT ALL COST! PROBLEM: WHEN CREATING A NEW SUBMATRIX, IT IS
							// UNCLEAR HOW MANY MORE SUBMATRICES TO CREATE. THUS, I DO NOT
							// KNOW WHERE TO INSERT THIS SUBMATRIX
							#pragma omp critical (constructor)
							// copy pointer to submatrix; O(log n)
							addSubmatrix(smx);
						}
					}

					// compute new column index
					columnIndex += columnSize;
				}

				// reset column index
				columnIndex = 0;
			}
		}

		/* Der Kopierkonstruktor kann nicht so ohne weiteres mit OpenMP Direktiven
		   beschleunigt werden, da die Funktion addSubmatrix() synchronisiert werden
		   muss. Weiterhin scheint der new-Operator ein Flaschenhals zu sein, die
		   Laufzeit verschlechtert sich bei zunehmender Anzahl an Threads jedenfalls.
		*/
		
		/* Copy constructor. Only parallelizes the copy loop for the submatrices.
		   Adding submatrices to the distributed sparse matrix is done sequential.
		*/
		DistributedSparseMatrix(const DistributedSparseMatrix<T>& matrix):
		DistributedDataStructure(matrix.n), m(matrix.m), r(matrix.r), c(matrix.c), zero(matrix.zero) {
			// number of locally available submatrices
			int ns = matrix.getSubmatrixCount();
			// array storing pointers to submatrices
			Submatrix<T>** smxs = new Submatrix<T>*[ns];
			// array storing pointers to the newly created submatrices
			Submatrix<T>** tmps = new Submatrix<T>*[ns];

			// init distributed sparse matrix
			init(*matrix.distribution, *matrix.submatrix);
			// get submatrices from given distributed sparse matrix
			matrix.getSubmatrices(smxs);

			#pragma omp parallel for
			// iterate over all locally available submatrices
			for(int i = 0; i < ns; i++) {
				// copy submatrix using implicit copy constructor
				tmps[i] = smxs[i]->clone();
			}

			// iterate over all newly created submatrices
			for(int i = 0; i < ns; i++) {
				// add submatrix to local vector
				addSubmatrix(tmps[i]);
			}

			// delete temporary array
			delete [] smxs;
			// delete temporary array
			delete [] tmps;
		}

		/* Copy constructor. Creates a new distributed sparse matrix by means of
		   the given matrix. Note that this constructor does not use any OpenMP
		   directives, since the new-operator does not scale very well. In fact,
		   performance is decreased when the code is executed by multiple threads.

		   The parameter i is necessary in order to distinguish between the "real"
		   copy constructor and this test version.
		*/
		DistributedSparseMatrix(const DistributedSparseMatrix<T>& matrix, int i):
		DistributedDataStructure(matrix.n), m(matrix.m), r(matrix.r), c(matrix.c), zero(matrix.zero) {
			// number of locally available submatrices
			int ns = matrix.getSubmatrixCount();
			// array storing pointers to submatrices
			Submatrix<T>** smxs = new Submatrix<T>*[ns];

			// init distributed sparse matrix
			init(*matrix.distribution, *matrix.submatrix);
			// get submatrices from given distributed sparse matrix
			matrix.getSubmatrices(smxs);

			// iterate over all locally available submatrices
			for(int i = 0; i < ns; i++) {
				// clone submatrix
				Submatrix<T>* smx = smxs[i]->clone();
				// add submatrix to local vector
				addSubmatrix(smx);
			}

			// delete temporary array
			delete [] smxs;
		}

		/* Destructor. Deletes all locally stored submatrices and
		   frees the space allocated for the array used to store
		   the submatrices. Additionally, the distribution-object
		   is deleted.
		*/
		~DistributedSparseMatrix() {
			// delete all locally stored submatrices
			deleteSubmatrices();

			// delete row proxy object
			delete rowProxy;
			// delete submatrix prototpye
			delete submatrix;
			// delete distribution prototpye
			delete distribution;
			// delete ranks array
			delete [] ranks;
		}

		// SKELETONS

		// SKELETONS / FILTER

		template<typename T2>
		void filter(T2 (*f)(T), T2* a) {
			filter(curry(f), a);
		}

		/* The skeleton filters all non-zero elements according to the given
		   function f and writes the result to the given buffer a. It is very
		   important to note that the size of the given array a must be large
		   enough to accomodate all filtered values!

		   The skeleton passes each non-zero element to the given function f.
		   If the value returned by f is not 0, it is stored in a temporary
		   vector. Otherwise, it is simply skipped. Afterwards, all processes
		   exchange their locally filtered elements. This is, unfortunately,
		   done in an inefficient way, since our serialized communication
		   functions are not able to send whole blocks of data. Beginning with
		   process 0, each process broadcasts its locally filtered elements to
		   all other processes. If all elements have been broadcasted, the
		   sending process broadcasts a special STOP message. This message
		   indicates that a process has broadcasted all of its local elements
		   and that the next process can continue to broadcast its elements.
		   After this communication step, all processes possess the same array.

		   IMPORTANT: Due to the distribution of the sparse matrix, it is not
		   guaranteed, that the filtered elements will be saved in row-major
		   order in the given array a.
		*/
		template<typename F>
		void filter(Fct1<T, T, F> f, T* a) {
			// index of the current element to store
			int index = 0;
			// number of elements of the current submatrix
			int ne;
			// number of currently stored submatrices
			int ns = getSubmatrixCount();
			// vector storing temporary results
			std::vector<T> tmp;
			// current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// current value
			T value;
			// filtered value
			T valueFiltered;
			// stop message
			T stop = zero;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for private(ne, smx, value, valueFiltered)
			// iterate over all currently stored submatrices
			for(int i = 0; i < ns; i++) {
				// store pointer to current submatrix
				smx = smxs[i];
				// determine number of elements
				ne = smx->getElementCountLocal();

				// iterate over all elements of the current submatrix
				for(int j = 0; j < ne; j++) {
					// determine current element
					value = smx->getElementLocal(j);
					
					// current element is non-zero
					if(value != zero) {
						// filter current element
						valueFiltered = f(value);

						// result is non-zero
						if(valueFiltered != zero) {
							#pragma omp critical
							// store result
							tmp.push_back(valueFiltered);
						}
					}
				}
			}

			// determine number of filtered elements
			ne = (int)tmp.size();

			// exchange local results

			// iterate over all collaborating processes
			for(int idProcess = 0; idProcess < np; idProcess++) {
				// process is sender
				if(id == idProcess) {
					// iterate over all values to broadcast
					for(int i = 0; i < ne; i++) {
						// determine current element
						valueFiltered = tmp.at(i);
						// broadcast current element
						broadcast(&valueFiltered, id);
						// store current element
						a[index++] = valueFiltered;
					}

					// send stop message
					broadcast(&stop, id);
				}
				// process is receiver
				else {
					do {
						// receive message
						broadcast(&valueFiltered, idProcess);

						// received message was not the stop message
						if(valueFiltered != stop) {
							// store received message
							a[index++] = valueFiltered;
						}
					}
					// received message was the stop message
					while(valueFiltered != stop);
				}
			}

			// delete submatrix pointers
			delete [] smxs;
		}
		
		template<typename T2>
		void filterIndex(T (*f)(T2, int, int), T2* a) const {
			filterIndex(curry(f), a);
		}

		/* This function does the same as filter, but the global index of the
		   current element is additionally passed to the given function f.

		   tested = true
		*/
		template<typename F>
		void filterIndex(Fct3<T, int, int, T, F> f, T* a) const {
			// index of the current element to store
			int index = 0;
			// number of locally stored elements
			int ne;
			// number of currently stored submatrices
			int ns = getSubmatrixCount();
			// vector storing temporary results
			std::vector<T> tmp;
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// value of the current element
			T value;
			// filtered value
			T valueFiltered;
			// stop message
			T stop = zero;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for private(ne, smx, value, valueFiltered)
			// iterate over all currently stored submatrices
			for(int i = 0; i < ns; i++) {
				// store pointer to current submatrix
				smx = smxs[i];
				// determine number of elements
				ne = smx->getElementCountLocal();

				// iterate over all elements
				for(int j = 0; j < ne; j++) {
					// apply given function to the current element
					value = smx->getElementLocal(j);
						
					// current element is non-zero
					if(value != zero) {
						// filter current element
						valueFiltered = f(value,
							smx->getRowIndexGlobal(j),
							smx->getColumnIndexGlobal(j));

						// result is non-zero
						if(valueFiltered != zero) {
							#pragma omp critical
							// store result
							tmp.push_back(valueFiltered);
						}
					}
				}
			}

			// delete submatrix pointers
			delete [] smxs;

			ne = (int)tmp.size();

			// exchange local results

			// iterate over all collaborating processes
			for(int idProcess = 0; idProcess < np; idProcess++) {
				// process is sender
				if(id == idProcess) {
					// iterate over all values to broadcast
					for(int i = 0; i < ne; i++) {
						// determine current element
						valueFiltered = tmp.at(i);
						// broadcast current element
						broadcast(&valueFiltered, id);
						// store current element
						a[index++] = valueFiltered;
					}

					// send stop message
					broadcast(&stop, id);
				}
				// process is receiver
				else {
					do {
						// receive message
						broadcast(&valueFiltered, idProcess);

						// received message was not the stop message
						if(valueFiltered != stop) {
							// store received message
							a[index++] = valueFiltered;
						}
					}
					// received message was the stop message
					while(valueFiltered != stop);
				}
			}
		}

		template<typename T2>
		void filterIndexColumn(T2 (*f)(T, int, int), T2* a, int columnIndex) const {
			filterIndexColumn(curry(f), a, columnIndex);
		}

		/* This function does the same as filterIndex, but limits its filtering
		   activities to the column with the given index.
		*/
		template<typename F>
		void filterIndexColumn(Fct3<T, int, int, T, F> f, T* a, int columnIndex) const {
			// index of the current element to store
			int index = 0;
			// number of elements of the current submatrix
			int ne;
			// number of currently stored submatrices
			int ns = getSubmatrixCount();
			// vector storing temporary results
			std::vector<T> tmp;
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// result of applying the given function to the current element
			T value;
			// filtered value
			T valueFiltered;
			// stop message
			T stop = zero;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for private(ne, smx, value, valueFiltered)
			// iterate over all currently stored submatrices
			for(int i = 0; i < ns; i++) {
				// store pointer to current submatrix
				smx = smxs[i];

				// current submatrix stores element with the given row index
				if(smx->columnIsLocal(columnIndex)) {
					// determine number of elements
					ne = smx->getElementCountLocal();

					// iterate over all columns of the current submatrix
					for(int j = 0; j < ne; j++) {
						// current element belongs to the given row
						if(columnIndex == smx->getColumnIndexGlobal(j)) {
							// determine value of the current element
							value = smx->getElementLocal(j);
							
							// current element is non-zero
							if(value != zero) {
								// filter current element
								valueFiltered = f(value, smx->getRowIndexGlobal(j), columnIndex);

								// result is non-zero
								if(valueFiltered != zero) {
									#pragma omp critical
									// store result
									tmp.push_back(valueFiltered);
								}
							}
						}
					}
				}
			}

			// delete submatrix pointers
			delete [] smxs;

			ne = (int)tmp.size();

			// exchange local results

			// instead of broadcasting each element an allgather
			// operation would probably be the better choice...

			// iterate over all collaborating processes
			for(int idProcess = 0; idProcess < np; idProcess++) {
				// process is sender
				if(id == idProcess) {
					// iterate over all values to broadcast
					for(int i = 0; i < ne; i++) {
						// determine current element
						valueFiltered = tmp.at(i);
						// broadcast current element
						broadcast(&valueFiltered, id);
						// store current element
						a[index++] = valueFiltered;
					}

					// send stop message
					broadcast(&stop, id);
				}
				// process is receiver
				else {
					do {
						// receive message
						broadcast(&valueFiltered, idProcess);

						// received message was not the stop message
						if(valueFiltered != stop) {
							// store received message
							a[index++] = valueFiltered;
						}
					}
					// received message was the stop message
					while(valueFiltered != stop);
				}
			}
		}

		void filterIndexRow(T (*f)(T, int, int), T* a, int rowIndex) const {
			filterIndexRow(curry(f), a, rowIndex);
		}

		/* This function does the same as filterIndex, but limits its filtering
		   activities to the row with the given index.
		*/
		template<typename F>
		void filterIndexRow(Fct3<T, int, int, T, F> f, T* a, int rowIndex) const {
			// index of the current element to store
			int index = 0;
			// number of elements of the current submatrix
			int ne;
			// number of currently stored submatrices
			int ns = getSubmatrixCount();
			// vector storing temporary results
			std::vector<T> tmp;
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// result of applying the given function to the current element
			T value;
			// filtered value
			T valueFiltered;
			// stop message
			T stop = zero;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for private(ne, smx, value, valueFiltered)
			// iterate over all currently stored submatrices
			for(int i = 0; i < ns; i++) {
				// store pointer to current submatrix
				smx = smxs[i];

				// current submatrix stores element with the given row index
				if(smx->rowIsLocal(rowIndex)) {
					// determine number of elements
					ne = smx->getElementCountLocal();

					// iterate over all columns of the current submatrix
					for(int j = 0; j < ne; j++) {
						// current element belongs to the given row
						if(rowIndex == smx->getRowIndexGlobal(j)) {
							// determine value of the current element
							value = smx->getElementLocal(j);
							
							// current element is non-zero
							if(value != zero) {
								// filter current element
								valueFiltered = f(value, rowIndex, smx->getColumnIndexGlobal(j));

								// result is non-zero
								if(valueFiltered != zero) {
									#pragma omp critical
									// store result
									tmp.push_back(valueFiltered);
								}
							}
						}
					}
				}
			}

			// delete submatrix pointers
			delete [] smxs;

			ne = (int)tmp.size();

			// exchange local results

			// instead of broadcasting each element an allgather
			// operation would probably the better choice...

			// iterate over all collaborating processes
			for(int idProcess = 0; idProcess < np; idProcess++) {
				// process is sender
				if(id == idProcess) {
					// iterate over all values to broadcast
					for(int i = 0; i < ne; i++) {
						// determine current element
						valueFiltered = tmp.at(i);
						// broadcast current element
						broadcast(&valueFiltered, id);
						// store current element
						a[index++] = valueFiltered;
					}

					// send stop message
					broadcast(&stop, id);
				}
				// process is receiver
				else {
					do {
						// receive message
						broadcast(&valueFiltered, idProcess);

						// received message was not the stop message
						if(valueFiltered != stop) {
							// store received message
							a[index++] = valueFiltered;
						}
					}
					// received message was the stop message
					while(valueFiltered != stop);
				}
			}
		}

		// SKELETONS / FOLD

		// NOTE: AS IT IS NOT ALWAYS POSSIBLE TO AVOID THE FOLDING OF ZERO
		// VALUES, THE USER IS RESPONSIBLE FOR DEALING WITH THEM. THIS
		// MEANS, THAT THE GIVEN FUNCTION MUST COPE WITH ZERO VALUES, NO
		// MATTER WHETHER IT IS THE FIRST OR THE SECOND PARAMETER. THIS
		// PROBLEM ARISES, WHEN THERE ARE MORE PROCESSES AVAILABLE THAN
		// SUBMATRICES TO STORE. NEVERTHELESS, THESE PROCESSES MUST RETURN
		// A VALUE, IN THIS CASE 0.

		// Note: The given skeleton function f must be able to accept two
		// values of type T. At this, the first value will always be the
		// value folded so far, whereas the second value will always be
		// the current, i.e. new value to fold.

		template<typename F>
		T fold(Fct2<T, T, T, F> f) const {
			int idThread;
			int ne;
			int ns = getSubmatrixCount();
			int nt = oal::getMaxThreads();
			Submatrix<T>*  smx;
			Submatrix<T>** smxs;
			T  glb = zero;
			T  tmp = zero;
			T* lcl = new T[nt];

			smxs = new Submatrix<T>*[ns];
			getSubmatrices(smxs);

			#pragma omp parallel private(idThread)
			{
				idThread = oal::getThreadNum();

				#pragma omp for
				for(int i = 0; i < nt; i++) {
					lcl[i] = zero;
				}

				#pragma omp for private(ne, smx, tmp)
				for(int i = 0; i < ns; i++) {
					smx = smxs[i];
					ne  = smx->getElementCountLocal();
					tmp = zero;
					
					for(int j = 0; j < ne; j++) {
						tmp = f(tmp, smx->getElementLocal(j));
					}

					lcl[idThread] = f(lcl[idThread], tmp);
				}
			}

			tmp = zero;
			
			for(int i = 0; i < nt; i++) {
				tmp = f(tmp, lcl[i]);
			}

			allreduce(&tmp, &glb, f);

			delete [] smxs;
			delete [] lcl;
			return glb;
		}

		T fold(T (*f)(T, T)) const {
			return fold(curry(f));
		}

		// SKELETONS / FOLD / FOLD COLUMNS

		template<typename F>
		T* foldColumns(Fct2<T, T, T, F> f, T* row) const {
			return foldColumns(curry(f), row);
		}

		/* Fold skeleton for the locally stored submatrices. The given
		   function is applied to all columns. The programmer must cope
		   with 0 values and is responsible for initializing the given
		   T-array correctly, i.e. row must be of length m. Returns a
		   pointer to the given argument array.
		*/
		T* foldColumns(T (*f)(T, T), T* row) const {
			// global column index of the current element
			int colIndexGlobal;
			// thread id
			int idThread;
			// number of locally stored elements
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// maximum number of threads
			int nt = oal::getMaxThreads();
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// current element
			T value;
			// array for all local fold results
			T** localResults = new T*[nt];

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel private(idThread)
			{
				// determine thread id
				idThread = oal::getThreadNum();

				#pragma omp for
				// iterate over all columns of the given row
				for(int i = 0; i < m; i++) {
					// set row to 0
					row[i] = 0;
				}
				
				#pragma omp for
				// iterate over all threads
				for(int i = 0; i < nt; i++) {
					// allocate space for temporary results
					localResults[i] = new T[m];
					// copy row to local results array
					memcpy(localResults[i], row, sizeof(T) * m);
				}

				// iterate over all locally stored submatrices
				#pragma omp for private(colIndexGlobal, ne, smx, value)
				for(int i = 0; i < ns; i++) {
					// store pointer to current submatrix
					smx = smxs[i];
					// determine number of elements
					ne = smx->getElementCountLocal();

					// iterate over all locally stored elements
					for(int j = 0; j < ne; j++) {
						// determine current element
						value = smx->getElementLocal(j);

						// current element is non-zero
						if(value != 0) {
							// determine global column index
							colIndexGlobal = smx->getColumnIndexGlobal(j);
							// fold value
							localResults[idThread][colIndexGlobal] =
								f(localResults[idThread][colIndexGlobal], value);
						}
					}
				}

				#pragma omp for
				// iterate over all columns
				for(int i = 0; i < m; i++) {
					// iterate over all threads
					for(int j = 0; j < nt; j++) {
						// fold elements
						row[i] = f(row[i], localResults[j][i]);
					}
				}
			}

			// exchange local results with all collaborating processes
			allreduce(row, row, f, m);

			// iterate over all threads
			for(int i = 0; i < nt; i++) {
				// delete temporary array
				delete [] localResults[i];
			}

			// delete temporary array
			delete [] localResults;
			// delete submatrix pointers
			delete [] smxs;
			// return result
			return row;
		}

		// SKELETONS / FOLD / FOLD COLUMNS INDEX

		T* foldColumnsIndex(T (*f)(T, T, int, int), T* row) const {
			return foldColumnsIndex(curry(f), row);
		}

		template<typename F>
		T* foldColumnsIndex(Fct4<T, T, int, int, T, F> f, T* row) const {
			// global column index of the current element
			int colIndexGlobal;
			// global row index of the current element
			int rowIndexGlobal;
			// thread id
			int idThread;
			// number of locally stored elements
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// maximum number of threads
			int nt = oal::getMaxThreads();
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// current element
			T value;
			// array for all local fold results
			T** localResults = new T*[nt];

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel private(idThread)
			{
				// determine thread id
				idThread = oal::getThreadNum();

				// iterate over all columns of the given row
				#pragma omp for
				for(int i = 0; i < m; i++) {
					// set row to 0
					row[i] = 0;
				}

				// iterate over all threads
				#pragma omp for
				for(int i = 0; i < nt; i++) {
					// allocate space for temporary results
					localResults[i] = new T[m];
					// copy row to local results array
					memcpy(localResults[i], row, sizeof(T) * m);
				}

				// iterate over all locally stored submatrices
				#pragma omp for private(colIndexGlobal, ne, rowIndexGlobal, smx, value)
				for(int i = 0; i < ns; i++) {
					// store pointer to current submatrix
					smx = smxs[i];
					// determine number of elements
					ne = smx->getElementCountLocal();

					// iterate over all locally stored elements
					for(int j = 0; j < ne; j++) {
						// determine current element
						value = smx->getElementLocal(j);

						// current element is non-zero
						if(value != 0) {
							// determine global column index
							colIndexGlobal = smx->getColumnIndexGlobal(j);
							// determine global row index
							rowIndexGlobal = smx->getRowIndexGlobal(j);
							// fold value
							localResults[idThread][colIndexGlobal] =
								f(localResults[idThread][colIndexGlobal], value,
									rowIndexGlobal, colIndexGlobal);
						}
					}
				}

				#pragma omp for
				// iterate over all columns
				for(int i = 0; i < m; i++) {
					// iterate over all threads
					for(int j = 0; j < nt; j++) {
						// fold elements
						row[i] = f(row[i], localResults[j][i], -1, j);
					}
				}
			}

			// exchange local results with all collaborating processes
			allreduceIndex(row, row, f, m);

			// iterate over all threads
			for(int i = 0; i < nt; i++) {
				// delete temporary array
				delete [] localResults[i];
			}

			// delete submatrix pointers
			delete [] smxs;
			// delete temporary array
			delete [] localResults;
			// return result
			return row;
		}

		// SKELETONS / FOLD / FOLD INDEX

		T foldIndex(T (*f)(T, T, int, int)) const {
			return foldIndex(curry(f));
		}

		/* Fold skeleton for the locally stored submatrices. The given
		   function is applied to all elements and may incorporate the
		   global index of the current element to fold. The programmer
		   must cope with 0 values, as it is not possible to otherwise
		   determine the neutral element. ATTENTION: After folding all
		   local submatrices, the local results are broadcasted to all
		   processes which fold them again. Since the local results do
		   not possess a row and column index respectively, -1 will be
		   passed instead. The given function must take this behaviour
		   into account!
		*/
		template<typename F>
		T foldIndex(Fct4<T, T, int, int, T, F> f) const {
			// id of the thread
			int idThread;
			// number of elements of the current submatrix
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// maximum number of OpenMP threads
			int nt = oal::getMaxThreads();
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// global result of the fold operation
			T globalResult = 0;
			// local result of the fold operation
			T localResult = 0;
			// current value
			T value;
			// buffer for all local results
			T* localResults = new T[nt];

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel private(idThread)
			{
				// determine thread id
				idThread = oal::getThreadNum();

				// iterate over all threads
				#pragma omp for
				for(int i = 0; i < nt; i++) {
					// set to 0
					localResults[i] = 0;
				}

				#pragma omp for private(ne, localResult, smx, value)
				// fold all locally stored submatrices
				for(int i = 0; i < ns; i++) {
					// store pointer
					smx = smxs[i];
					// determine number of elements
					ne = smx->getElementCountLocal();
					// reset local result
					localResult = 0;
					
					// iterate over all locally available elements of the current submatrix
					for(int j = 0; j < ne; j++) {
						// determine current element
						value = smx->getElementLocal(j);

						// value is not zero
						if(value != 0) {
							// fold current value
							localResult = f(localResult, value,
								smx->getRowIndexGlobal(j),
								smx->getColumnIndexGlobal(j));
						}
					}

					// store value
					localResults[idThread] = f(localResult, localResults[idThread], -1, -1);
				}
			}

			// delete submatrix pointers
			delete [] smxs;
			// reset local result
			localResult = 0;

			// iterate over all threads
			for(int i = 0; i < nt; i++) {
				// fold local result of each thread
				localResult = f(localResult, localResults[i], -1, -1);
			}

			// exchange local results with all collaborating processes
			allreduceIndex(&localResult, &globalResult, f);
			// delete temporary array
			delete [] localResults;
			// return result
			return globalResult;
		}

		// SKELETONS / FOLD / FOLD ROWS INDEX

		T* foldRowsIndex(T (*f)(T, T, int, int), T* column) const {
			return foldRowsIndex(curry(f), column);
		}

		template<typename F>
		T* foldRowsIndex(Fct4<T, T, int, int, T, F> f, T* column) const {
			// global column index of the current element
			int colIndexGlobal;
			// global row index of the current element
			int rowIndexGlobal;
			// thread id
			int idThread;
			// number of locally stored elements
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// maximum number of threads
			int nt = oal::getMaxThreads();
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// current element
			T value;
			// array for all local fold results
			T** localResults = new T*[nt];

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel private(idThread)
			{
				// determine thread id
				idThread = oal::getThreadNum();

				// iterate over all rows of the given column
				#pragma omp for
				for(int i = 0; i < n; i++) {
					// set column to 0
					column[i] = 0;
				}

				// iterate over all threads
				#pragma omp for
				for(int i = 0; i < nt; i++) {
					// allocate space for temporary results
					localResults[i] = new T[m];
					// copy row to local results array
					memcpy(localResults[i], column, sizeof(T) * m);
				}

				// iterate over all locally stored submatrices
				#pragma omp for private(ne, smx, value)
				for(int i = 0; i < ns; i++) {
					// store pointer to current submatrix
					smx = smxs[i];
					// determine number of elements
					ne = smx->getElementCountLocal();

					// iterate over all locally stored elements
					for(int j = 0; j < ne; j++) {
						// determine current element
						value = smx->getElementLocal(j);

						// current element is non-zero
						if(value != 0) {
							// determine global column index
							colIndexGlobal = smx->getRowIndexGlobal(j);
							// determine global row index
							rowIndexGlobal = smx->getRowIndexGlobal(j);
							// fold value
							localResults[idThread][rowIndexGlobal] =
								f(localResults[idThread][rowIndexGlobal], value,
									rowIndexGlobal, colIndexGlobal);
						}
					}
				}
			}

			// delete submatrix pointers
			delete [] smxs;

			// iterate over all threads
			for(int i = 0; i < nt; i++) {
				// iterate over all rows
				for(int j = 0; j < n; j++) {
					// fold elements
					column[j] = f(column[j], localResults[i][j], j, -1);
				}
			}

			// exchange local results with all collaborating processes
			allreduceIndex(column, column, f, n);

			// iterate over all threads
			for(int i = 0; i < nt; i++) {
				// delete temporary array
				delete [] localResults[i];
			}

			// delete temporary array
			delete [] localResults;
			// return result
			return column;
		}

		// SKELETONS / FOLD / FOLD ROWS

		template<typename F>
		T* foldRows(Fct2<T, T, T, F> f, T* column) const {
			return foldRows(curry(f), column);
		}

		/* Fold skeleton for the locally stored submatrices. The given
		   function is applied to all rows. The programmer must cope
		   with 0 values and is responsible for initializing the given
		   T-array correctly, i.e. result must be of length n. Returns
		   a pointer to the given argument array.
		*/
		T* foldRows(T (*f)(T, T), T* column) const {
			// global row index of the current element
			int rowIndexGlobal;
			// thread id
			int idThread;
			// number of locally stored elements
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// number of threads
			//int nt = omp_get_max_threads();
			int nt = oal::getMaxThreads();
			// pointer to current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// current element
			T value;
			// array for all local fold results
			T** localResults = new T*[nt];

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel private(idThread)
			{
				// determine thread id
				idThread = oal::getThreadNum();

				// iterate over all rows of the given column
				#pragma omp for
				for(int i = 0; i < n; i++) {
					// set column to 0
					column[i] = 0;
				}

				// iterate over all threads
				#pragma omp for
				for(int i = 0; i < nt; i++) {
					// allocate space for temporary results
					localResults[i] = new T[m];
					// copy row to local results array
					memcpy(localResults[i], column, sizeof(T) * m);
				}

				// iterate over all locally stored submatrices
				#pragma omp for private(ne, smx, value)
				for(int i = 0; i < ns; i++) {
					// store pointer to current submatrix
					smx = smxs[i];
					// determine number of elements
					ne = smx->getElementCountLocal();

					// iterate over all locally stored elements
					for(int j = 0; j < ne; j++) {
						// determine current element
						value = smx->getElementLocal(j);

						// current element is non-zero
						if(value != 0) {
							// determine global column index
							rowIndexGlobal = smx->getRowIndexGlobal(j);
							// fold value
							localResults[idThread][rowIndexGlobal] =
								f(localResults[idThread][rowIndexGlobal], value);
						}
					}
				}
			}

			// iterate over all threads
			for(int i = 0; i < nt; i++) {
				// iterate over all rows
				for(int j = 0; j < n; j++) {
					// fold elements
					column[j] = f(column[j], localResults[i][j]);
				}
			}

			// exchange local results with all collaborating processes
			allreduce(column, column, f, n);

			// iterate over all threads
			for(int i = 0; i < nt; i++) {
				// delete temporary array
				delete [] localResults[i];
			}

			// delete submatrix pointers
			delete [] smxs;
			// delete temporary array
			delete [] localResults;
			// return result
			return column;
		}

		// SKELETONS / MAP

		template<typename T2>
		DistributedSparseMatrix<T2>* map(T2 (*f)(T)) const {
			return map(curry(f));
		}

		/* 

		   tested: true
		*/
		template<typename F, typename T2>
		DistributedSparseMatrix<T2>* map(Fct1<T, T2, F> f) const {
			// number of elements of the current submatrix
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// the new constructed sparse matrix
			DistributedSparseMatrix<T2>* result = new DistributedSparseMatrix<T2>(*this);
			// current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// current value
			T value;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			result->getSubmatrices(smxs);

			#pragma omp parallel for private(ne, smx, value)
			// iterate over all locally stored submatrices
			for(int i = 0; i < ns; i++) {
				// store pointer to current smx of the resulting matrix
				smx = smxs[i];
				// determine number of elements of the current submatrix
				ne = smx->getElementCountLocal();

				// iterate over all elements of the current submatrix
				for(int j = 0; j < ne; j++) {
					// determine current value
					value = smx->getElementLocal(j);
						
					// current value is non-zero
					if(value != 0) {
						// apply f to current value and store the result
						smx->setElementLocal(f(value), j);
					}
				}
			}

			// delete submatrix pointers
			delete [] smxs;
			// return result
			return result;
		}

		template<typename T2>
		DistributedSparseMatrix<T2>* mapIndex(T2 (*f)(T, int, int)) const {
			return mapIndex(curry(f));
		}

		/* 

		   tested: false.
		*/
		template<typename F, typename T2>
		DistributedSparseMatrix<T2>* mapIndex(Fct3<T, int, int, T2, F> f) const {
			// number of elements of the current submatrix
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// the new constructed sparse matrix
			DistributedSparseMatrix<T2>* result = new DistributedSparseMatrix<T2>(*this);
			// current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// current value
			T value;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			result->getSubmatrices(smxs);

			#pragma omp parallel for private(ne, smx, value)
			// iterate over all locally stored submatrices
			for(int i = 0; i < ns; i++) {
				// store pointer to current submatrix
				smx = smxs[i];
				// determine number of elements of the current submatrix
				ne = smx->getElementCountLocal();

				// iterate over all elements of the current submatrix
				for(int j = 0; j < ne; j++) {
					// determine current value
					value = smx->getElementLocal(j);
						
					// current value is non-zero
					if(value != 0) {
						// apply f to current value and store the result
						smx->setElementLocal(f(value,
							smx->getRowIndexGlobal(j),
							smx->getColumnIndexGlobal(j)), j);
					}
				}
			}

			// delete submatrix pointers
			delete [] smxs;
			// return result
			return result;
		}

		void mapIndexInPlace(T (*f)(T, int, int)) {
			mapIndexInPlace(curry(f));
		}

		/* 

		   tested: true
		*/
		template<typename F>
		void mapIndexInPlace(Fct3<T, int, int, T, F> f) {
			// number of elements of the current submatrix
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// current value
			T value;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for private(ne, smx, value)
			// iterate over all locally stored submatrices
			for(int i = 0; i < ns; i++) {
				// copy current submatrix using implicit copy constructor
				smx = smxs[i];
				// determine number of elements of the current submatrix
				ne = smx->getElementCountLocal();

				// iterate over all elements of the current submatrix
				for(int j = 0; j < ne; j++) {
					// determine current value
					value = smx->getElementLocal(j);
						
					// current value is non-zero
					if(value != 0) {
						// apply f to current value and store the result
						smx->setElementLocal(f(value,
							smx->getRowIndexGlobal(j),
							smx->getColumnIndexGlobal(j)), j);
					}
				}
			}

			// delete submatrix pointers
			delete [] smxs;
		}

		void mapInPlace(T (*f)(T)) {
			mapInPlace(curry(f));
		}

		/*
		*/
		template<typename F>
		void mapInPlace(Fct1<T, T, F> f) {
			// number of elements of the current submatrix
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// current value
			T value;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for private(ne, smx, value)
			// iterate over all locally stored submatrices
			for(int i = 0; i < ns; i++) {
				// store pointer to current submatrix
				smx = smxs[i];
				// determine number of elements
				ne = smx->getElementCountLocal();

				// iterate over all elements
				for(int j = 0; j < ne; j++) {
					// determine current value
					value = smx->getElementLocal(j);
					
					// current value is non-zero
					if(value != 0) {
						// apply given function f to value and set element
						smx->setElementLocal(f(value), j);
					}
				}
			}

			// delete submatrix pointers
			delete [] smxs;
		}

		void mapInPlaceRow(T (*f)(T), int rowIndex) {
			mapInPlaceRow(curry(f), rowIndex);
		}

		/* mapInPlaceRow iteriert über ALLE submatrizen, was zweifelsohne
		   als ineffizient angesehen werden kann. verbesserungsvorschläge:
		   - start- und endmatrix suchen. problem dabei: sowohl die start-
			 als auch die endmatrix sind u.u. nicht gespeichert, da diese
			 nur nullen enthalten.
		   - weiteres problem: alle möglichkeiten müssen damit auskommen,
			 dass die iterationen der for-schleife, die über die submatrizen
			 iteriert, fest sein muss, um mit openmp parallelisierbar zu
			 sein. d.h. im wesentlichen muss der anfang und das ende be-
			 kannt sein.
		*/
		template<typename F>
		void mapInPlaceRow(Fct1<T, T, F> f, int rowIndex) {
			// number of elements of the current submatrix
			int ne;
			// number of locally stored submatrices
			int ns = getSubmatrixCount();
			// current submatrix
			Submatrix<T>* smx;
			// pointers to all locally stored submatrices
			Submatrix<T>** smxs;
			// current value
			T value;

			// allocate space for smxs
			smxs = new Submatrix<T>*[ns];
			// get submatrix pointers
			getSubmatrices(smxs);

			#pragma omp parallel for private(ne, smx, value)
			// iterate over all locally stored submatrices
			for(int i = 0; i < ns; i++) {
				// store pointer to current submatrix
				smx = smxs[i];

				// current submatrix stores elements of the given row index
				if(smx->rowIsLocal(rowIndex)) {
					// determine number of elements
					ne = smx->getElementCountLocal();

					// iterate over all elements
					for(int j = 0; j < ne; j++) {
						// current element belongs to the given row
						if(rowIndex == smx->getRowIndexGlobal(j)) {
							// determine value of the current element
							value = smx->getElementLocal(j);
							
							// value is non-zero
							if(value != 0) {
								// set element
								smx->setElementLocal(f(value), j);
							}
						}
					}
				}
			}

			// delete submatrix pointers
			delete [] smxs;
		}

		// SKELETONS / ZIP

		template<typename T2, typename T3> DistributedSparseMatrix<T3>*
		zip(const DistributedSparseMatrix<T2>& matrix, T3 (*f)(T, T2)) const {
			return zip(matrix, curry(f));
		}

		/* Merges the given matrix with ourself. This is done by applying the given
		   function f to corresponding elements of both matrices. Note that f is only
		   applied if at least one of the corresponding elements is non-zero. For
		   this function to execute as expected, following preconditions apply:

		   - Both matrices must have the same dimension, i.e. n1 == n2 and m1 == m2.
		   - Both matrices must be partitioned identical, i.e. r1 == r2 and c1 == c2.
		   - Both matrices must use the same distribution scheme. Thus, submatrices
			 with the same id are stored on the same process. This is taken care of
			 by the compiler, since the signature forces the user to pass a distributed
			 sparse matrix of type DistributedSparseMatrix<T, S, D>. Thus, both Ds are
			 identical.

		   Note that the implementation of this function is not trivial, as both
		   matrices will most likely not store the same submatrices. For example, the
		   given matrix could store submatrices which are not stored by ourself et vice
		   versa. Thus, it is not sufficient to iterate only over the locally available
		   submatrices but merely over all submatrices which had to be stored locally
		   according to the distribution object.
		*/
		template<typename F> DistributedSparseMatrix<T>*
		zip(const DistributedSparseMatrix<T>& matrix, Fct2<T, T, T, F> f) const {
			// local column index of the current element
			int colIndexLocal;
			// maximum number of submatrices
			int max = getMaxSubmatrixCount();
			// number of local elements of the current submatrix
			int ne;
			// local row index of the current element
			int rowIndexLocal;
			// contains all previously zipped indexes of the current submatrix
			std::set<MatrixIndex> indexes;
			// the new constructed sparse matrix
			DistributedSparseMatrix<T>* result = new DistributedSparseMatrix<T>(n, m, r, c, zero);
			// pointer to own submatrix
			Submatrix<T>* smx1;
			// pointer to submatrix from the given distributed sparse matrix
			Submatrix<T>* smx2;
			// new created submatrix
			Submatrix<T>* smx3;
			// first value
			T value1;
			// second value
			T value2;
			// folded value
			T value3;

			// checking preconditions
			if(n == matrix.getRowCount() && m == matrix.getColumnCount() &&
				r == matrix.getR() && c == matrix.getC()) {
				#pragma omp parallel for private(colIndexLocal, indexes, ne, rowIndexLocal, smx1, smx2, smx3, value1, value2, value3)
				// iterate over all submatrices
				for(int idSubmatrix = 0; idSubmatrix < max; idSubmatrix++) {
					// current submatrix had to be stored locally
					if(distribution->isStoredLocally(id, idSubmatrix)) {
						// determine whether we store the current submatrix
						smx1 = getSubmatrix(idSubmatrix);
						// determine whether given matrix stores the current submatrix
						smx2 = matrix.getSubmatrix(idSubmatrix);
						// set pointer to new submatrix to NULL
						smx3 = NULL;

						// we store the current submatrix
						if(smx1 != NULL) {
							// determine number of locally stored elements
							ne = smx1->getElementCountLocal();

							// iterate over all locally stored elements
							for(int i = 0; i < ne; i++) {
								// determine first value
								value1 = smx1->getElementLocal(i);
								// determine local column index
								colIndexLocal = smx1->getColumnIndexLocal(i);
								// determine local row index
								rowIndexLocal = smx1->getRowIndexLocal(i);

								// given matrix does not store the current submatrix
								if(smx2 == NULL) {
									// set second value to 0
									value2 = zero;
								}
								// given matrix stores the current submatrix
								else {
									// get element from given matrix
									value2 = smx2->getElement(rowIndexLocal, colIndexLocal);
								}

								// at least one of the values is non-zero
								if(value1 != zero || value2 != zero) {
									// fold value
									value3 = f(value1, value2);
									// create matrix index
									MatrixIndex mi(rowIndexLocal, colIndexLocal);
									// add matrix index to set
									indexes.insert(mi);

									// submatrix has not been created yet
									if(smx3 == NULL) {
										// create new submatrix
										smx3 = submatrix->clone();
										// initialize submatrix
										smx3->initialize(idSubmatrix, smx1->getLocalN(),
											smx1->getLocalM(), smx1->getRowIndexStart(),
											smx1->getColumnIndexStart(), value3,
											rowIndexLocal, colIndexLocal);
									}
									// submatrix has already been created
									else {
										// store folded value in local submatrix
										smx3->setElement(value3, rowIndexLocal, colIndexLocal);
									}
								}
							}
						}

						// given matrix stores the current submatrix
						if(smx2 != NULL) {
							// determine number of locally stored elements
							ne = smx2->getElementCountLocal();

							// iterate over all locally stored elements
							for(int i = 0; i < ne; i++) {
								// determine local column index
								colIndexLocal = smx2->getColumnIndexLocal(i);
								// determine local row index
								rowIndexLocal = smx2->getRowIndexLocal(i);
								// create matrix index
								MatrixIndex mi(rowIndexLocal, colIndexLocal);

								// current element has not already been zipped
								if(indexes.count(mi) == 0) {
									// determine second value
									value2 = smx2->getElementLocal(i);

									// we do not store the current submatrix
									if(smx1 == NULL) {
										// set second value to 0
										value1 = zero;
									}
									// we store the current submatrix
									else {
										// get element
										value1 = smx1->getElement(rowIndexLocal, colIndexLocal);
									}

									// at least one of the values is non-zero
									if(value1 != zero || value2 != zero) {
										// fold value
										value3 = f(value1, value2);

										// submatrix has not been created yet
										if(smx3 == NULL) {
											// create new submatrix
											smx3 = submatrix->clone();
											// initialize submatrix
											smx3->initialize(idSubmatrix, smx2->getLocalN(),
												smx2->getLocalM(), smx2->getRowIndexStart(),
												smx2->getColumnIndexStart(), value3,
												rowIndexLocal, colIndexLocal);
										}
										// submatrix has already been created
										else {
											// store folded value in local submatrix
											smx3->setElement(value3, rowIndexLocal, colIndexLocal);
										}
									}
								}
							}
						}

						// new created submatrix stores values
						if(smx3 != NULL) {
							// synchronize access to result matrix, since addSubmatrix is not const!
							#pragma omp critical (zipIndex)
							// add submatrix to result matrix
							result->addSubmatrix(smx3);
						}

						// clear elements from set
						indexes.clear();
					}
				}
			}

			// return result
			return result;
		}

		// SKELETONS / ZIP / ZIP INDEX

		DistributedSparseMatrix<T>* zipIndex(const DistributedSparseMatrix<T>& matrix,
			T (*f)(T, T, int, int)) const {
			return zipIndex(matrix, curry(f));
		}

		/*
		*/
		template<typename F> DistributedSparseMatrix<T>*
		zipIndex(const DistributedSparseMatrix<T>& matrix, Fct4<T, T, int, int, T, F> f) const {
			// global column index of the current element
			int colIndexGlobal;
			// local column index of the current element
			int colIndexLocal;
			// starting column index of the current submatrix
			int colIndexStart;
			// maximum number of submatrices
			int max = getMaxSubmatrixCount();
			// number of local elements of the current submatrix
			int ne;
			// global row index of the current element
			int rowIndexGlobal;
			// local row index of the current element
			int rowIndexLocal;
			// starting row index of the current submatrix
			int rowIndexStart;
			// contains all previously zipped indexes of the current submatrix
			std::set<MatrixIndex> indexes;
			// the new constructed sparse matrix
			DistributedSparseMatrix<T>* result = new DistributedSparseMatrix<T>(n, m, r, c, zero);
			// pointer to own submatrix
			Submatrix<T>* smx1;
			// pointer to submatrix from the given distributed sparse matrix
			Submatrix<T>* smx2;
			// new created submatrix
			Submatrix<T>* smx3;
			// first value
			T value1;
			// second value
			T value2;
			// folded value
			T value3;

			// checking preconditions
			if(n == matrix.getRowCount() && m == matrix.getColumnCount() &&
				r == matrix.getR() && c == matrix.getC()) {
				#pragma omp parallel for private(colIndexGlobal, colIndexLocal, colIndexStart, indexes, ne, rowIndexGlobal, rowIndexLocal, rowIndexStart, smx1, smx2, smx3, value1, value2, value3)
				// iterate over all submatrices
				for(int idSubmatrix = 0; idSubmatrix < max; idSubmatrix++) {
					// current submatrix had to be stored locally
					if(distribution->isStoredLocally(id, idSubmatrix)) {
						// determine whether we store the current submatrix
						smx1 = getSubmatrix(idSubmatrix);
						// determine whether given matrix stores the current submatrix
						smx2 = matrix.getSubmatrix(idSubmatrix);
						// set pointer to new submatrix to NULL
						smx3 = NULL;

						// we store the current submatrix
						if(smx1 != NULL) {
							// determine number of locally stored elements
							ne = smx1->getElementCountLocal();
							// determine starting column index
							colIndexStart = smx1->getColumnIndexStart();
							// determine starting row index
							rowIndexStart = smx1->getRowIndexStart();

							// iterate over all locally stored elements
							for(int i = 0; i < ne; i++) {
								// determine first value
								value1 = smx1->getElementLocal(i);
								// determine local column index
								colIndexLocal = smx1->getColumnIndexLocal(i);
								// determine local row index
								rowIndexLocal = smx1->getRowIndexLocal(i);

								// given matrix does not store the current submatrix
								if(smx2 == NULL) {
									// set second value to 0
									value2 = zero;
								}
								// given matrix stores the current submatrix
								else {
									// get element from given matrix
									value2 = smx2->getElement(rowIndexLocal, colIndexLocal);
								}

								// at least one of the values is non-zero
								if(value1 != zero || value2 != zero) {
									// compute global column index
									colIndexGlobal = colIndexStart + colIndexLocal;
									// compute global row index
									rowIndexGlobal = rowIndexStart + rowIndexLocal;
									// fold value
									value3 = f(value1, value2, rowIndexGlobal, colIndexGlobal);
									// create matrix index
									MatrixIndex mi(rowIndexLocal, colIndexLocal);
									// add matrix index to set
									indexes.insert(mi);

									// submatrix has not been created yet
									if(smx3 == NULL) {
										// create new submatrix
										smx3 = submatrix->clone();
										// initialize submatrix
										smx3->initialize(idSubmatrix, smx1->getLocalN(),
											smx1->getLocalM(), rowIndexStart, colIndexStart,
											value3, rowIndexLocal, colIndexLocal);
									}
									// submatrix has already been created
									else {
										// store folded value in local submatrix
										smx3->setElement(value3, rowIndexLocal, colIndexLocal);
									}
								}
							}
						}

						// given matrix stores the current submatrix
						if(smx2 != NULL) {
							// determine number of locally stored elements
							ne = smx2->getElementCountLocal();
							// determine starting column index
							colIndexStart = smx2->getColumnIndexStart();
							// determine starting row index
							rowIndexStart = smx2->getRowIndexStart();

							// iterate over all locally stored elements
							for(int i = 0; i < ne; i++) {
								// determine local column index
								colIndexLocal = smx2->getColumnIndexLocal(i);
								// determine local row index
								rowIndexLocal = smx2->getRowIndexLocal(i);
								// calculate global column index
								colIndexGlobal = colIndexStart + colIndexLocal;
								// calculate global row index
								rowIndexGlobal = rowIndexStart + rowIndexLocal;
								// create matrix index
								MatrixIndex mi(rowIndexLocal, colIndexLocal);

								// current element has not already been zipped
								if(indexes.count(mi) == 0) {
									// determine second value
									value2 = smx2->getElementLocal(i);

									// we do not store the current submatrix
									if(smx1 == NULL) {
										// set second value to 0
										value1 = zero;
									}
									// we store the current submatrix
									else {
										// get element
										value1 = smx1->getElement(rowIndexLocal, colIndexLocal);
									}

									// at least one of the values is non-zero
									if(value1 != zero || value2 != zero) {
										// fold value
										value3 = f(value1, value2, rowIndexGlobal, colIndexGlobal);

										// submatrix has not been created yet
										if(smx3 == NULL) {
											// create new submatrix
											smx3 = submatrix->clone();
											// initialize submatrix
											smx3->initialize(idSubmatrix, smx2->getLocalN(),
												smx2->getLocalM(), rowIndexStart, colIndexStart,
												value3, rowIndexLocal, colIndexLocal);
										}
										// submatrix has already been created
										else {
											// store folded value in local submatrix
											smx3->setElement(value3, rowIndexLocal, colIndexLocal);
										}
									}
								}
							}
						}

						// new created submatrix stores values
						if(smx3 != NULL) {
							// synchronize access to result matrix, since addSubmatrix is not const!
							#pragma omp critical (zipIndex)
							// add submatrix to result matrix
							result->addSubmatrix(smx3);
						}

						// clear elements from set
						indexes.clear();
					}
				}
			}

			// return result
			return result;
		}

		// SKELETONS / ZIP / ZIP INDEX IN PLACE

		template<typename T2>
		void zipIndexInPlace(const DistributedSparseMatrix<T2>& matrix, T (*f)(T, T2, int, int)) {
			return zipIndexInPlace(matrix, curry(f));
		}

		/* This version of the zipIndexInPlace skeleton does not use any critical
		   directives. Instead, it stores newly created submatrices in a temporary
		   vector. Each thread has its own vector, all vectors are stored in a
		   temporary array. Finally, all submatrices must be added by the master
		   thread. Unexpectedly, this version does not clearly outperform the
		   version which uses critical directives to synchronize the threads. I
		   assume that the chance that two or more threads want to execute the
		   addSubmatrix method simultaneously is simply too low since there are a
		   lot of other statements to execute. However, you can create a test
		   scenario where zipIndexInPlace clearly outperforms zipIndexInPlaceCritical
		   (cf. testcases::testZipIndexInPlace). Here, the runtime is about 50%
		   lower!
		*/
		template<typename F, typename T2>
		void zipIndexInPlace(const DistributedSparseMatrix<T2>& matrix, Fct4<T, T2, int, int, T, F> f) {
			// global column index of the current element
			int colIndexGlobal;
			// local column index of the current element
			int colIndexLocal;
			// starting column index of the current submatrix
			int colIndexStart;
			// maximum number of submatrices
			int max = getMaxSubmatrixCount();
			// number of local elements of the current submatrix
			int ne;
			// global row index of the current element
			int rowIndexGlobal;
			// local row index of the current element
			int rowIndexLocal;
			// starting row index of the current submatrix
			int rowIndexStart;
			// contains all previously zipped indexes of the current submatrix
			std::set<MatrixIndex> indexes;
			// pointer to own submatrix
			Submatrix<T>* smx1;
			// pointer to submatrix from the given distributed sparse matrix
			Submatrix<T2>* smx2;
			// first value
			T value1;
			// second value
			T2 value2;
			// folded value
			T value3;

			// thread id
			int idThread;
			// maximum number of threads
			int nt = oal::getMaxThreads();
			// array containing vectors of submatrix pointers
			std::vector<Submatrix<T>*>* smxs = new std::vector<Submatrix<T>*>[nt];
			// size of the current vector
			int size;

			// checking preconditions
			if(n == matrix.getRowCount() && m == matrix.getColumnCount() &&
				r == matrix.getR() && c == matrix.getC()) {
				#pragma omp parallel private(idThread)
				{
					// determine thread id
					idThread = oal::getThreadNum();

					#pragma omp for private(colIndexGlobal, colIndexLocal, colIndexStart, indexes, ne, rowIndexGlobal, rowIndexLocal, rowIndexStart, smx1, smx2, value1, value2, value3)
					// iterate over all submatrices
					for(int idSubmatrix = 0; idSubmatrix < max; idSubmatrix++) {
						// current submatrix had to be stored locally
						if(distribution->isStoredLocally(id, idSubmatrix)) {
							// determine whether we store the current submatrix
							smx1 = getSubmatrix(idSubmatrix);
							// determine whether given matrix stores the current submatrix
							smx2 = matrix.getSubmatrix(idSubmatrix);

							// we store the current submatrix
							if(smx1 != NULL) {
								// determine number of locally stored elements
								ne = smx1->getElementCountLocal();
								// determine starting column index
								colIndexStart = smx1->getColumnIndexStart();
								// determine starting row index
								rowIndexStart = smx1->getRowIndexStart();

								// iterate over all locally stored elements
								for(int i = 0; i < ne; i++) {
									// determine first value
									value1 = smx1->getElementLocal(i);
									// determine local column index
									colIndexLocal = smx1->getColumnIndexLocal(i);
									// determine local row index
									rowIndexLocal = smx1->getRowIndexLocal(i);

									// given matrix does not store the current submatrix
									if(smx2 == NULL) {
										// set second value to 0
										value2 = zero;
									}
									// given matrix stores the current submatrix
									else {
										// get element from given matrix
										value2 = smx2->getElement(rowIndexLocal, colIndexLocal);
									}

									// at least one of the values is non-zero
									if(value1 != zero || value2 != zero) {
										// compute global column index
										colIndexGlobal = colIndexStart + colIndexLocal;
										// compute global row index
										rowIndexGlobal = rowIndexStart + rowIndexLocal;
										// fold value
										value3 = f(value1, value2, rowIndexGlobal, colIndexGlobal);
										// create matrix index
										MatrixIndex mi(rowIndexLocal, colIndexLocal);
										// add matrix index to set
										indexes.insert(mi);

										// submatrix has not been created yet
										if(smx1 == NULL) {
											// create new submatrix
											smx1 = submatrix->clone();
											// initialize submatrix
											smx1->initialize(idSubmatrix, smx1->getLocalN(),
												smx1->getLocalM(), rowIndexStart, colIndexStart,
												value3, rowIndexLocal, colIndexLocal);
											// add submatrix to vector. alternative: 
											// #pragma omp critical (zipIndexInPlace)
											// addSubmatrix(smx1);
											smxs[idThread].push_back(smx1);
										}
										// submatrix has already been created
										else {
											// store folded value in local submatrix
											smx1->setElement(value3, rowIndexLocal, colIndexLocal);
										}
									}
								}
							}

							// given matrix stores the current submatrix
							if(smx2 != NULL) {
								// determine number of locally stored elements
								ne = smx2->getElementCountLocal();
								// determine starting column index
								colIndexStart = smx2->getColumnIndexStart();
								// determine starting row index
								rowIndexStart = smx2->getRowIndexStart();

								// iterate over all locally stored elements
								for(int i = 0; i < ne; i++) {
									// determine local column index
									colIndexLocal = smx2->getColumnIndexLocal(i);
									// determine local row index
									rowIndexLocal = smx2->getRowIndexLocal(i);
									// calculate global column index
									colIndexGlobal = colIndexStart + colIndexLocal;
									// calculate global row index
									rowIndexGlobal = rowIndexStart + rowIndexLocal;
									// create matrix index
									MatrixIndex mi(rowIndexLocal, colIndexLocal);

									// current element has not already been zipped
									if(indexes.count(mi) == 0) {
										// determine second value
										value2 = smx2->getElementLocal(i);

										// we do not store the current submatrix
										if(smx1 == NULL) {
											// set second value to 0
											value1 = zero;
										}
										// we store the current submatrix
										else {
											// get element
											value1 = smx1->getElement(rowIndexLocal, colIndexLocal);
										}

										// at least one of the values is non-zero
										if(value1 != zero || value2 != zero) {
											// fold value
											value3 = f(value1, value2, rowIndexGlobal, colIndexGlobal);

											// submatrix has not been created yet
											if(smx1 == NULL) {
												// create new submatrix
												smx1 = submatrix->clone();
												// initialize submatrix
												smx1->initialize(idSubmatrix, smx2->getLocalN(),
													smx2->getLocalM(), rowIndexStart, colIndexStart,
													value3, rowIndexLocal, colIndexLocal);
												// add submatrix to vector. alternative:
												// #pragma omp critical (zipIndexInPlace)
												// addSubmatrix(smx1);
												smxs[idThread].push_back(smx1);
											}
											// submatrix has already been created
											else {
												// store folded value in local submatrix
												smx1->setElement(value3, rowIndexLocal, colIndexLocal);
											}
										}
									}
								}
							}

							// clear elements from set
							indexes.clear();
						}
					}
				}
				// end of parallel region

				// iterate over all threads
				for(int i = 0; i < nt; i++) {
					// determine number of submatrices
					size = (int)smxs[i].size();

					// iterate over all submatrices
					for(int j = 0; j < size; j++) {
						// add submatrix to ourself
						addSubmatrix(smxs[i][j]);
					}
				}

				// delete array of submatrices
				delete [] smxs;
			}
		}

		//
		// BEGIN: JUST FOR TESTING PURPOSES.
		//

		void zipIndexInPlaceCritical(const DistributedSparseMatrix<T>& matrix,
			T (*f)(T, T, int, int)) {
			return zipIndexInPlaceCritical(matrix, curry(f));
		}

		/* Variant of the zipIndexInPlace skeleton which uses the critical directive
		   to synchronize the usage of the addSubmatrix method. Surprisingly, this
		   method performs faster than its counterpart which uses an array to store
		   the submatrices and thus avoids the critical directive.
		*/
		template<typename F>
		void zipIndexInPlaceCritical(const DistributedSparseMatrix<T>& matrix,
			Fct4<T, T, int, int, T, F> f) {
			// global column index of the current element
			int colIndexGlobal;
			// local column index of the current element
			int colIndexLocal;
			// starting column index of the current submatrix
			int colIndexStart;
			// maximum number of submatrices
			int max = getMaxSubmatrixCount();
			// number of local elements of the current submatrix
			int ne;
			// global row index of the current element
			int rowIndexGlobal;
			// local row index of the current element
			int rowIndexLocal;
			// starting row index of the current submatrix
			int rowIndexStart;
			// contains all previously zipped indexes of the current submatrix
			std::set<MatrixIndex> indexes;
			// pointer to own submatrix
			Submatrix<T>* smx1;
			// pointer to submatrix from the given distributed sparse matrix
			Submatrix<T>* smx2;
			// first value
			T value1;
			// folded value
			T value3;
			// second value
			T value2;

			// checking preconditions
			if(n == matrix.getRowCount() && m == matrix.getColumnCount() &&
				r == matrix.getR() && c == matrix.getC()) {
				#pragma omp parallel for private(colIndexGlobal, colIndexLocal, colIndexStart, indexes, ne, rowIndexGlobal, rowIndexLocal, rowIndexStart, smx1, smx2, value1, value2, value3)
				// iterate over all submatrices
				for(int idSubmatrix = 0; idSubmatrix < max; idSubmatrix++) {
					// current submatrix had to be stored locally
					if(distribution->isStoredLocally(id, idSubmatrix)) {
						// determine whether we store the current submatrix
						smx1 = getSubmatrix(idSubmatrix);
						// determine whether given matrix stores the current submatrix
						smx2 = matrix.getSubmatrix(idSubmatrix);

						// we store the current submatrix
						if(smx1 != NULL) {
							// determine number of locally stored elements
							ne = smx1->getElementCountLocal();
							// determine starting column index
							colIndexStart = smx1->getColumnIndexStart();
							// determine starting row index
							rowIndexStart = smx1->getRowIndexStart();

							// iterate over all locally stored elements
							for(int i = 0; i < ne; i++) {
								// determine first value
								value1 = smx1->getElementLocal(i);
								// determine local column index
								colIndexLocal = smx1->getColumnIndexLocal(i);
								// determine local row index
								rowIndexLocal = smx1->getRowIndexLocal(i);

								// given matrix does not store the current submatrix
								if(smx2 == NULL) {
									// set second value to 0
									value2 = zero;
								}
								// given matrix stores the current submatrix
								else {
									// get element from given matrix
									value2 = smx2->getElement(rowIndexLocal, colIndexLocal);
								}

								// at least one of the values is non-zero
								if(value1 != zero || value2 != zero) {
									// compute global column index
									colIndexGlobal = colIndexStart + colIndexLocal;
									// compute global row index
									rowIndexGlobal = rowIndexStart + rowIndexLocal;
									// fold value
									value3 = f(value1, value2, rowIndexGlobal, colIndexGlobal);
									// create matrix index
									MatrixIndex mi(rowIndexLocal, colIndexLocal);
									// add matrix index to set
									indexes.insert(mi);

									// submatrix has not been created yet
									if(smx1 == NULL) {
										// create new submatrix
										smx1 = submatrix->clone();
										// initialize submatrix
										smx1->initialize(idSubmatrix, smx1->getLocalN(),
											smx1->getLocalM(), rowIndexStart, colIndexStart,
											value3, rowIndexLocal, colIndexLocal);
										// add submatrix to ourself
										#pragma omp critical (zipIndexInPlace)
										addSubmatrix(smx1);
									}
									// submatrix has already been created
									else {
										// store folded value in local submatrix
										smx1->setElement(value3, rowIndexLocal, colIndexLocal);
									}
								}
							}
						}

						// given matrix stores the current submatrix
						if(smx2 != NULL) {
							// determine number of locally stored elements
							ne = smx2->getElementCountLocal();
							// determine starting column index
							colIndexStart = smx2->getColumnIndexStart();
							// determine starting row index
							rowIndexStart = smx2->getRowIndexStart();

							// iterate over all locally stored elements
							for(int i = 0; i < ne; i++) {
								// determine local column index
								colIndexLocal = smx2->getColumnIndexLocal(i);
								// determine local row index
								rowIndexLocal = smx2->getRowIndexLocal(i);
								// calculate global column index
								colIndexGlobal = colIndexStart + colIndexLocal;
								// calculate global row index
								rowIndexGlobal = rowIndexStart + rowIndexLocal;
								// create matrix index
								MatrixIndex mi(rowIndexLocal, colIndexLocal);

								// current element has not already been zipped
								if(indexes.count(mi) == 0) {
									// determine second value
									value2 = smx2->getElementLocal(i);

									// we do not store the current submatrix
									if(smx1 == NULL) {
										// set second value to 0
										value1 = zero;
									}
									// we store the current submatrix
									else {
										// get element
										value1 = smx1->getElement(rowIndexLocal, colIndexLocal);
									}

									// at least one of the values is non-zero
									if(value1 != zero || value2 != zero) {
										// fold value
										value3 = f(value1, value2, rowIndexGlobal, colIndexGlobal);

										// submatrix has not been created yet
										if(smx1 == NULL) {
											// create new submatrix
											smx1 = submatrix->clone();
											// initialize submatrix
											smx1->initialize(idSubmatrix, smx2->getLocalN(),
												smx2->getLocalM(), rowIndexStart, colIndexStart,
												value3, rowIndexLocal, colIndexLocal);
											// add submatrix to ourself
											#pragma omp critical (zipIndexInPlace)
											addSubmatrix(smx1);
										}
										// submatrix has already been created
										else {
											// store folded value in local submatrix
											smx1->setElement(value3, rowIndexLocal, colIndexLocal);
										}
									}
								}
							}
						}

						// clear elements from set
						indexes.clear();
					}
				}
			}
		}

		//
		// END: JUST FOR TESTING PURPOSES.
		//

		// SKELETONS / ZIP / ZIP IN PLACE

		void zipInPlace(const DistributedSparseMatrix<T>& matrix, T (*f)(T, T)) {
			return zipInPlace(matrix, curry(f));
		}

		template<typename F>
		void zipInPlace(const DistributedSparseMatrix<T>& matrix, Fct2<T, T, T, F> f) {
			// local column index of the current element
			int colIndexLocal;
			// thread id
			int idThread;
			// maximum number of submatrices
			int max = getMaxSubmatrixCount();
			// number of local elements of the current submatrix
			int ne;
			// number of submatrices
			int ns;
			// number of threads
			int nt = oal::getMaxThreads();
			// local row index of the current element
			int rowIndexLocal;
			// contains all previously zipped indexes of the current submatrix
			std::set<MatrixIndex> indexes;
			// temporary field of newly created submatrices
			std::vector<Submatrix<T>*>* tmp = new std::vector<Submatrix<T>*>[nt];
			// pointer to own submatrix
			Submatrix<T>* smx1;
			// pointer to submatrix from the given distributed sparse matrix
			Submatrix<T>* smx2;
			// first value
			T value1;
			// folded value
			T value3;
			// second value
			T value2;


			// checking preconditions
			if(distribution->equals(*matrix.distribution)) {
				#pragma omp parallel private(idThread)
				{
					// determine thread id
					idThread = oal::getThreadNum();

					#pragma omp for private(colIndexLocal, indexes, ne, rowIndexLocal, smx1, smx2, value1, value2, value3)
					// iterate over all submatrices
					for(int idSubmatrix = 0; idSubmatrix < max; idSubmatrix++) {
						// current submatrix had to be stored locally
						if(distribution->isStoredLocally(id, idSubmatrix)) {
							// determine whether we store the current submatrix
							smx1 = getSubmatrix(idSubmatrix);
							// determine whether given matrix stores the current submatrix
							smx2 = matrix.getSubmatrix(idSubmatrix);

							// given matrix does not store the current submatrix
							if(smx2 == NULL) {
								// set value 2 to zero
								value2 = zero;
							}

							// we store the current submatrix
							if(smx1 != NULL) {
								// determine number of locally stored elements
								ne = smx1->getElementCountLocal();

								// iterate over all locally stored elements
								for(int k = 0; k < ne; k++) {
									// determine first value
									value1 = smx1->getElementLocal(k);
									// determine local column index
									colIndexLocal = smx1->getColumnIndexLocal(k);
									// determine local row index
									rowIndexLocal = smx1->getRowIndexLocal(k);

									// given matrix stores the current submatrix
									if(smx2 != NULL) {
										// get element from given matrix
										value2 = smx2->getElement(rowIndexLocal, colIndexLocal);
									}

									// create matrix index
									MatrixIndex mi(rowIndexLocal, colIndexLocal);
									// add matrix index to set
									indexes.insert(mi);
									// store folded value in local submatrix
									smx1->setElement(f(value1, value2), rowIndexLocal, colIndexLocal);
								}
							}

							// given matrix stores the current submatrix
							if(smx2 != NULL) {
								// determine number of locally stored elements
								ne = smx2->getElementCountLocal();

								// iterate over all locally stored elements
								for(int i = 0; i < ne; i++) {
									// determine local column index
									colIndexLocal = smx2->getColumnIndexLocal(i);
									// determine local row index
									rowIndexLocal = smx2->getRowIndexLocal(i);
									// create matrix index
									MatrixIndex mi(rowIndexLocal, colIndexLocal);

									// current element has not already been zipped
									if(indexes.count(mi) == 0) {
										// determine second value
										value2 = smx2->getElementLocal(i);
										// fold value. note that value1 is always set to zero.
										// this can be done, since the current value2 would
										// already have been folded by smx1, if it were non-zero
										value3 = f(zero, value2);

										// submatrix has not been created yet
										if(smx1 == NULL) {
											// create new submatrix
											smx1 = submatrix->clone();
											// initialize submatrix
											smx1->initialize(idSubmatrix, getRowCount(idSubmatrix),
												getColumnCount(idSubmatrix),
												getRowIndexStart(idSubmatrix),
												getColumnIndexStart(idSubmatrix),
												value3, rowIndexLocal, colIndexLocal);
											// add submatrix to ourself
											//#pragma omp critical (zipIndexInPlace)
											//addSubmatrix(smx1);
											// add submatrix to field of vectors
											tmp[idThread].push_back(smx1);
										}
										// submatrix has already been created
										else {
											// store folded value in local submatrix
											smx1->setElement(value3, rowIndexLocal, colIndexLocal);
										}
									}
								}
							}

							// clear elements from set
							indexes.clear();
						}
					}
				}

				// iterate over all threads
				for(int i = 0; i < nt; i++) {
					// determine number of submatrices
					ns = tmp[i].size();

					// iterate over all submatrices
					for(int j = 0; j < ns; j++) {
						// add current submatrix to ourself
						addSubmatrix(tmp[i][j]);
					}
				}

				// delete temporary field of submatrices
				delete [] tmp;
			}
		}

	};

}

#endif
