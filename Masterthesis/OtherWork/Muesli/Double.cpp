// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#include "Double.h"

// Double

msl::test::Double::Double(): val(0) {
}

msl::test::Double::Double(double val): val(val) {
}

msl::test::Double::~Double() {
}

double msl::test::Double::getValue() const {
	return val;
}

void msl::test::Double::setValue(double value) {
	val = value;
}

// MSL_Serializable

int msl::test::Double::getSize() {
	return sizeof(val);
}

void msl::test::Double::reduce(void* pBuffer, int /*bufferSize*/) {
	((double*)pBuffer)[0] = val;
}

void msl::test::Double::expand(void* pBuffer, int /*bufferSize*/) {
	val = ((double*)pBuffer)[0];
}

// Operators

msl::test::Double msl::test::operator+(const msl::test::Double& i, const msl::test::Double& j) {
	msl::test::Double k(i.getValue() + j.getValue());

	return k;
}

bool msl::test::operator==(const msl::test::Double& i, const msl::test::Double& j) {
	return i.getValue() == j.getValue();
}

bool msl::test::operator!=(const msl::test::Double& i, const msl::test::Double& j) {
	return i.getValue() != j.getValue();
}

std::ostream& msl::test::operator<<(std::ostream& os, const msl::test::Double& i) {
	os << i.getValue();

	return os;
}
