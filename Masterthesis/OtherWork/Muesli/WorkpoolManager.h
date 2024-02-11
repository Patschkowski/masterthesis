// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef WORKPOOL_MANAGER_H
#define WORKPOOL_MANAGER_H

#include "Frame.h"

namespace msl {

	// Die Probleme werden in einer verketteten Liste verwaltet.
	template<class Data> class WorkpoolManager {

	private:

		// soviele Probleme muessen mindestens im Pool sein, damit Last abgegeben wird
		// ! Wenn Threshold = 1, dann ist es moeglich, dass das Originalproblem aufrund
		// einer Arbeitsanfrage unmittelbar an einen anderen Prozessor abgegeben wird
		// und die Terminierung probleme bereitet.
		static const int THRESHOLD = 2;

		class ListElem {
			friend class WorkpoolManager<Data>;
			Frame<Data>* data;
			ListElem *pred, *succ;
			ListElem(Frame<Data>* pDat): data(pDat), pred(NULL), succ(NULL) {}
		};

		ListElem *head, *tail;
		int size;

	public:

		// erzeugt einen neuen WorkpoolManager
		WorkpoolManager() {
			size = 0;
			head = new ListElem(NULL);
			tail = new ListElem(NULL);
			head->succ = tail;
			tail->pred = head;
		}

		// gibt Speicher fuer die sortierte Liste wieder frei
		~WorkpoolManager() {
			if(!isEmpty()) {
				ListElem* current = head->succ;

				while(current!=tail) {
					current = current->succ;
					delete current->pred;
				}
			}

			delete head;
			head = NULL;
			delete tail;
			tail = NULL;
		}

		// zeigt an, ob der Workpoolpool leer ist bzw. voll genug, um Last abzugeben
		inline bool isEmpty() {
			return size == 0;
		}

		inline bool hasLoad() {
			return size >= THRESHOLD;
		}

		inline int getSize() {
			return size;
		}

		void insert(Frame<Data>* f) {
			ListElem* e = new ListElem(f);
			e->succ = head->succ;
			e->pred = head;
			e->succ->pred = e;
			head->succ = e;
			size++;
		}

		Frame<Data>* get() {
			if(isEmpty()) {
				std::cout << Muesli::MSL_myId << ": EMPTY WORKPOOL EXCEPTION (get)" << std::endl;

				throw "empty workpool exception";
			}
			else {
				ListElem* firstElement = head->succ;
				Frame<Data>* pProblem = firstElement->data;
				head->succ = firstElement->succ;
				head->succ->pred = head;
				// Speicher fuer das ListElem freigeben. Probleme?
				delete firstElement;
				firstElement = NULL;
				size--;

				return pProblem;
			}
		}

		Frame<Data>* getLoad() {
			if(!hasLoad()) {
				std::cout << Muesli::MSL_myId << ": NO LOAD EXCEPTION (getLoad)" << std::endl;

				throw "no load exception";
			}
			else {
				ListElem* lastElement = tail->pred;
				Frame<Data>* pLoad = lastElement->data;
				tail->pred = lastElement->pred;
				tail->pred->succ = tail;
				delete lastElement;
				lastElement = NULL;
				size--;

				return pLoad;
			}
		}

		void show() {
			if(isEmpty())
				std::cout << Muesli::MSL_myId << ": Workpool: []" << std::endl;
			else {
				std::cout << Muesli::MSL_myId << ": Workpool: [";
				ListElem* currentElem = head->succ;

				while(currentElem->succ != tail) {
					std::cout << currentElem->data->getID() << ", ";
					currentElem = currentElem->succ;
				}

				std::cout << currentElem->data->getID() << "]" << std::endl;
			}
		}

	};

}

#endif
