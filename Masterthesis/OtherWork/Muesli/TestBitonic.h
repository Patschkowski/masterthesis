// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_BITONIC_H
#define TEST_BITONIC_H

#include "DistributedArray.h"

namespace msl {

	namespace test {

		namespace bitonic {

			// general assumption: higher bits to the left of lower bits
			static int log2size;
			static int n;
			static int np;
			static int nt;

			// auxiliary functions

			bool adjustDirection(const DistributedArray<bool>& Dir, int i, bool d);
			bool initDirection(int i);
			bool odd(int i);
			int bitsum(int i);
			int clsh(int k, int i);
			int random(int i);

			template<class C> C min(C x, C y);
			template<class C> C max(C x, C y);
			template<class C> C sorter(const DistributedArray<C>& A,
				const DistributedArray<bool>& Dir, int i, C x);
			template<class C> DistributedArray<C> bitonic( DistributedArray<C> A);

			// main test function

			void testBitonic(int n, int nt, bool showArrays);

		}

	}

}

#endif
