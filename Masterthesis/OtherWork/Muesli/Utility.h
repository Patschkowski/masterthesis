// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef UTILITY_H
#define UTILITY_H

#include <cstdio>
#include <limits>
#include <ostream>
#include <string>

#include "Muesli.h"

namespace msl {

	namespace utl {

		/* Tests, whether the two given double values are equal
		   up to the given number of decimal places. For example,
		   calling equals(1.23456, 1.23450, 4) yields true,
		   calling equals(1.23456, 1.23450, 5) yields false
		   (6 != 0). Note that the given precision is interpreted
		   as an absolute value.
		*/
		bool equals(double a, double b, int precision);

		/* Returns the value which represents the negative
		   infinity for the given type T. In case the given
		   type has no representation for infinitiy, the
		   minimum value is returned.
		*/
		template<typename T> T getNegativeInfinity() {
			// given type has a value for infinity
			if(std::numeric_limits<T>::has_infinity) {
				// return value for negative infinity
				return - std::numeric_limits<T>::infinity();
			}
			// given type has no value for infinity
			else {
				// return minimum value for the given type
				return std::numeric_limits<T>::min();
			}
		}

		/* Returns the value which represents the positive
		   infinity for the given type T. In case the given
		   type has no representation for infinity, the
		   maximum value is returned.
		*/
		template<typename T> T getPositiveInfinity() {
			// given type has a value for infinity
			if(std::numeric_limits<T>::has_infinity) {
				// return value for positive infinity
				return std::numeric_limits<T>::infinity();
			}
			// given type has no value for infinity
			else {
				// return maximum value for the given type
				return std::numeric_limits<T>::max();
			}
		}

		/* Returns the current time in the given format. If
		   no format string is given, the time is returned
		   in the format hh:mm:ss. The second argument
		   determines the length of the output string. This
		   is needed for internal reasons, since the
		   formatted time string is first copied to a char
		   array. The given argument determines the size of
		   this array.
		   
		   ATTENTION: When passing the length of the char
		   array manually, keep in mind that the char-array
		   needs an additional termination character!

		   NOTE: Since the default format hh:mm:ss has eight
		   characters, the default length of the char array
		   has to be nine characters to include the termina-
		   tion character.
		*/
		std::string getTime(std::string format = "%X", int length = 9);

		/* Initializes the pseudo random number generator
		   with the current time. This is useful, if you
		   do not want to create identical sequences over
		   and over again, e.g. when using the class
		   MatrixGenerator.
		*/
		void initSeed();

		/* Function for printing vectors of arbitrary types.
		   The vector is enclosed by square brackets ("["
		   and "]"), elements are divided by blanks (" ").
		*/
		template <typename E> void print(E* vector, int size) {	
			std::ostringstream stream;

			stream << getTime() << ", " << Muesli::MSL_myId << ": [";

			for(int i = 0; i < size; i++) {
				stream << vector[i];

				if(i < size - 1) {
					stream << " ";
				}
			}

			stream << "]";

			printf("%s\n", stream.str().c_str());
		}

		/* Prints the given format string by preceding it
		   with the current time (cf. getTime()) and the
		   Muesli process id.
		*/
		void printt(const char* format, ...);

		/* Prints the given format string to standard output
		   by calling printf. Note that this is only done by
		   the process with id 0!
		*/
		void printv(const char* format, ...);
		
		/* Generates a pseudo-random number between 0 and 1.
		   See also initSeed() which is used to set the seed
		   of the pseudo random number generator.
		*/
		double random();

		/* Truncates the given double value to the given number
		   of decimal places. Calling truncate(1.2345, 3) yields
		   "1.234", calling truncate(1.2345, 6) yields "1.234500".
		   Note that the given precision is interpreted as an
		   absolute value.
		*/
		std::string truncate(double value, int precision);

	}

}

#endif
