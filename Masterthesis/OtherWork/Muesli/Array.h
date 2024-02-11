// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef ARRAY_H
#define ARRAY_H

#include "Serializable.h"
#include <sstream>
#include <iostream>

namespace msl {

	namespace test {

		template<typename T> class Array: public Serializable {

		private:

			T* values;
			int n;

			void init(int n) {
				this->n = n;
				values = new T[n];
			}

		protected:

		public:

			Array() {
			}

			Array(int size) {
				init(size);
			}

			~Array() {
				delete [] values;
			}

			int getLength() const {
				return n;
			}

			T getValue(int i) const {
				return values[i];
			}

			void setValue(int i, T value) {
				values[i] = value;
			}

			void print() const {               
                std::cout << "[" << values[0];
                for (int i = 1; i < n; i++)
                {
                    std::cout << " " << values[i];
                }
                std::cout << "] \n";
			}

			
			int getSize() {
				return sizeof(n) + n * sizeof(T);
			}

			void reduce(void* buffer, int bufferSize) {
				msl::write(buffer, n, 0);

				for(int i = 0; i < n; i++) {
					msl::write(buffer, values[i], msl::SOI + i * msl::SOI);
				}
			}

			void expand(void* buffer, int bufferSize) {
				init(msl::read<int>(buffer, 0));

				for(int i = 0; i < n; i++) {
					values[i] = msl::read<int>(buffer, msl::SOI + i * msl::SOI);
				}
			}

		};

	}

}

#endif
