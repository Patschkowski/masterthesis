// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef PROCESS_H
#define PROCESS_H

#include "Muesli.h"


namespace msl {
    
	// abstrakte Oberklasse aller Taskparalleler Skelette
	class Process {

	private:

		// fuer die interne Berechnung des Empfaengers der naechsten
		// Nachricht; Für die zyklische Bestimmung des Nachfolgers
		int nextReceiver;

	protected:
typedef int ProcessorNo;
		// Prozess empfaengt von mehreren Prozessoren
		ProcessorNo* predecessors;
		// Prozess sendet an mehrere Prozessoren
		ProcessorNo* successors;
		// Skelett hat mehrere Entrances
		ProcessorNo* entrances;
		// Skelett hat mehrere Exits
		ProcessorNo* exits;
		// size of predecessors array
		int numOfPredecessors;
		// size of successors array
		int numOfSuccessors;
		int numOfEntrances;
		int numOfExits;

		// fuer Zeitmessungen
		double processBeginTime, processEndTime, processSendTime, processRecvTime;

		// counter fuer empfangene Tags
		// = 0
		int receivedStops;
		// = 0
		int receivedTT;

		// Dieses Flag wird benutzt um herauszufinden, ob ein Prozessor an einem bestimmten
		// Prozess beteiligt ist oder nicht. Jedes Skelett wird auf eine bestimmte Prozessor-
		// menge abgebildet. Anhand seiner eigenen Id kann jeder Prozessor feststellen, ob er
		// Teil eines bestimmten Skeletts ist (finished=false) oder eben nicht (finished=true).
		// Anhand des Zustands dieser Variable wird der Programmablauf gesteuert und parallelisiert.
		bool finished;

	public:

		// Konstruktor: numOfEntrances/numOfExits vordefiniert mit 1 (nur Farm hat i.d.R. mehrere)
		Process(): nextReceiver(-1), numOfEntrances(1), numOfExits(1), receivedStops(0), receivedTT(0) {
		};

		virtual ~Process() {
		};

		ProcessorNo* getSuccessors() const {
			return successors;
		}

		ProcessorNo* getPredecessors() const {
			return predecessors;
		}

		ProcessorNo* getEntrances() const {
			return entrances;
		}

		ProcessorNo* getExits() const {
			return exits;
		}

		// Methoden zum Verwalten der Tags und zur Prozesssteuerung
		int getReceivedStops() {
			return receivedStops;
		}

		int getReceivedTT() {
			return receivedTT;
		}

		void addReceivedStops() {
			receivedStops++;
		}

		void addReceivedTT() {
			receivedTT++;
		}

		void resetReceivedStops() {
			receivedStops = 0;
		}

		void resetReceivedTT() {
			receivedTT = 0;
		}

		int getNumOfPredecessors() {
			return numOfPredecessors;
		}

		int getNumOfSuccessors() {
			return numOfSuccessors;
		}

		int getNumOfEntrances() {
			return numOfEntrances;
		}

		int getNumOfExits() {
			return numOfExits;
		}

		// fuer Zeitmessungen
		void addProcessSendTime(double t) {
			processSendTime += t;
		}

		double getProcessSendTime() {
			return processSendTime;
		}

		void addProcessRecvTime(double t) {
			processRecvTime += t;
		}

		double getProcessRecvTime() {
			return processRecvTime;
		}

		// Soll der Empfaenger einer Nachricht per Zufall ausgewaehlt werden, kann mit Hilfe dieser
		// Methode der Seed des Zufallsgenerators neu gesetzt werden. Als Seed wird die Systemzeit
		// gewaehlt.
		void newSeed() {
			srand((unsigned)time(NULL));
		}

		// jeder Prozess kann einen zufaelligen Empfaenger aus seiner successors-Liste bestimmen
		// Den Seed kann jeder Prozess mit newSeed() auf Wunsch selbst neu setzten.
		inline ProcessorNo getRandomReceiver() {
			int i = rand() % numOfSuccessors;

			return successors[i];
		}

		// jeder Prozess kann den Nachrichtenempfaenger zyklisch aus seiner successors-Liste bestimmen.
		inline ProcessorNo getNextReceiver() {
			if(nextReceiver == -1) {
				printf("INITIALIZATION ERROR: first receiver in cyclic mode was not defined\n");
			}
			
			// Index in successors-array zyklisch weitersetzen
			nextReceiver = (nextReceiver + 1) % numOfSuccessors;

			return successors[nextReceiver];
		}

		// jeder Prozess kann den Nachrichtenempfaenger zyklisch aus seiner successors-Liste bestimmen.
		inline ProcessorNo getReceiver() {
			// RANDOM MODE
			if(Muesli::MSL_DISTRIBUTION_MODE == MSL_RANDOM_DISTRIBUTION) {
				return getRandomReceiver();
			}
			// CYCLIC MODE: Index in successors-array zyklisch weitersetzen
			else {
				return getNextReceiver();
			}
		}

		// jeder Prozessor kann den Empfaenger seiner ersten Nachricht frei waehlen. Dies ist in
		// Zusammenhang mit der zyklischen Empfaengerwahl sinnvoll, um eine Gleichverteilung der
		// Nachrichten und der Prozessorlast zu erreichen. Wichtig ist dies insbesondere bei einer
		// Pipe von Farms.
		void setNextReceiver(int index) {
			// receiver 0 existiert immer
			if(index == 0 || (index > 0 && index < numOfSuccessors)) {
				nextReceiver = index;
			}
			else {
				printf("error in Process %d: index out of bounds -> index = %d, numOfSuccessors = %d\n",
					Muesli::MSL_myId, index, numOfSuccessors);
				throw(UndefinedDestinationException());
			}
		}

		// zeigt an, ob der uebergebene Prozessor in der Menge der bekannten Quellen ist, von denen
		// Daten erwartet werden. Letztlich wird mit Hilfe dieser Methode und dem predecessors-array
		// eine Prozessgruppe bzw. Kommunikator simuliert. Kommunikation kann nur innerhalb einer
		// solchen Prozessgruppe stattfinden. Werden Nachrichten von einem Prozess ausserhalb dieser
		// Prozessgruppe empfangen fuehrt das zu einer undefinedSourceException. Damit sind die Skelette
		// deutlich weniger fehleranfaellig. Auf die Verwendung der MPI-Kommunikatoren wurde aus Gruenden
		// der Portabilitaet bewusst verzichtet.
		bool isKnownSource(ProcessorNo no) {
			for(int i = 0; i < numOfPredecessors; i++) {
				if(predecessors[i] == no) {
					return true;
				}
			}

			return false;
		}

		// >> !!! Der Compiler kann moeglicherweise den Zugriff auf folgende virtuelle Methoden
		// >> optimieren, wenn der Zugriff auf diese statisch aufzuloest werden kann. Ansonsten
		// >> wird der Zugriff ueber die vtbl (virtual table) einen geringen Performanceverlust
		// >> bedeuten ==> ggf. ueberdenken, ob das "virtual" wirklich notwendig ist... !!!

		// Teilt einem Prozess mit, von welchen anderen Prozessoren Daten empfangen werden koennen.
		// Dies sind u.U. mehrere, z.B. dann, wenn eine Farm vorgelagert ist. In diesem Fall darf
		// der Prozess von jedem worker Daten entgegennehmen.
		// @param p			Array mit Prozessornummern
		// @param length	arraysize
		virtual void setPredecessors(ProcessorNo* p, int length) {
			numOfPredecessors = length;
			predecessors = new ProcessorNo[length];
			
			for(int i = 0; i < length; i++) {
				predecessors[i] = p[i];
			}
		}

		// Teilt einem Prozess mit, an welche anderen Prozessoren Daten gesendet werden koennen.
		// Dies sind u.U. mehrere, z.B. dann, wenn eine Farm nachgelagert ist. In diesem Fall darf
		// der Prozess an einen beliebigen worker Daten senden.
		// @param p			Array mit Prozessornummern
		// @param length	arraysize
		virtual void setSuccessors(ProcessorNo* p, int length) {
			numOfSuccessors = length;
			successors = new ProcessorNo[length];
			
			for(int i = 0; i < length; i++) {
				successors[i] = p[i];
			}
		}

		virtual void start() = 0;

		virtual Process* copy() = 0;

		virtual void show() const = 0;

	};

/* Template-Funktion MSL_get: Liefert einen Zeiger auf ein empfangenes Datenpaket zurueck.

	   Diese Funktion sollte nur im Rumpf der Argumentfunktion des Filter-Prozesses aufgerufen
	   werden. Wird ein MSLT_STOP vom Chef empfangen, wird dieses korrekt an die Slaves
	   weitergeleitet. Ein empfangenes MSLT_STOP setzt den Zustand des Prozessors auf finished
	   und seine Arbeit wird mit dem zurcksenden von NULL beendet.
	   Get liefert der user defined function einen empfangenen Wert. Es duerfen daher keine(!)

	   nicht-blockierenden receive-operationen verwendet werden, da sichergestellt sein muss,
	   dass der user buffer die Daten auf jeden Fall enthaelt, wenn MSL_get() die Kontrolle an
	   die aufrufende user function zurueckgibt.
	*/
	// should only be used in the body of the argument function of Filter
	template<class Problem>
	Problem* MSL_get() {
		// Schutz gegen unbefugten Aufruf. Aufruf nur in Verbindung mit Filter erlaubt.
		if(Muesli::MSL_myProcess == NULL)
			throws(IllegalGetException());

		bool debugCommunication = false;
		Problem* pProblem;
		MPI_Status status;
		ProcessorNo source;
		int flag = 0;
		int predecessorIndex = 0;
		ProcessorNo* predecessors = Muesli::MSL_myProcess->getPredecessors();
		int numOfPredecessors = Muesli::MSL_myProcess->getNumOfPredecessors();
		bool finished = false;

		// Master process nimmt Daten an und leitet sie ggf. an Slaves weiter
		if(Muesli::MSL_myId == Muesli::MSL_myEntrance) {
			// faire Annahme einer Nachricht von den Vorgaengern
			flag = 0;
			
			if(debugCommunication)
				std::cout << Muesli::MSL_myId <<
				": Filter::MSL_get wartet auf Nachricht von " <<
				predecessors[predecessorIndex] << std::endl;
			
			while(!flag) {
				MPI_Iprobe(predecessors[predecessorIndex], MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
				predecessorIndex = (predecessorIndex+1)%numOfPredecessors;
			}

			source = status.MPI_SOURCE;

			if(debugCommunication)
				std::cout << Muesli::MSL_myId <<
				": Filter::MSL_get empfaengt Nachricht von " << source << std::endl;
			
			// STOP_TAG
			if(status.MPI_TAG == MSLT_STOP) {
				if(debugCommunication)
					std::cout << Muesli::MSL_myId <<
					": Filter::MSL_get hat STOP empfangen" << std::endl;
				
				MSL_ReceiveTag(source, MSLT_STOP);
				// STOPs sammeln
				Muesli::MSL_myProcess->addReceivedStops();
				
				if(Muesli::MSL_myProcess->getReceivedStops() == Muesli::MSL_myProcess->getNumOfPredecessors()) {
					if(debugCommunication)
						std::cout << Muesli::MSL_myId <<
						": Filter::MSL_get hat #numOfPredecessors STOPS empfangen -> Terminiere" <<
						std::endl;
					
					// genug gesammelt! alle eigenen Mitarbeiter alle nach Hause schicken
					for(int i = Muesli::MSL_myEntrance + 1; i < Muesli::MSL_myEntrance +
						Muesli::MSL_numOfLocalProcs; i++) {
						MSL_SendTag(i, MSLT_STOP);
					}

					// alle Nachfolger benachrichtigen
					ProcessorNo* successors = Muesli::MSL_myProcess->getSuccessors();
					int numOfSuccessors = Muesli::MSL_myProcess->getNumOfSuccessors();
					
					for(int i = 0; i < numOfSuccessors; i++) {
						MSL_SendTag(successors[i], MSLT_STOP);
					}

					Muesli::MSL_myProcess->resetReceivedStops();
					finished = true;
				}
				// noch nicht genug STOPS gesammelt. Gebe naechstes Datenpaket zurueck
				else {
					return MSL_get<Problem>();
				}
			}
			// user data
			else {
				if(debugCommunication)
					std::cout << Muesli::MSL_myId <<
					": Filter::MSL_get empfaengt ein Problem" << std::endl;
				
				// Masterprocess empfaengt ein neues Problem
				pProblem = new Problem();
				MSL_Receive(source, pProblem, MSLT_ANY_TAG, &status);
				
				// Problem an Mitarbeiter weiterleiten
				for(int i = Muesli::MSL_myEntrance + 1; i < Muesli::MSL_myEntrance +
					Muesli::MSL_numOfLocalProcs; i++) {
					if(debugCommunication)
						std::cout << Muesli::MSL_myId <<
						": Filter::MSL_get sendet Problem an datenparallelen Mitarbeiter " <<
						i << std::endl;
					
					MSL_Send(i, pProblem);
				}
			}
			// user data
		}
		// slaves
		else {
			// Datenparallel eingesetzte Prozessoren warten auf den
			// Parameter fuer die Argumentfunktion vom Masterprocess
			if(debugCommunication)
				std::cout << Muesli::MSL_myId <<
				": Atomic-Mitarbeiter wartet auf Daten von " << Muesli::MSL_myEntrance << std::endl;
			
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
		
		// --- Master and Slave ---
		if(finished)
			return NULL;
		else
			return pProblem;
	}

	// should only be used in the body of the argument function of Filter
	template<class Solution>
	inline void MSL_put(Solution* pSolution) {
		if(Muesli::MSL_myProcess == NULL)
			throws(IllegalPutException());

		bool debugCommunication = false;

		// nur der Masterprocess sendet Loesung an Nachfolger
		if(Muesli::MSL_myId == Muesli::MSL_myEntrance) {
			int receiver = Muesli::MSL_myProcess->getReceiver();
			
			if(debugCommunication) {
				std::cout << Muesli::MSL_myId <<
					": Final::MSL_put sendet Loesung an " << receiver << std::endl;
			}

			MSL_Send(receiver, pSolution);
		}
	}

}

#endif
