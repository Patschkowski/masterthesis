// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef ROW_DISTRIBUTION_H
#define ROW_DISTRIBUTION_H

#include "Distribution.h"

namespace msl {

	class RowDistribution: public Distribution {

	private:

		// determines the number of submatrices per row
		int getSubmatrixCountPerRow() const;

	protected:

	public:

		// returns the id of the process which is responsible
		// for storing the submatrix with the given id.
		int getIdProcess(int idSubmatrix);

		// clone function for row distributions
		RowDistribution* clone() const;

	};

}

#endif
