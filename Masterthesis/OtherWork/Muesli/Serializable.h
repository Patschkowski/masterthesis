// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

namespace msl {

	const int SOB = sizeof(bool);
	const int SOC = sizeof(char);
	const int SOD = sizeof(double);
	const int SOF = sizeof(float);
	const int SOI = sizeof(int);
	const int SOL = sizeof(long);
	const int SOS = sizeof(short);

	/* Oberklasse aller serialisierbaren Klassen
	*/
	class Serializable {

	// Message vererbt den Zeiger "this" (4 Byte)
	public:

		Serializable() {
		}

		virtual ~Serializable() {
		}

		virtual int getSize() = 0;

		virtual void reduce(void* pBuffer, int bufferSize) = 0;

		virtual void expand(void* pBuffer, int bufferSize) = 0;

	};

	/* The following function are intended to be used inside
	   the user-defined reduce and expand functions. They are
	   capable of writing and reading values of arbitrary type
	   to and from a void* buffer, respectively. This is done
	   by treating the void* buffer as a char* buffer and
	   accessing the desired position using an index. The index
	   is interpreted as a number in bytes and is supposed to
	   point at the starting position of the element to store
	   or determine, respectively.

	   ATTENTION: BOTH FUNCTIONS ONLY WORK ON PLATFORMS, WHERE
	   SIZEOF(CHAR) = 1.
	*/

	/* This template function can be used to write the
	   given value of type T to a void* buffer at given
	   index. The index is interpreted as a number in
	   bytes and is supposed to point at the starting
	   position of the value to store.

	   The current implementation is merely a shortcut
	   for the following lines of code:
	   
	   // cast void* to char*
	   char* tmp1 = (char*)buffer;
	   // take address at given index
	   char* tmp2 = &tmp1[index];
	   // cast address to given type T
	   T* tmp3 = (T*)tmp2;
	   // set value
	   tmp3[0] = value;
	*/
	template<typename T>
	void write(void* buffer, T value, int index) {
		// all in one step
		((T*)(&(((char*)buffer)[index])))[0] = value;
	}

	/* The function template can be used to read a value
	   of type T from the given void* buffer at the given
	   index. The index is interpreted as a number in bytes
	   and is supposed to point at the starting position of
	   the value to read.

	   The current implementation is merely a shortcut for
	   the following lines of code:
	   
	   // cast void* to char*
	   char* tmp1 = (char*)buffer;
	   // take address at given index
	   char* tmp2 = &tmp1[index];
	   // cast address to given type T
	   T* tmp3 = (T*)tmp2;
	   // dereference and return
	   return *tmp3;
	*/
	template<typename T>
	T read(void* buffer, int index) {
		// all in one step
		return *((T*)(&((char*)buffer)[index]));
	}

}

#endif
