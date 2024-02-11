// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef SOLUTIONPOOL_MANAGER_2_H
#define SOLUTIONPOOL_MANAGER_2_H

#include "Frame.h"
#include "Muesli.h"

namespace msl {

	/* A solutionpool is a kind of a sorted stack
	   KindID = VaterID * D + 1, VaterID = (KindID - 1) / D, WurzelID = 0
	*/
	template<class Solution> class SolutionpoolManager2 {

	private:

		// zeigt an, wie viele Probleme gleichzeitig verwaltet werden muessen -> determiniert #stacks
		int numOfStacks;
		// SolutionStack; array of pointers to array of pointers to Frame-objects (I hate C++ :-)
		Frame<Solution>*** stack;
		// number of elements that can be stored in the stack[i]
		int* sizeOfStack;
		// index of the top element stored in the stack[i]
		int* lastElementOfStack;
		// sendqueue stores Solutionframes to send back to a solver
		Frame<Solution>** sendQueue;
		//
		int sizeOfSendQueue;
		//
		int lastElementOfSendQueue;
		//
		DFct1<Solution**,Solution*> comb;
		// number of child nodes within the computation tree
		int D;

	public:

		/* Erzeugt einen neuen SolutionpoolManager2
		*/
		SolutionpoolManager2(int nos, DFct1<Solution**,Solution*>& combineFct, int d):
		numOfStacks(nos), comb(combineFct), D(d) {
			// fuer jedes theoretisch koexistente initiale
			  // DC-Problem einen eigenen Solutionstack anlegen
			sizeOfStack = new int[numOfStacks];
			lastElementOfStack = new int[numOfStacks];
			stack = new Frame<Solution>**[numOfStacks];
			
			for(int i = 0; i < numOfStacks; i++) {
				// default value
				sizeOfStack[i] = 8;
				lastElementOfStack[i] = -1;
				// default: Array mit 8 Zeigern auf Frames
				stack[i] = new Frame<Solution>*[8];
			}

			// default value
			sizeOfSendQueue = 8;
			sendQueue = new Frame<Solution>*[sizeOfSendQueue];
			lastElementOfSendQueue = -1;
		}


		// gibt Speicher fuer die sortierten Listen wieder frei
		~SolutionpoolManager2() {
			for(int i = 0; i < numOfStacks; i++) {
				delete[] stack[i];
				stack[i] = NULL;
			}

			delete[] stack;
			stack = NULL;
			delete[] sizeOfStack;
			sizeOfStack = NULL;
			delete[] lastElementOfStack;
			lastElementOfStack = NULL;
			delete[] sendQueue;
			sendQueue = NULL;
		}

		inline bool sendQueueIsEmpty() {
			return lastElementOfSendQueue == -1;
		}

		inline bool sendQueueIsFull() {
			return lastElementOfSendQueue == sizeOfSendQueue-1;
		}

		inline void removeElementFromSendQueue() {
			if(sendQueueIsEmpty()) {
				std::cout << Muesli::MSL_myId << ": EMPTY SENDQUEUE EXCEPTION (remove)" << std::endl;

				throw "empty SENDQUEUE exception";
			}
			else {
				// DRAFT: erstes Element loeschen, alles andere umkopieren.
				// TODO verkettete Liste daraus bauen! TODO
				delete sendQueue[0];
				Muesli::numSF--;
				
				for(int i = 0; i < lastElementOfSendQueue; i++)
					sendQueue[i] = sendQueue[i + 1];

				sendQueue[lastElementOfSendQueue--] = NULL;
			}
		}

		inline Frame<Solution>* readElementFromSendQueue() {
			if(sendQueueIsEmpty()) {
				std::cout << Muesli::MSL_myId << ": EMPTY SENDQUEUE EXCEPTION (read)" << std::endl;
				throw "empty SENDQUEUE exception";
			}
			else
				return sendQueue[0];
		}

		void writeElementToSendQueue(Frame<Solution>* pFrame) {
			// sendQueue is full -> extend sendQueue
			if(sendQueueIsFull()) {
				Frame<Solution>** s = new Frame<Solution>*[sizeOfSendQueue * 2];
				
				for(int i = 0; i < sizeOfSendQueue; i++)
					// copy pointers to new sendqueue
					s[i] = sendQueue[i];
				
				sizeOfSendQueue *= 2;
				// free memory
				delete[] sendQueue;
				sendQueue = s;
			}

			sendQueue[++lastElementOfSendQueue] = pFrame;
		}

		void showSendQueue() {
			if(lastElementOfSendQueue == -1)
				std::cout << Muesli::MSL_myId << ": SendQueue: []" << std::endl;
			else {
				std::cout << Muesli::MSL_myId << ": SendQueue: [";

				for(int i = 0; i < lastElementOfSendQueue; i++)
					std::cout << sendQueue[i]->getID() << ", ";

				std::cout << sendQueue[lastElementOfSendQueue]->getID() << "]" << std::endl;
			}
		}

		// zeigt an, ob der Solutionpool i leer/voll ist
		inline bool isEmpty(int i) {
			return lastElementOfStack[i] == -1;
		}

		inline bool isFull(int i) {
			return lastElementOfStack[i] == sizeOfStack[i] - 1;
		}

		inline Frame<Solution>* top(int stackID) {
			if(isEmpty(stackID)) {
				std::cout << Muesli::MSL_myId << ": EMPTY SOLUTIONPOOL EXCEPTION (top)" << std::endl;
				throw "empty solutionpool exception";
			}
			else
				return stack[stackID][lastElementOfStack[stackID]];
		}

		inline void pop(int stackID) {
			if(isEmpty(stackID)) {
				std::cout << Muesli::MSL_myId << ": EMPTY SOLUTIONPOOL EXCEPTION (pop)" << std::endl;

				throw "empty solutionpool exception";
			}
			else {
				delete stack[stackID][lastElementOfStack[stackID]]->getData();
				Muesli::numS--;
				delete stack[stackID][lastElementOfStack[stackID]];
				Muesli::numSF--;
				stack[stackID][lastElementOfStack[stackID]] = NULL;
				lastElementOfStack[stackID]--;
			}
		}

		// Legt ein Frame auf den Stack. Der Stack wird entsprechend der Frame-IDs sortiert
		// gehalten. Wenn ein Problem in N Subprobleme aufgeteilt wird, koennen ggf. nach dem
		// Versinken die oberen N Probleme auf dem Stack zu einer neuen Loesung kombiniert werden.
		void insert(Frame<Solution>* pFrame) {
			// bestimme den Stack, in den der Frame geschrieben werden muss
			int stackID = pFrame->getPoolID();

			// stack is full -> extend stack
			if(isFull(stackID)) {
				Frame<Solution>** s = new Frame<Solution>*[sizeOfStack[stackID] * 2];
				
				for(int i = 0; i < sizeOfStack[stackID]; i++)
					// copy pointers to new stack
					s[i] = stack[stackID][i];
				
				sizeOfStack[stackID] *= 2;
				// free memory
				delete[] stack[stackID];
				stack[stackID] = s;
			}

			// can be -1 !
			int index = lastElementOfStack[stackID]++;
			stack[stackID][lastElementOfStack[stackID]] = pFrame;
			// sink

			while(index >= 0 && stack[stackID][index]->getID() > stack[stackID][index + 1]->getID()) {
				Frame<Solution>* pHelp = stack[stackID][index + 1];
				stack[stackID][index + 1] = stack[stackID][index];
				stack[stackID][index] = pHelp;
				index--;
			}
		}

		inline bool isLeftSon(int id) {
			return (id % D) == 1;
		}

		inline bool isRightSon(int id) {
			return (id % D) == 0;
		}

		// deepCombine scannt alle Stacks nach kombinierbaren Teilloesungen, die durch empfangene
		// Teilloesungen tief im Stack vorliegen koennen. Sind solche Loesungen vorhanden, wird
		// pro Aufruf genau ein combine durchgefuehrt. Ein erfolgreiches Kombinieren wird durch
		// den Rueckgabewert true angezeigt.
		bool deepCombine() {
			bool combined = false;

			for(int i = 0; i < numOfStacks; i++) {
				if(!isEmpty(i)) {
					int currentID;
					int end = lastElementOfStack[i] - D + 1;

					// Durchlaufe den Stack von tiefsten bis zum obersten Element
					for(int currentFrameIndex = 0; currentFrameIndex <= end; currentFrameIndex++) {
						currentID = stack[i][currentFrameIndex]->getID();
						// suche einen linken Sohn
						if(isLeftSon(currentID)) {
							// sind seine Brueder auch da? Wenn ja, dann kann kombiniert werden!
							if(stack[i][currentFrameIndex + D - 1]->getID() == currentID + D - 1) {
								// kombiniere Elemente i bis i+(D-1)
								Solution** partialSolution = new Solution*[D];
								
								// alle Bruderknoten aus den Frames entpacken
								for(int j = 0; j < D; j++)
									partialSolution[j] = stack[i][currentFrameIndex+j]->getData();
								
								// fasse Teilloesungen zusammen, berechne
								// neue ID und baue daraus einen Frame
								// Muscle-Function call
								Solution* solution = comb(partialSolution);
								Muesli::numS++;
								// KindID = VaterID * D + 1; VaterID = (KindID - 1) / D; WurzelID = 0
								long parentID = (stack[i][currentFrameIndex]->getID() - 1) / D;
								long rootNodeID = stack[i][currentFrameIndex]->getRootNodeID();
								long originator = stack[i][currentFrameIndex]->getOriginator();
								long poolID = stack[i][currentFrameIndex]->getPoolID();
								Frame<Solution>* solutionFrame = new Frame<Solution>(parentID,
									rootNodeID, originator, poolID, solution);
								Muesli::numSF++;
								// Speicher freigeben
								delete partialSolution;
								partialSolution = NULL;
								
								for(int j = 0; j < D; j++) {
									delete stack[i][currentFrameIndex+j]->getData();
									Muesli::numS--;
									delete stack[i][currentFrameIndex+j];
									Muesli::numSF--;
								}

								// umkopieren: Mitten im Stack ist ein Loch
								// entstanden. Alle elemente fallen herunter
								while(currentFrameIndex + D <= lastElementOfStack[i]) {
									stack[i][currentFrameIndex] = stack[i][currentFrameIndex + D];
									currentFrameIndex++;
								}

								while(currentFrameIndex <= lastElementOfStack[i])
									stack[i][currentFrameIndex++] = NULL;
								
								lastElementOfStack[i] -= D;
								Muesli::numSF++;

								if(rootNodeID == parentID) {
									// Loesung in SendQueue schreiben
									writeElementToSendQueue(solutionFrame);
									//insert(solutionFrame);
									solutionFrame = NULL;
								}
								// schreibe neu berechneten Frame in den Pool
								else {
									insert(solutionFrame);
								}

								// an dieser Stelle ist (currentFrameIndex == end) und die
								// for-schleife bricht ab
								combined = true;
							}
							// if
						}
						// if
					}
					// for
				}
				// if

				if(combined)
					break;
			}
			// for

			return combined;
		}

		// kombiniert, sofern moeglich, alle top D Elemente
		// auf den Stacks, die kombiniert werden koennen.
		void combine() {
			// bestimme den Stack, der kombinierbare Loesungen
			// enthaelt (ist bei Aufruf (noch) gegeben)
			int stackID = -1;
			
			for(int pool = 0; pool < numOfStacks; pool++) {
				// Wenn ein rechter Sohn auf dem betrachteten Stack liegt
				if(!isEmpty(pool) && stack[pool][lastElementOfStack[pool]]->getID() % D == 0) {
					// und alle Bruderknoten ebenfalls vorhanden sind
					int leftSonIndex = lastElementOfStack[pool] - D + 1;
					
					if((leftSonIndex >= 0) && (stack[pool][leftSonIndex]->getID() + D - 1 ==
						stack[pool][lastElementOfStack[pool]]->getID())) {
						// beende die Suche
						stackID = pool;
						break;
					}
				}
			}

			// wenn es nichts zu kombinieren gibt, ist man hier schon fertig
			if(stackID == -1)
				return;
			// ansonsten verweist stackID auf den Stack, der kombinierbare Teilloesungen enthaelt
			else {
				// Array mit Teilloesungen vorbereiten
				Solution** partialSolution = new Solution*[D];
				int currentFrameIndex = lastElementOfStack[stackID] - D + 1;
				
				// alle Bruderknoten aus den Frames entpacken und ins Array schreiben
				for(int i = 0; i < D; i++)
					partialSolution[i] = stack[stackID][currentFrameIndex++]->getData();
				
				// kombiniere Teilloesungen, berechne neue ID und baue daraus einen Frame
				// Muscle-Function call
				Solution* solution = comb(partialSolution);
				Muesli::numS++;
				delete partialSolution;
				partialSolution = NULL;
				// KindID = VaterID * D + 1; VaterID = (KindID - 1) / D; WurzelID = 0
				long parentID = (stack[stackID][lastElementOfStack[stackID] - D + 1]->getID() - 1) / D;
				long rootNodeID = stack[stackID][lastElementOfStack[stackID] - D + 1]->getRootNodeID();
				long originator = stack[stackID][lastElementOfStack[stackID] - D + 1]->getOriginator();
				long poolID = stack[stackID][lastElementOfStack[stackID] - D + 1]->getPoolID();
				Frame<Solution>* solutionFrame = new Frame<Solution>(parentID, rootNodeID, originator,
					poolID, solution);
				Muesli::numSF++;
				
				// entferne Teilloesungen aus dem Solutionpool, die soeben kombiniert wurden
				for(int i = 0; i < D; i++)
					// Speicher wird freigegeben
					pop(stackID);
				
				if(rootNodeID == parentID) {
					// Loesung in SendQueue schreiben
					writeElementToSendQueue(solutionFrame);
					solutionFrame = NULL;
				}
				// schreibe neu erzeugten Frame in den Pool
				else {
					insert(solutionFrame);
				}

				// wiederhole dies rekursiv, bis nichts mehr zusammengefasst werden kann
				combine();
			}
		}

		// KindID = VaterID * D + 1; VaterID = (KindID - 1) / D; WurzelID = 0
		inline bool hasSolution(int stackID) {
			if(!isEmpty(stackID))
				return (stack[stackID][lastElementOfStack[stackID]]->getID() == 0);
			else
				return false;
		}

		void show() {
			for(int stackID = 0; stackID < numOfStacks; stackID++) {
				if(lastElementOfStack[stackID] == -1)
					std::cout << Muesli::MSL_myId << ": Solutionpool " << stackID << ": []" << std::endl;
				else {
					std::cout << Muesli::MSL_myId << ": Solutionpool " << stackID << ": [";
					
					for(int i = 0; i < lastElementOfStack[stackID]; i++)
						std::cout << stack[stackID][i]->getID() << ", ";
					
					std::cout << stack[stackID][lastElementOfStack[stackID]]->getID() << "]" << std::endl;
				}
			}
		}

	};

}

#endif
