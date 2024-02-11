// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef PIPE_H
#define PIPE_H

#include "Curry.h"
#include "Muesli.h"
#include "Process.h"

namespace msl {

	/* TODO: Beschreibung Überarbeiten! Kommt noch aus Skeleton.h
	   Diese Klasse konstruiert eine Pipeline aus den uebergebenen
	   Prozessen. Ein Prozess kann ein beliebiges Konstrukt aus
	   verschachtelten taskparallelen Skeletten sein. Ein solches
	   Konstrukt hat mindestens einen Eingang (entrances[0]) und
	   Ausgang (exits[0]) zum Empfangen bzw. zum Senden von Daten.
	   Dies entspricht in gewissem Sinne einer Blackbox- Sichtweise.
	   Wie das Konstrukt intern aus anderen Prozessen zusammengesetzt
	   bzw. verknuepft ist, interessiert hier nicht weiter. Es wird
	   lediglich eine Schnittstelle zur Kommunikation mit einem
	   Prozess definiert. Die interne Verknpfung (hier als Pipeline)
	   wird ueber predecessors[] und successors[] erreicht. Der
	   Eingang des Pipe-Konstrukts ist der Eingang des ersten
	   Konstrukts in der Pipe (entrance = p1.getEntrance()) und der
	   Ausgang ist entsprechend der Ausgang des letzten Konstrukts
	   in der Pipe (exit = p3.getExit()). Um die Verkettung zu
	   erreichen muss das erste Konstrukt in der Pipe an den Eingang
	   des zweiten Konstrukts in der Pipe Daten schicken, das zweite
	   Konstrukt in der Pipe an den Eingang des dritten Konstruks usw.
	   (p1.setOut(p2.getEntrance()), p2.setOut(p3.getEntrance())...).
	   Nun muss den Konstrukten noch mitgeteilt werden, von welchem
	   Ausgang eines anderen Konstrukt es Daten empfangen kann. Also
	   Konstrukt 2 erwartet Daten von K1, K3 von K2 etc.
	   (p2.setIn(p1.getExit()),...).
    */

	class Pipe: public Process {

		// Zeiger auf Array von Adressen der Prozesse
		Process** p;
		int i;
		// Laenge der Pipe
		int length;

	public:

		// Konstruktor fuer 2 Prozesse
		// @param p1	Adresse von Prozess1
		// @param p2	Adresse von Prozess2
		Pipe(Process& p1, Process& p2): Process() {
			setNextReceiver(0);
			// Laenge der Pipe merken und Speicherplatz fuer die Pipe-Elemente reservieren
   			length = 2;
    		p = new Process* [length];

			// interne Verknpfung herstellen
			// p1 schickt an p2
			p1.setSuccessors(p2.getEntrances(), p2.getNumOfEntrances());
    		// p2 empfaengt von p1
			p2.setPredecessors(p1.getExits(), p1.getNumOfExits());

			// fuer jeden Worker der Empfaenger seiner ersten Nachricht festlegen
			if(Muesli::MSL_DISTRIBUTION_MODE == MSL_CYCLIC_DISTRIBUTION) {
				// bestimme die Anzahl der Ausgaenge von p1 und die Anzahl der Eingaenge von p2
				int p1Exits = p1.getNumOfExits();
				int p2Entrances = p2.getNumOfEntrances();
				
				// Skelette mit einem Eingang oder Ausgang sind per Default korrekt
				// verknuepft. derzeit muessen nur Farmen mit Farmen vernetzt werden
				if(p1Exits>1 && p2Entrances>1) {
					// weise nun Ausgang von p1 zyklisch einen der Eingaenge von p2 zu
					int recv;

					for(int skel = 0; skel < p1Exits; skel++) {
						recv = skel % p2Entrances;
						p1.setNextReceiver(recv);
					}
				}
			}

			// Eingang und Ausgang der Pipe merken
    		entrances = p1.getEntrances();
			numOfEntrances = p1.getNumOfEntrances();
    		exits = p2.getExits();
			numOfExits = p2.getNumOfExits();
			// Adressen der beteiligten Prozesse sichern
    		p[0] = &p1;
			p[1] = &p2;

		} // ende Kunstruktor

		// Konstruktor fr 3 Prozesse
		Pipe(Process& p1, Process& p2, Process& p3): Process() {
			setNextReceiver(0);
			// Laenge der Pipe merken und Speicherplatz fuer die Pipe-Elemente reservieren
			length = 3;
			p = new Process*[length];
			// interne Verknpfung herstellen
			// p1 schickt an p2
			p1.setSuccessors(p2.getEntrances(), p2.getNumOfEntrances());
    		// p2 empfaengt von p1
			p2.setPredecessors(p1.getExits(), p1.getNumOfExits());
    		// p2 schickt an p3
			p2.setSuccessors(p3.getEntrances(), p3.getNumOfEntrances());
    		// p3 empfaengt von p2
			p3.setPredecessors(p2.getExits(), p2.getNumOfExits());

			// erst jetzt kann fuer jeden Worker der Empfaenger seiner ersten Nachricht
			// festgelegt werden
			if(Muesli::MSL_DISTRIBUTION_MODE == MSL_CYCLIC_DISTRIBUTION) {
				// bestimme die Anzahl der Ausgaenge von p1 und die Anzahl der Eingaenge von p2
				int p1Exits = p1.getNumOfExits();
				int p2Entrances = p2.getNumOfEntrances();
				
				// Skelette mit einem Eingang oder Ausgang sind per Default korrekt verknuepft
				// Farms und Pipes koennen mehrere Ein- und Ausgaenge haben, die vernetzt werden
				// muessen
				if(p1Exits > 1 && p2Entrances > 1) {
					// weise nun Ausgang von p1 zyklisch einen der Eingaenge von p2 zu
					int recv;

					for(int skel = 0; skel < p1Exits; skel++) {
						recv = skel % p2Entrances;
						p1.setNextReceiver(recv);
					}
				}

				// (zur besseren Lesbarkeit wurden fuer Prozess 2 und 3 neue Variablen definiert)
				int p2Exits = p2.getNumOfExits();
				int p3Entrances = p3.getNumOfEntrances();

				if(p2Exits > 1 && p3Entrances > 1) {
					// weise nun Ausgang von p2 zyklisch einen der Eingaenge von p3 zu
					int recv;

					for(int skel = 0; skel < p1Exits; skel++) {
						recv = skel % p2Entrances;
						p1.setNextReceiver(recv);
					}
				}
			}

			// debug
			dbg({
				ProcessorNo* no;
				int num;

				std::cout << "Pipe::Pipe --> verknuepfe Prozesse" << std::endl;
				no = p2.getEntrances();
				num = p2.getNumOfEntrances();
				std::cout << "setze << " << num << "Empfaenger von p1 (init): ";

				for(i = 0; i < num; i++)
					std::cout << no[i] << "  ";

				std::cout << std::endl;
				no = p1.getExits();
				num = p1.getNumOfExits();
				std::cout << "setze << " << num << "Quellen von p2 (farm) (worker): ";

				for(i = 0; i < num; i++)
					std::cout << no[i] << "  ";

				std::cout << std::endl;
				no = p3.getEntrances();
				num = p3.getNumOfEntrances();
				std::cout << "setze << " << num << "Empfaenger von p2 (farm) (worker): ";

				for(i = 0; i < num; i++)
					std::cout << no[i] << "  ";

				std::cout << std::endl;
				no = p2.getExits();
				num = p2.getNumOfExits();
				std::cout << "setze << " << num << "Quellen von p3 (final): ";

				for(i = 0; i < num; i++)
					std::cout << no[i] << "  ";

				std::cout << std::endl;})

			// Eingang und Ausgang der Pipe merken
			entrances = p1.getEntrances();
			numOfEntrances = p1.getNumOfEntrances();
			exits = p3.getExits();
			numOfExits = p3.getNumOfExits();
			// Adressen der uebergebenen Prozesse sichern
    		p[0] = &p1;
			p[1] = &p2;
			p[2] = &p3;
		}
		// ende Konstruktor

		~Pipe() {
			delete[] p;
			p = NULL;
		}

		// setzt alle Nachfolger der Pipe
		inline void setSuccessors(ProcessorNo* drn, int len) {
			numOfSuccessors = len;
			successors = new ProcessorNo[len];

			for(int i = 0; i < len; i++) {
				successors[i] = drn[i];
			}

			(*(p[length-1])).setSuccessors(drn, len);
		}

		// setzt alle Vorgaenger der Pipe
		inline void setPredecessors(ProcessorNo* src, int len) {
			numOfPredecessors = len;
			predecessors = new ProcessorNo[len];

			for(i = 0; i < len; i++) {
				predecessors[i] = src[i];
			}

			(*(p[0])).setPredecessors(src, len);
		}

		// der Reihe nach werden alle Prozesse in der Pipe gestartet
		void start() {
			for(int i = 0; i < length; i++) {
				(*(p[i])).start();
			}
		}

		// erstellt eine Kopie der Pipe und liefert ihre Adresse
		Process* copy() {
  			return new Pipe(*((*(p[0])).copy()), *((*(p[1])).copy()));
		}

		// zeigt auf, welche Prozesse in der Pipe haengen
		void show() const {
			if(Muesli::MSL_myId == 0) {
				std::cout << std::endl;
				std::cout << "**********************************************************" << std::endl;
				std::cout << "*                   Process-Topology                     *" << std::endl;
				std::cout << "**********************************************************" << std::endl;
				std::cout << "Pipe (entrance at " << entrances[0] <<
					") with " << length << " stage(s): " << std::endl;
				
				for(int i = 0; i < length; i++) {
					std::cout << "  Stage " << (i + 1) << ": ";
  					(*(p[i])).show();
				}

				std::cout << "**********************************************************" << std::endl;
				std::cout << std::endl;
			}
		}

	};

}

#endif
