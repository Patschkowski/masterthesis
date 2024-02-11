// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#include "BsrIndex.h"

#include <sstream>

// PUBLIC

int msl::BsrIndex::getColumnIndex() const {
	return columnIndex;
}

int msl::BsrIndex::getId() const {
	return id;
}

int msl::BsrIndex::getRowIndex() const {
	return rowIndex;
}

void msl::BsrIndex::setColumnIndex(int columnIndex) {
	this->columnIndex = columnIndex;
}

void msl::BsrIndex::setId(int id) {
	this->id = id;
}

void msl::BsrIndex::setRowIndex(int rowIndex) {
	this->rowIndex = rowIndex;
}

std::string msl::BsrIndex::toString() const {
	std::ostringstream stream;

	stream << "id = " << getId() <<
		", rowIndex = " << getRowIndex() <<
		", columnIndex = " << getColumnIndex();

	return stream.str();
}

// PUBLIC / CONSTRUCTORS

msl::BsrIndex::BsrIndex():
id(-1), rowIndex(-1), columnIndex(-1) {
}

msl::BsrIndex::BsrIndex(int id, int rowIndex, int columnIndex):
id(id), rowIndex(rowIndex), columnIndex(columnIndex) {
}
