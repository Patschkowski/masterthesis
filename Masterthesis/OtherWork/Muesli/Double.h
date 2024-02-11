// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef DOUBLE_H
#define DOUBLE_H

#include <ostream>

#include "Serializable.h"

namespace msl {
	
	namespace test {

		class Double: public msl::Serializable {

		private:

			double val;

		protected:

		public:

			Double();

			Double(double);

			~Double();

			double getValue() const;

			void setValue(double);

			int getSize();

			void reduce(void* pBuffer, int bufferSize);

			void expand(void* pBuffer, int bufferSize);

		};

		Double operator+(const Double& i, const Double& j);

		bool operator==(const Double& i, const Double& j);

		bool operator!=(const Double& i, const Double& j);

		std::ostream& operator<<(std::ostream& os, const Double& i);

	}

}

#endif
