// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef SUBMATRIX_H
#define SUBMATRIX_H

#include <string>
#include <vector>

namespace msl {

	template<typename T = double> class Submatrix {

	private:

	protected:

		// ATTRIBUTES

		// id of the submatrix
		int id;
		// local number of rows
		int nLocal;
		// local number of columns
		int mLocal;
		// global row index of the first element
		int i0;
		// global column index of the first element
		int j0;
		// stores the elements of the submatrix
		std::vector<T> values;
		// zero element
		T zero;

		// FUNCTIONS

		// deletes the element at the given index from the given vector
		template <typename T1> void remove(std::vector<T1>& v, int index) {
			typename std::vector<T1>::iterator iter = v.begin();
			// increase pointer
			iter += index;
			// delete element
			v.erase(iter);
		}
		
		/* Translates the given local array index into the global
		   column index.
		*/
		int getGlobalColIndex(int localColumnIndex) const {
			return localColumnIndex + j0;
		}

		/* Translates the given local array index into the global
		   row index.
		*/
		int getGlobalRowIndex(int localRowIndex) const {
			return localRowIndex + i0;
		}

		/* If the given argument is set to false, the function returns the number
		   of non-zero elements. If set to true, the function returns the total
		   number of locally stored elements.
		*/
		int getElementCount(bool includeZeros) const {
			// number of non-zero elements
			int nnz = 0;
			// number of locally stored elements
			int size = (int)values.size();

			// count all elements
			if(includeZeros) {
				// return size of vector
				return size;
			}
			// count only non-zero elements
			else {
				// iterate over all locally available elements
				for(int i = 0; i < size; i++) {
					// current element is non-zero
					if(values[i] != 0) {
						// increase number of non-zero elements
						nnz++;
					}
				}

				// return number of non-zero elements
				return nnz;
			}
		}

		/* Initializes some basic variables.
		*/
		void init(int id, int nLocal, int mLocal, int i0, int j0) {
			this->id = id;
			this->nLocal = nLocal;
			this->mLocal = mLocal;
			this->i0 = i0;
			this->j0 = j0;
		}

		// inserts the given value at the given index into the given vector
		template <typename T1> void insert(std::vector<T1>& v, int index, T1 value) {
			// iterator for the elements of the vector
			typename std::vector<T1>::iterator iter;
			
			// index is in bounds
			if(index < (int)v.size()) {
				// get iterator to the elements of the vector
				iter = v.begin();
				// increase iterator to given element
				iter += index;
				// insert given element at the given position
				v.insert(iter, value);
			}
			// index is out of bounds
			else {
				// insert given value to the end of the vector
				v.push_back(value);
			}
		}

	public:

		// ATTRIBUTES

		// FUNCTIONS

		virtual void debug() const = 0;

		/* Used for the prototype design pattern. Creates a deep copy of
		   the submatrix object on which this function is invoked and
		   returns a pointer to the newly created object.
		*/
		virtual Submatrix<T>* clone() const = 0;

		/* Returns the global column index of the element with the given
		   index. The index is supposed to be the index of an element
		   stored by the vector values. This function is used when batch
		   processing the elements of a submatrix.
		*/
		virtual int getColumnIndexGlobal(int k) const {
			return j0 + getColumnIndexLocal(k);
		}

		/* Returns the column index of the element with the given index.
		   The index is supposed to be the index of an element stored by
		   the vector values. This function is used when batch accessing
		   the elements of a submatrix.
		*/
		virtual int getColumnIndexLocal(int k) const = 0;

		/* Returns the starting column index.
		*/
		int getColumnIndexStart() const {
			return j0;
		}

		/* True, if the given global column index is stored locally.
		*/
		bool columnIsLocal(int globalColumnIndex) const {
			return (globalColumnIndex >= j0) && (globalColumnIndex < j0 + mLocal);
		}

		/* returns the element with the given local row and local column index
		*/
		virtual T getElement(int localRowIndex, int localColumnIndex) const = 0;

		/* Returns the element with the given local index
		*/
		T getElementLocal(int k) const {
			return values[k];
		}

		/* Returns the number of elements without zeros.
		*/
		int getElementCount() const {
			return getElementCount(false);
		}

		/* Returns the local number of elements including zeros.
		*/
		int getElementCountLocal() const {
			return getElementCount(true);
		}

		/* Returns the id of the submatrix.
		*/
		int getId() const {
			return id;
		}

		/* Returns the number of local columns, i.e. mLocal.
		*/
		int getLocalM() const {
			return mLocal;
		}

		/* Returns the number of local rows, i.e. nLocal.
		*/
		int getLocalN() const {
			return nLocal;
		}

		/* Cf. getColumnIndexStart(int index).
		*/
		virtual int getRowIndexGlobal(int k) const {
			return i0 + getRowIndexLocal(k);
		}

		/* Cf. getColumnIndexLocal(int index).
		*/
		virtual int getRowIndexLocal(int k) const = 0;

		/* Returns the starting row index.
		*/
		int getRowIndexStart() const {
			return i0;
		}

		/* Returns an uncompressed copy of the submatrix.
		*/
		T** getUncompressed() const {
			// column index of the current element
			int ci;
			// row index of the current element
			int ri;
			// number of locally stored elements
			int ne = getElementCountLocal();
			// initialize result array
			T** result = new T*[nLocal];

			// iterate over all rows
			for(int i = 0; i < nLocal; i++) {
				// allocate space for the result array
				result[i] = new T[mLocal];

				// iterate over all columns
				for(int j = 0; j < mLocal; j++) {
					// store zero element
					result[i][j] = zero;
				}
			}

			// iterate over all locally available elements
			for(int k = 0; k < ne; k++) {
				// determine row index of the current element
				ri = getRowIndexLocal(k);
				// determine column index of the current element
				ci = getColumnIndexLocal(k);
				// determine current element and set it at the right position
				result[ri][ci] = getElementLocal(k);
			}

			// return result array
			return result;
		}

		/* True, if the block only contains elements equal to 0. False otherwise.
		*/
		bool isEmpty() const {
			return getElementCount() == 0;
		}

		/* Re-compresses the submatrix, since by using the function getElements(),
		   some values equal to zero may have been stored. This function is not
		   pure virtual, since some storage schemes, such as BSR, do not need to
		   be recompressed.
		*/
		virtual void pack() {
		}

		/* Prints the submatrix to standard output using the toString function.
		*/
		void print() const {
			printf("%s\n", this->toString().c_str());
		}

		/* True, if the given global row index is stored locally.

		   tested: true
		*/
		bool rowIsLocal(int globalRowIndex) const {
			return (globalRowIndex >= i0) && (globalRowIndex < i0 + nLocal);
		}

		/* Sets the element with the given local row and local column index.
		   
		   ATTENTION: As indicated by the missing const modifier, the function
		   alters the state of the submatrix. As a consequence, this function
		   must be executed atomically inside any OpenMP parallel directive.
		   Otherwise, the program is very likely to crash.
		*/
		virtual void setElement(T value, int localRowIndex, int localColumnIndex) = 0;
		
		/* Sets the element with the given local index k. k is expected to
		   index an element in the values vector, i.e. it is only safe to
		   access element within the range from 0 to getElementCount(false).
		*/
		virtual void setElementLocal(T value, int k) {
			values[k] = value;
		}

		/* Sets the zero element.
		*/
		void setZero(T zero) {
			this->zero = zero;
		}
		
		/* Returns a string representation of the submatrix.
		   Just for debugging.
		*/
		virtual std::string toString() const = 0;

		// FUNCTIONS / CONSTRUCTOR

		// default constructor
		Submatrix() {
		}

		// virtual destructor
		virtual ~Submatrix() {
		}

		// FUNCTIONS / INITIALIZERS

		/* Initializes an empty submatrix
		*/
		virtual void initialize(int id, int nLocal, int mLocal, int i0, int j0) = 0;

		/* initializes the submatrix by means of the given matrix.
		   the constructor copies the relevant part of the given
		   matrix, which is indicated by i0, nLocal,
		   columnIndexStart and mLocal, to the local submatrix.
		*/
		virtual void initialize(int id, int nLocal, int mLocal, int i0, int j0,
			const T* const * const matrix, bool copyGlobal) = 0;

		/* Initializes the submatrix by means of the given value.
		   Could be removed, since I added an initializier for
		   empty submatrices. Consequently, I had to replace all
		   occurences of this initializer by the empty initializer
		   followed by a call to the setElement function.
		*/
		virtual void initialize(int id, int nLocal, int mLocal, int i0, int j0,
			T value, int rowIndex, int colIndex) = 0;

	};

}

template<typename T>
std::ostream& operator<<(std::ostream& os, const msl::Submatrix<T>& s) {
	os << "id = " << s.getId();

	return os;
}

/* Overloaded operator < in order to use submatrices in a
   std::set. The operator simply compares the ids of the
   given submatrices.
*/
template<typename T>
bool operator<(const msl::Submatrix<T>& s, const msl::Submatrix<T>& t) {
	// id of s is smaller than id of t
	if(s.getId() < t.getId()) {
		// s < t, return true
		return true;
	}
	// id of s is not smaller than in of t
	else {
		// s !< t, return false
		return false;
	}

	// return (s.getId() < t.getId()) ? true : false;
}

#endif
