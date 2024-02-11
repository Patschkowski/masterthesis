// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_MISC_H
#define TEST_MISC_H

namespace msl {

	namespace test {

		namespace misc {

			static int n;
			static int m;
			static int r;
			static int c;
			static int id;
			static int np;
			static int nt;
			static int runs;
			static double t0, t1, t2, t3, t4, t5, t6;

			// AUXILIARY

			// AUXILIARY / ARGUMENT FUNCTIONS

			double add(double a, double b);

			double addIndex(double a, double b, int i, int j);

			// TEST FUNCTIONS

			void testConstructors();

			void testMultiply();

			void testZipIndexInPlace();

			// TEST FUNCTIONS / MPI

			void testGroups();

			void testMpiAllgather();

			void testMpiAllreduce();

			void testMpiBroadcast();

			void testMpiGather();

			void testMpiReduce();

			// TEST FUNCTIONS / VARIOUS

			void testWriteRead();

		}

	}

}

#endif
