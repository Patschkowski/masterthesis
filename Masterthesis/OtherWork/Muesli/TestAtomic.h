// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_ATOMIC_H
#define TEST_ATOMIC_H

namespace msl {

	namespace test {

		namespace atomic {

			// constants

			static int PROCESSORS = 1;

			// test functions

			void testAtomic();
			void testConstructors();
			void testDestructor();
			void testFunctions();

			// argument function

			int* f(int*);

		}

	}

}

#endif
