// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef DISTRIBUTED_ARRAY_H
#define DISTRIBUTED_ARRAY_H

#include <sstream>
#include <string>
#include <iostream>

#include "mpi.h"

#include "Curry.h"
#include "DistributedDataStructure.h"
#include "Exception.h"
#include "Muesli.h"

namespace msl {

	// ***************************************************************************************************
	// Distributed Arrays with data parallel skeletons map, fold, zip, gather, scan, permute, broadcast...
	// ***************************************************************************************************

	/* The correct functionality of the DistributedArray is based on the following
	assumptions, which are basically made for simplifying the computations:
	- The array is devided into np blocks of equal size n, i.e. np * nLocal = n.
	- The number of blocks and processes respectively is a power of 2, i.e. np = 2 ^ x
	- The number of processes divides n without remainder, i.e. n % np = 0.
	- The template parameter E is one of the following primitive datatypes: int, long,
	double or float. Modifiers like 'unsigned' or 'l' are permitted.

	These assumptions restrict the usage of the skeleton library and will be removed in
	future versions.
	*/
	template<class E>
	class DistributedArray : public DistributedDataStructure {

  		// first index in local partition; assuming division mode: block
		int first;
  		// first index in next partition
		int nextfirst;
		// temporal variable used for the index operator []
		E  dummy;
  		// locally stored elements; E a[nLocal]
		E* a;
		// pointer to temporal variable
		E* dummyPointer;
		// array containing the mpi ids of the processes belonging to the distributed array
		int* ranks;
		// total number of mpi processes
		int np;

	private:

		/* Initializes the variables of the distributed array.
		*/
  		inline void init() {
			if((Muesli::MSL_myExit == MSL_UNDEFINED) || (Muesli::MSL_myEntrance == MSL_UNDEFINED)) {
				throws(MissingInitializationException());
			}
					
      		/* problematisch, da für n % np != 0 elemente
			   "verschluckt" werden, d.h. sie werden als
			   "0" ausgegeben. Ansätze:
			   1. letzter Prozess erhält kleinere Partition.
			   2. Partition des letzten Prozesses wird mit
			   Dummies aufgefüllt.
			   Problem dabei: Was soll bei permute passieren?
			*/

			// calculate number of locally stored elements
      		nLocal = n / Muesli::MSL_numOfLocalProcs;
			// calculate own process id
      		id = Muesli::MSL_myId - Muesli::MSL_myEntrance;
			// calculate index of the first locally stored element
      		first = id * nLocal;
			// calculate index of the first locally stored element of the next process
      		nextfirst = first + nLocal;
			// initialize number of processes
			np = Muesli::MSL_numOfLocalProcs;
			// create local array
      		a = new E[nLocal];
			//initialize ranks array
			ranks = new int[np];
            for(int i = 0; i < np; i++) {
                ranks[i] = i + Muesli::MSL_myEntrance;
            }
		}

	public:

		// METHODS / CONSTRUCTORS

		/* Default constructor. Creates a distributed array with the
		   given total number of elements 'n'. The elements are not
		   initialized.
		*/
  		DistributedArray(int size): DistributedDataStructure(size) {
			init();
		}

		/* Enhanced constructor. Creates a distributed array with
		   the given total number of elements 'size' and initalizes
		   all elements with the given value 'initial'.
		*/
  		DistributedArray(int size, E initial): DistributedDataStructure(size) {
    		init();

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				a[i] = initial;
			}
		}

		/* Enhanced constructor. Creates a distributed array with the
		   given total number of elements 'size' and initializes the
		   elements with the values given through the array 'b'. This
		   procedure assumes, that the number of elements stored in
		   the array b is equal to 'size'. If the size of the array	b
		   is smaller than 'size', the program will crash.
		*/
  		DistributedArray(int size, const E b[]): DistributedDataStructure(size) {
    		init();

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				a[i] = b[i + first];
			}
		}

		/* Enhanced constructor. Creates a distributed array with the
		   given total number of elements 'size' and initializes all
		   elements via the given function 'f'. This function may
		   incorporate the global index to initialize an element.
		*/
  		DistributedArray(int size, E (*f)(int)): DistributedDataStructure(size) {
    		init();
			
			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				a[i] = f(i + first);
			}
		}

		/* Enhanced constructor. Creates a distributed array with the
		   given total number of elements 'size' and initializes all
		   elements via the given function 'f'. This function may
		   incorporate the global index to initialize an element.
		*/
  		template<class F>
  		DistributedArray(int size, const Fct1<int, E, F>& f): DistributedDataStructure(size) {
    		init();

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				a[i] = f(i + first);
			}
		}

		/* Copy constructor.
		*/
		DistributedArray(const DistributedArray<E>& b): DistributedDataStructure(b.n) {
			init();

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				a[i] = b.a[i];
			}
		}

		// METHODS / DESTRUCTOR

		/* Destructs the distributed array by freeing the memory
		   for the locally stored elements.
		*/
		~DistributedArray() {
			delete [] a;
		}

		// METHODS / HELPER

		/* Returns a copy of the distributed array. The user is
		   responsible for cleaning up the memory after using the copy.
		*/
		inline DistributedArray<E>* copy() const {
			return new DistributedArray<E>(*this);
		}

		/* Returns a copy of the distributed array. The user is
		   responsible for cleaning up the memory after using the copy.
		*/
  		DistributedArray<E>* copyWithGap(int size, E dummy) const {
    		// gaps are filled with dummy
			DistributedArray<E>* copy = new DistributedArray<E>(size, dummy);
    		int number = (nLocal < copy->getLocalSize()) ? nLocal : copy->getLocalSize();

			#pragma omp parallel for
			for(int i = 0; i < number; i++) {
				copy->setLocal(i, a[i]);
			}

    		return copy;
		}
		
		inline E get(int i) const {
			// old, but strange implementation without broadcast.
			// necessary for some old benchmarks, e.g. bitonic sort
     		//return a[i - first];

			// id of the process which stores the indexed element
			int idSource;;
			// the element to get
			E message;

			// indexed element is locally available
			if(isLocal(i)) {
				// copy element
				message = a[i - first];
				// determine id
				idSource = Muesli::MSL_myId;
			}
			// indexed element is not locally available
			else {
				// determine id of the process which stores the indexed element
				idSource = (int)(i / nLocal);
			}

			// broadcast element
			msl::broadcast(&message, ranks, np, idSource);
			// return result
			return message;
		}

  		/* Gets a reference to the element with the given global index 'i'. The
		   program will suffer starvation, if i >= n, i.e. the global index is
		   out of bounds. Otherwise the indexed element will be broadcasted by
		   the process which stores the element locally. All other processes will
		   wait for this broadcast to happen. This procedure ensures, that all
		   processes will return the same global element.

		   Since this method returns a reference to the desired element, it can
		   be used in combination with the index operator [] (see below). Due to
		   the fact that the data structure is distributed, returning a reference
		   to an element which is not stored locally is a bit tricky, but so is
		   the implementation. The basis are the two variables 'E dummy' and 'E*
		   dummyPointer'. While the former is used to store the indexed element,
		   the latter is simply a pointer to this value. The process which stores
		   the desired element first stores the reference to the desired element
		   in the dummy pointer. Afterwards, it dereferences the pointer and copies
		   its content to the dummy variable. Thus, only the process which stores
		   the indexed element locally has copied the element to its own dummy
		   variable and holds a pointer to it, all other processes have not done
		   anything so far. Next, the process which stores the element locally
		   broadcasts the dummy value to all other processes which themselves store
		   the value in their own dummy variable. Finally, and this is the main
		   trick, all processes return the dereferenced dummy pointer. In case of
		   reading an element, all processes return the same value, since all dummy
		   pointers point to the dummy value storing the broadcasted element. In
		   case of writing back an element, only the dummy pointer of the process
		   which stores the indexed element locally has a pointer to the real value.
		   All other dummy pointers point to the dummy value and will write the
		   value back to it, but this will not change the global value. For this
		   mechanism to function properly, it is absolutely necessary to reset the
		   dummy pointer each time this method is called. Otherwise, the dummy
		   pointer still points to an element in the distributed array while the
		   user wants to access another element (sort of dangling pointer problem).
		   As I said, tricky :-)
		*/
		inline E& getNew(int i) {
			// id of the process which stores the indexed element locally
			int idSource;
			// reset pointer to dummy value
			dummyPointer = &dummy;

			// element with the given index is stored locally
			if(isLocal(i)) {
				// store pointer to original value
				dummyPointer = &a[i - first];
				// copy element
				dummy = *dummyPointer;
				// determine id
				idSource = Muesli::MSL_myId;
			}
			// element with the given index is not stored locally
			else {
				// determine id of the process storing the desired element
				idSource = (int)(i / nLocal);
			}

			// broadcast element
			msl::broadcast(&dummy, ranks, np, idSource);
			// return dummy pointer
			return *dummyPointer;
		}

		/* Returns the global index of the first locally stored element.
		*/
  		inline int getFirst() const {
			return first;
		}

		/* Returns the element with the given local index 'i' assuming that
		   0 <= i < localsize. If the index is out of the local bounds, the
		   program will crash. Note that this is not checked due to
		   performance reasons.
		*/
  		//inline E getLocal(int i) const {
		//	return a[i];
		//}

		/* 
		* only applicable to locally available elements;
		* pass 'true' if given index is global. 
		*/
		inline E getLocal(int i, bool globalIndex = false) const {
			if (globalIndex) return a[i-first];
			else return a[i];
		}

		// *deprecated*
		inline E getLocal(int i, int dummy) const {
			return a[i-first];
		}

		/* Returns a pointer to the local partition.
		*/
		E** getPartition() {
			return &a;
		}

		/* True, if the given index 'i' is locally available, false otherwise.
		*/
  		inline bool isLocal(int i) const {
			return (i >= first) && (i < nextfirst);
		}

		/* Overwrites the element with the given global index 'i' with the given
		   value 'v'. If the element is stored locally, the element is overwritten.
		   Otherwise no action takes place.
		*/
		inline void set(int i, E v) {
			// element is stored locally
			if(isLocal(i)) {
				// store element
				a[i - first] = v;
			}
		}

		/* Overwrites the element with the given local index 'i' with the
		   given value 'v' assuming that 0 <= i < localsize. If the index
		   is out of the local bounds, the program will crash. Note that
		   this is not checked due to performance reasons.
		*/
  		inline void setLocal(int i, E v) {
			 a[i] = v;
		}

		/* Prints the contents of the distributed array to standard output.
		   Note that this method is only executed by the process with the
		   id 0, so the distributed array is only printed once. alternatively,
		   use << (see below).
		*/
		inline void show() const {
			// array to store all values to
			E* b = new E[n];
			// output stream
			std::ostringstream s;

			gather(b);
			//msl::gatherAll(a, b, nLocal * sizeof(a[0]));

			if(Muesli::MSL_myId == Muesli::MSL_myEntrance) {
				s << "[";

				for(int i = 0; i < n - 1; i++) {
					s << b[i] << " ";
				}

				s << b[n - 1] << "]" << std::endl;
			}

			// delete b
			delete [] b;
			// print array
			printf("%s", s.str().c_str());
		}

		// METHODS / HELPER / DEBUG

		/* The following functions are only intended to be used while
		   debugging the library. They should be deleted or commented out
		   in the final version of the library.
		*/

		/* Just for debugging purposes; prints all local variables to
		   standard output. If printValues is set to true, the method
		   additionally prints all values to standard output. If set
		   to false, the values are not printed.
		*/
		void debug(bool printValues = false) const {
			std::ostringstream s;

			s << "id        = " << id        << std::endl;
			s << "n         = " << n         << std::endl;
			s << "nLocal    = " << nLocal    << std::endl;
			s << "first     = " << first     << std::endl;
			s << "nextFirst = " << nextfirst << std::endl;

			if(printValues) {
				s << "a         = [";

				for(int i = 0; i < nLocal; i++) {
					s << a[i];

					if(i < nLocal - 1) {
						s << ", ";
					}
					else {
						s << "]\n";
					}
				}
			}

			printf("%s", s.str().c_str());
		}

		// METHODS / OPERATORS

		/* Overloads the index operator [] used to access elements of the
		   distributed array. Note that this operator can be used to both
		   retrieve and overwrite elements of the distributed array. This
		   is achieved by means of two auxiliary variables 'E dummy' and
		   'E* dummyPointer' (cf. E& get(int)).
		*/
		E& operator[](int index) {
			return getNew(index);
		}

		// METHODS / SKELETONS

		// METHODS / SKELETONS / COMPUTATION / COUNT

		template<typename F>
		int count(const Fct1<E, bool, F>& f) const {
			// set result to zero
			int result = 0;

			// iterate over all locally available elements
			#pragma omp parallel for reduction(+:result)
			for(int i = 0; i < nLocal; i++) {
				// result of applying given function to current value is true
				if(f(a[i])) {
					// increase counter
					result++;
				}
			}
			
			// exchange results

    		int power = 1;
    		int neighbor;
    		int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);
    		int result2;

    		for(int i = 0; i < log2noprocs; i++) {
      			neighbor = Muesli::MSL_myEntrance + (id ^ power);
      			power *= 2;
				MSL_SendReceive(neighbor, &result, &result2);
				result += result2;
			}

			// return result
    		return result;
		}

		int count(bool (*f)(E)) const {
			return count(curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / FOLD

		template<class F>
  		E fold(const Fct2<E, E, E, F>& f) const {
    		// assumption: f is associative
    		// parallel prefix-like algorithm
    		// step 1: local fold
    		E result = a[0];

			for(int i = 1; i < nLocal; i++) {
    			result = f(result, a[i]);
			}

    		// step 2: global folding
    		int power = 1;
    		int neighbor;
    		E result2;
    		int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);

    		for(int i = 0; i < log2noprocs; i++) {
      			neighbor = Muesli::MSL_myEntrance + (id ^ power);
      			power *= 2;
				msl::MSL_SendReceive(neighbor, &result, &result2);

				if(Muesli::MSL_myId < neighbor) {
					result = f(result, result2);
				}
				else {
					result = f(result2, result);
				}
			}

    		return result;
		}

  		inline E fold(E (*f)(E, E)) const {
			return fold(curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / FOLD INDEX

		template<class F, class G>
		E foldIndex(const Fct4<E, E, int, int, E, F>& f, const Fct2<E, E, E, G>& g,
			const int* const indexes, int size) {
			// current index
			int index;
			// global result of the fold operation
			E globalResult = 0;
			// local result of the fold operation
			E localResult = 0;
			// buffer for all local results of each process
			E* globalResults = new E[Muesli::MSL_numOfTotalProcs];

			// fold all locally stored submatrices
			for(int i = 0; i < size; i++) {
				// determine current index
				index = indexes[i];
				// fold current value
				localResult = f(localResult, a[index - first], index, i);
			}

			// exchange local results with all collaborating processes
			//MSL_Allgather(&localResult, globalResults);
			MPI_Allgather(&localResult, sizeof(E), MPI_BYTE, globalResults, sizeof(E),
				MPI_BYTE, MPI_COMM_WORLD);

			// fold all local results
			for(int i = 0; i < Muesli::MSL_numOfTotalProcs; i++) {
				// fold current value
				globalResult = g(globalResult, globalResults[i]);
			}

			// delete global results array
			delete [] globalResults;
			// return result
			return globalResult;
		}

		E foldIndex(E (*f)(E, E, int, int), E (*g)(E, E), const int* const indexes, int size) {
			return foldIndex(curry(f), curry(g), indexes, size);
		}

		/* The skeleton only folds the elements indexed by the given
		   array 'indexes'. 'Indexes' is supposed to store global
		   indexes. An index is simply skipped, if it is not locally
		   available. The given function 'f' is passed the arguments
		   as follows:

		   1. E: Result of the folding opertion so far.
		   2. E: Current element to fold.
		   3. int: Global index of the current element.
		   4. int: Loop index of the current element.

		   The given function 'g' is passed the arguments as follows:

		   1. E: Result of the folding operation so far.
		   2. E: Current element to fold.
		*/
		template<class F, class G>
		E foldIndexOMP(const Fct4<E, E, int, int, E, F>& f, const Fct2<E, E, E, G>& g,
			const int* const indexes, int size) {
			// OpenMP thread id
			int idThread;
			// current index
			int index;
			// maximum number of OpenMP threads
			int nt = omp_get_max_threads();
			// global result of the fold operation
			E globalResult = 0;
			// local result of the fold operation
			E localResult = 0;
			// buffer for all local results of each thread
			E* localResults = new E[nt];
			// buffer for all local results of each process
			E* globalResults = new E[Muesli::MSL_numOfTotalProcs];

			// iterate over all threads
			for(int i = 0; i < nt; i++) {
				// set to 0
				localResults[i] = 0;
			}

			#pragma omp parallel private(idThread, index)
			{
				// determine thread id
				idThread = omp_get_thread_num();

				#pragma omp for
				// fold all locally stored submatrices
				for(int i = 0; i < size; i++) {
					// determine current index
					index = indexes[i];

					// indexed element is stored locally
					if(isLocal(index)) {
						// fold current value
						localResults[idThread] = f(localResults[idThread], a[index - first], index, i);
					}
				}
			}

			// iterate over all threads
			for(int i = 0; i < nt; i++) {
				// fold local result of each thread
				localResult = g(localResult, localResults[i]);
			}

			// exchange local results with all collaborating processes
			//MSL_Allgather(&localResult, globalResults);
			MPI_Allgather(&localResult, sizeof(E), MPI_BYTE, globalResults, sizeof(E),
				MPI_BYTE, MPI_COMM_WORLD);

			// fold all local results
			for(int i = 0; i < Muesli::MSL_numOfTotalProcs; i++) {
				// fold current value
				globalResult = g(globalResult, globalResults[i]);
			}

			// delete local results array
			delete [] localResults;
			// delete global results array
			delete [] globalResults;
			// return result
			return globalResult;
		}

		E foldIndexOMP(E (*f)(E, E, int, int), E (*g)(E, E), const int* const indexes, int size) {
			return foldIndexOMP(curry(f), curry(g), indexes, size);
		}

		// METHODS / SKELETONS / COMPUTATION / FOLD LOCAL

		/* Folds all values by means of the given function f,
		   but does NOT broadcast the result. Instead, the
		   result is stored in the given variable 'destination'.
		*/
		void foldLocal(E (*f)(E, E), E& destination) const {
			// id of the current thread
			int idThread;
			// determine number of openmp threads
			int nt = omp_get_max_threads();
			// local result is stored here
			E result = 0;
			// local results are stored here
			E* results = new E[nt];

			// iterate over all threads
			for(int i = 0; i < nt; i++) {
				// set local result to 0
				results[i] = 0;
			}

			#pragma omp parallel private(idThread)
			{
				// determine openmp thread id
				idThread = omp_get_thread_num();

				#pragma omp for
				// iterate over all locally stored elements
				for(int i = 0; i < nLocal; i++) {
					// fold current element
    				results[idThread] = f(results[idThread], a[i]);
				}
			}

			// iterate over all threads
			for(int i = 0; i < nt; i++) {
				// fold current value
				result = f(result, results[i]);
			}
			
			// store result in the given variable
			destination = result;
		}

		// METHODS / SKELETONS / COMPUTATION / FOLD PARTITIONS IN PLACE

		/* This skeleton folds all local partitions as a whole by
		   applying the given function f to each of them. The re-
		   sult is stored locally.
		*/
		template<typename F>
		void foldPartitionsInPlace(const Fct2<E*, E*, E*, F>& f) {
			// power of 2
    		int power = 1;
			// id of the communication partner
    		int neighbor;
			// number of communication rounds
    		int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);
			// the new array containing the folded values
    		E* result = new E[nLocal];
			// buffer for receiving values from neighbours
    		E* buffer = new E[nLocal];

			// copy local elements to result array
			memcpy(result, a, nLocal * sizeof(E));

			// iterate over all communication rounds
    		for(int i = 0; i < log2noprocs; i++) {
				// compute id of the communication partner
      			neighbor = Muesli::MSL_myEntrance + (id ^ power);
				// double value of power
      			power *= 2;
				// exchange elements
				MSL_SendReceive(neighbor, result, buffer, nLocal);
				// fold elements
				result = f(result, buffer);
			}

			// delete buffer
			delete [] buffer;
			// delete old values
			delete [] a;
			// set pointer to new values
			a = result;
		}

		/* See above.
		*/
		void foldPartitionsInPlace(E* (*f)(E*, E*)) {
			foldPartitionsInPlace(curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / ITERATE

		/* Iterates over all locally stored elements and applies
		   the given function to each element. Can be used to
		   program with side effects or with curried functions.
		*/
		template<typename F>
		void iterate(const Fct1<E, void, F>& f) const {
			#pragma omp parallel for
			// iterate over all locally available elements
			for(int i = 0; i < nLocal; i++) {
				// apply given function f to current element
				f(a[i]);
			}
		}

		/* See above.
		*/
		void iterate(void (*f)(E)) const {
			iterate(curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / MAP

		template<class R, class F>
  		inline DistributedArray<R>* map(const Fct1<E, R, F>& f) const {
    		DistributedArray<R>* b = new DistributedArray<R>(n);

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				b->setLocal(i, f(a[i]));
			}

    		return b;
		}

  		template<class R>
  		inline DistributedArray<R>* map(R (*f)(E)) const {
			return map(curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / MAP / MAP INDEX

  		template<class R, class F>
		// requires no communication
  		inline DistributedArray<R>* mapIndex(const Fct2<int, E, R, F>& f) const {
    		DistributedArray<R>* b = new DistributedArray<R>(n);

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				b->setLocal(i, f(i + first, a[i]));
			}

    		return b;
		}

  		template<class R>
  		inline DistributedArray<R>* mapIndex(R (*f)(int, E)) const {
			return mapIndex(curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / MAP / MAP INDEX IN PLACE

  		template<class F>
  		inline void mapIndexInPlace(const Fct2<int, E, E, F>& f) {
			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				a[i] = f(i + first, a[i]);
			}
		}

  		inline void mapIndexInPlace(E (*f)(int, E)) {
			mapIndexInPlace(curry(f));
		}

		/* Applies the given function 'f' to all elements which are indexed by the given array
		   'indexes'. 'indexes' is supposed to store global indexes, its length must be given
		   by the parameter 'size'. If an indexed element is not stored locally, it is simply
		   skipped.

		   - const Fct3<E, int, int, E, F>& f - The function applied to the indexed elements.
			 The first parameter is the value of the current element. The second parameter is
			 the global index of the current element. The third parameter is the loop index
			 with 0 <= loop index < size.
		   - const int* const indexes - The array storing the global indexes of the elements
			 to which the given function f is applied to. Must be of length 'size'. If it is
			 smaller, the program is very likely to crash. If it is larger, surplus elements
			 are simply ignored.
		   - int size - The length of the given array 'indexes'.
		*/
		template<class F>
		void mapIndexInPlace(const Fct3<E, int, int, E, F>& f, const int* const indexes, int size) {
			// current global index
			int index;

			#pragma omp parallel for private(index)
			// iterate over all elements of the given index array
			for(int i = 0; i < size; i++) {
				// determine current index
				index = indexes[i];
				// apply given function to current element
				a[index - first] = f(a[index - first], index, i);
			}
		}

		void mapIndexInPlace(E (*f)(E, int, int), const int* const indexes, int size) {
			mapIndexInPlace(curry(f), indexes, size);
		}

		// METHODS / SKELETONS / COMPUTATION / MAP / MAP INDEX IN PLACE FOLD

		/* g is used to map, f is used to fold.
		*/
  		template<class F1, class F2>
  		E mapIndexInPlaceFold(const Fct2<int, E, E, F1>& g, const Fct2<E, E, E, F2>& f) const {
    		// result on every collaborating processor
    		int i;

    		// assumption: f is associative
    		// parallel prefix-like algorithm
    		// step 1: local fold
    		E result = g(first, a[0]);

			for(i = 1; i < nLocal; i++) {
    			result = f(result, g(i + first, a[i]));
			}

    		// step 2: global folding
    		int power = 1;
    		int neighbor;
    		E result2;
    		int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);

    		for(i = 0; i < log2noprocs; i++) {
      			neighbor = Muesli::MSL_myEntrance + (id ^ power);
      			power *= 2;
      			// important: this has to be synchronous!
				sendReceive(neighbor, &result, &result2, sizeof(E));
	      		
				if(Muesli::MSL_myId < neighbor) {
					result = f(result, result2);
				}
				else {
					result = f(result2, result);
				}
			}

    		return result;
		}

		/* Variant expecting two function pointers.
		*/
  		inline E mapIndexInPlaceFold(E (*g)(int, E), E (*f)(E, E)) const {
			return mapIndexInPlaceFold(curry(g), curry(f));
		}

		/* Variant expectiong a function pointer for g and a partial application for f.
		*/
  		template<class F>
  		inline E mapIndexInPlaceFold(E (*g)(int, E), const Fct2<E, E, E, F>& f) const {
			return mapIndexInPlaceFold(curry(g), f);
		}

		/* Variant expectiong a partial application for g and a function pointer for f.
		*/
  		template<class F>
  		inline E mapIndexInPlaceFold(const Fct2<int, E, E, F>& g, E (*f)(E, E)) const {
			return mapIndexInPlaceFold(g, curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / MAP / MAP IN PLACE

  		template<class F>
		// requires no communication
  		inline void mapInPlace(const Fct1<E, E, F>& f) {
			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				a[i] = f(a[i]);
			}
		}

  		inline void mapInPlace(E (*f)(E)) {
			mapInPlace(curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / MAP / MAP PARTITION IN PLACE

		/* These skeletons are kind of awkward, since the given function f
		   must be aware of the local size of each partition, i.e. is must
		   know, how the DistributedArray partitions the global array.
		*/

  		template<class F>
  		inline void mapPartitionInPlace(const Fct1<E*, void, F>& f) {
			f(a);
		}

  		inline void mapPartitionInPlace(void (*f)(E*)) {
			f(a);
		}

		// METHODS / SKELETONS / COMPUTATION / SCAN

  		template<class F>
  		void scan(const Fct2<E, E, E, F>& f) const {
    		int i;

    		// assumption: f is associative
    		// step 1: local scan
			for(i = 1; i < nLocal; i++) {
      			a[i]  = f(a[i - 1], a[i]);
			}

    		E sum = a[nLocal - 1];
    		// step 2: global scan
    		int power = 1;
    		int neighbor;
    		E neighborSum;
    		int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);

    		for(i = 0; i < log2noprocs; i++) {
      			neighbor = Muesli::MSL_myEntrance + (id ^ power);
      			power *= 2;
      			sendReceive(neighbor, &sum, &neighborSum, sizeof(E));

      			if(Muesli::MSL_myId > neighbor) {
        			for(int j = 0; j < nLocal; j++)
						a[j] = f(neighborSum, a[j]);

        			sum = f(neighborSum, sum);
				}
				else {
					sum = f(sum,neighborSum);
				}
			}
		}

  		inline void scan(E (*f)(E, E)) const {
			scan(curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / MAP / MAP INDEX IN PLACE PERMUTE PARTITION

  		template<class F1, class F2>
  		void mapIndexInPlacePermutePartition(const Fct2<int, E, E, F1>& f,
			const Fct1<int, int, F2>& g) {
    		int i;
			MPI_Status stat[1];
            MPI_Request req[1];
    		int receiver = Muesli::MSL_myEntrance + g(id);

			if((receiver < Muesli::MSL_myEntrance) ||
				(receiver >= Muesli::MSL_myEntrance + Muesli::MSL_numOfLocalProcs)) {
				throws(IllegalPartitionException());
			}

    		int sender = MSL_UNDEFINED;
	    	
			for(i = 0; i < Muesli::MSL_numOfLocalProcs; i++) {
				// determine sender by computing invers of g
				if(g(i) + Muesli::MSL_myEntrance == Muesli::MSL_myId) {
					if(sender == MSL_UNDEFINED) {
						sender = Muesli::MSL_myEntrance + i;
					}
         			// g is not bijective
					else {
						throws(IllegalPermuteException());
					}
				}
			}

			// g is not bijective
			if(sender == MSL_UNDEFINED) {
				throws(IllegalPermuteException());
			}
	    	
			if(receiver != Muesli::MSL_myId) {
      			int half = nLocal / 2;
				int half2 = nLocal - half;
      			E* b1 = new E[half];
      			E* b2 = new E[half2];

				for(i = 0; i < half; i++) {
					b1[i] = f(i + first, a[i]);
				}

				// assumption: messages with same sender and receiver don't pass each other
				msl::MSL_Send(receiver, b1, msl::MSLT_MYTAG, half);
				msl::MSL_Receive(sender, a, msl::MSLT_MYTAG, &stat[1], half);
      			int j = 0;

				for(i = half; i < nLocal; i++) {
					b2[j++] = f(i + first, a[i]);
				}

      			// assumption: messages with same 	sender and receiver
				msl::MSL_Send(receiver, b2, msl::MSLT_MYTAG, half2);
				msl::MSL_Receive(sender, &a[half], msl::MSLT_MYTAG, &stat[3], half2);
	      		
				delete[] b1;
				delete[] b2;
				delete[] req;
				delete[] stat;
			}
		}

  		inline void mapIndexInPlacePermutePartition(E (*f)(int, E), int (*g)(int)) {
    		return mapIndexInPlacePermutePartition(curry(f), curry(g));
		}

		// METHODS / SKELETONS / COMPUTATION / MAP / MAP INDEX IN PLACE SCAN

		template<class F1, class F2>
		void mapIndexInPlaceScan(const Fct2<int, E, E, F1>& g, const Fct2<E, E, E, F2>& f) const {
    		// result on every collaborating processor
    		int i;

    		// assumption: f is associative
    		// step 1: local scan
    		a[0] = g(first, a[0]);
	    	
			for(i = 1; i < nLocal; i++) {
      			a[i]  = f(a[i - 1], g(i + first, a[i]));
			}

    		E sum = a[nLocal - 1];

    		// step 2: global scan
    		int power = 1;
    		int neighbor;
    		E neighborSum;
    		int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);

    		for(i = 0; i < log2noprocs; i++) {
      			neighbor = Muesli::MSL_myEntrance + (id ^ power);
      			power *= 2;
      			sendReceive(neighbor, &sum, &neighborSum, sizeof(E));

      			if(Muesli::MSL_myId > neighbor) {
					for(int j = 0; j < nLocal; j++) {
						a[j] = f(neighborSum, a[j]);
					}

        			sum = f(neighborSum, sum);
				}
				else {
					sum = f(sum,neighborSum);
				}
			}
		}

		/* Variant expection a function pointer for each argument.
		*/
  		inline void mapIndexInPlaceScan(E (*g)(int, E), E (*f)(E, E)) const {
			mapIndexInPlaceScan(curry(g), curry(f));
		}
		
		/* Variant expection a function pointer for g and a partial application for f.
		*/
  		template<class F>
  		inline void mapIndexInPlaceScan(E (*g)(int, E), const Fct2<E, E, E, F>& f) const {
			mapIndexInPlaceScan(curry(g), f);
		}

		/* Variant expection a partial application for g and a function pointer for f.
		*/
  		template<class F>
  		inline void mapIndexInPlaceScan(const Fct2<int, E, E, F>& g, E (*f)(E, E)) const {
			mapIndexInPlaceScan(g, curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / ZIP WITH

  		template<class E2, class R, class F>
  		inline DistributedArray<R> zipWith(const DistributedArray<E2>& b,
		const Fct2<E, E2, R, F>& f) const {
    		DistributedArray<R> c(n);

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				c.setLocal(i, f(a[i], b.getLocal(i)));
			}

    		return c;
		}

  		template<class E2, class R>
  		inline DistributedArray<R> zipWith(const DistributedArray<E2>& b, R (*f)(E, E2)) const {
    		return zipWith(b, curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / ZIP WITH / ZIP WITH IN PLACE

		// requires no communication
  		template<class E2, class F>
  		inline void zipWithInPlace(const DistributedArray<E2>& b, const Fct2<E, E2, E, F>& f) {
			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				a[i] = f(a[i], b.getLocal(i));
			}
		}

		// fill gap at the end with dummy
  		template<class E2>
  		inline void zipWithInPlace(DistributedArray<E2>& b, E (*f)(E, E2)) {
    		zipWithInPlace(b, curry(f));
		}

		/* Variant of zipWithInPlace for 2 argument arrays. The
		   given function f is expected to receive the arguments
		   of the arrays in the following order: a (this), b, c.
		*/
  		template<class E2, class E3, class F>
  		inline void zipWithInPlace(const DistributedArray<E2>& b,
		const DistributedArray<E3>& c, const Fct3<E, E2, E3, E, F>& f) {
			#pragma omp parallel for
			// iterate over all locally available elements
			for(int i = 0; i < nLocal; i++) {
				// apply given function and store the result directly
				a[i] = f(a[i], b.getLocal(i), c.getLocal(i));
			}
		}

		/* See above.
		*/
  		template<class E2, class E3>
  		inline void zipWithInPlace(const DistributedArray<E2>& b,
		const DistributedArray<E3>& c, E (*f)(E, E2, E3)) {
    		zipWithInPlace(b, c, curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / ZIP WITH / ZIP WITH INDEX

		// requires no communication
  		template<class E2, class R, class F>
  		inline DistributedArray<R> zipWithIndex(const DistributedArray<E2>& b,
		const Fct3<int, E, E2, R, F>& f) const {
    		DistributedArray<R> c(n);

			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				c.setLocal(i, f(i + first, a[i], b.getLocal(i)));
			}

    		return c;
		}

  		template<class E2, class R>
  		inline DistributedArray<R> zipWithIndex(const DistributedArray<E2>& b,
		R (*f)(int, E, E2)) const {
    		return zipWithIndex(b, curry(f));
		}

		// METHODS / SKELETONS / COMPUTATION / ZIP WITH / ZIP WITH INDEX IN PLACE

		// requires no communication
  		template<class E2, class F>
  		inline void zipWithIndexInPlace(const DistributedArray<E2>& b,
		const Fct3<int, E, E2,E, F>& f) {
			#pragma omp parallel for
			for(int i = 0; i < nLocal; i++) {
				a[i] = f(i + first, a[i], b.getLocal(i));
			}
		}

  		template<class E2>
  		inline void zipWithIndexInPlace(const DistributedArray<E2>& b, E (*f)(int, E, E2)) {
    		zipWithIndexInPlace(b, curry(f));
		}

		// METHODS / SKELETONS / COMMUNICATION

		// METHODS / SKELETONS / COMMUNICATION / ALL TO ALL

  		/* allToAll: each collaborating processor sends a block of elements to every
		   other processor; the beginnings of all blocks are specified by an index
		   array; the received blocks are concatenated without gaps in arbitrary order.
		*/
  		void allToAll(const DistributedArray<int*>& Index, E dummy) {
    		MPI_Status stat;
    		E* b = new E[nLocal];
    		int i, current;
    		int start = (Index.get(id))[id];
    		int end = (Index.get(id))[id + 1];
    		int no1 = end - start;
    		int no2;

			// keep own block
			for(current = 0; current < no1; current++) {
				b[current] = a[start + current];
			}
	    	
			// exchange blocks with all others
			for(i = 1; i < Muesli::MSL_numOfLocalProcs; i++) {
    			start = (Index.get(id))[id ^ i];
      			end = (Index.get(id))[(id ^ i) + 1];
      			no1 = end - start;
      			sendReceive(Muesli::MSL_myId ^ i, &no1, &no2, sizeof(int));

				if(current + no2 > nLocal) {
					throws(IllegalAllToAllException());
				}

      			if(Muesli::MSL_myId > (Muesli::MSL_myId ^ i)) {
					if(no1 > 0) {
						syncSend(Muesli::MSL_myId ^ i, &a[start], sizeof(E) * no1);
					}
	        		
					if(no2 > 0) {
						MSL_receive(Muesli::MSL_myId ^ i, &b[current], sizeof(E) * no2, &stat);
					}
				}
      			else {
					if(no2 > 0) {
						MSL_receive(Muesli::MSL_myId ^ i, &b[current], sizeof(E) * no2, &stat);
					}

					if(no1 > 0) {
						syncSend(Muesli::MSL_myId ^ i, &a[start], sizeof(E) * no1);
					}
				}

      			current += no2;
			}

			for(i = 0; i < current; i++) {
				a[i] = b[i];
			}
			
			// fill gap at the end with dummy
			for(i = current; i < nLocal; i++) {
				a[i] = dummy;
			}

    		delete[] b;
		}

		// METHODS / SKELETONS / COMMUNICATION / BROADCAST

  		void broadcast(int index) {
    		int i;
     		int block = index / nLocal;
     		int localindex = index % nLocal;

			if((block < 0) || (block > n / nLocal)) {
				throws(IllegalPartitionException());
			}

     		unsigned int neighbor;
     		unsigned int power = 1;
     		// 2^30 - 2
			unsigned int mask = 1073741822;
     		MPI_Status stat;
     		int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);

     		for(i = 0; i < log2noprocs; i++) {
      			if((id & mask) == (block & mask)) {
        			neighbor = Muesli::MSL_myEntrance + (id ^ power);

					if((id & power) == (block & power)) {
          				syncSend(neighbor, &a[localindex], sizeof(E));
					}
					else {
						msl::MSL_Receive(neighbor, &a[localindex], msl::MSLT_MYTAG, &stat);
					}
				}

      			power *= 2;
      			mask &= ~power;
			}

			for(i = 0; i < nLocal; i++) {
				a[i] = a[localindex];
			}
		}

		// METHODS / SKELETONS / COMMUNICATION / BROADCAST PARTITION

  		void broadcastPartition(int block) {
			if((block < 0) || (block >= n / nLocal)) {
				throws(IllegalPartitionException());
			}

    		unsigned int neighbor;
     		unsigned int power = 1;
     		// 2^30 - 2
			unsigned int mask = 1073741822;
     		MPI_Status stat;
     		int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);

     		for(int i = 0; i < log2noprocs; i++) {
      			if((id & mask) == (block & mask)) {
        			neighbor = Muesli::MSL_myEntrance + (id ^ power);

					if((id & power) == (block & power)) {
          				syncSend(neighbor, a, sizeof(E) * nLocal);
					}
					else {
						msl::MSL_Receive(neighbor, a, msl::MSLT_MYTAG, &stat, nLocal);
					}
				}

      			power *= 2;
      			mask &= ~power;
			}
		}

		// METHODS / SKELETONS / COMMUNICATION / GATHER

  		void gather(E b[]) const {
			int i;

			// lokalen Array kopieren
			#pragma omp parallel for
			for(i = 0; i < nLocal; i++) {
				b[first + i] = a[i];
			}

			int power = 1;
			int neighbor;
			int log2noprocs = log2(Muesli::MSL_numOfLocalProcs);
			// can be avoided by massive index calculations
			int *indexstack = new int[n];
			indexstack[0] = first; 
			int top = 0;
			int oldtop;

			for(i = 0; i < log2noprocs; i++) {
				neighbor = Muesli::MSL_myEntrance + (id ^ power);
				power *= 2;
				oldtop = top;

				for(int j = 0; j <= oldtop; j++) { 
					// important: communication has to be synchronous!
					sendReceive(neighbor, &indexstack[j], &indexstack[++top], sizeof(int));
					sendReceive(neighbor, &b[indexstack[j]], &b[indexstack[top]], sizeof(E) * nLocal);
				}
			}

			delete [] indexstack;
		}

		// METHODS / SKELETONS / COMMUNICATION / PERMUTE

		// METHODS / SKELETONS / COMMUNICATION / PERMUTE / PERMUTE

  		template<class F>
  		void permute(const Fct1<int, int, F>& f) {
			bool isPermutation = true;
    		int k, l;
    		int newposition;
    		int receiver;
    		int sender;
    		int* f_inv = new int[n];
    		E* b = new E[nLocal];
    		MPI_Status stat;
			MPI_Request req;

			#pragma omp parallel for
			for(k = 0; k < n; k++) {
				f_inv[k] = MSL_UNDEFINED;
			}

			#pragma omp parallel for
    		for(k = 0; k < n; k++) {
				// check, if f is permutation (Exception, if not)
      			int dest = f(k);

				if((dest < 0) || (dest >= n) || (f_inv[dest] != MSL_UNDEFINED)) {
					isPermutation = false;
				}

      			f_inv[dest] = k;
			}

			if(!isPermutation) {
				throws(IllegalPermuteException());
			}

    		for(k = 0; k < nLocal; k++) {
				// send and receive local elements (with minimal buffer space)
      			newposition = f(first + k);
      			receiver = Muesli::MSL_myEntrance + newposition / nLocal;

				if(receiver != Muesli::MSL_myId) {
					msl::MSL_Send(receiver, &a[k]);
				}
				else {
					b[newposition - first] = a[k];
				}

      			for(l = k; l < n; l += nLocal) {
        			sender = Muesli::MSL_myEntrance + l / nLocal;
        			newposition = f(l);

					if((newposition >= first) && (newposition < nextfirst) &&
						(sender != Muesli::MSL_myId)) {
						msl::MSL_Receive(sender, &b[newposition - first], msl::MSLT_MYTAG, &stat);
					}
				}
			}

			#pragma omp parallel for
			for(k = 0; k < nLocal; k++) {
      			a[k] = b[k];
			}

    		delete[] b;
			delete[] f_inv;
		}

  		inline void permute(int (*f)(int)) {
			return permute(curry(f));
		}

		// METHODS / SKELETONS / COMMUNICATION / PERMUTE / PERMUTE PARTITION

		template<class F>
  		void permutePartition(const Fct1<int, int, F>& f) {
    		int i;
    		MPI_Request req[1];
			MPI_Status stat[1];
    		int sender = MSL_UNDEFINED;
    		int receiver = Muesli::MSL_myEntrance + f(id);

			if((receiver < Muesli::MSL_myEntrance) ||
				(receiver >= Muesli::MSL_myEntrance + Muesli::MSL_numOfLocalProcs)) {
				throws(IllegalPartitionException());
			}

			for(i = 0; i <  Muesli::MSL_numOfLocalProcs; i++) {
				// determine sender by computing invers of f
				if(f(i) + Muesli::MSL_myEntrance == Muesli::MSL_myId) {
					if(sender == MSL_UNDEFINED) {
						sender = Muesli::MSL_myEntrance + i;
					}
					// f is not bijective
					else {
						throws(IllegalPermuteException());
					}
				}
			}

			// f is not bijective
			if(sender == MSL_UNDEFINED) {
				throws(IllegalPermuteException());
			}

    		if(receiver != Muesli::MSL_myId) {
      			E* b = new E[nLocal];

				for(i = 0; i < nLocal; i++) {
					b[i] = a[i];
				}

				MSL_ISend(receiver, &b[0], req, stat, msl::MSLT_MYTAG, nLocal);
                MSL_Receive(sender, &a[0], msl::MSLT_MYTAG, stat, nLocal);
				MPI_Wait(req, stat);

      			delete[] b;
			}
		}

  		inline void permutePartition(int (*f)(int)) {
			return permutePartition(curry(f));
		}

        template<class F>
	void multiMapIndexInPlace(DistributedArray<E>& A, const Fct2<E, E, E, F>& f,
	DistributedArray<E>& B, const Fct2<E, E, E, F>& g) {
		int first = A.getFirst();
		int localsize = A.getLocalSize();

		if((first != B.getFirst()) || (nLocal != B.getLocalSize()))
			throws(NonLocalAccessException());
	    
		for(int i = 0; i < localsize; i++) {
			A.setLocal(i, f(i + first, A.getLocal(i)));
			A.setLocal(i, g(i + first, A.getLocal(i)));
		}
	}

};


	template<class E>
	std::ostream& operator<<(std::ostream& os, const msl::DistributedArray<E>& a) {
		int n = a.getSize();
		E* b = new E[n];

		a.gather(b);
		
		if(Muesli::MSL_myId == Muesli::MSL_myEntrance) {
			os << "[";

			for(int i = 0; i < n - 1; i++) {
				os << b[i] << " ";
			}

			os << b[n - 1] << "]";
		}

		delete [] b;
		return os;
	}

}

#endif
