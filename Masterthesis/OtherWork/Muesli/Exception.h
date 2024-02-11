// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

// **********************************************************
// Exceptions 
// **********************************************************

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <sstream>

namespace msl {

	/* Abstract
	*/
	class Exception {

	public:
		
		virtual ~Exception() {
		};

		virtual const char* toString() const = 0;

	};

	// ***************** Exception for Skeletons ****************

	class UndefinedSourceException: public Exception {

	public:
		
		const char* toString() const {
			return "UndefinedSourceException";
		}

	};

	class UndefinedDestinationException: public Exception {

	public:
		
		const char* toString() const {
			return "UndefinedDestinationException";
		}

	};

	class NonLocalAccessException: public Exception {

	public:

		const char* toString() const {
			return "NonLocalAccessException";
		}

	};

	class MissingInitializationException: public Exception {

	public:

		const char* toString() const {
			return "MissingInitializationException";
		}

	};

	class IllegalGetException: public Exception {

	public:

		const char* toString() const {
			return "IllegalGetException";
		}

	};

	class IllegalPutException: public Exception {

	public:

		const char* toString() const {
			return "IllegalPutException";
		}

	};

	class IllegalPartitionException: public Exception {

	public:

		const char* toString() const {
			return "IllegalPartitionException";
		}

	};

	class PartitioningImpossibleException: public Exception {

	public:

	   const char* toString() const {
		   return "PartitioningImpossibleException";
	   }

	};

	class IllegalPermuteException: public Exception {

	public:

		const char* toString() const {
			return "IllegalPermuteException";
		}

	};

	class IllegalAllToAllException: public Exception {

	public:

		const char* toString() const {
			return "IllegalAllToAllException";
		}

	};

	class NoSolutionException: public Exception {

	public:

		const char* toString() const {
			return "NoSolutionException";
		}

	};

	class InternalErrorException: public Exception {

	public:
		
		const char* toString() const {
			return "InternalErrorException";
		}

	};

	// ***************** Exceptions for Collections *************

	class EmptyHeapException: public Exception {

	public:

		const char* toString() const {
			return "EmptyHeapException";
		}

	};

	class EmptyStackException: public Exception {

	public:
		
		const char* toString() const {
			return "EmptyStackException";
		}

	};

	class EmptyQueueException: public Exception {

	public:
		
		const char* toString() const {
			return "EmptyQueueException";
		}

	};

	// ***************** Various *************

	inline std::ostream& operator<<(std::ostream& os, const Exception& e) {
		os << e.toString() << std::endl;

		return os;
	}

	// ***************** DistributedSparseMatrix Exceptions *************

	class IndexOutOfBoundsException: public Exception {

	public:
		
		const char* toString() const {
			return "IndexOutOfBoundsException";
		}

	};

}

#endif
