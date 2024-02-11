// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef OAL_H
#define OAL_H

namespace msl {

	namespace oal {

		/* Determines the maximum number of threads to be used. If
		   OpenMP is available and enabled, the function returns
		   omp_get_max_threads. Otherwise, it returns 1.
		*/
		int getMaxThreads();

		/* Determines the id of the thread. If OpenMP is
		   available and enabled, the function returns
		   omp_get_thread_num. Otherwise, it returns 0.
		*/
		int getThreadNum();

		/* Sets the maximum number of threads to be used in a
		   parallel region. If OpenMP is available and enabled,
		   the function calls omp_set_num_threads. Otherwise,
		   it does nothing.
		*/
		void setNumThreads(int);

	}

}

#endif
