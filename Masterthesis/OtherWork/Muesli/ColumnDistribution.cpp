// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#include <math.h>

#include "ColumnDistribution.h"

// PRIVATE

int msl::ColumnDistribution::getSubmatrixCountPerRow() const {
	int result = m / c;

	if(m % c == 0) {
		return result;
	}
	else {
		return result + 1;
	}
}

// PROTECTED

// PUBLIC

/* returns the id of the process which is responsible for
   storing the submatrix with the given id.
*/
int msl::ColumnDistribution::getIdProcess(int idSubmatrix) {
	// return result
	return (idSubmatrix % getSubmatrixCountPerRow()) % np;
}

msl::ColumnDistribution* msl::ColumnDistribution::clone() const {
	return new msl::ColumnDistribution(*this);
}
