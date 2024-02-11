// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#include "MatrixIndex.h"

msl::MatrixIndex::MatrixIndex(int rowIndex, int columnIndex):
rowIndex(rowIndex), colIndex(columnIndex) {
}

int msl::MatrixIndex::getRowIndex() const {
	return rowIndex;
}

int msl::MatrixIndex::getColumnIndex() const {
	return colIndex;
}

bool msl::operator<(const msl::MatrixIndex& e, const msl::MatrixIndex& f) {
	// row index of index e is smaller than row index of index f
	if(e.getRowIndex() < f.getRowIndex()) {
		return true;
	}
	else {
		// both indexes have the same row index and the column index
		// of index e is smaller than the column index of index f
		if(e.getRowIndex() == f.getRowIndex() && e.getColumnIndex() < f.getColumnIndex()) {
			return true;
		}
		else {
			return false;
		}
	}
}
