// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef DISTRIBUTION_H
#define DISTRIBUTION_H

namespace msl {

	/* This abstract class is the root for all classes, which are
	   responsible for distributing the submatrices among all
	   collaborating processes. A distribution object controls
	   which submatrix, identified by its id, must be stored by
	   which process, identified by its id, too. Simultaneously,
	   a distribution object can tell you, how many submatrices
	   must be accomodated for by each process. These two functions
	   must be overridden by concrete implementations of this class.

	   Furthermore, this class offers three auxiliary functions
	   which do some calculations regarding the number of submatrices
	   per column and row, respectively. Additionally, the maximum
	   number of submatrices can be computed.
	*/
	class Distribution {

	private:

	protected:

		// global number of rows
		int n;
		// global number of columns
		int m;
		// parameter r
		int r;
		// parameter c
		int c;
		// number of processes
		int np;
		// maximum number of submatrices
		int max;

	public:

		// initializer for the distribution
		virtual void initialize(int np, int n, int m, int r, int c, int max);

		// true, if the given distribution object returns the
		// same process id for each submatrix id. false otherwise
		bool equals(Distribution& d);

		// true, if the process with the given id stores
		// the submatrix with the given id. false otherwise.
		bool isStoredLocally(int idProcess, int idSubmatrix);

		// returns the id of the process which is responsible
		// for storing the submatrix with the given id.
		virtual int getIdProcess(int idSubmatrix) = 0;

		// clone function for distribution objects -> factory method pattern
		virtual Distribution* clone() const = 0;
                
                virtual ~Distribution() {}

	};

}

#endif
