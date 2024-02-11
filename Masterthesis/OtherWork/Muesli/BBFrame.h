// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef BBFRAME_H
#define BBFRAME_H

#include <cstdio>

#include "Muesli.h"
#include "Serializable.h"

namespace msl {

	/* Generische Klasse, die ein Objekt des angegebenen Datentyps kapselt.
	 * Die Klasse kann serialisiert versendet werden und initiiert dabei
	 * zugleich die Serialisierung der gekapselten Daten, sofern diese
	 * ebenfalls von der Klasse Serializable abgeleitet sind.
	 * */
	template<class Data>
	class BBFrame: public Serializable {

	private:

		// ID eines (Teil)Problems
		unsigned long id;
		// Zeiger auf das Vaterproblem
		BBFrame* parentProblem;
		// Absender eines abgegebenen Teilproblems
		long originator;
		// Anzahl der erzeugten Kindprobleme
		int numOfSubProblems;
		// Anzahl der geloesten Kindprobleme
		int numOfSubProblemsSolved;
		// void* pData; ??? -> Nein, da auf dem Data-Objekt reduce/expand
		// aufgerufen werden muss, was sonst nicht geht!
		Data* pData;

		static const int internalSize = sizeof(long) + sizeof(long) + 2 * sizeof(int) + sizeof(BBFrame*);

	public:

		BBFrame(long myId, BBFrame* parent, long snd, int numSub, Data* myData):
		id(myId), parentProblem(parent), originator(snd), numOfSubProblems(numSub), pData(myData) {
		}

		BBFrame(): id(0), parentProblem(NULL), originator(-1), numOfSubProblems(-1), numOfSubProblemsSolved(-1), pData(NULL) {
		}

		~BBFrame(void) {
			// delete pData;
		}

		inline unsigned long getID() {
			return id;
		}

		inline BBFrame* getParentProblem() {
			return parentProblem;
		}

		inline long getOriginator() {
			return originator;
		}

		inline int getNumOfSubProblems() {
			return numOfSubProblems;
		}

		inline int getNumOfSolvedSubProblems() {
			return numOfSubProblemsSolved;
		}

		inline Data* getData() {
			return pData;
		}

		inline void setID(unsigned long i) {
			id = i;
		}

		inline void setParentProblem(BBFrame* parent) {
			parentProblem = parent;
		}

		inline void setOriginator(int o) {
			originator = (long)o;
		}

		inline void setNumOfSubProblems(int num) {
			numOfSubProblems = num;
		}

		inline void setNumOfSolvedSubProblems(int num) {
			numOfSubProblemsSolved = num;
		}

		inline void setData(Data* d) {
			pData = d;
		}

		// liefert die Groeﬂe des Frames in Bytes im serialisierten Zustand
		inline int getSize() {
			// Wenn die Kommunikation serialisiert erfolgt, dann ist Data von
			// Serializable abgeleitet und ein reduce/expand/getSize existiert
			if(pData == NULL)
				return internalSize;
			else if(MSL_isSerialized())
				return internalSize + (reinterpret_cast<Serializable*>(pData))->getSize();
			else
				return internalSize + sizeof(Data);
		}

		// Serialisiert den Frame
		void reduce(void* pBuffer, int bufferSize) {
			long* pos = (long*) memcpy(pBuffer, &(this->id), sizeof(long));
			pos+=2;
			pos = (long*) memcpy(pos, &(this->parentProblem), sizeof(parentProblem));
			pos++;
			pos = (long*) memcpy(pos, &(this->originator), sizeof(long));
			pos++;
			pos = (long*) memcpy(pos, &(this->numOfSubProblems), sizeof(int));
			pos++;
			pos = (long*) memcpy(pos, &(this->numOfSubProblemsSolved), sizeof(int));
			pos++;

			// user data kopieren, falls welches zu senden ist
			if(bufferSize == internalSize)
				return;

			if(MSL_isSerialized())
				(reinterpret_cast<Serializable* >(pData))->reduce(pos,
				bufferSize - (sizeof(long) + sizeof(long) + 2 * sizeof(int) + sizeof(parentProblem)));
			else
				memcpy(pos, pData, sizeof(Data));
		}

		// entpackt den Frame
		void expand(void* pBuffer, int bufferSize) {
			// frameinterne Daten entpacken
			long* pos = (long*) pBuffer;
			memcpy(&(this->id), pos, sizeof(long));
			pos+=2;
			memcpy(&(this->parentProblem), pos, sizeof(parentProblem));
			pos++;
			memcpy(&(this->originator), pos, sizeof(originator));
			pos++;
			memcpy(&(this->numOfSubProblems), pos, sizeof(numOfSubProblems));
			pos++;
			memcpy(&(this->numOfSubProblemsSolved), pos, sizeof(numOfSubProblemsSolved));
			pos++;

			if(bufferSize == internalSize) return;
			// address of user data
			pData = new Data();

			// Userdata kopieren
			if(MSL_isSerialized())
				(reinterpret_cast<Serializable* >(pData))->expand(pos,
				bufferSize - (sizeof(long) + sizeof(long) + 2 * sizeof(int) + sizeof(parentProblem)));
			else
				memcpy(pData, pos, sizeof(Data));
		}

	};

}

#endif
