// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef TEST_FFT_H
#define TEST_FFT_H

#include "DistributedArray.h"

namespace msl {

	namespace test {

		namespace fft {

			// variables

			// general assumption: higher bits to the left of lower bits
			static int log2p;
			static int log2size;
			static int n;
			static int np;
			static int nt;
			static double pi = 3.141592653589793238462643383;

			// structs

			struct complex {
				
				double real;
				double imag;

				complex(double r, double i): real(r), imag(i) {
				}
				
				complex() {
				}

			};

			// auxiliary functions

			complex operator+(complex c1, complex c2);
			complex operator*(complex c1, complex c2);
			std::ostream& operator<<(std::ostream& os, const complex& c);
			complex copy(const msl::DistributedArray<complex>& A, int i, complex Ai);
			complex init(const msl::DistributedArray<double>& A, int i);
			int bitcomplement(int k, int i);
			complex omega(int j, int i);
			complex combine(const msl::DistributedArray<complex>& T, int j, int i, complex Ai);
			complex fetch(const msl::DistributedArray<complex>& R, int j, int i, complex Ti);

			// test function

			void testFft(int n, int nt, bool flag);
			msl::DistributedArray<complex> fft(const msl::DistributedArray<double>& A, int n);

		}

	}

}

#endif
