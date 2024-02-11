// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef DISTRIBUTED_DC_H
#define DISTRIBUTED_DC_H

#include "DCSolver.h"
#include "Process.h"

namespace msl {

	template<class Problem, class Solution>
	class DistributedDC: public Process {

		// Solver; solver[0] ist Mastersolver (Eingang des Skeletts)
		DCSolver<Problem,Solution>** p;
		// #Solver
		int length;

	public:

		/* Ein dezentrales Divide and Conquer-Skelett. Dieser Konstruktor erzeugt (l-1)
		   Kopien des uebergebenen DCSolvers. Insgesamt besteht dieses Skelett aus l Solvern.
		   
		   Externe Schnittstellen: Jeder Solver verfuegt ueber genau einen Eingang und einen
		   Ausgang. Das DistributedDC-Skelett hat genau einen Eingang. Dieser wird auf einen
		   der Solver gemappt. Die Ausgaenge der Solver sind die Ausgaenge des BranchAndBoundOld-
		   Skeletts.
		   
		   Interne schnittstellen: Jeder Solver kennt die Ein- und Ausgaenge seiner Kollegen.
		   Über left und right ist zudem eine logische Ringstruktur definiert, die fuer das
		   versenden des Tokens im Rahmen des Termination Detection Algorithmus genutzt wird.
		*/
		DistributedDC(DCSolver<Problem, Solution>& solver, int l): length(l), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
			p = new DCSolver<Problem,Solution>*[length];
			p[0] = &solver;

			for(int i = 1; i < length; i++)
        		p[i] = (DCSolver<Problem,Solution>*)solver.copy();

			// externe Schnittstellen definieren: Es gibt genau
			// einen Mastersolver, der als Ein- und Ausgang fungiert.
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
				(*(p[i])).setWorkmates(p, length, i);
		}


		DistributedDC(Problem** (*divide)(Problem*), Solution* (*combine)(Solution**),
			Solution* (*solve)(Problem*), bool (*isSimple)(Problem*), int d, int l): length(l), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
			p = new DCSolver<Problem, Solution>* [length];

			for(int i = 0; i < length; i++)
        		p[i] = new DCSolver<Problem, Solution>(divide, combine, solve, isSimple, d, 1);

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
				(*(p[i])).setWorkmates(p,length,i);
		}


		~DistributedDC() {
			delete[] p;
			delete[] entrances;
			delete[] exits;
			p = NULL;
			entrances = NULL;
			exits = NULL;
		}


		/* von diesen l vorgelagerten Skeletten werden Daten empfangen
		   nur der Mastersolver kommuniziert mit den Sendern
		*/
		inline void setPredecessors(ProcessorNo* src, int l) {
			numOfPredecessors = l;
			(*(p[0])).setPredecessors(src,l);
		}

		/* an diese l nachgelagerten Skelette werden Daten gesendet.
		   alle Solver koennen Daten verschicken
		*/
		inline void setSuccessors(ProcessorNo* drn, int l) {
			numOfSuccessors = l;

			for(int i = 0; i < length; i++)
				(*(p[i])).setSuccessors(drn,l);
		}

		/* startet alle Solver
		*/
		inline void start() {
			for(int i = 0; i < length; i++)
				(*(p[i])).start();
		}

		Process* copy() {
			return new DistributedDC<Problem, Solution>(*(DCSolver<Problem,
				Solution>*)(*(p[0])).copy(), length);
		}

		inline void show() const {
			if(Muesli::MSL_myId == 0) {
				std::cout << "DistributedDC (entrance at " << entrances[0] << ") with " <<
					length << " Solver(s) " << std::endl << std::flush;
	            
				for(int i = 0; i < length; i++)
					(*(p[i])).show();
			}
		}

	};

}

#endif
