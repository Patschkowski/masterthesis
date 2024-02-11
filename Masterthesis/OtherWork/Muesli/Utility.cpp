// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#include <cstdarg>
#include <ctime>
#include <iomanip>
#include <math.h>
#include <sstream>
#include <string>

#ifdef _OPENMP
#include "omp.h"
#endif

#include "Utility.h"

bool msl::utl::equals(double a, double b, int precision) {
	return (int)(fabs(a - b) * pow(10., abs(precision))) == 0;
}

std::string msl::utl::getTime(std::string format, int length) {
	// current time
	char* tc;
	// current time
	time_t theTime;

	// get current time
	time(&theTime);
	// allocate space for the current time
	tc = new char[length];
	// format local time and
	strftime(tc, length * sizeof(char), format.c_str(), localtime(&theTime));
	// create result string
	std::string result(tc);

	// delete current time string
	delete tc;
	// return result
	return result;
}

void msl::utl::initSeed() {
	srand((unsigned)time(NULL));
}

void msl::utl::printt(const char* format, ...) {
	// string stream
	std::stringstream s;
	// pointer to the current argument
	va_list argp;
	// init variable argument pointer
	va_start(argp, format);

	// insert current time, separator, process
	// id, separator, and given format string
	s << getTime() << ", " << Muesli::MSL_myId << ": " << format;
	// print new format string
	vprintf(s.str().c_str(), argp);
	// clean up variable argument pointer
	va_end(argp);
}

void msl::utl::printv(const char* format, ...) {
	// pointer to the current argument
	va_list argp;
	// init variable argument pointer
	va_start(argp, format);

	// id of the process is equal to 0
	if(Muesli::MSL_myId == 0) {
		// print given format string
		vprintf(format, argp);
	}

	// clean up variable argument pointer
	va_end(argp);
}

double msl::utl::random() {
	return (rand() / (double)RAND_MAX);
}

std::string msl::utl::truncate(double value, int precision) {
	// create string stream
	std::stringstream s;
	
	// set precision of the stream and insert given value
	s << std::fixed << std::setprecision(precision) << value;

	// return result
	return s.str();
}
