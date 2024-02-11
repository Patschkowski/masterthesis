// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef BLOCK_DISTRIBUTION_H
#define BLOCK_DISTRIBUTION_H

#include "Distribution.h"

namespace msl {

	class BlockDistribution: public Distribution {

	private:

		int getNumberOfBigBlocks() const;

		int getNumberOfSmallBlocks() const;

		int getSubmatrixCountInBigBlock() const;

		int getSubmatrixCountInSmallBlock() const;

	protected:

	public:

		// returns the id of the process which is responsible
		// for storing the submatrix with the given id.
		int getIdProcess(int idSubmatrix);

		// clone function for block distributions
		BlockDistribution* clone() const;

	};

}

#endif
