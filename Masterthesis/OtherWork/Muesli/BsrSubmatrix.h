// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef BSR_SUBMATRIX_H
#define BSR_SUBMATRIX_H

#include <sstream>

#include "Submatrix.h"

namespace msl {

	template<typename T = double> class BsrSubmatrix: public Submatrix<T> {

	private:

		// ATTRIBUTES

		// FUNCTIONS
		
		/* Translates the given row and column index into a 1D array position.
		*/
		int getLocalIndex(int localRowIndex, int localColumnIndex) const {
			return localRowIndex * this->mLocal + localColumnIndex;
		}

		/* Returns the lenght of the array which stores the values.
		*/
		int getLocalLength() const {
			return this->nLocal * this->mLocal;
		}

	protected:

		// ATTRIBUTES

		// FUNCTIONS

	public:

		// ATTRIBUTES

		// FUNCTIONS
		
		void debug() const {
			std::ostringstream stream;

			stream << "id = "       << this->id;
			stream << ", nLocal = " << this->nLocal;
			stream << ", mLocal = " << this->mLocal;
			stream << ", ris = "    << this->i0;
			stream << ", cis = "    << this->j0<< std::endl;

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

			printf("%s", stream.str().c_str());
		}

		void test() const {
		}

		/*
		*/
		BsrSubmatrix* clone() const {
			return new BsrSubmatrix(*this);
		}

		/*
		*/
		int getColumnIndexLocal(int index) const {
			return index % this->mLocal;
		}

		/* Retrieves the element which is stored at the given local row and local
		   column index, respectively. The translation between global and local
		   indices must be performed by the DistributedSparseMatrix.
		 */
		T getElement(int localRowIndex, int localColumnIndex) const {
			return this->values[getLocalIndex(localRowIndex, localColumnIndex)];
		}

		/*
		*/
		int getRowIndexLocal(int index) const {
			return index / this->mLocal;
		}

		/* Sets the element with the given local row and local column index,
		   respectively. Again, the translation between global and local indices
		   must be performed by the DistributedSparseMatrix.
		*/
		void setElement(T value, int localRowIndex, int localColumnIndex) {
			// local index of the searched element
			int index = getLocalIndex(localRowIndex, localColumnIndex);
			// set value to given one
			this->values[index] = value;
		}

		/* Returns a string representation of the BsrBlock.
		*/
		std::string toString() const {
			std::ostringstream stream;

			// iterator over all rows
			for(int rowIndex = 0; rowIndex < this->nLocal; rowIndex++) {
				stream << "[";

				// iterator over all columns
				for(int columnIndex = 0; columnIndex < this->mLocal; columnIndex++) {
					stream << this->values[getLocalIndex(rowIndex, columnIndex)];

					// insert separator character
					if(columnIndex < this->mLocal - 1) {
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
		BsrSubmatrix(): Submatrix<T>() {
		}

		// FUNCTIONS / DESTRUCTOR

		/* Destructor.
		*/
		~BsrSubmatrix() {
		}

		// FUNCTIONS / INITIALIZERS

		/* Hiding problem (FAQ 23.9)! Alternative: using Submatrix<T>::initialize;
		*/
		void initialize(int id, int nLocal, int mLocal, int rowIndexStart, int columnIndexStart) {
			// length of the local array
			int length;
			
			// init submatrix
			this->init(id, nLocal, mLocal, rowIndexStart, columnIndexStart);
			// get local length
			length = getLocalLength();

			// iterate over all elements
			for(int i = 0; i < length; i++) {
				// store 0 in row-major order
				this->values.push_back(this->zero);
			}
		}
		
		/* Enhanced "constructor". Initializes a new submatrix with zeros except for the given
		   element at the given row and column index.
		*/
		void initialize(int id, int nLocal, int mLocal, int rowIndexStart, int columnIndexStart,
			T value, int rowIndex, int columnIndex) {
			// initialize all elements with 0
			this->initialize(id, nLocal, mLocal, rowIndexStart, columnIndexStart);
			// set the only one element
			setElement(value, rowIndex, columnIndex);
		}

		/* Constructs a BsrBlock out of the given parameters. The constructor expects a reference to
		   the global matrix and its total number of rows and columns, respectively. The constructor
		   then copies a submatrix from the global matrix and stores it locally. The number of rows
		   and columns to copy is specified by the parameters nLocal and mLocal, respectively. The
		   parameters rowIndexStart and columnIndexStart point to a position in the global matris
		   from where to start to copy the elements. The parameters in detail:
		   -- id				The global id of the BsrBlock.
		   -- nLocal			Total number of rows to store locally.
		   -- mLocal			Total number of columns to store locally.
		   -- matrix			The global matrix.
		   -- rowIndexStart		The global row index from which to start to copy the global matrix.
		   -- columnIndexStart	The global column index from which to start to copy the global matrix.
		*/
		void initialize(int id, int nLocal, int mLocal, int rowIndexStart, int columnIndexStart,
		const T* const * const matrix, bool copyGlobal) {
			// set starting row to 0
			int i0 = 0;
			// set starting column to 0
			int j0 = 0;

			// init submatrix
			this->init(id, nLocal, mLocal, rowIndexStart, columnIndexStart);

			// given matrix shall be interpreted as a bigger matrix
			if(copyGlobal) {
				// set starting row index to given rowIndexStart
				i0 = rowIndexStart;
				// set starting column index to given columnIndexStart
				j0 = columnIndexStart;
			}

			// iterate over all rows
			for(int i = i0; i < i0 + this->nLocal; i++) {
				// iterate over all columns
				for(int j = j0; j < j0 + this->mLocal; j++) {
					// store current element in row-major order
					this->values.push_back(matrix[i][j]);
				}
			}
		}

	};

}

#endif
