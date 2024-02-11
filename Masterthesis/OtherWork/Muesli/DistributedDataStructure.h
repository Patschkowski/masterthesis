// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef DISTRIBUTED_DATA_STRUCTURE_H
#define DISTRIBUTED_DATA_STRUCTURE_H

namespace msl {

	/* The class extracts common properties from all distributed data
	   structures. Currently, it only consists of two variables, namely
	   id and n. But maybe, there is more to come...
	*/
	class DistributedDataStructure {

	private:

	protected:

		// position of processor in data parallel group of processors; zero-base
		int id;
		// dimension of the global data structure
		int n;
		// dimension of the local data structure
		int nLocal;

		/* Default constructor. Explicitly protected,
		   such that no objects can be created.
		*/
		DistributedDataStructure(int size): n(size) {
		}

	public:

		int getId() const;

		int getSize() const;

		int getLocalSize() const;

	};

}

#endif
