// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef SOLUTIONPOOL_MANAGER_H
#define SOLUTIONPOOL_MANAGER_H

#include "Frame.h"
#include "Muesli.h"

namespace msl {

	// A solutionpool is a kind of a sorted stack
	// KindID = VaterID * D + 1; VaterID = (KindID - 1) / D; WurzelID = 0
	template<class Solution> class SolutionpoolManager {

	private:

		// SolutionStack; array of pointers to Frame-objects
		Frame<Solution>** stack;
		// number of elements that can be stored in the stack
		int size;
		// index of the top element stored in the stack
		int lastElement;
		// sendqueue stores Solutionframes to send back to a solver
		Frame<Solution>** sendQueue;
		int sizeOfSendQueue;
		int lastElementOfSendQueue;
		// combine-function
		Solution* (*comb)(Solution**);
		// number of child nodes within the computation tree
		int D;

	public:

		// erzeugt einen neuen SolutionpoolManager
		SolutionpoolManager(Solution* (*combineFct)(Solution**), int d): comb(combineFct), D(d) {
			// default value
			size = 8;
			// default value
			sizeOfSendQueue = 8;
			// Default: Array mit 8 Zeigern auf Frames
			stack = new Frame<Solution>*[size];
			sendQueue = new Frame<Solution>*[sizeOfSendQueue];
			lastElement = -1;
			lastElementOfSendQueue = -1;
		}

		// gibt Speicher fuer die sortierte Liste wieder frei
		~SolutionpoolManager() {
			delete[] stack;
			stack = NULL;
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
				delete sendQueue[0];
				numSF--;
				
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
			if(lastElementOfSendQueue==-1)
				std::cout << Muesli::MSL_myId << ": SendQueue: []" << std::endl;
			else {
				std::cout << Muesli::MSL_myId << ": SendQueue: [";
				
				for(int i = 0; i < lastElementOfSendQueue; i++)
					std::cout << sendQueue[i]->getID() << ", ";
				
				std::cout << sendQueue[lastElementOfSendQueue]->getID() << "]" << std::endl;
			}
		}

		// zeigt an, ob der Solutionpool leer/voll ist
		inline bool isEmpty() {
			return lastElement == -1;
		}

		inline bool isFull() {
			return lastElement == size-1;
		}

		inline Frame<Solution>* top() {
			if(isEmpty()) {
				std::cout << Muesli::MSL_myId << ": EMPTY SOLUTIONPOOL EXCEPTION (top)" << std::endl;

				throw "empty solutionpool exception";
			}
			else
				return stack[lastElement];
		}

		inline void pop() {
			if(isEmpty()) {
				std::cout << Muesli::MSL_myId << ": EMPTY SOLUTIONPOOL EXCEPTION (pop)" << std::endl;
				throw "empty solutionpool exception";
			}
			else {
				delete stack[lastElement]->getData();
				numS--;
				delete stack[lastElement];
				numSF--;
				stack[lastElement] = NULL;
				lastElement--;
			}
		}

		// Legt ein Frame auf den Stack. Der Stack wird entsprechend der Frame-IDs sortiert gehalten.
		// Wenn ein Problem in N Subprobleme aufgeteilt wird, koennen ggf. nach dem Versinken die
		// oberen N Probleme auf dem Stack zu einer neuen Loesung kombiniert werden.
		void insert(Frame<Solution>* pFrame) {
			// stack is full -> extend stack
			if(isFull()) {
				Frame<Solution>** s = new Frame<Solution>*[size*2];
				
				for(int i = 0; i < size; i++)
					// copy pointers to new stack
					s[i] = stack[i];

				size *= 2;
				// free memory
				delete[] stack;
				stack = s;
			}

			// can be -1 !
			int index = lastElement++;
			stack[lastElement] = pFrame;
			// sink

			while(index >= 0 && stack[index]->getID() > stack[index + 1]->getID()) {
				Frame<Solution>* pHelp = stack[index + 1];
				stack[index + 1] = stack[index];
				stack[index] = pHelp;
				index--;
			}
		}

		inline bool isLeftSon(int id) {
			return (id%D) == 1;
		}

		inline bool isRightSon(int id) {
			return (id%D) == 0;
		}

		// Wenn ein Frame mit der ID eines rechten Sohns auf dem Stack liegt, ist ein combine
		// denkbar. Es muss jedoch nicht explizit geprueft werden, ob alle Bruderknoten auf dem
		// Stack liegen. Da der Stack sortiert ist, reicht es zu pruefen, ob an der Stelle, an
		// der der linkeste Bruders stehen sollte, tatsaechlich auch der linkeste Bruder steht.
		// In diesem Fall muessen alle anderen Bruderknoten ebenfalls auf dem Stack liegen.
		inline bool combineIsPossible() {
			// Wenn rechter Sohn
			// Solutionpool ist bei Aufruf per Definition nicht leer
			if(!isEmpty() && stack[lastElement]->getID() % D == 0) {
				int leftSonIndex = lastElement - D + 1;
				
				if((leftSonIndex >= 0) && (stack[leftSonIndex]->getID() + D - 1 ==
					stack[lastElement]->getID()))
					return true;
			}

			return false;
		}

		// deepCombine scannt den Stack nach kombinierbaren Teilloesungen, die durch empfangene
		// Teilloesungen tief im Stack vorliegen koennen. Sind solche Loesungen vorhanden, wird
		// pro Aufruf genau ein combine durchgefuehrt. Ein erfolgreiches Kombinieren wird durch
		// den Rueckgabewert true angezeigt.
		bool deepCombine() {
			bool combined = false;
			
			if(!isEmpty()) {
				int currentID;
				int end = lastElement - D + 1;
				
				for(int currentFrameIndex = 0; currentFrameIndex <= end; currentFrameIndex++) {
					currentID = stack[currentFrameIndex]->getID();
					// suche einen linken Sohn
					if(isLeftSon(currentID)) {
						// sind seine Brueder auch da?
						if(stack[currentFrameIndex + D - 1]->getID() == currentID + D - 1) {
							// kombiniere Elemente i bis i+(D-1)
							Solution** partialSolution = new Solution*[D];
							
							// alle Bruderknoten aus den Frames entpacken
							for(int j = 0; j < D; j++)
								partialSolution[j] = stack[currentFrameIndex + j]->getData();
							
							// fasse Teilloesungen zusammen, berechne
							// neue ID und baue daraus einen Frame
							// Muscle-Function call
							Solution* solution = comb(partialSolution);
							numS++;
							// KindID = VaterID * D + 1; VaterID = (KindID - 1) / D; WurzelID = 0
							long parentID = (stack[currentFrameIndex]->getID()-1)/D;
							long rootNodeID = stack[currentFrameIndex]->getRootNodeID();
							long originator = stack[currentFrameIndex]->getOriginator();
							long poolID = stack[currentFrameIndex]->getPoolID();
							Frame<Solution>* solutionFrame = new Frame<Solution>(parentID,
								rootNodeID, originator, poolID, solution);
							numSF++;
							// Speicher freigeben
							delete partialSolution;
							partialSolution = NULL;
							
							for(int j = 0; j < D; j++) {
								delete stack[currentFrameIndex+j]->getData();
								numS--;
								delete stack[currentFrameIndex+j];
								numSF--;
							}

							// umkopieren
							while(currentFrameIndex+D <= lastElement) {
								stack[currentFrameIndex] = stack[currentFrameIndex+D];
								currentFrameIndex++;
							}

							while(currentFrameIndex <= lastElement)
								stack[currentFrameIndex++] = NULL;

							lastElement -= D;
							numSF++;

							if(rootNodeID==parentID) {
								// Loesung in SendQueue schreiben
								writeElementToSendQueue(solutionFrame);
								solutionFrame = NULL;
							}
							// schreibe neu berechneten Frame in den Pool
							else {
								insert(solutionFrame);
							}

							combined = true;
						}
					}
				}
			}

			return combined;
		}

		void combine() {
			Solution** partialSolution = new Solution*[D];
			int currentFrameIndex = lastElement - D + 1;
			
			// alle Bruderknoten aus den Frames entpacken
			for(int i = 0; i < D; i++)
				partialSolution[i] = stack[currentFrameIndex++]->getData();

			// fasse Teilloesungen zusammen, berechne
			// neue ID und baue daraus einen Frame
			// Muscle-Function call
			Solution* solution = comb(partialSolution);
			numS++;
			delete partialSolution;
			partialSolution = NULL;
			// KindID = VaterID * D + 1; VaterID = (KindID - 1) / D; WurzelID = 0
			long parentID = (stack[lastElement - D + 1]->getID()-1)/D;
			long rootNodeID = stack[lastElement - D + 1]->getRootNodeID();
			long originator = stack[lastElement - D + 1]->getOriginator();
			long poolID = stack[lastElement - D + 1]->getPoolID();
			Frame<Solution>* solutionFrame = new Frame<Solution>(parentID, rootNodeID,
				originator, poolID, solution);
			numSF++;
			
			// entferne Teilloesungen aus dem Solutionpool, die soeben zusammengefasst wurden
			for(int i = 0; i < D; i++)
				// Speicher wird freigegeben
				pop();
			
			if(rootNodeID==parentID) {
				// Loesung in SendQueue schreiben
				writeElementToSendQueue(solutionFrame);
				solutionFrame = NULL;
			}
			// schreibe neu berechneten Frame in den Pool
			else {
				insert(solutionFrame);
			}

			// wiederhole dies rekursiv, bis nichts mehr zusammengefasst werden kann
			if(combineIsPossible())
				combine();
		}

		// KindID = VaterID * D + 1; VaterID = (KindID - 1) / D; WurzelID = 0
		inline bool hasSolution() {
			if(!isEmpty())
				return (stack[lastElement]->getID() == 0);
			else
				return false;
		}

		void show() {
			if(lastElement == -1)
				std::cout << Muesli::MSL_myId << ": Solutionpool: []" << std::endl;

			else {
				std::cout << Muesli::MSL_myId << ": Solutionpool: [";

				for(int i = 0; i < lastElement; i++)
					std::cout << stack[i]->getID() << ", ";

				std::cout << stack[lastElement]->getID() << "]" << std::endl;
			}
		}

	};

}

#endif
