// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_GAUSS_H
#define TEST_GAUSS_H

#include "DistributedMatrix.h"

namespace msl {

	namespace test {

		namespace gauss {

			// variables

			static int np;
			static int nt;

			// auxiliary functions

			inline double init(const int a, const int b);

			inline double copyPivot(const msl::DistributedMatrix<double>& A, int k, int i, int j,
				double Pij);

			inline double pivotOp(const msl::DistributedMatrix<double>& A,
				const msl::DistributedMatrix<double>& Pivot, int k, int i, int j, double Pij);

			msl::DistributedMatrix<double> gauss(const msl::DistributedMatrix<double>& A, int n);

			// test function

			void testGauss(int n, int nt, bool flag);

		}

	}

}

#endif
