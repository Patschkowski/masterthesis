// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef BBFRAME_WORKPOOL_H
#define BBFRAME_WORKPOOL_H

#include "BBFrame.h"
#include "Curry.h"
#include "Exception.h"
#include "Workpool.h"

namespace msl {

	/* Workpool, der Frames verwaltet. Arbeitet im Gegensatz zum obigen Workpool mit Zeigern
	 * auf die Elemente statt mit Kopien. Der Workpool ist in Form eines Heaps realisiert,
	 * dessen Priorisierung gemaeﬂ der unteren Schranken der Verwalteten Probleme erfolgt.
	 * Um die Heapbedingung herzustellen, benoetigt er die betterThan Funktion des BranchAndBound
	 * Skeletts.
	 */
	template<class I>
	class BBFrameWorkpool {

		bool debug;

		int last;
		int size;
		// Array von Pointern auf Frames
		BBFrame<I>** heap;
		// Funktion zum Vergleich zweier Elemente des Workpools
		DFct2<I*, I*, bool> betterThan;

		int statMaxSize;
		int statCumulatedSize;
		int statNumOfInserts;

	  public:

		BBFrameWorkpool(DFct2<I*, I*, bool> less): betterThan(less), last(-1), size(8) {
  			heap = new BBFrame<I>*[size];
  			debug = false;
			statCumulatedSize = 0;
			statMaxSize = 0;
			statNumOfInserts = 0;
  		}

  		BBFrameWorkpool(bool (*less)(I*, I*)):
		last(-1), size(8), betterThan(Fct2<I*, I*, bool, bool (*)(I*, I*)>(less)) {
  			heap = new BBFrame<I>*[size];
  			debug = false;
  			statCumulatedSize = 0;
  			statMaxSize = 0;
  			statNumOfInserts = 0;
  		}

  		~BBFrameWorkpool() {
  			delete [] heap;
  		}

  		bool isEmpty() {
			return last < 0;
		}

		/* Gibt einen Zeiger auf das Frame ganz oben auf dem Heap zurueck.
		*/
  		BBFrame<I>* top() {
    		if(isEmpty())
				throws(EmptyHeapException());

    		return heap[0];
		}

		/* Liefert das oberste Element des Heaps und stellt die Heapbedingung wieder her.
		*/
  		BBFrame<I>* get() {
    		if(isEmpty())
				throws(EmptyHeapException());

    		// Ergebnis merken
			BBFrame<I>* result = heap[0];
    		// Heapbedingung wieder herstellen
    		heap[0] = heap[last--];
    		int current = 0;
    		int next = 2 * current + 1;

    		while(next <= last + 1) {
      			if((next <= last) && (betterThan((heap[next + 1]->getData()), (heap[next]->getData()))))
					next++;

      			if(betterThan((heap[next]->getData()), (heap[current]->getData()))) {
        			BBFrame<I>* aux = heap[next];
					heap[next] = heap[current];
					heap[current] = aux;
        			current = next;
        			next = 2 * current + 1;
        		}
      			else
					next = last + 2;
      		}
			// stop while loop

    		return result;
		}

		/* Fuegt ein Element in den Heap ein und stellt die Heapbedingung wieder her.
		*/
  		void insert(BBFrame<I>* val) {
    		int current = ++last;
    		
			// extend heap
			if(last >= size) {
      			if(debug)
					std::cout << "Workpool::insert() : extending heap" << std::endl << std::flush;

      			BBFrame<I>** newheap = new BBFrame<I>*[2 * size];
	      		
				for(int i = 0; i < size; i++)
					newheap[i] = heap[i];

      			size *= 2;
	      		
				if(debug)
					std::cout << "Workpool::insert() : deleting old heap" << std::endl;

      			// Speicher wieder freigeben
				delete[] heap;
	      		
				if(debug)
					std::cout << "Workpool::insert() : heap deleted" << std::endl;

      			heap = newheap;
      		}

    		int next;
    		heap[last] = val;
	    	
			while(current > 0) {
      			next = (current - 1) / 2;

      			if(betterThan((heap[current]->getData()), (heap[next]->getData()))) {
        			BBFrame<I>* aux = heap[next];
        			heap[next] = heap[current];
        			heap[current] = aux;
        			current = next;
        		}
      			else
					current = 0;
      		}

    		statMaxSize = (last > statMaxSize) ? last : statMaxSize;
    		statCumulatedSize += last;
    		statNumOfInserts++;
		}
		// stop loop

  		DFct2<I*, I*, bool> getBetterThanFunction() {
  			return betterThan;
  		}
		
		/* Setzt den Heap auf den Ausgangszustand zurueck.
		*/
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
      			std::cout << heap[i]->getID() << "," << heap[i]->getData() << " ; ";

    		std::cout << "]" << std::endl;
		}

		int getMaxLength() {
			return statMaxSize;
		}

		int getAverageLength() {
			if(statNumOfInserts > 0)
				return (statCumulatedSize / statNumOfInserts);

			return 0;
		}

	};

}

#endif
