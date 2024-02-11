// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef MATRIX_VALUE_H
#define MATRIX_VALUE_H

#include <sstream>
#include "Element.h"

namespace msl {

	/* This class is used as a container for matrix elements, since
	   it stores the value, the row, and the column index of the
	   element.

	   Note that the class overloads the operators << and <. The
	   output operator << is used to easily write matrix elements to
	   a file, the relational operator < is used in order to be able
	   to insert matrix elements in a std::set<MatrixElement>.
	*/
	template<typename T> class MatrixElement: public Element<T> {

	private:

		int rowIndex;
		int colIndex;

	protected:

	public:

		MatrixElement(): Element<T>(), rowIndex(-1), colIndex(-1) {
		}

		MatrixElement(int rowIndex, int columnIndex, T value):
			Element<T>(value), rowIndex(rowIndex), colIndex(columnIndex) {
		}

		int getColumnIndex() const {
			return colIndex;
		}

		int getRowIndex() const {
			return rowIndex;
		}

		void setColumnIndex(int columnIndex) {
			colIndex = columnIndex;
		}

		void setRowIndex(int rowIndex) {
			this->rowIndex = rowIndex;
		}

		void print() const {
			std::ostringstream s;

			s << this << std::endl;

			printf("%s", s.str().c_str());
		}

	};

}

/* Outputs the elements in a MatrixMarket-format to the given output stream.
*/
template<typename T>
std::ostream& operator<<(std::ostream& os, const msl::MatrixElement<T>& e) {
	os << e.getRowIndex() << " " << e.getColumnIndex() << " " << e.getValue();

	return os;
}

/* Compares the matrix elements in a "natural" way by considering their indexes
   and returns true, if e < f. Otherwise, it returns false. An element e is
   smaller than an element f, if e.rowIndex < f.rowIndex or if e.rowIndex ==
   f.rowIndex and e.columnIndex < f.columnIndex. Thus, the elements are sorted
   in row-major order.

   Note that this function must have the following properties in order to be
   used with std::set:

   1. e < e must always be false.
   2. if e < f, f < e must be false.
   3. if e < f and f < g, e < g must also be true (transitivity).
*/
template<typename T>
bool operator<(const msl::MatrixElement<T>& e, const msl::MatrixElement<T>& f) {
	// row index of element e is smaller than row index of element f
	if(e.getRowIndex() < f.getRowIndex()) {
		return true;
	}
	else {
		// both element have the same row index and the column index
		// of element e is smaller than the column index of element f
		if(e.getRowIndex() == f.getRowIndex() && e.getColumnIndex() < f.getColumnIndex()) {
			return true;
		}
		else {
			return false;
		}
	}
}

#endif
