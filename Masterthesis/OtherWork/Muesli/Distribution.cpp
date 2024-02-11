// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#include "Distribution.h"

// PRIVATE

// PROTECTED

// PUBLIC

bool msl::Distribution::equals(Distribution& d) {
	// at least one attribute is not equal
	if(n != d.n || m != d.m || r != d.r || c != d.c || max != d.max || np != d.np) {
		// return false
		return false;
	}

	// iterate over all submatrix ids
	for(int id = 0; id < max; id++) {
		// process ids of current submatrix do not match
		if(this->getIdProcess(id) != d.getIdProcess(id)) {
			// distribution objects are not equal
			return false;
		}
	}

	// distribution objects are equal
	return true;
}

void msl::Distribution::initialize(int np, int n, int m, int r, int c, int maxSubmatrixCount) {
	this->n = n;
	this->m = m;
	this->r = r;
	this->c = c;
	this->np = np;
	max = maxSubmatrixCount;
}

// PUBLIC / DESTRUKTOR

// PUBLIC / KONSTRUKTOREN

bool msl::Distribution::isStoredLocally(int idProcess, int idSubmatrix) {
	return idProcess == getIdProcess(idSubmatrix);
}
