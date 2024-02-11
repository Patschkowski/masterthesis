// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef STREAM_DC_H
#define STREAM_DC_H

#include "Muesli.h"
#include "Process.h"

namespace msl {

	/* StreamDC ist ein hinsichtlich streambasierter Berechnungen optimiertes dezentrales
	   Divide and Conquer Skelett. Die Optimierung besteht im Wesentlichen darin, zeitgleich
	   mehrere DC-Probleme durch das Skelett bearbeiten zu lassen. Auf diese Weise ueberlappen
	   sich die Startup- und Endphasen einzelner Loesungsprozesse, was zu einer Reduktion der
	   Idle-Zeiten fuehren kann. Zudem kann die Generierung von Teilproblemen grobgranularer
	   erfolgen, d.h. der Rekursionsabbruch kann eher erfolgen (oder anders: der DC-Baum jedes
	   DC-Problems kann in seiner Hoehe beschnitten werden). Dies spart in vielen Anwendungen
	   Speicherplatz und Rechenzeit, da die Anzahl der divide()- und combine()-Aufrufe sowie
	   die Ausfuehrungszeit von solve() reduziert wird (es gilt: time(solve(N)) <= D *
	   time(solve(N / D)), mit D = Grad des DC-Baums). Des Weiteren kann die Anzahl der
	   zeitgleich zu loesenden Probleme an den insgesamt verfuegbaren verteilten Speicher
	   angepasst werden. Bei N Solvern und N zeitgleich zu loesenden Problemen wird sich das
	   Verhalten dieses Skeletts an eine Farm von sequentiell arbeitenden DCSolvern anpassen.
	   Vermutlich wird jedoch eine Farm von sequentiell arbeitenden DC-Solvern etwas schneller
	   sein (was zu zeigen waere).
	*/
	template<class Problem, class Solution>
	class StreamDC: public Process {

		// Solver; solver[0] ist Mastersolver (Eingang des Skeletts)
		DCStreamSolver<Problem, Solution>** p;
		// #Solver
		int length;
		int numOfMastersolvers;

	public:

		/* Ein dezentrales Divide and Conquer-Skelett. Dieser Konstruktor erzeugt (l-1) Kopien
		   des uebergebenen DCStreamSolvers. Insgesamt besteht dieses Skelett aus l Solvern.
		   
		   Externe Schnittstellen: Jeder Solver verfuegt ueber genau einen Eingang und einen
		   Ausgang. Das StreamDC-Skelett hat genau einen Eingang. Dieser wird auf einen der
		   Solver gemappt. Die Ausgaenge der Solver sind die Ausgaenge des BranchAndBoundOld-
		   Skeletts.
		   
		   Interne schnittstellen: Jeder Solver kennt die Ein- und Ausgaenge seiner Kollegen.
		   Über left und right ist zudem eine logische Ringstruktur definiert, die fuer das
		   versenden des Tokens im Rahmen des Termination Detection Algorithmus genutzt wird.
		*/
		StreamDC(DCStreamSolver<Problem, Solution>& solver, int l, int e):
		  length(l), numOfMastersolvers(e), Process() {
			if(e < 1) {
				std::cout << "#MasterSolvers < 1 in StreamDC! Setting #MasterSolvers = 1 " \
					"for this run" << std::endl;
				e = l;
			}

			if(e > l) {
				std::cout << "#MasterSolvers > #Processors in StreamDC! Setting #MasterSolvers = " \
					"#Processors for this run" << std::endl;
				e = l;
			}

			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
			p = new DCStreamSolver<Problem, Solution>* [length];
			p[0] = &solver;
	        
			for(int i = 1; i < length; i++)
        		p[i] = (DCStreamSolver<Problem,Solution>*)solver.copy();

			// bestimme die Mastersolver
			for(int i = 0; i < numOfMastersolvers; i++)
				(*(p[i])).setMastersolver();

			// externe Schnittstellen des StreamDC-Skeletts definieren:
			// Frage dazu die MasterSolver nach ihren Ein- und Ausgaengen.
			// Jeder Mastersolver hat genau einen Ein- und Ausgang.
			numOfEntrances = numOfMastersolvers;
			entrances = new ProcessorNo[numOfMastersolvers];
			ProcessorNo* entr;

			for(int i = 0; i < numOfMastersolvers; i++) {
        		// Mastersolver i hat nur einen Eingang
        		entr = (*(p[i])).getEntrances();
        		entrances[i] = entr[0];
			}

			numOfExits = numOfMastersolvers;
			exits = new ProcessorNo[numOfMastersolvers];
			ProcessorNo* ext;

			for(int i = 0; i < numOfMastersolvers; i++) {
        		// Mastersolver i hat nur einen Ausgang
        		ext = (*(p[i])).getExits();
        		exits[i] = ext[0];
			}

			// interne Schnittstellen definieren: jeder kennt jeden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p, length, i);
		}

		StreamDC(Problem** (*divide)(Problem*), Solution* (*combine)(Solution**),
			Solution* (*solve)(Problem*), bool (*isSimple)(Problem*), int d, int l, int e):
		length(l), numOfMastersolvers(e), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
			p = new DCStreamSolver<Problem,Solution>* [length];
	        
			for(int i = 0; i < length; i++)
        		p[i] = new DCStreamSolver<Problem, Solution>(divide, combine, solve, isSimple, d,
				numOfMastersolvers, 1);

			// bestimme die Mastersolver
			for(int i = 0; i < numOfMastersolvers; i++)
				(*(p[i])).setMastersolver();

			// externe Schnittstellen des StreamDC-Skeletts definieren:
			// Frage dazu die MasterSolver nach ihren Ein- und Ausgaengen.
			// Jeder Mastersolver hat genau einen Ein- und Ausgang.
			numOfEntrances = numOfMastersolvers;
			entrances = new ProcessorNo[numOfMastersolvers];
			ProcessorNo* entr;
	        
			for(int i = 0; i < numOfMastersolvers; i++) {
        		// Mastersolver i hat nur einen Eingang
        		entr = (*(p[i])).getEntrances();
        		entrances[i] = entr[0];
			}

			numOfExits = numOfMastersolvers;
			exits = new ProcessorNo[numOfMastersolvers];
			ProcessorNo* ext;
	       
			for(int i = 0; i < numOfMastersolvers; i++) {
        		// Mastersolver i hat nur einen Ausgang
        		ext = (*(p[i])).getExits();
        		exits[i] = ext[0];
			}

			// interne Schnittstellen definieren: jeder kennt jeden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p,length,i);
		}

		~StreamDC() {
			delete[] p;
			delete[] entrances;
			delete[] exits;
			p = NULL;
			entrances = NULL;
			exits = NULL;
		}

		// von diesen L vorgelagerten Skeletten werden Daten empfangen
		inline void setPredecessors(ProcessorNo* src, int L) {
			numOfPredecessors = L;

			// allen Mastersolvern ihre vorgelagerten Skelette mitteilen
			for(int i = 0; i < numOfMastersolvers; i++)
				(*(p[i])).setPredecessors(src,L);
		}

		// an diese L nachgelagerten Skelette werden Daten gesendet.
		inline void setSuccessors(ProcessorNo* drn, int L) {
			numOfSuccessors = L;

			// allen Mastersolvern ihre nachgelagerten Skelette mitteilen
			for(int i = 0; i < numOfMastersolvers; i++)
				(*(p[i])).setSuccessors(drn,L);
		}

		// startet alle Solver
		inline void start() {
			for(int i = 0; i < length; i++)
				(*(p[i])).start();
		}

		Process* copy() {
			return new StreamDC<Problem, Solution>(*(DCStreamSolver<Problem,
				Solution>*)(*(p[0])).copy(), length, numOfMastersolvers);
		}

		inline void show() const {
			if(Muesli::MSL_myId == 0) {
				std::cout << "StreamDC (entrances at PID ";
	            
				for(int i = 0; i < numOfEntrances - 1; i++)
            		std::cout << entrances[i] << ",";
	            
				std::cout << entrances[numOfEntrances-1] << ") with " <<
					length << " Solver(s) " << std::endl << std::flush;
	            
				for(int i = 0; i < length; i++)
					(*(p[i])).show();
			}
		}

	};

}

#endif
