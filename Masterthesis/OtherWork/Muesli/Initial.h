// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef INITIAL_H
#define INITIAL_H

#include "Curry.h"
#include "mpi.h"
#include "Muesli.h"
#include "Process.h"

namespace msl {

	// Objekte dieser Klasse benoetigen keinen Eingabestrom. Die weitergeleiteten Daten werden mit
	// Hilfe der Argumentfunktion erzeugt (z.B. Einlesen einer Datei). Fuer dieses atomare Skelett
	// ist nur ein Prozessor vorgesehen. Die Daten werden je nach gewaehltem Modus per Zufall oder
	// zyklisch an den oder die Nachfolger weitergeleitet. Die Datenuebertragung erfolgt blockierend,
	// asynchron und ggf. serialisiert.

	template<class O> class Initial: public Process {

		// speichert die im Konstruktor uebergebene Argumentfunktion
		DFct1<Empty, O*> fct;

	public:

		// wenn Initial das erste Skelett ist, liegt entrance auf Prozessor 0 (vgl. initSkeletons)
		Initial(O* (*f)(Empty)): Process(), fct(Fct1<Empty, O*, O* (*)(Empty)>(f)) {
			numOfEntrances = 1;
			numOfExits = 1;
			entrances = new ProcessorNo[numOfEntrances];
			exits = new ProcessorNo[numOfExits];
			entrances[0] = Muesli::MSL_runningProcessorNo++;
			exits[0] = entrances[0];
			// Empfaenger der ersten Nachricht festlegen
			setNextReceiver(0);
		}

		// wenn Initial das erste Skelett ist, liegt entrance auf Prozessor 0 (vgl. initSkeletons)
		Initial(const DFct1<Empty, O*>& f): Process(), fct(f) {
			numOfEntrances = 1;
			numOfExits = 1;
			entrances = new ProcessorNo[numOfEntrances];
			exits = new ProcessorNo[numOfExits];
			entrances[0] = Muesli::MSL_runningProcessorNo++;
			exits[0] = entrances[0];
			// Empfaenger der ersten Nachricht festlegen
			setNextReceiver(0);
		}

		~Initial() {
			delete[] entrances;
			delete[] exits;
			entrances = NULL;
			exits = NULL;
		}

		void start() {
			// alle unbeteiligten Prozessoren rausfiltern
			finished = !(Muesli::MSL_myId == entrances[0]);
			
			if(finished) {
				return;
			}

			O* pProblem = NULL;
			// void-parameter fuer Argumentfunktion
			Empty dummy;
			bool debug = false;

    		while(!finished) {
				// Argumentfunktion anwenden
				pProblem = fct(dummy);
				
				// wenn die Funktion NULL liefert, stop-tag senden
				if(pProblem == NULL) {
					for(int i = 0; i < numOfSuccessors; i++) {
						if(debug) {
							std::cout << Muesli::MSL_myId << ": Initial sendet STOP an " <<
								successors[i] << std::endl;
						}
						
						MSL_SendTag(successors[i], MSLT_STOP);
					}

					finished = true;
				}
				// ansonsten leite die Daten an einen bekannten Nachfolger weiter
				else {
					int receiver = getReceiver();
					
					if(debug) {
						std::cout << Muesli::MSL_myId << ": Initial sendet Problem an " <<
							receiver << std::endl;
					}
					
					MSL_Send(receiver, pProblem);
				}
			}

			if(debug) {
				std::cout << Muesli::MSL_myId << ": Initial terminiert. " << std::endl;
			}
		}

		Process* copy() {
			return new Initial(fct);
		}

		void show() const {
			if(Muesli::MSL_myId == 0) {
				std::cout << "Initial (PID = " << entrances[0] << ")" << std::endl << std::flush;
			}
		}

	};

}

#endif
