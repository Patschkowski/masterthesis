// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef MATRIX_MARKET_MATRIX_READER_H
#define MATRIX_MARKET_MATRIX_READER_H

#include "MatrixElement.h"
#include "MatrixReader.h"

namespace msl {

	class MatrixMarketMatrixReader : public MatrixReader {

	private:

		void readParameters() {
			fscanf(f, "%d %d %d\n", &n, &m, &nnz);
		}

	protected:

	public:

		MatrixMarketMatrixReader(std::string filename): MatrixReader(filename) {
			// read parameters
			readParameters();
		}

		~MatrixMarketMatrixReader() {
		}

		void getNextValue(MatrixElement<double>* const value) {
			int rowIndex = 0;
			int colIndex = 0;
			double val = 0;
			
			fscanf(f, "%d %d %lf\n", &rowIndex, &colIndex, &val);

			value->setColumnIndex(colIndex);
			value->setRowIndex(rowIndex);
			value->setValue(val);
		}

	};

}

#endif
