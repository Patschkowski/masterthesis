// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef MATRIX_READER_H
#define MATRIX_READER_H

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>

#include "MatrixElement.h"

namespace msl {

	class MatrixReader {

	private:

	protected:

		// number of rows
		int n;
		// number of columns
		int m;
		// number of non-zero elements
		int nnz;
		// file to be read
		FILE* f;

	public:

		MatrixReader(std::string filename) {
			// open file
			f = fopen(filename.c_str(), "r");
		}

		virtual ~MatrixReader() {
			// close file
			fclose(f);
		}

		virtual void getNextValue(MatrixElement<double>* const value) = 0;

		int getColumnCount() const {
			return m;
		}

		int getElementCount() const {
			return nnz;
		}

		int getRowCount() const {
			return n;
		}

	};

}

#endif
