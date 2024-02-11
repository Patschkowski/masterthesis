// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef FINAL_H
#define FINAL_H

#include "Curry.h"
#include "mpi.h"
#include "Muesli.h"
#include "Process.h"

namespace msl {

	// Annahme: Final ist niemals Teil einer Farm. Obwohl: warum eigentlich nicht? Damit kann
	// man zur Not auch einen Prozessor sparen...(durchdenken!). Die Kommunikation erfolgt
	// nichtblockierend und asynchron.
	template<class Solution> class Final: public Process {

		DFct1<Solution*, void> fct;

	public:

		Final(void (*f)(Solution*)): Process(), fct(Fct1<Solution*, void, void (*)(Solution*)>(f)) {
			numOfEntrances = 1;
			numOfExits = 1;
			entrances = new ProcessorNo[numOfEntrances];
			exits = new ProcessorNo[numOfExits];
			entrances[0] = Muesli::MSL_runningProcessorNo++;
			exits[0] = entrances[0];
			// Anzahl bereits eingegangener Stop-Tags
			receivedStops = 0;
		}

		Final(const DFct1<Solution*,void>& f): Process(), fct(f) {
			numOfEntrances = 1;
			numOfExits = 1;
			entrances = new ProcessorNo[numOfEntrances];
			exits = new ProcessorNo[numOfExits];
			entrances[0] = Muesli::MSL_runningProcessorNo++;
			exits[0] = entrances[0];
			// Anzahl bereits eingegangener Stop-Tags
			receivedStops = 0;
		}

		~Final() {
			delete[] entrances;
			delete[] exits;
			entrances = NULL;
			exits = NULL;
		}

		void start() {
			dbg({std::cout << Muesli::MSL_myId << " in Final::start" << std::endl << std::flush;})
			// alle unbeteiligten Prozessoren rausfiltern
			finished = !(Muesli::MSL_myId == entrances[0]);
			
			if(finished) {
				return;
			}

			bool debugCommunication = false;
			// Attribute: MPI_SOURCE, MPI_TAG, MPI_ERROR
			MPI_Status status;
			Solution* pSolution;
			int predecessorIndex = 0;
			receivedStops = 0;
			int flag;

			while(!finished) {
				// faire Annahme einer Nachricht von den Vorgaengern
				flag = 0;

				while(!flag) {
					MPI_Iprobe(predecessors[predecessorIndex], MPI_ANY_TAG, MPI_COMM_WORLD,
						&flag, &status);
					predecessorIndex = (predecessorIndex + 1) % numOfPredecessors;
				}

				ProcessorNo source = status.MPI_SOURCE;
				
				if(debugCommunication) {
					std::cout << Muesli::MSL_myId << ": Final empfaengt Nachricht von " <<
						source << std::endl;
				}

				// TAGS
				if(status.MPI_TAG == MSLT_STOP) {
					if(debugCommunication) {
						std::cout << Muesli::MSL_myId << ": Final hat STOP empfangen" << std::endl;
					}
					
					MSL_ReceiveTag(source, MSLT_STOP);
					receivedStops++;

					if(receivedStops == numOfPredecessors) {
						if(debugCommunication) {
							std::cout << Muesli::MSL_myId <<
								": Final hat #numOfPredecessors STOPS empfangen -> Terminiere" <<
								std::endl;
						}
						
						receivedStops = 0;
						finished = true;
					}
				}
				// user data
				else {
					if(debugCommunication) {
						std::cout << Muesli::MSL_myId << ": Final empfaengt Daten" << std::endl;
					}
					
					pSolution = new Solution();
					MSL_Receive(source, pSolution, MSLT_ANY_TAG, &status);
					fct(pSolution);
				}
			}
		}

		Process* copy() {
			return new Final(fct);
		}

		void show() const {
			if(Muesli::MSL_myId == 0) {
				std::cout << "Final (PID = " << entrances[0] << ")" << std::endl << std::flush;
			}
		}

	};

}

#endif
