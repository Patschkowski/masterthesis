// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef	TEST_BRANCH_AND_BOUND_H
#define	TEST_BRANCH_AND_BOUND_H

#include "Double.h"

namespace msl {

	namespace test {

		namespace bab {

			// test functions

			// the bool is just used to distinguish the function
			// from the other branch and bound test function
			void testBranchAndBound(bool);
			void testBranchAndBoundFrame();
			void testBranchAndBoundFrameWorkpool();
			void testBranchAndBoundProblemTracker();
			void testBranchAndBoundSolver();
			void testBranchAndBound();

			// auxiliary functions

			void bound(msl::test::Double*);
			msl::test::Double** branch(msl::test::Double*, int*);
			int getLowerBound(msl::test::Double*);
			bool isSolution(msl::test::Double*);
			bool less(msl::test::Double*, msl::test::Double*);

		}

	}

}

#endif
