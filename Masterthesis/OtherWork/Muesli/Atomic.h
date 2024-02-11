// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef ATOMIC_H
#define ATOMIC_H

#include "mpi.h"

#include "Curry.h"
#include "Muesli.h"
#include "Process.h"

namespace msl {

	/* nicht blockierendes, asynchrones Receive; blockierendes, asynchrones Send
	 * 21.2.06: Verhalten bei STOP
	 * Sammeln von 1 STOP von jedem Vorgaenger, dann Versenden von STOP an jeden
	 * Nachfolger.
	 */
	template<class Problem, class Solution>
	class Atomic: public Process {

		// speichert die uebergebene Argumentfunktion f; Klasse DFct1 aus curry.h
		DFct1<Problem*, Solution*> fct;
		// anzahl gewuenschter Prozessoren fuer diesen Prozess
		int noprocs; // assumption: uses contiguous block of processors

	public:

		/* Konstruktor. Fct1 in Initialisierungsliste aus curry.h. Die Argumentfunktion
		   f beschreibt, wie die Inputdaten transformiert werden sollen
		 */ 
		Atomic(Solution* (*f)(Problem*), int n):
		Process(), fct(Fct1<Problem*, Solution*, Solution* (*)(Problem*)>(f)), noprocs(n) {
			numOfEntrances = 1;
			numOfExits = 1;
			entrances = new ProcessorNo[numOfEntrances];
			exits = new ProcessorNo[numOfExits];
			entrances[0] = Muesli::MSL_runningProcessorNo;
			exits[0] = entrances[0];
			Muesli::MSL_runningProcessorNo += n;
			// Default-Receiver ist 0. Den gibt's immer
			setNextReceiver(0);
			// Anzahl bereits eingegangener Stop-Tags
			receivedStops = 0;
		}

		/* Konstruktor. Fct1 in Initialisierungsliste aus curry.h. Die Argumentfunktion
		   f beschreibt, wie die Inputdaten transformiert werden sollen
		 */ 
		//Atomic(const DFct1<Problem*, Solution*>& f, int n):
		Atomic(DFct1<Problem*, Solution*> f, int n):
		 Process(), fct(f), noprocs(n) {
			numOfEntrances = 1;
			numOfExits = 1;
			entrances = new ProcessorNo[numOfEntrances];
			exits = new ProcessorNo[numOfExits];
			entrances[0] = Muesli::MSL_runningProcessorNo;
			exits[0] = entrances[0];
			Muesli::MSL_runningProcessorNo += n;
			// Default-Receiver ist 0. Den gibt's immer
			setNextReceiver(0);
			// Anzahl bereits eingegangener Stop-Tags
			receivedStops = 0;
		}

		~Atomic() {
			delete[] entrances;
			delete[] exits;
			entrances = NULL;
			exits = NULL;
		}

		void start() {
			// unbeteiligte Prozessoren steigen hier aus
			finished = ((Muesli::MSL_myId < entrances[0]) ||
				(Muesli::MSL_myId >= entrances[0] + noprocs));

			if(finished) {
				return;
			}

			bool debugCommunication = false;

			if(debugCommunication) {
				std::cout << Muesli::MSL_myId << ": starting Atomic" << std::endl;
			}

			// Attribute: MPI_SOURCE, MPI_TAG, MPI_ERROR
			MPI_Status status;
			Problem* pProblem;
			Solution* pSolution;
			ProcessorNo source;

			Muesli::MSL_myEntrance = entrances[0];
			Muesli::MSL_myExit = exits[0];
			Muesli::MSL_numOfLocalProcs = noprocs;
			int flag = 0;
			int predecessorIndex = 0;
			receivedStops = 0;

			// solange Arbeit noch nicht beendet ist
			while(!finished) {
				// Masterprocess
				if(Muesli::MSL_myId == Muesli::MSL_myEntrance) {
					// faire Annahme einer Nachricht von den Vorgaengern
					flag = 0;
					
					if(debugCommunication) {
						std::cout << Muesli::MSL_myId << ": Atomic wartet auf Nachricht von " <<
							predecessors[predecessorIndex] << std::endl;
					}
					
					while(!flag) {
						MPI_Iprobe(predecessors[predecessorIndex], MPI_ANY_TAG, MPI_COMM_WORLD,
							&flag, &status);
						predecessorIndex = (predecessorIndex +1 ) % numOfPredecessors;
					}

					source = status.MPI_SOURCE;
					
					if(debugCommunication) {
						std::cout << Muesli::MSL_myId << ": Atomic empfaengt Nachricht von " <<
							source << std::endl;
					}
					
					// STOP_TAG
					if(status.MPI_TAG == MSLT_STOP) {
						if(debugCommunication) {
							std::cout << Muesli::MSL_myId << ": Atomic hat STOP empfangen" << std::endl;
						}
						
						MSL_ReceiveTag(source, MSLT_STOP);
						receivedStops++;

						if(receivedStops == numOfPredecessors) {
							if(debugCommunication) {
								std::cout << Muesli::MSL_myId <<
									": Atomic hat #numOfPredecessors STOPS empfangen -> Terminiere" <<
									std::endl;
							}
							
							// genug gesammelt! alle eigenen Mitarbeiter alle nach Hause schicken
							for(int i = Muesli::MSL_myEntrance + 1; i < Muesli::MSL_myEntrance + noprocs; i++) {
								MSL_SendTag(i, MSLT_STOP);
							}

							// alle Nachfolger benachrichtigen
							for(int i = 0; i < numOfSuccessors; i++) {
								MSL_SendTag(successors[i], MSLT_STOP);
							}

							receivedStops = 0;
							finished = true;
						}
					}
					// user data
					else {
						if(debugCommunication) {
							std::cout << Muesli::MSL_myId <<
								": Atomic empfaengt ein Problem" << std::endl;
						}
						
						// Masterprocess empfaengt ein neues Problem
						pProblem = new Problem();
						MSL_Receive(source, pProblem, MSLT_ANY_TAG, &status);

						// Problem an Mitarbeiter weiterleiten
						for(int i = Muesli::MSL_myEntrance + 1; i < Muesli::MSL_myEntrance + noprocs; i++) {
							if(debugCommunication) {
								std::cout << Muesli::MSL_myId <<
									": Atomic sendet Problem an datenparallelen Mitarbeiter " <<
									i << std::endl;
							}
							
							MSL_Send(i, pProblem);
						}
					} // user data
				}
				// Datenparallel eingesetzte Prozessoren warten auf den
				// Parameter fuer die Argumentfunktion vom Masterprocess
				else {
					if(debugCommunication) {
						std::cout << Muesli::MSL_myId <<
							": Atomic-Mitarbeiter wartet auf Daten von " <<
							Muesli::MSL_myEntrance << std::endl;
					}
					
					flag = 0;

					// warte auf Nachricht
					while(!flag) {
						MPI_Iprobe(Muesli::MSL_myEntrance, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
					}

					if(status.MPI_TAG == MSLT_STOP) {
						MSL_ReceiveTag(Muesli::MSL_myEntrance, MSLT_STOP);
						finished = true;
					}
					else {
						pProblem = new Problem();
						MSL_Receive(Muesli::MSL_myEntrance, pProblem, MSLT_ANY_TAG, &status);
					}
				}
				// alle datenparallel eingesetzten Prozessoren haben
				// nun den Parameter fuer die Argumentfunktion

				// das Ergebnis berechnen alle Prozessoren gemeinsam
				if(!finished) {
					// pSolution wird ggf. datenparallel berechnet
					pSolution = fct(pProblem);
					//delete pProblem; // p4_error: interrupt SIGSEGV: 11 ???
					//pProblem = NULL; // p4_error: interrupt SIGSEGV: 11 ???
					
					if(debugCommunication) {
						std::cout << Muesli::MSL_myId << ": intermediate result " <<
							*pSolution << std::endl << std::flush;
					}
					
					// nur der Masterprocess sendet Loesung an Nachfolger
					if(Muesli::MSL_myId == Muesli::MSL_myEntrance) {
						int receiver = getReceiver();
						
						if(debugCommunication) {
							std::cout << Muesli::MSL_myId << ": Atomic sendet Loesung an " <<
								receiver << std::endl;
						}
						
						MSL_Send(receiver, pSolution);
					}

					// DO NOT delete pSolution, if MSL_Send is based on a non blocking
					// MPI-send-operation which returns before the pSolution is written
					// to the MPI send buffer!
					//delete pSolution;
					//pSolution = NULL;
				}
			}
			// ende while

			if(debugCommunication) {
				std::cout << Muesli::MSL_myId << ": Atomic finished " << std::endl << std::flush;
			}
		}
		// ende start

		Process* copy() {
			return new Atomic(fct, noprocs);
		}

		void show() const {
			if(Muesli::MSL_myId == 0) {
				std::cout << "Atomic (PID = " << entrances[0] << ")" << std::endl << std::flush;
			}
		}

	};

}

#endif
