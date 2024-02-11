// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_DISTRIBUTED_SPARSE_MATRIX_H
#define TEST_DISTRIBUTED_SPARSE_MATRIX_H

#include "DistributedSparseMatrix.h"

namespace msl {

	namespace test {

		namespace dsm {

			// constants

			const int PROCESSORS = 4;
			const int C = 3;
			const int M = 6;
			const int N = 4;
			const int R = 2;

			// test functions

			void testAuxiliaryFunctions();
			void testConstructors();
			void testDestructor();
			void testDistributedSparseMatrix();
			void testSkeletons();
			void testSkeletonsCommunication();
			void testSkeletonsComputation();
			void testSkeletonsMap();
			void testSkeletonsRemaining();
			void testSkeletonsZip();

			// argument functions

			int add(int, int);
			int addIndex(int, int, int, int);
			bool count(int);
			bool elementCount(int);
			bool elementCountIndex(int, int, int);
			int filter(int);
			int filterIndex(int, int, int);
			int increase(int);
			int increaseIndex(int, int, int);
			int rotateColumns(int);
			int rotateRows(int);

			// various functions

			void init();

		}

	}

}

#endif
