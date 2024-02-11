// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef WORKPOOL_H
#define WORKPOOL_H

#include "Muesli.h"

namespace msl {

	// *** demand driven work distribution ***

	/* Lastverteilung: mit best. WS. Jeder Prozessor schickt an seinen linken Nachbarn eine
	   Info-Message mit der lowerBound seines besten Teilproblems im Workpool. Eine Lastver-
	   teilung erfolgt nur dann, wenn der linke Nachbar ein Problem abgeben kann, das besser
	   ist. LB-Info-Messages werden immer verschickt, wenn der Solver einen nicht leeren
	   Workpool hat. Ist sein Pool leer, wird nur einmalig eine Empty-Pool-Nachricht ver-
	   schickt. Dies verhindert, dass am Anfang das System mit Anfragen ueberschwemmt werden.
	 */
	template<class I> class Workpool {

		bool debug;

		int last;
		int size;
		I* heap;
		DFct2<I*, I*, bool> betterThan;

	public:

		Workpool(DFct2<I*, I*, bool> less): betterThan(less), last(-1), size(8) {
  			heap = new I[size];
  			debug = false;
  		}

  		//Workpool(bool (*less)(I, I)):
		Workpool(bool (*less)(I*, I*)):
		last(-1), size(8), betterThan(Fct2<I*, I*, bool, bool (*)(I*, I*)>(less)) {
  			heap = new I[size];
  			debug = false;
  		}

  		bool isEmpty() {
			return last < 0;
		}

  		I top() {
    		if(isEmpty())
				throws(EmptyHeapException());

    		return heap[0];
		}

  		I get() {
    		if(isEmpty())
				throws(EmptyHeapException());

    		I result = heap[0];
    		heap[0] = heap[last--];
    		int current = 0;
    		int next = 2 * current + 1;

    		while(next <= last + 1) {
				if((next <= last) && (betterThan(&heap[next + 1], &heap[next])))
					next++;

				if(betterThan(&heap[next], &heap[current])) {
        			I aux = heap[next];
					heap[next] = heap[current];
					heap[current] = aux;
        			current = next;
        			next = 2 * current + 1;
        		}
      			else
					next  = last + 2;
      		}
			// stop while loop

    		return result;
		}

  		void insert(I val) {
    		int current = ++last;

			// extend heap
    		if(last >= size) {
      			if(debug)
					std::cout << "Workpool::insert() : extending heap" << std::endl << std::flush;

      			I* newheap = new I[2 * size];
	      		
				for(int i = 0; i < size; i++)
					newheap[i] = heap[i];

      			size *= 2;
	      		
				if(debug)
					std::cout << "Workpool::insert() : deleting old heap" << std::endl;

      			if(debug)
					std::cout << "Workpool::insert() : heap deleted" << std::endl;

      			heap = newheap;
      		}

    		int next;
    		heap[last] = val;

    		while(current > 0) {
      			next = (current - 1) / 2;

				if(betterThan(&heap[current], &heap[next])) {
        			I aux = heap[next];
        			heap[next] = heap[current];
        			heap[current] = aux;
        			current = next;
        		}
      			else
					current = 0;
      		}
			// stop loop
		}

		DFct2<I*, I*, bool> getBetterThanFunction() {
  			return betterThan;
  		}

		void reset() {
			last = -1;
		}

  		Workpool<I>* fresh() {
  			if(debug)
				std::cout << "Workpool::fresh() invoked" << std::endl;

  			Workpool<I>* h = new Workpool<I>(betterThan);
			return h;
		}

		void show(ProcessorNo n) {
    		std::cout << "Prozessor " << n << " hat Workpool: [";

    		for(int i = 0; i <= last; i++)
      			std::cout << heap[i] << " ";

    		std::cout << "]" << std::endl << std::flush;
		}

	};

}

#endif
