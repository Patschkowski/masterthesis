// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef FARM_H
#define FARM_H

#include "Muesli.h"
#include "Process.h"

namespace msl {

	/* Eine Farm verwaltet sich dezentral. Alle Worker werden in einem logischen Ring verwaltet.
	   Jeder Worker dieser Farm kann Ein- bzw. Ausgang des Skeletts sein. Der vorgelagerte Prozess
	   waehlt per Zufall (gleichverteilte Zufallsvariable) einen Arbeiter aus, dem er eine Nachricht
	   zusendet. Handelt es sich hierbei um "normale" Daten, dann verarbeitet der worker diese und
	   leitet sie an einen der nachgelagerten Empfaenger weiter (es kann mehrere geben, z.B. wiederum
	   eine Farm). Handelt es sich um ein STOP- oder TERMINATION-TEST-TAG, so wird diese Nachricht
	   einmal durch den Ring geschickt, bis diese bei dem urspruenglichen Empfaenger wieder angekommen
	   ist ("Stille	Post"-Prinzip). Dann leitet er diese Nachricht an einen der nachgelagerten
	   Empfaenger weiter.
	*/

	template<class I, class O> class Farm: public Process {

		// Worker
		Process** p;
		int i, j, k;
		// size der Farm
		int length;

	public:

		Farm(Process& worker, int l): length(l), Process() {
			ProcessorNo* entr;
			ProcessorNo* ext;
			// Arbeiter sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
			p = new Process*[length];
			p[0] = &worker;

			for(i = 1; i < length; i++) {
        		p[i] = worker.copy();
			}

			// alle Worker sind gleichzeitig Ein- und Ausgang des Skeletts. Die Anzahl der Ein- und
			// Ausgaenge der Farm ergibt sich aus der Anzahl der Worker und der Anzahl ihrer Ein-
			// und Ausgaenge

			// Eingaenge berechnen
			numOfEntrances = length * (*(p[0])).getNumOfEntrances();
			entrances = new ProcessorNo[numOfEntrances];
			k = 0;

			// alle worker durchlaufen
			for(i = 0; i < length; i++) {
				entr = (*(p[i])).getEntrances();

				// alle ihre Eingaenge durchlaufen
				for(j = 0; j < (*(p[i])).getNumOfEntrances(); j++) {
					entrances[k++] = entr[j];
				}
			}

			// Ausgaenge berechnen
			numOfExits = length * (*(p[0])).getNumOfExits();
			exits = new ProcessorNo[numOfExits];
			k = 0;

			// alle worker durchlaufen
			for(i = 0; i < length; i++) {
				ext = (*(p[i])).getExits();

				// alle ihre Ausgaenge durchlaufen
				for(j = 0; j < (*(p[i])).getNumOfExits(); j++) {
					exits[k++] = ext[j];
				}
			}

			// Empfaenger der ersten Nachricht festlegen
			setNextReceiver(0);
		}

		// Teilt allen workern die Sender mit. Dies sind u.U. mehrere, z.B. dann, wenn eine Farm
		// vorgelagert ist. In diesem Fall darf jeder Worker dieser Farm von jedem worker der
		// vorgelagerten Farm Daten entgegennehmen.
		// @param src	predecessors
		// @param l		arraysize
		inline void setPredecessors(ProcessorNo* src, int l) {
			numOfPredecessors = l;

			for(int i = 0; i < length; i++)
				(*(p[i])).setPredecessors(src,l);
		}

		// Teilt allen workern die Empfaenger mit. Dies sind u.U. mehrere, z.B. dann, wenn eine
		// Farm nachgelagert ist. In diesem Fall darf jeder Worker dieser Farm an jeden worker
		// der nachgelagerten Farm Daten senden.
		// @param drn	successors
		// @param l		arraysize
		inline void setSuccessors(ProcessorNo* drn, int l) {
			numOfSuccessors = l;

			for(int i = 0; i < length; i++)
				(*(p[i])).setSuccessors(drn,l);
		}

		// startet alle worker
		inline void start() {
			for(int i = 0; i < length; i++)
				(*(p[i])).start();
		}

		Process* copy() {
			return new Farm(*((*(p[0])).copy()),length);
		}

		inline void show() const {
			if(Muesli::MSL_myId == 0) {
				printf("id: %d, Farm, input: %d\n", Muesli::MSL_myId, entrances[0]);
	            
				for(int i = 0; i < length; i++) {
					(*(p[i])).show();
				}
			}
		}

	};

}

#endif
