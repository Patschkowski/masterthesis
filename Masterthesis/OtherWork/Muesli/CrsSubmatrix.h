// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef CRS_SUBMATRIX_H
#define CRS_SUBMATRIX_H

#include <iostream>
#include <sstream>

#include "Submatrix.h"

namespace msl {

	template<typename T = double> class CrsSubmatrix: public Submatrix<T> {

	private:

		// ATTRIBUTES

		// stores the starting row indexes
		std::vector<int> ia;
		// stores the column index of each element
		std::vector<int> ja;

		// FUNCTIONS

		/* Adjusts the values of vector ia by increasing them by 1.
		   This ensures that the pointers point to the right element
		   after inserting an element.
		*/
		void adjustVectorIa(int index) {
			// number of locally stored elements
			int size = (int)ia.size();
			// temporary value
			int tmp;

			// iterate over all locally available elements starting with the given index
			for(int i = index; i < size; i++) {
				// determine current value of vector ia
				tmp = ia.at(i);

				// current value is not -1
				if(tmp != -1) {
					// increase current value by 1
					ia.at(i) = tmp + 1;
				}
			}
		}

		/* Deletes the element at the given local row and local column index.
		*/
		void deleteElement(int localRowIndex, int localColIndex) {
			// number of elements in the given row
			int elems = getElementCountInRow(localRowIndex);
			// index of the ja array
			int jaIndex = getIndexJa(localRowIndex, localColIndex);
			// temporary value
			int tmp;

			// last element of the given row is deleted
			if(elems == 1) {
				// set value of vector ia at the given local row index
				// to -1, i.e. the row does not contain any elements
				ia.at(localRowIndex) = -1;
			}

			// remove element from values vector
			this->remove(this->values, jaIndex);
			// remove element from vector ja
			this->remove(ja, jaIndex);
			
			// decrease all further values of vector ia
			for(int i = localRowIndex + 1; i < (int)ia.size(); i++) {
				// determine current value of vector ia
				tmp = ia.at(i);

				// current value is not -1
				if(tmp != -1) {
					// decrease current value
					ia.at(i) = tmp - 1;
				}
			}
		}
		
		/* Return the total number of elements in the given row.
		*/
		int getElementCountInRowNew(int localRowIndex) const {
			int rowI;
			int rowJ;

			// ATTENTION 2: submatrix can be completely empty,
			// i.e. ia only stores one element, namely 0
			if(ia.size() == 1) {
				return 0;
			}

			// per definition: #elements(row) = ia[row + 1] - ia[row]
			// ATTENTION 1: rows can be empty, i.e. rowJ and rowI can be -1
			rowI = ia.at(localRowIndex + 1);
			rowJ = ia.at(localRowIndex);

			// given row is empty
			if(rowJ == -1) {
				return 0;
			}

			// search for the next non-empty row
			if(rowI == -1) {
				rowI = getNextNonEmptyRow(localRowIndex + 2);
			}

			return rowI - rowJ;
		}

		int getElementCountInRow(int localRowIndex) const {
			// per definition: #elements(row) = ia[row + 1] - ia[row]
			// ATTENTION: rows can be empty, i.e. rowJ and rowI can be -1
			int rowI = ia.at(localRowIndex + 1);
			int rowJ = ia.at(localRowIndex);

			// given row is empty
			if(rowJ == -1) {
				return 0;
			}

			// search for the next non-empty row
			if(rowI == -1) {
				rowI = getNextNonEmptyRow(localRowIndex + 2);
			}

			return rowI - rowJ;
		}
		
		/* Returns the index of the first element of the given row. The index is
		   used to access the desired element in vector a.
		*/
		int getIndexA(int localRowIndex) const {
			return ia.at(localRowIndex);
		}
		
		/* Searches the given element in the given vector in the given range. The method
		   implements a binary search in the vector whose elements are supposed to be
		   sorted. Thus, the asymptotical cost are O(log m) instead of O(m) when using a
		   linear search algorithm.
		*/
		int getIndexEquals(const std::vector<int>& v, int element, int indexLeft, int indexRight) const {
			// the element in the middle
			double median;
			// the index of the median
			int indexMedian;
			
			// determine index of middle element
			indexMedian = (indexLeft + indexRight) / 2;
			// extract middle element
			median = v.at(indexMedian);
			
			// median matches the searched element
			if(median == element) {
				return indexMedian;
			}
			
			// no element could be found
			if(indexLeft >= indexRight) {
				return -1;
			}
			
			// middle element is smaller than searched one
			if(median < element) {
				return getIndexEquals(v, element, indexMedian + 1, indexRight);
			}
			// middle element is greater than searched one
			else {
				return getIndexEquals(v, element, indexLeft, indexMedian - 1);
			}
		}
		
		/* Linearly searches the given vector in the given range and returns the index whose
		   value is greater than the given value. The function assumes, that the elements in
		   the given vector are sorted.

		   tested: true
		*/
		int getIndexGreater(const std::vector<int>& v, int element, int indexLeft, int indexRight) const {
			// iterate over all elements in the given range
			for(int i = indexLeft; i <= indexRight; i++) {
				// current element is greater than the given one
				if(element < v.at(i)) {
					// return current index
					return i;
				}
			}

			// no element is greater than given one
			return indexRight + 1;
		}
		
		/* Determines the index of the vector ja which stores the searched element.
		   If no element can be found with the given position -1 is returned. The
		   asymptotical costs of the method are O(log m).
		*/
		int getIndexJa(int localRowIndex, int localColIndex) const {
			// number of elements in the given row
			int elems = getElementCountInRow(localRowIndex);
			// index of the first element of the given row
			int first = getIndexA(localRowIndex);
			
			// given column index is smaller than first column index in ja
			if(localColIndex < ja.at(first)) {
				return -1;
			}
			
			// search for the element in vector ja
			return getIndexEquals(ja, localColIndex, first, first + elems - 1);
		}

		/* Returns the index of the next row which is not equal to -1.
		*/
		int getNextNonEmptyRow(int rowIndexStart) const {
			// iterate over all rows starting with the given row index
			for(int i = rowIndexStart; i < this->nLocal + 1; i++) {
				// current value is not -1
				if(ia.at(i) != -1) {
					// return current vlaue
					return ia.at(i);
				}
			}

			// no empty row found; return -1
			return -1;
		}

		/* Inserts the given value at the given local row and local column index. The
		   function assumes, that the given position is not used yet, i.e. "stores" a 0.
		*/
		void insertElement(int localRowIndex, int localColIndex, T value) {
			// number of elements in the current row
			int elems = getElementCountInRow(localRowIndex);
			// index of the vector ja where to store the new element
			int jaIndex;

			switch(elems) {
				// given row does not story any elements yet
				case 0: {
					// determine next non-empty row index
					jaIndex = getNextNonEmptyRow(localRowIndex + 1);
					// set pointer in vector ia
					ia.at(localRowIndex) = jaIndex;
					// break switch-case-statement
					break;
				}
				// given row stores one element
				case 1: {
					// determine ja index of the stored element
					jaIndex = ia.at(localRowIndex);
					// new element has to be inserted after the existing element
					if(ja.at(jaIndex) < localColIndex) {
						jaIndex++;
					}
					// break switch-case-statement
					break;
				}
				// given row stores more than one element
				default: {
					// determine index where to insert the given element
					jaIndex = getIndexGreater(ja, localColIndex, ia.at(localRowIndex),
						ia.at(localRowIndex) + elems - 1);
					// break switch-case-statement
					break;
				}
			}

			// insert value into values vector
			this->insert(this->values, jaIndex, value);
			// insert column into ja vector
			this->insert(ja, jaIndex, localColIndex);
			// increase following pointers of vector ia
			adjustVectorIa(localRowIndex + 1);
		}

		/* Replaces the element at the given ja index with the given value.
		*/
		void replaceElement(int localRowIndex, int localColIndex, T value) {
			int jaIndex = getIndexJa(localRowIndex, localColIndex);

			this->values.at(jaIndex) = value;
		}

	protected:

		// ATTRIBUTES

		// FUNCTIONS

	public:

		// ATTRIBUTES

		// FUNCTIONS

		void debug() const {
			std::ostringstream stream;

			stream << "id  = " << this->id     << std::endl;
			stream << "n   = " << this->nLocal << std::endl;
			stream << "m   = " << this->mLocal << std::endl;
			stream << "ris = " << this->i0     << std::endl;
			stream << "cis = " << this->j0     << std::endl;

			stream << "v   = [";

			for(int i = 0; i < (int)(this->values.size()); i++) {
				stream << this->values.at(i);

				if(i < (int)(this->values.size()) - 1) {
					stream << "; ";
				}
			}

			stream << "]" << std::endl << "ia  = [";

			for(int i = 0; i < (int)ia.size(); i++) {
				stream << (int)ia.at(i);

				if(i < (int)ia.size() - 1) {
					stream << "; ";
				}
			}

			stream << "]" << std::endl << "ja  = [";

			for(int i = 0; i < (int)ja.size(); i++) {
				stream << ja.at(i);

				if(i < (int)ja.size() - 1) {
					stream << "; ";
				}
			}

			stream << "]";

			printf("%s\n", stream.str().c_str());
		}

		/*
		*/
		CrsSubmatrix* clone() const {
			return new CrsSubmatrix(*this);
		}

		/* 
		*/
		int getColumnIndexLocal(int index) const {
			return ja[index];
		}

		/* Returns the element at the given position.
		*/
		T getElement(int localRowIndex, int localColumnIndex) const {
			int jaIndex;

			// row could be empty
			if(ia.at(localRowIndex) == -1) {
				return this->zero;
			}

			// determine ja index
			jaIndex = getIndexJa(localRowIndex, localColumnIndex);

			// element could not be found
			if(jaIndex == -1) {
				return this->zero;
			}
			// element could be found
			else {
				return this->values.at(jaIndex);
			}
		}

		/* Calculates the row index of the element with the given index.
		   The given index is supposed to be an index referring to the
		   values vector.
		*/
		int getRowIndexLocal(int index) const {
			// current index of ia
			int indexIa;
			// result
			int result = 0;
			// size of ia
			int size = (int)ia.size();

			// iterate over all rows
			for(int i = 0; i < size; i++) {
				// determine current starting index
				indexIa = ia.at(i);

				// current index of ia is greater than given index
				if(indexIa > index) {
					// return last row index
					return result;
				}
				// current index of ia is not greater than given index
				else {
					// current index must not be -1
					if(indexIa != -1) {
						// store current index of ia
						result = i;
					}
				}
			}

			// return result
			return result;
		}

		/* Used to re-compress the submatrix after using the getElements
		   function
		*/
		void pack() {
			// column index of a zero element
			int colIndex;
			// row index of a zero element
			int rowIndex;
			// number of locally stored elements
			int size = (int)this->values.size();

			// iterate over all locally stored elements
			for(int i = size - 1; i >= 0; i--) {
				// current element is zero
				if(this->values.at(i) == this->zero) {
					// determine column index
					colIndex = ja.at(i);
					// determine row index
					rowIndex = getRowIndexLocal(i);
					// delete current element
					deleteElement(rowIndex, colIndex);
				}
			}
		}

		/* Sets the given element at the given local row and local column index.
		*/
		void setElement(T value, int localRowIndex, int localColumnIndex) {
			// number of elements in the current row
			int elems = getElementCountInRow(localRowIndex);
			// the old value of the submatrix
			T oldValue;

			// given row does not store any elements
			if(elems == 0) {
				// given value is not equal to zero
				if(value != this->zero) {
					// insert element
					insertElement(localRowIndex, localColumnIndex, value);
				}
			}
			// given row stores at least one element
			else {
				// retrieve old value
				oldValue = getElement(localRowIndex, localColumnIndex);

				if(oldValue != this->zero && value == this->zero) {
					// delete element
					deleteElement(localRowIndex, localColumnIndex);
				}

				if(oldValue == this->zero && value != this->zero) {
					// insert element
					insertElement(localRowIndex, localColumnIndex, value);
				}

				if(oldValue != this->zero && value != this->zero) {
					// replace element
					replaceElement(localRowIndex, localColumnIndex, value);
				}
			}
		}
		
		void test() const {
			int globalRowIndex;
			int globalColIndex;

			for(int row = 0; row < this->nLocal; row++) {
				for(int col = 0; col < this->mLocal; col++) {
					globalRowIndex = this->getGlobalRowIndex(row);
					globalColIndex = this->getGlobalColIndex(col);

					printf("id = %d, (%d, %d) -> (%d, %d)\n",
						this->id, row, col, globalRowIndex, globalColIndex);
				}
			}
		}
		
		/* Returns a string representation of the submatrix.
		*/
		std::string toString() const {
			std::ostringstream stream;

			for(int i = 0; i < this->nLocal; i++) {
				stream << "[";

				for(int j = 0; j < this->mLocal; j++) {
					stream << getElement(i, j);

					if(j < this->mLocal - 1) {
						stream << "; ";
					}
				}

				stream << "]" << std::endl;
			}

			return stream.str();
		}

		// FUNCTIONS / CONSTRUCTORS
		
		/* Default constructor. Only needed for the inheritance.
		*/
		CrsSubmatrix(): Submatrix<T>() {
		}

		/* Destructor
		*/
		~CrsSubmatrix() {
		}

		// FUNCTIONS / INITIALIZERS

		/* Hiding problem (FAQ 23.9)! Alternative: using Submatrix<T>::initialize;
		*/
		void initialize(int id, int nLocal, int mLocal, int rowIndexStart, int columnIndexStart) {
			// init submatrix
			this->init(id, nLocal, mLocal, rowIndexStart, columnIndexStart);
			
			// iterate over all rows
			for(int i = 0; i < nLocal; i++) {
				// push back -1
				ia.push_back(-1);
			}

			// push back 0 to last position
			ia.push_back(0);
		}

		/* Enhanced "constructor". Constructs a submatrix with only one element.
		*/
		void initialize(int id, int nLocal, int mLocal, int rowIndexStart, int columnIndexStart,
			T value, int rowIndex, int columnIndex) {
			// temporary value for the column value
			int tmp;

			// init submatrix
			this->init(id, nLocal, mLocal, rowIndexStart, columnIndexStart);
			// insert value to values vector
			this->values.push_back(value);
			// insert column to ja vector
			ja.push_back(columnIndex);

			// create ia vector
			for(int i = 0; i < nLocal; i++) {
				// current column matches given column
				if(i == rowIndex) {
					tmp = 0;
				}
				// current column does not match given column
				else {
					tmp = -1;
				}

				// insert value into ia vector
				ia.push_back(tmp);
			}

			// insert pointer to nirvana
			ia.push_back(1);
		}

		/* Default "constructor". Copies a part of the given matrix in order to initialize
		   its own values. The part to copy is specified by the starting row and column
		   indices. The size of the local submatrix is determined by the parameters nLocal
		   and mLocal respectively.
		*/
		void initialize(int id, int nLocal, int mLocal, int rowIndexStart, int columnIndexStart,
		const T* const * const matrix, bool copyGlobal) {
			// true if the first element of a row is to be copied
			bool first = true;
			// set starting row index to 0
			int i0 = 0;
			// set starting column index to 0
			int j0 = 0;
			// the current element
			T element;
			// pointer for the arrays position in ia
			int k = 0;

			// init submatrix
			this->init(id, nLocal, mLocal, rowIndexStart, columnIndexStart);

			// given matrix is to be interpreted as a bigger matrix
			if(copyGlobal) {
				// set starting row index to given rowIndexStart
				i0 = rowIndexStart;
				// set starting column index to given columnIndexStart
				j0 = columnIndexStart;
			}

			// iterate over all rows
			for(int i = i0; i < i0 + nLocal; i++) {
				// iterate over all columns
				for(int j = j0; j < j0 + mLocal; j++) {
					// copy current element
					element = matrix[i][j];

					// current element is non-zero
					if(element != this->zero) {
						// store current element
						this->values.push_back(element);
						// store column index
						ja.push_back(j - this->j0);

						if(first) {
							// store column of the first element
							ia.push_back(k);
							// reset flag
							first = false;
						}

						// increase pointer to array position
						k++;
					}
				}

				// a row is completely empty
				if(first) {
					ia.push_back(-1);
				}

				// a new row is entered
				first = true;
			}

			// last element points to nirvana
			ia.push_back(k);
		}

	};

}

#endif
