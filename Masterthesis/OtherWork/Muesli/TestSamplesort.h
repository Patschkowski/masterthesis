// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_SAMPLESORT_H
#define TEST_SAMPLESORT_H

#include "DistributedArray.h"

namespace msl {

	namespace test {

		namespace samplesort {

			// variables

			static int n;
			static int np;
			static int nt;

			// auxiliary functions

			int random(int i);

			template<class C> int* initIndex(const msl::DistributedArray<C>& A, int selection[], int i);

			template<class C> C takeSample(const msl::DistributedArray<C>& A, int i);

			template<class C> C moveResult(C *sorted, int i, C x);

			template<class C> void quicksort(int low, int high, C* _a);

			template<class C> void samplesort(msl::DistributedArray<C>& A, C maxVal);

			// test functions
			
			void testSampleSort(int n, int nt, bool flag);

		}

	}

}

#endif
