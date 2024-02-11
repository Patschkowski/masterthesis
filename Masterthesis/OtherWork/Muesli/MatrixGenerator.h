// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef MATRIX_GENERATOR_H
#define MATRIX_GENERATOR_H

#include <fstream>
#include <set>
#include <string>

#include "MatrixElement.h"
#include "Utility.h"

namespace msl {

	/* This class can be used to generate random matrices. The user
	   only has to specify the dimensions of the matrix (n and m),
	   the number of non-zero elements (nnz), the minimum and maximum
	   values of the matrix to create (min and max), and the filename
	   where the created matrix is saved. Note that this generator
	   generates matrices according to the MatrixMarket file format.
	*/
	template<typename T> class MatrixGenerator {

	private:

		// number of rows
		int n;
		// number of columns
		int m;
		// number of non-zero elements
		int nnz;
		// minimum value to create
		int min;
		// maximum value to create
		int max;
		// filename where to store the generated matrix
		std::string filename;

		/* Writes the given set of matrix elements to a file specified
		   by the filename.
		*/
		void write(std::set<MatrixElement<T> >& elements) {
			// iterator used to iterate over all elements of the given set
			typename std::set<MatrixElement<T> >::iterator iter = elements.begin();
			// output file stream to write the element into
			std::ofstream s;

			// open file stream
			s.open(filename.c_str());
			// write parameters n, m, and nnz to stream
			s << n << " " << m << " " << nnz << std::endl;

			// iterate over all elements of the given set
			for(iter; iter != elements.end(); iter++) {
				// write current element to output stream
				// using the overloaded << operator
				s << *iter << std::endl;
			}

			// close file stream
			s.close();
		}

	protected:

	public:

		/* Default constructor. Expects the size of the matrix (n and m), the number
		   of non-zero elements to create (nnz), the minimum and the maximum value of
		   each element (min and max), and the filename where to store the elements.
		*/
		MatrixGenerator(int n, int m, int nnz, int min, int max, std::string filename):
			n(n), m(m), nnz(nnz), min(min), max(max), filename(filename) {
		}

		/* Generates a pseudo random sparse matrix with the given
		   parameters.
		*/
		void generate() {
			// column index of the current element
			int colIndex;
			// number of elements still to create
			int counter = nnz;
			// row index of the current element
			int rowIndex;
			// set containing the created elements
			std::set<MatrixElement<T> > elements;
			// value of the current element
			T value;

			// initialize the random generator with the current time
			utl::initSeed();

			// there are still some more elements to create
			while(counter > 0) {
				// create a random column index
				colIndex = (int)(utl::random() * m);
				// create a random row index
				rowIndex = (int)(utl::random() * n);

				// created column index is equal to m
				if(colIndex == m) {
					// reduce column index by one
					colIndex--;
				}

				// created row index is equal to n
				if(rowIndex == n) {
					// reduce row index by one
					rowIndex--;
				}

				// create a random value
				value = (T)(utl::random() * (max - min) + min);
				// create a new matrix element
				MatrixElement<T> e(rowIndex, colIndex, value);

				// current matrix element is not already contained in the set
				if(elements.count(e) == 0) {
					// add current matrix element to set
					elements.insert(e);
					// decrease number of elements still to create
					counter--;
				}
			}

			// write elements to file
			write(elements);
		}

	};

}

#endif
