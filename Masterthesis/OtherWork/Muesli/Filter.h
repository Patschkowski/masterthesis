// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef FILTER_H
#define FILTER_H

#include "Curry.h"
#include "Muesli.h"
#include "Process.h"

namespace msl {

	// ****************************  Filter   ******************************

	// class I: Input type, class O: Output type
	template<class I, class O> class Filter: public Process {

		// speichert die uebergebene Argumentfunktion f; Klasse DFct1 aus curry.h
		DFct1<Empty,void> fct;
		// Anzahl der in diesem Skelett beteiligten Prozessoren
		// assumption: uses contiguous block of processors
		int noprocs;

	public:

		// Konstruktor. Fct1 in Initialisierungsliste aus curry.h. Die Argumentfunktion f
		// beschreibt, wie die Inputdaten transformiert werden sollen und stuetzt sich auf
		// die Methoden MSL_get und MSL_put (s.u.). Die Funktion kann ueber MSL_get bel.
		// viele Daten entgegennehmen und diese in bel. viele Outputdaten transformieren.
		Filter(void (* f)(Empty), int n): fct(Fct1<Empty, void, void (*)(Empty) >(f)),
			Process(), noprocs(n) {
			numOfEntrances = 1;
			numOfExits = 1;
			entrances = new ProcessorNo[numOfEntrances];
			exits = new ProcessorNo[numOfExits];
			entrances[0] = Muesli::MSL_runningProcessorNo;
			exits[0] = entrances[0];
			Muesli::MSL_runningProcessorNo += n;
			// Default-Receiver ist 0. Den gibt's immer
			setNextReceiver(0);
			receivedStops = 0;
			receivedTT = 0;
			processSendTime = 0;
			processRecvTime = 0;
		}

		// Konstruktor. Fct1 in Initialisierungsliste aus curry.h. Die Argumentfunktion f
		// beschreibt, wie die Inputdaten transformiert werden sollen und sttzt sich auf die
		// Methoden MSL_get und MSL_put (s.u.). Die Funktion kann ber MSL_get bel.
		// viele Daten entgegennehmen und diese in bel. viele Outputdaten transformieren.
		Filter(const DFct1<Empty,void>& f, int n):  Process(), fct(f), noprocs(n) {
			numOfEntrances = 1;
			numOfExits = 1;
			entrances = new ProcessorNo[numOfEntrances];
			exits = new ProcessorNo[numOfExits];
			entrances[0] = Muesli::MSL_runningProcessorNo;
			exits[0] = entrances[0];
			Muesli::MSL_runningProcessorNo += n;
			// Default-Receiver ist 0. Den gibt's immer
			setNextReceiver(0);
			receivedStops = 0;
			receivedTT = 0;
			processSendTime = 0;
			processRecvTime = 0;
		}

		void start() {
			// unbeteiligte Prozessoren steigen hier aus
			finished = ((Muesli::MSL_myId < entrances[0]) ||
				(Muesli::MSL_myId >= entrances[0] + noprocs));
			
			if(finished)
				return;

			// alle Prozessoren, die am Prozess/Skelett beteiligt sind, merken sich in ihren
			// globalen Variablen den Ein- und Ausgang des Skeletts und einen Zeiger auf das
			// Skelett selbst Ein- und Ausgang des Skeletts merken
			
			// entrance PID
			Muesli::MSL_myEntrance = entrances[0];
			// exit PID
			Muesli::MSL_myExit = exits[0];
			Muesli::MSL_myProcess = this;
			// size (> 1, if data parallel)
			Muesli::MSL_numOfLocalProcs = noprocs;
			// used for termination detection
			receivedStops = 0;

			// uebergebene Funktion ohne Parameter aufrufen (dummy steht fuer leere Param-liste)
			// In dieser Funktion werden die Methoden MSL_get und MSL_put benutzt um Daten zu
			// empfangen und Daten zu senden. Auf diese Weise koennen beliebig viele Daten
			// empfangen und in beliebig viele Outputdaten transformiert werden. Diese Funktion
			// enthaelt i.d.R. eine Endlosschleife, in welcher die Daten verarbeitet werden, bis
			// ausreichend STOPs empfangen werden.
			Empty dummy;
			fct(dummy);
		}
		// ende start

		Process* copy() {
			return new Filter(fct, Muesli::MSL_numOfLocalProcs);
		}

		void show() const {
			if(Muesli::MSL_myId == 0)
				std::cout << "Filter (PID = " << entrances[0] << ")" << std::endl << std::flush;
		}

	};

}

#endif
