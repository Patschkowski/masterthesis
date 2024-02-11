// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_FARM_H
#define TEST_FARM_H

namespace msl {

	namespace test {

		namespace farm {

			// constants

			const int PROCESSORS = 3;

			// variables

			static int counter = 0;

			// test functions

			void testConstructors();
			void testDestructor();
			void testFarm();
			void testFunctions();

			// argument functions

			int* atomic(int*);
			void final(int*);
			int* initial(Empty e);

		}

	}

}

#endif
