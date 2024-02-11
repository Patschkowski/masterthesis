// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef WORKPOOL_MANAGER_2_H
#define WORKPOOL_MANAGER_2_H

#include "Frame.h"

namespace msl {

	// Die Probleme werden in einer verketteten Liste verwaltet.
	template<class Data> class WorkpoolManager2 {

	private:

		// soviele Probleme muessen mindestens im jedem Pool sein, damit Last abgegeben wird
		// ! Wenn Threshold = 1, dann ist es moeglich, dass das Originalproblem aufrund einer
		// Arbeitsanfrage unmittelbar an einen anderen Prozessor abgegeben wird und die Termi-
		// nierung Probleme bereitet.
		static const int THRESHOLD = 3;

		class ListElem {
			friend class WorkpoolManager2<Data>;
			Frame<Data>* data;
			ListElem *pred, *succ;
			ListElem(Frame<Data>* pDat): data(pDat), pred(NULL), succ(NULL) {}
		};

		// zeigt an, wie viele DCP gleichzeitig im System vorhanden sein koennen
		int numOfPools;
		int	primaryPoolID;

		ListElem** head;
		ListElem** tail;
		int* sizeOfPool;

	public:

		// erzeugt einen neuen WorkpoolManager2
		WorkpoolManager2(int nop): numOfPools(nop) {
			primaryPoolID = -1;
			sizeOfPool = new int[nop];
			head = new ListElem*[nop];
			tail = new ListElem*[nop];

			// initialisiere die benoetigten Workpools
			for(int i = 0; i < numOfPools; i++) {
				sizeOfPool[i] = 0;
				head[i] = new ListElem(NULL);
				tail[i] = new ListElem(NULL);
				head[i]->succ = tail[i];
				tail[i]->pred = head[i];
			}
		}

		// gibt Speicher fuer die sortierte Liste wieder frei
		~WorkpoolManager2() {
			ListElem* current = NULL;

			for(int i = 0; i < numOfPools; i++) {
				// Speicher fuer alle noch im Pool liegenden Elemente
				// freigeben (sollte nie durchlaufen werden)
				if(sizeOfPool[i] > 0) {
					// pool i ist nicht leer
					current = head[i]->succ;

					while(current!=tail[i]) {
						current = current->succ;
						delete current->pred;
					}
				}

				// Sentinels loeschen
				delete head[i];
				delete tail[i];
				head[i] = NULL;
				tail[i] = NULL;
			}

			// Arrays loeschen
			delete[] head;
			delete[] tail;
			delete[] sizeOfPool;
			tail = NULL;
			head = NULL;
			sizeOfPool = NULL;
		}

		// zeigt an, ob der Workpoolpool leer ist
		inline bool isEmpty() {
			for(int i = 0; i < numOfPools; i++)
				if(sizeOfPool[i] > 0)
					return false;

			return true;
		}

		// zeigt an, ob der Workpoolpool voll genug ist, um Last abzugeben
		inline bool hasLoad() {
			for(int i = 0; i < numOfPools; i++)
				if(sizeOfPool[i] >= THRESHOLD)
					return true;

			return false;
		}


		inline int getSize(int p) {
			return sizeOfPool[p];
		}

		void insert(Frame<Data>* f) {
			int poolID = f->getPoolID();
			ListElem* e = new ListElem(f);
			e->succ = head[poolID]->succ;
			e->pred = head[poolID];
			e->succ->pred = e;
			head[poolID]->succ = e;
			sizeOfPool[poolID]++;
		}

		Frame<Data>* get() {
			if(isEmpty()) {
				std::cout << Muesli::MSL_myId << ": EMPTY WORKPOOL EXCEPTION (get)" << std::endl;

				throw "empty workpool exception";
			}
			// sonst liegt im Workpool mindestens ein Problem
			else {
				// bestimme einen nicht leeren Pool. Hierbei sind verschiedene Strategien denkbar.
				// 1) Nimm den ersten nicht leeren Pool: Vorteil: Speicherplatzsparend, weil schnell
				//    gemeinsam ein bestimmtes Problem geloest wird
				// 2) bearbeite erst alle fremden Teilprobleme
				// 3) waehle Pool mit den wenigsten/kleinsten Problemen aus.
				ListElem* firstElement = NULL;
				int poolID = 0;

				bool probFound = false;
				
				// Es gilt: primaryPoolID := -1 fuer alle Nicht-MS. Nicht-MS finden also immer
				// Fremdprobleme
				for(poolID = 0; poolID < numOfPools; poolID++) {
					// Wenn ich ein Fremdproblem habe, wird das bevorzugt
					if(poolID != primaryPoolID && sizeOfPool[poolID]>0) {
						firstElement = head[poolID]->succ;
						probFound = true;
						break;
					}
				}

				// wenn keine Fremdprobleme im Pool liegen, nimm ein Problem vom primaryPool
				if(!probFound) {
					firstElement = head[primaryPoolID]->succ;
					poolID = primaryPoolID;
				}

				Frame<Data>* pProblem = firstElement->data;
				head[poolID]->succ = firstElement->succ;
				head[poolID]->succ->pred = head[poolID];
				// Speicher fuer das ListElem freigeben. Probleme?
				delete firstElement;
				firstElement = NULL;
				sizeOfPool[poolID]--;
				return pProblem;
			}
		}

		Frame<Data>* getLoad() {
			if(!hasLoad()) {
				std::cout << Muesli::MSL_myId << ": NO LOAD EXCEPTION (getLoad)" << std::endl;
				throw "no load exception";
			}
			else {
				// bestimme einen nicht leeren Pool. Hierbei sind verschiedene Strategien denkbar.
				// 1) Nimm den ersten nicht leeren Pool: Vorteil: Speicherplatzsparend, weil schnell
				//    gemeinsam ein bestimmtes Problem geloest wird
				// 2) waehle Pool mit den wenigsten/kleinsten Problemen aus.
				ListElem* lastElement = NULL;
				int poolID = 0;

				// Strategie 3: Gebe vorzugsweise ein Fremdproblem ab
				// im Pool muss ein Fremdproblem liegen
				bool probFound = false;
				
				for(poolID = 0; poolID < numOfPools; poolID++) {
					if(poolID!=primaryPoolID && sizeOfPool[poolID] >= THRESHOLD) {
						lastElement = tail[poolID]->pred;
						probFound = true;
						break;
					}
				}

				// wenn keins gefunden, nimm eins vom primaryPool
				if(!probFound) {
					lastElement = tail[primaryPoolID]->pred;
					poolID = primaryPoolID;
				}

				if(lastElement == NULL)
					std::cout << "ERROR in Workpool2::getLoad(): *lastElement == NULL" << std::endl;

				Frame<Data>* pLoad = lastElement->data;
				tail[poolID]->pred = lastElement->pred;
				tail[poolID]->pred->succ = tail[poolID];
				delete lastElement;
				lastElement = NULL;
				sizeOfPool[poolID]--;
				return pLoad;
			}
		}

		inline void setPrimaryPoolID(int ppid) {
			primaryPoolID = ppid;
		}

		void show() {
			ListElem* currentElem = NULL;

			for(int poolID = 0; poolID < numOfPools; poolID++) {
				if(sizeOfPool[poolID]==0)
					std::cout << Muesli::MSL_myId << ": Workpool " << poolID << ":[]" << std::endl;
				else {
					std::cout << Muesli::MSL_myId << ": Workpool " << poolID << ": [";
					currentElem = head[poolID]->succ;

					while(currentElem->succ != tail[poolID]) {
						std::cout << currentElem->data->getID() << ", ";
						currentElem = currentElem->succ;
					}

					std::cout << currentElem->data->getID() << "]" << std::endl;
				}
			}
		}

	};

}

#endif
