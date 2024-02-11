// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_SHORTEST_H
#define TEST_SHORTEST_H

#include "DistributedMatrix.h"

namespace msl {

	namespace test {

		namespace shortest {

			// auxiliary functions

			int negate(const int a);

            int min(const int a, const int b);

            inline int init(int x, int y);

            template <class C>
                int copy(const msl::DistributedMatrix<C>& A, int row, int col, C x);

			template<class C> C skprod(const msl::DistributedMatrix<C>& A,
				const msl::DistributedMatrix<C>& B, int i, int j, C Cij);

			template<class C> msl::DistributedMatrix<C> matmult(msl::DistributedMatrix<C> A,
				msl::DistributedMatrix<C> B);

            template <class C> 
            msl::DistributedMatrix<C> shortestPaths(msl::DistributedMatrix<C>& A, int n);

			// test functions

			void testShortest(int n, bool flag);

		}

	}

}

#endif
