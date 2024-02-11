// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef BRANCH_AND_BOUND_H
#define BRANCH_AND_BOUND_H

#include "BBSolver.h"
#include "Muesli.h"
#include "Process.h"

namespace msl {

	/* Zentrale Ausgangsklasse des Branch-and-Bound Skeletts. Der Nutzer bindet
	 * diese Klasse in seine Anwendung ein, um das BnB Skelett zu nutzen.
	 */
	template<class Problem>
	class BranchAndBound: public Process {

		// Solver; solver[0] ist Mastersolver (Eingang des Skeletts)
		BBSolver<Problem>** p;
		// #Solver
		int length;

	public:

		/* Ein dezentrales Branch&Bound-Skelett. Dieser Konstruktor erzeugt
		 * (l-1) Kopien des uebergebenen BBSolvers. Insgesamt besteht dieses
		 * Skelett aus l Solvern.
		 *
		 * Externe Schnittstellen: Jeder Solver verfuegt ueber genau einen
		 * Eingang und einen Ausgang. Das BranchAndBound-Skelett hat genau
		 * einen Eingang. Dieser wird auf einen der Solver gemappt. Die
		 * Ausgaenge der Solver sind die Ausgaenge des BranchAndBound-Skeletts.
		 *
		 * Interne schnittstellen: Jeder Solver kennt die Ein- und Ausgaenge
		 * seiner Kollegen. Über pred und succ ist zudem eine logische
		 * Ringstruktur definiert, die fuer das versenden des Tokens im Rahmen
		 * des Termination Detection Algorithmus genutzt wird.
		 */
		BranchAndBound(BBSolver<Problem>& solver, int l, int topology = MSL_BB_TOPOLOGY_ALLTOALL):
		length(l), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
			p = new BBSolver<Problem>*[length];
			p[0] = &solver;
			
			for(int i = 1; i < length; i++)
				p[i] = (BBSolver<Problem>*)solver.copy();

			// externe Schnittstellen definieren: Es gibt genau einen Mastersolver, der als Eingang
			// fungiert. Der (einzige) Eingang des Mastersolvers ist (einziger) Eingang des Skeletts,
			// alle Solverausgaenge sind Ausgaenge des Skeletts.
			numOfEntrances = 1;
			entrances = new ProcessorNo[numOfEntrances];
			// Mastersolver
			ProcessorNo* entr = (*(p[0])).getEntrances();
			entrances[0] = entr[0];

			numOfExits = 1;
			exits = new ProcessorNo[numOfExits];
			ProcessorNo* ext = (*(p[0])).getExits();
			exits[0] = ext[0];

			// interne Schnittstellen definieren: jeder kennt jeden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p, length, i, topology);
		}

		BranchAndBound(Problem** (*branch)(Problem*, int*), void (*bound)(Problem*),
		bool (*betterThan)(Problem*, Problem*), bool (*isSolution)(Problem*),
		int (*getLowerBound)(Problem*), int numMaxSub, int numSolver,
		int topology = MSL_BB_TOPOLOGY_ALLTOALL): length(numSolver), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
			p = new BBSolver<Problem>* [length];
			
			for(int i = 0; i < length; i++)
				p[i] = new BBSolver<Problem>(branch, bound, betterThan, isSolution, getLowerBound,
				numMaxSub, 1);

			// externe Schnittstellen definieren: Es gibt genau einen Mastersolver, der als Eingang
			// fungiert. Der (einzige) Eingang des Mastersolvers ist (einziger) Eingang des Skeletts,
			// alle Solverausgaenge sind Ausgaenge des Skeletts.
			numOfEntrances = 1;
			entrances = new ProcessorNo[numOfEntrances];
			// Mastersolver
			ProcessorNo* entr = (*(p[0])).getEntrances();
			entrances[0] = entr[0];

			numOfExits = 1;
			exits = new ProcessorNo[numOfExits];
			ProcessorNo* ext = (*(p[0])).getExits();
			exits[0] = ext[0];

			// interne Schnittstellen definieren: jeder kennt jeden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p, length, i, topology);
		}

		~BranchAndBound() {
			// Zeiger Array loeschen
			delete[] p;
			delete[] entrances;
			delete[] exits;
		}

		/* von diesen l vorgelagerten Skeletten werden Daten empfangen
		   nur der Mastersolver kommuniziert mit den Sendern
		 */
		inline void setPredecessors(ProcessorNo* src, int l) {
			numOfPredecessors = l;
			(*(p[0])).setPredecessors(src, l);
		}

		/* an diese n nachgelagerten Skelette werden Daten gesendet.
		   alle Solver koennen Daten verschicken
		 */		
		inline void setSuccessors(ProcessorNo* succs, int n) {
			numOfSuccessors = n;
			
			for(int i = 0; i < length; i++)
				(*(p[i])).setSuccessors(succs, n);
		}

		/* startet alle Solver
		 */
		inline void start() {
			for(int i = 0; i < length; i++)
				(*(p[i])).start();
		}

		Process* copy() {
			return new BranchAndBound<Problem>(*(BBSolver<Problem>*)(*(p[0])).copy(), length);
		}

		inline void show() const {
			if(Muesli::MSL_myId == 0) {
				std::cout << Muesli::MSL_myId << " BranchAndBound " <<
					entrances[0] << std::endl << std::flush;

				for(int i = 0; i < length; i++)
					(*(p[i])).show();
			}
		}

	};

}

#endif
