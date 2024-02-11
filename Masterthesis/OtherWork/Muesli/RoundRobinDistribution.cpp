// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#include "RoundRobinDistribution.h"

// PRIVATE

// PROTECTED

// PUBLIC

/* returns the id of the process which is responsible
   for storing the submatrix with the given id.
*/
int msl::RoundRobinDistribution::getIdProcess(int idSubmatrix) {
	return idSubmatrix % np;
}

msl::RoundRobinDistribution* msl::RoundRobinDistribution::clone() const {
	return new msl::RoundRobinDistribution(*this);
}
