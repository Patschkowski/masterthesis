// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_DIVIDE_AND_CONQUER_H
#define TEST_DIVIDE_AND_CONQUER_H

#include "Array.h"

namespace msl {

	namespace test {

		namespace dac {

			// constants

			// number of subproblems to generate if problem is not simple
			static const int D = 2;

			// variables

			static bool flag = true;

			// test functions

			void testDivideAndConquer();

			// argument functions
			
			msl::test::Array<int>* initial(msl::Empty);

			bool isSimple(msl::test::Array<int>*);
			msl::test::Array<int>** divide(msl::test::Array<int>*);
			msl::test::Array<int>* solve(msl::test::Array<int>*);
			msl::test::Array<int>* combine(msl::test::Array<int>**);

			void final(msl::test::Array<int>*);

		}

	}

}

#endif
