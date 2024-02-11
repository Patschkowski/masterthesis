// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#include <math.h>

#include "BlockDistribution.h"

// PRIVATE

/* Return the number of blocks which store the large number of
   submatrices.
*/
int msl::BlockDistribution::getNumberOfBigBlocks() const {
	int big = getSubmatrixCountInBigBlock();
	int small;

	if(max % np == 0) {
		return max / big;
	}
	else {
		small = getSubmatrixCountInSmallBlock();

		return (max - np * small) / (big - small);
	}
}

/* Returns the number of blocks which store the small number of
   submatrices.
*/
int msl::BlockDistribution::getNumberOfSmallBlocks() const {
	if(max % np == 0) {
		return 0;
	}
	else {
		return np - getNumberOfBigBlocks();
	}
}

/* Returns the number of submatrices which are stored in a big block.
*/
int msl::BlockDistribution::getSubmatrixCountInBigBlock() const {
	return (int)ceil((double)max / np);
}

/* Returns the number of submatrices which are stored in a small block.
   Returns 0, if the number of submatrices which are stored in a small
   block is equal to the number of submatrices which are stored in a big
   block. This function assumes, that the submatrices are distributed
   equally among all collaborating processes.
*/
int msl::BlockDistribution::getSubmatrixCountInSmallBlock() const {
	int big = getSubmatrixCountInBigBlock();
	int small = (int)floor((double)max / np);

	if(big == small) {
		return 0;
	}
	else {
		return small;
	}
}

// PROTECTED

// PUBLIC

/* Returns the id of the process which stores the submatrix with the given id.
*/
int msl::BlockDistribution::getIdProcess(int idSubmatrix) {
	int numberOfBigBlocks = getNumberOfBigBlocks();
	int submatrixCountInBigBlock = getSubmatrixCountInBigBlock();
	int submatricesInBigBlocks = numberOfBigBlocks * submatrixCountInBigBlock;

	// id belongs to a process with a big block
	if(idSubmatrix < submatricesInBigBlocks) {
		return (int)floor((double)idSubmatrix / submatrixCountInBigBlock);
	}
	// id belongs to a process with a small block
	else {
		return (int)floor((double)(idSubmatrix - submatricesInBigBlocks) /
			getSubmatrixCountInSmallBlock()) + numberOfBigBlocks;
	}
}

/* Clone method.

   tested: false
*/
msl::BlockDistribution* msl::BlockDistribution::clone() const {
	return new msl::BlockDistribution(*this);
}
