// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef MATRIX_INDEX_H
#define MATRIX_INDEX_H

namespace msl {

	class MatrixIndex {

	private:

		int rowIndex;
		int colIndex;

	protected:

	public:

		MatrixIndex(int rowIndex, int columnIndex);

		int getRowIndex() const;

		int getColumnIndex() const;

	};
	
	bool operator<(const MatrixIndex& e, const MatrixIndex& f);

}

#endif
