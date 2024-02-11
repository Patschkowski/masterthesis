// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_PIPE_H
#define TEST_PIPE_H

#include "Muesli.h"

namespace msl {

	namespace test {

		namespace pipe {

			// constants

			static const int PROCESSORS = 3;
			static int I = 0;

			// test functions

			void testConstructors();
			void testCopy();
			void testDestructor();
			void testFunctions(bool flag);
			void testPipe(bool flag = true);
			void testStart(bool flag);

			// argument functions

			int* atomic(int*);
			void final(int*);
			int* initial(msl::Empty e);


		}

	}

}

#endif
