// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_DISTRIBUTED_ARRAY_H
#define TEST_DISTRIBUTED_ARRAY_H

namespace msl {

	namespace test {

		namespace da {

			// constants

			const int PROCESSORS = 2;
			const int SIZE = 2 * PROCESSORS;
			const int INITIAL = -1;

			// test functions

			void testDistributedArray();
			void testAuxiliaryFunctions();
			void testConstructors();
			void testDestructor();
			void testSkeletons();
			void testSkeletonsCommunication();
			void testSkeletonsComputation();
			void testSkeletonsMap();
			void testSkeletonsRemaining();
			void testSkeletonsZip();

			// argument functions

			int add(int, int);
			int addIndex(int, int, int);
			int decrease(int);
			int identity(int);
			int increase(int);
			int increaseIndex(int, int);
			int square(int);

			int leftShiftPartition(int);
			int noShift(int);
			int rightShiftElement(int);

		}

	}

}

#endif
