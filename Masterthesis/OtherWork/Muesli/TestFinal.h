// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_FINAL_H
#define TEST_FINAL_H

namespace msl {

	namespace test {

		namespace final {

			// constants

			static const int PROCESSORS = 1;

			// test functions

			void testConstructors();
			void testDestructor();
			void testFinal();
			void testFunctions();

			// argument functions

			void f(int*);

		}

	}

}

#endif
