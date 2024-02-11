// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#include "DistributedDataStructure.h"

int msl::DistributedDataStructure::getId() const {
	return id;
}

int msl::DistributedDataStructure::getSize() const {
	return n;
}

int msl::DistributedDataStructure::getLocalSize() const {
	return nLocal;
}
