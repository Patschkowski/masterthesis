// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifdef _OPENMP
#include <omp.h>
#endif

#include "OAL.h"

int msl::oal::getMaxThreads() {
	#ifdef _OPENMP
	return omp_get_max_threads();
	#else
	return 1;
	#endif
}

int msl::oal::getThreadNum() {
	#ifdef _OPENMP
	return omp_get_thread_num();
	#else
	return 0;
	#endif
}

void msl::oal::setNumThreads(int nt) {
	#ifdef _OPENMP
	omp_set_num_threads(nt);
	#endif
}
