// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_DISTRIBUTED_MATRIX_H
#define TEST_DISTRIBUTED_MATRIX_H

namespace msl {

	namespace test {

		namespace dm {

			// constants

			const int PROCESSORS = 4;
			const int N = 4;
			const int M = 4;
			const int N_LOCAL = 2;
			const int M_LOCAL = 2;
			const int INITIAL = -1;

			// test functions

			void testDistributedMatrix();
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
			int addIndex(int, int, int, int);
			int increase(int);
			int increaseIndex(int, int, int);
			int init(int, int);
			int newCol(int, int);
			int newRow(int, int);
			int rotateCols(int);
			int rotateRows(int);

		}

	}

}

#endif
