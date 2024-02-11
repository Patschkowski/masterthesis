// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef ROW_PROXY_H
#define ROW_PROXY_H


namespace msl {

	// forward declaration of class DistributedSparseMatrix is needed here,
	// since the class uses the class RowProxy and vice versa. But since
	// RowProxy only needs to store a pointer to the distributed sparse
	// matrix, the forward declaration is possible in this case.
	template<typename T> class DistributedSparseMatrix;

	/* The class is used to access elements of the distributed sparse matrix
	   by using the index operator []. Thus, the user can access elements by
	   simply writing dsm[i][j], for example. The class only stores a pointer
	   to the distributed sparse matrix object and the element's row index.
	   Additionally, it of course overwrites the index operator [].
	*/
	template<typename T> class RowProxy {

	private:

		// pointer to the distributed sparse matrix
		const DistributedSparseMatrix<T>& m;
		// row index of the element to retrieve
		int rowIndex;

	protected:

	public:

		/* Default constructor. Constructs a RowProxy object by means
		   of the given reference to the distributed sparse matrix
		   object and the row index of the element to access.

		   - DistributedSparseMatrix<T, S, D>& dsm: A reference to the
			 distributed sparse matrix object which stores the element
			 to access.
		   - int rowIndex: The row index of the element to access.
		*/
		RowProxy(const DistributedSparseMatrix<T>& m): m(m) {
		}

		/* Sets the stored row index to the given row index.
		*/
		void setRowIndex(int index) {
			rowIndex = index;
		}

		/* Overwrites the index operator []. Accesses the element of
		   the distributed sparse matrix by calling the getElement
		   method.
		*/
		T operator[](int columnIndex) const {
			return m.getElement(rowIndex, columnIndex);
		}

	};

}

#endif
