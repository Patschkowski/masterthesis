// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef MATRIX_MARKET_DSM_READER_H
#define MATRIX_MARKET_DSM_READER_H

#include "DistributedSparseMatrix.h"
#include "MatrixMarketMatrixReader.h"
#include "MatrixElement.h"
#include "Utility.h"


namespace msl {

	/* Author: Holger Sunke
	*/
	class MatrixMarketDistributedSparseMatrixReader: public MatrixMarketMatrixReader {

	private:

	protected:

		std::string fName;

	public:

		MatrixMarketDistributedSparseMatrixReader(std::string filename):
		MatrixMarketMatrixReader(filename) {
			fName = filename;
		}

		/* Uses the MatrixMarketMatrixReader to create a DistributedSparseMatrix
		*/
		template<typename T>
		DistributedSparseMatrix<T>* readMatrixMarketDistributedSparseMatrix(int rowsPerSubmatrix,
			int columnsPerSubmatrix, bool roundValues = false, bool positiveOnly = false) {
			double start = MPI_Wtime();
			int i = 0;
			int ci;
			int ri;
			int rows = getRowCount();
			int columns = getColumnCount();
			int nnz = getElementCount();
			MatrixElement<double> value;
			DistributedSparseMatrix<T>* matrix = new DistributedSparseMatrix<T>(rows, columns,
				rowsPerSubmatrix, columnsPerSubmatrix, 0);
			T element;

			for(int i = 0; i < nnz; i++) {
				getNextValue(&value);
				
				element = roundValues ? (T)((int)value.getValue()) : value.getValue();
				
				if(positiveOnly && element < 0) {
					element = -element;
				}

				ci = value.getColumnIndex();
				ri = value.getRowIndex();

				if(ci == m) {
					ci--;
				}

				if(ri == n) {
					ri--;
				}
				
				matrix->setElement(element, ri, ci);
			}
			
			return matrix;
		}

	};

}

#endif
