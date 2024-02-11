// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_INITIAL_H
#define TEST_INITIAL_H

#include "Muesli.h"

namespace msl {

	namespace test {

		namespace initial {

			// constants

			static const int PROCESSORS = 1;
			static int I = 0;

			// test functions

			void testConstructors();
			void testDestructor();
			void testFunctions();
			void testInitial();

			// argument functions

			int* f(msl::Empty);

		}

	}

}

#endif
