// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef ELEMENT_H
#define ELEMENT_H

#include <string>

#include "Serializable.h"

namespace msl {

	/* This struct simply stores an element of type T and is mainly
	   used as a "superclass" for the struct MatrixElement.
	*/
	template<typename T> class Element {

	private:

	protected:

		T value;

	public:

		Element(): value(0) {
		}

		Element(T value): value(value) {
		}

		virtual ~Element() {
		}

		T getValue() const {
			return value;
		}

		void setValue(T value) {
			this->value = value;
		}

		virtual void print() const = 0;

	};

}

#endif
