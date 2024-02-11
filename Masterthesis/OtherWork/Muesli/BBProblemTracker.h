// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef BB_PROBLEM_TRACKER_H
#define BB_PROBLEM_TRACKER_H

#include "BBFrame.h"

namespace msl {

	/* Die Klasse BBProblemTracker verwaltet einen Baum von BBFrame-Objekten. Sie wird
	 * im Rahmen der Termination Detection des Branch-and-Bound Skelettes genutzt. Dabei
	 * wird ausgenutzt, dass die BBFrames intern Verweise auf das Vater-frame speichern.
	 * So koennen einem Frame zugehoerige Probleme als im Vaterframe geloest markiert
	 * werden. Sind alle Kinder geloest, so ist das Vaterproblem geloest und dessen Vater
	 * wird benachrichtigt.
	 */
	template<class Problem>
	class BBProblemTracker {

	private:

		class ListElement {

			friend class BBProblemTracker;
			ListElement *predecessor, *successor;
			BBFrame<Problem>* problemFrame;

			ListElement(): predecessor(NULL), successor(NULL), problemFrame(NULL) {
			}

			ListElement(BBFrame<Problem>* prob): predecessor(NULL), successor(NULL), problemFrame(prob) {
			}

		};

		ListElement *headSolved, *tailSolved;
		int sizeTracker;
		int sizeSolved;
		int numOfMaxSubProblems;

		int statMaxSize;
		int statMaxSizeSolved;
		int statCumulatedSize;
		int statCumulatedSizeSolved;
		int statNumOfInserts;
		int statNumOfInsertsSolved;
		bool debug;

	public:

		/* Erzeugt ein neues Tracker Objekt.
		*/
		BBProblemTracker(int subproblems):
		numOfMaxSubProblems(subproblems), sizeTracker(0), sizeSolved(0) {
			headSolved = new ListElement();
			tailSolved = new ListElement();

			headSolved->successor = tailSolved;
			tailSolved->predecessor = headSolved;

			statCumulatedSize = 0;
			statCumulatedSizeSolved = 0;
			statMaxSize = 0;
			statMaxSizeSolved = 0;
			statNumOfInserts = 0;
			statNumOfInsertsSolved = 0;
			debug = false;
		}

		~BBProblemTracker() {
			delete headSolved;
			delete tailSolved;
		}

		/* Prueft, ob der Tracker leer ist.
		*/
		bool isTrackerEmpty() {
			return sizeTracker == 0;
		}

		/* Fuegt ein neues Problem in den Tracker ein.
		*/
		void addProblem(BBFrame<Problem>* prob) {
			// Die Verweise zum Vater bestehen bereits, es ist lediglich
			// sicherzustellen, dass kein Verweis auf die Nutzdaten existiert.
			// Diese werden durch das Skelett geloescht.
			prob->setData(NULL);

			sizeTracker++;
			statMaxSize = (sizeTracker>statMaxSize) ? sizeTracker : statMaxSize;
			statNumOfInserts++;
			statCumulatedSize += sizeTracker;
		}

		/* Markiert ein Problem im Tracker als geloest.
		*/
		void problemSolved(BBFrame<Problem>* prob) {
			if(prob->getID() == 0)
				// Urproblem geloest; nichts zu erledigen
				return;

			bool solved = true;
			BBFrame<Problem>* parent;
			BBFrame<Problem>* old;
			int numSolved;

			// Traversiert den Baum von der untersten bis ggf. zur obersten Ebene
			// und aktualisiert die Status aller betroffen Vaterprobleme
			while(solved) {
				if(prob->getID() == 0)
					solved = false;
				// vater benachrichtigen
				else if(prob->getOriginator() == Muesli::MSL_myId) {
					if(debug)
						std::cout << "Tracker: Problem " << prob->getID() << " geloest" << std::endl;
					
					parent = prob->getParentProblem();
					numSolved = parent->getNumOfSolvedSubProblems();
					parent->setNumOfSolvedSubProblems(++numSolved);
					old = prob;

					// Vater ist geloest
					if(parent->getNumOfSolvedSubProblems() == parent->getNumOfSubProblems()) {
						if(debug)
							std::cout << "Tracker: Alle Kinder des VaterProblems " <<
							parent->getID() << " geloest" << std::endl;
						
						prob = parent;
						solved = true;
						sizeTracker--;
					}
					else
						solved = false;
				}
				else {
					writeToSolvedQueue(prob);
					solved = false;
				}
			}
		}

		/* Prueft, ob die termporaere Liste leer ist.
		*/
		bool isSolvedQueueEmpty() {
			return sizeSolved == 0;
		}

		/* Schreibt ein Problem in die Liste der geloesten, zu versendenden Probleme.
		*/
		void writeToSolvedQueue(BBFrame<Problem>* prob) {
			ListElement *newProblem;
			newProblem = new ListElement(prob);

			// hinten anhaengen!
			tailSolved->predecessor->successor = newProblem;
			newProblem->predecessor = tailSolved->predecessor;
			tailSolved->predecessor = newProblem;
			newProblem->successor = tailSolved;
			sizeSolved++;

			statMaxSizeSolved = (sizeSolved>statMaxSizeSolved) ? sizeSolved : statMaxSizeSolved;
			statCumulatedSizeSolved += sizeSolved;
			statNumOfInsertsSolved++;
		}

		/* Liest ein Problem aus der Liste der geloesten, zu versendenden Probleme aus.
		*/
		BBFrame<Problem>* readFromSolvedQueue() {
			if(isSolvedQueueEmpty())
				throws(EmptyQueueException());
			else
				return headSolved->successor->problemFrame;
		}

		/* Loescht ein Problem aus der Liste der geloesten, zu versendenden Probleme
		*/
		void removeFromSolvedQueue() {
			if(isSolvedQueueEmpty())
				throws(EmptyQueueException());
			else {
				ListElement* old = headSolved->successor;
				headSolved->successor->successor->predecessor = headSolved;
				headSolved->successor = headSolved->successor->successor;
				sizeSolved--;
				delete old->problemFrame;
				delete old;
			}
		}

		// Methoden zum Auslesen von Statistiken

		int getSolvedQueueLength() {
			return sizeSolved;
		}

		int getProblemTrackerLength() {
			return sizeTracker;
		}

		int getProblemTrackerMaxLength() {
			return statMaxSize;
		}

		int getSolvedQueueMaxLength() {
			return statMaxSizeSolved;
		}

		int getProblemTrackerAverageLength() {
			if(statNumOfInserts > 0)
				return (statCumulatedSize / statNumOfInserts);

			return 0;
		}

		int getSolvedQueueAverageLength() {
			if(statNumOfInsertsSolved > 0)
				return (statCumulatedSizeSolved / statNumOfInsertsSolved);

			return 0;
		}

	};

}

#endif
