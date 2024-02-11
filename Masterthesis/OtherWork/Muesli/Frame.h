// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef FRAME_H
#define FRAME_H

#include "Muesli.h"
#include "Serializable.h"

namespace msl {

	/* Ein Frame fuer das interne Versenden von serialisierten Objekten, die mit weiteren Informationen
	   ausgestattet werden muessen. Idee: Reserviere Speicherbereich, der den kompletten Frame inkl.
	   serialisieren Datenteil aufnehmen kann. Am Anfang stehen die Frame-internen Daten, dann folgen
	   die Nutzerdaten. Die Adresse, ab der die Nutzdaten stehen koennen, wird der reduce-Methode
	   uebergeben, die der Nutzer implementieren muss.
	*/
	template<class Data> class Frame: public Serializable {

	private:

		// ID eines (Teil)Problems
		long id;
		// ID eines abgegebenen Teilproblems, das beim Empfaenger als Wurzelknoten interpretiert wird
		long rootNodeID;
		// Absender eines abgegebenen Teilproblems
		long originator;
		// ProzessorID des Initialproblems, aus dem das Teilproblem hervorgegangen ist
		long poolID;
		// void* pData; ??? -> Nein, da auf dem Data-Objekt reduce/expand aufgerufen werden muss, was
		// sonst nicht geht!
		Data* pData;

	public:
		
		Frame(long myId, long origID, long snd, long pool, Data* myData):
		id(myId), rootNodeID(origID), originator(snd), poolID(pool), pData(myData) {
		}

		// used in the scope of serialization
		Frame(): id(-1), rootNodeID(-1), originator(-1), poolID(-1), pData(NULL) {
		}

		~Frame(void) {
		}

		inline long getID() {
			return id;
		}

		inline long getRootNodeID() {
			return rootNodeID;
		}

		inline long getOriginator() {
			return originator;
		}

		inline long getPoolID() {
			return poolID;
		}

		inline Data* getData() {
			return pData;
		}

		inline void setID(long i) {
			id = i;
		}

		inline void setRootNodeID(long oid) {
			rootNodeID = oid;
		}

		inline void setOriginator(int o) {
			originator = (long)o;
		}

		inline void setPoolID(int id) {
			poolID = (long)id;
		}

		inline void setData(Data* d) {
			pData = d;
		}

		// liefert die Groeße des Frames in Bytes im serialisierten Zustand
		inline int getSize() {
			// Wenn die Kommunikation serialisiert erfolgt, dann ist Data von Serializable
			// abgeleitet und ein reduce/expand/getSize existiert
			if(MSL_isSerialized())
				return 4 * sizeof(long) + pData->getSize();
			else /* !serialized */
				return 4 * sizeof(long) + sizeof(Data);
		}

		// Serialisiert den Frame
		void reduce(void* pBuffer, int bufferSize) {
			// frameinterne Dinge kopieren
			long* pos = (long*) memcpy(pBuffer, &(this->id), sizeof(long));
			pos++;
			pos = (long*) memcpy(pos, &(this->rootNodeID), sizeof(long));
			pos++;
			pos = (long*) memcpy(pos, &(this->originator), sizeof(long));
			pos++;
			pos = (long*) memcpy(pos, &(this->poolID), sizeof(long));
			pos++;

			// user data kopieren
			if(MSL_isSerialized())
				pData->reduce(pos, bufferSize - (4 * sizeof(long)));
			else
				memcpy(pos, pData, sizeof(Data));
		}

		// entpackt den Frame
		void expand(void* pBuffer, int bufferSize) {
			// frameinterne Daten entpacken
			long* pos = (long*) pBuffer;
			id = *pos; 			pos++;
			rootNodeID = *pos; 	pos++;
			originator = *pos; 	pos++;
			poolID = *pos; 		pos++;
			// address of user data
			// Platz fuer die Solution/das Problem schaffen
			pData = new Data();

			// Userdata kopieren
			if(MSL_isSerialized())
				pData->expand(pos, bufferSize - 4 * sizeof(long));
			else
				memcpy(pData, pos, sizeof(Data));
		}

	};

}

#endif
