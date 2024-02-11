// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef MUESLI_H
#define MUESLI_H

//***********************************************************************************
//*                                                                                 *
//*         C++ Draft Proposal of Muesli 2.0 (Muenster Skeleton Library)            *
//*                                                                                 *
//*            (c) Steffen Ernsting and Philipp Ciechanowicz  and                   *
//*				      Michael Poldner  and  Herbert Kuchen                          *
//*          {s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de               *
//*                        Nov. 2001 - Feb. 2011                                    *
//*                                                                                 *
//* including several improvements by Joerg Striegnitz <J.Striegnitz@fz-juelich.de> *
//*                                                                                 *
//***********************************************************************************

#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

#include "Collection.h"
#include "Conversion.h"
#include "Curry.h"
#include "Exception.h"
#include "mpi.h"
#include "omp.h"
#include "Serializable.h"


namespace msl {

	#ifdef DEBUG
	static int Debug = DEBUG;
	#else
// 	static int Debug = 0;
	#endif

	#ifdef DEBUG
	#  if DEBUG > 0
	#    define dbg(x) x
	#  else
	#    define dbg(x)
	#  endif
	#else
	#  define dbg(x)
	#endif

	// **************************************************
	// * global constants
	// **************************************************

	// Muesli version
	static const int MSL_VERSION_MAJOR = 2;
	static const int MSL_VERSION_MINOR = 1;
	// MPI_ANY_TAG = -1; DO NOT ASSIGN -1 to ANY OTHER TAG!!!
	static const int MSLT_ANY_TAG = MPI_ANY_TAG;
	// is used for ordinary messages containing data.
	static const int MSLT_MYTAG = 1;
	// is used to stop the following process;
	static const int MSLT_STOP = 2;
	static const int MSLT_TERMINATION_TEST = 3;
	static const int MSLT_TOKEN_TAG = 4;
	static const int MSLT_PROBLEM_TAG = 5;
	static const int MSLT_BBINCUMBENT_TAG = 6;
	static const int MSLT_LB_TAG = 7;
	static const int MSLT_SOLUTION = 8;
	static const int MSLT_SUBPROBLEM = 9;
	static const int MSLT_WORKREQUEST = 10;
	static const int MSLT_REJECTION = 11;
	static const int MSLT_SENDREQUEST = 12;
	static const int MSLT_READYSIGNAL = 13;
	// Token zur Termination Detection
	static const int MSLT_BB_TERMINATIONTOKEN = 14;
	// Problem, dass waehrend Lastverteilung versendet wird
	static const int MSLT_BB_PROBLEM = 15;
	// aktuell gefundene beste Loesung
	static const int MSLT_BB_INCUMBENT = 16;
	// Nachricht mit der aktuell besten Abschaetzung
	static const int MSLT_BB_LOADBALANCE = 17;
	// Nachricht mit der aktuell besten Abschaetzung
	static const int MSLT_BB_LOADBALANCE_REJECTION = 18;
	// zur Anfrage, ob ein Incumbent gesendet werden darf
	static const int MSLT_BB_INCUMBENT_SENDREQUEST = 19;
	// Antwort, dass das Incumbent gesendet werden darf
	static const int MSLT_BB_INCUMBENT_READYSIGNAL = 20;
	// zur Anfrage, ob ein Problem gesendet werden darf
	static const int MSLT_BB_PROBLEM_SENDREQUEST = 21;
	// Antwort, das das Problem gesendet werden darf
	static const int MSLT_BB_PROBLEM_READYSIGNAL = 22;
	static const int MSLT_BB_PROBLEM_SOLVED = 23;
	static const int MSLT_BB_STATISTICS = 24;

	// tag used by the MSL_Allgather function
	static const int MSLT_ALLGATHER = 50;
	// tag used by the MSL_Allreduce function
	static const int MSLT_ALLREDUCE = 51;
	// tag used by the MSL_Broadcast function
	static const int MSLT_BROADCAST = 52;
	// tag used by the MSL_BroadcastSerial function; currently not in use
	static const int MSLT_BROADCAST_SERIAL = 53;
	// tag used by the rotate function
	static const int MSLT_ROTATE = 54;

	static const int MSL_BB_TOPOLOGY_ALLTOALL = 1;
	static const int MSL_BB_TOPOLOGY_HYPERCUBE = 2;
	static const int MSL_BB_TOPOLOGY_RING = 3;

	/* deprecated */
	// deaktiviert serialisierten Kommunikationmodus (Arg. fuer InitSkeletons)
	static const bool MSL_NOT_SERIALIZED = false;
	/* deprecated */
	// aktiviert serialisierten Kommunikationmodus (Arg. fuer InitSkeletons)
	static const bool MSL_SERIALIZED = true;

	static const int MSL_RANDOM_DISTRIBUTION = 1;
	static const int MSL_CYCLIC_DISTRIBUTION = 2;
	static const int MSL_DEFAULT_DISTRIBUTION = MSL_CYCLIC_DISTRIBUTION;
	// aktiviert/deativiert die Zeitmessung
	static const bool MSL_TIMER = false;
	static const int MSL_UNDEFINED = -1;

	// **************************************************
	// * structs, typedefs, and classes
	// **************************************************

	// needed in nullary argument functions of skeletons;  otherwise
	struct Empty {
	};

	// Typ einer ProcessorID
	typedef int ProcessorNo;

	// Kapselt Statistiken fuer die Experimente mit dem Branch-and-Bound Skelett
	typedef struct {
		// Statistik Variablen
		int statNumMsgProblemSolvedSent;
		int statNumMsgProblemSolvedReceived;
		int statNumMsgWorkPoolEmptySent;
		int statNumMsgWorkPoolEmptyRejectionReceived;
		int statNumMsgBoundInfoSent;
		int statNumMsgBoundInfoReceived;
		int statNumMsgBoundRejectionSent;
		int statNumMsgBoundRejectionReceived;
		int statNumProblemsSent;
		int statNumProblemsReceived;
		int statNumProblemsSolved;
		int statNumIncumbentReceivedAccepted;
		int statNumIncumbentReceivedDiscarded;
		int statNumIncumbentSent;
		int statNumProblemsBranched;
		int statNumProblemsBounded;
		int statNumSolutionsFound;
		int statNumProblemsTrackedTotal;
		int statNumProblemsKilled;
		double statTimeProblemProcessing;
		double statTimeCommunication;
		double statTimeIncumbentHandling;
		double statTimeLoadBalancing;
		double statTimeTrackerSolvedProblemsReceived;
		double statTimeTrackerSolvedProblemsSent;
		double statTimeCleanWorkpool;
		double statTimeSubProblemSolvedInsert;
		double statTimeIdle;
		double statTimeInitialIdle;
		double statTimeTotal;
		double statTimeSinceWorkpoolClean;
		int problemTrackerMaxLength;
		int problemTrackerAverageLength;
		int solvedProblemsQueueMaxLength;
		int polvedProblemsQueueAverageLength;
		int workpoolMaxLength;
		int workpoolAverageLength;
	} statistics;

    // Vorwaertsdeklaration
    class Process;

	class Muesli {

	private:

	protected:

	public:

		// **************************************************
		// * global variables
		// **************************************************

		/* deprecated */
		// Default
		static bool MSL_COMMUNICATION;

		// Modus wird InitSkeletons als Parameter übergeben
		static int MSL_DISTRIBUTION_MODE;

		// TODO DELETE IN DCSOLVER TODO
		static int numP;
		static int numS;
		static int numPF;
		static int numSF;

		/* deprecated */
		// = -1 // verweist auf den Prozessor, der den Eingang eines Skeletts darstellt
		static ProcessorNo MSL_myEntrance;
		/* deprecated */
		// verweist auf den Prozessor, der den Ausgang eines Skeletts darstellt
		static ProcessorNo MSL_myExit;		
		// Zeiger auf die Prozessklasse zu dem auch Muesli::MSL_myId gehoert
		static Process* MSL_myProcess;
		// Anzahl der zu einem Zeitpunkt arbeitender/integrierter Prozessoren (dynamisch)
		static ProcessorNo MSL_runningProcessorNo;
		// ID des betrachteten Processes
		static ProcessorNo MSL_myId;
		// number of processes used by atomic skeleton where Muesli::MSL_myId collaborates
		static int MSL_numOfLocalProcs;
		// total number of processors used
		static int MSL_numOfTotalProcs;

		// MPI_Wtime();
		static double MSL_Starttime;
		// argv[0]
		static char* MSL_ProgrammName;
		// argv[1]  size
		static int MSL_ARG1;
		// argv[2]	1: MSL_RANDOM_DISTRIBUTION, 2: MSL_CYCLIC_DISTRIBUTION
		static int MSL_ARG2;
		// argv[3]
		static int MSL_ARG3;
		// argv[4]
		static int MSL_ARG4;

	};

	// **************************************************
	// * initialize and terminate skeleton library
	// **************************************************

	/* Initializes the skeleton library.
	*/
	void InitSkeletons(int argc, char** argv, bool serialization = MSL_NOT_SERIALIZED);

	/* Cleans up after using the skeleton library.
	*/
	void TerminateSkeletons();

	// AUXILIARY FUNCTIONS

	/* Function used to determine the number of non-zero elements
	   of the distributed sparse matrix (cf. getElementCount and
	   allreduce).
	*/
	template<typename T>
	T add(T a, T b) {
		return a + b;
	}

	// assumption: n > 0
	inline int log2(int n) {
		int i;
		
		for(i = 0; n > 0; i++) {
			n /= 2;
		}
		
		return i - 1;
	}

	void throws(const Exception& e);

	template<class C1, class C2>
	inline C1 proj1_2(C1 a, C2 b) {
		return a;
	}

	template<class C1, class C2>
	inline C2 proj2_2(C1 a, C2 b) {
		return b;
	}

	template<class C>
	inline C extend(C (*f)(), Empty dummy) {
		return (*f)();
	}

	template<class F>
	inline static int auxRotateRows(const Fct1<int, int, F>& f, int blocks, int row, int col) {
		return (col + f(row) + blocks) % blocks;
	}

	template<class F>
	inline static int auxRotateCols(const Fct1<int, int, F>& f, int blocks, int row, int col) {
		return (row + f(col) + blocks) % blocks;
	}

	inline bool MSL_isSerialized() {
		return Muesli::MSL_COMMUNICATION == MSL_SERIALIZED;
	}

	// COMMUNICATION FUNCTIONS

	// COMMUNICATION FUNCTIONS / MSL_Send

	// Implementierung von MSL_Send fuer zu serialisierende Objekte
	template<class Data>
	inline void MSL_Send(ProcessorNo destination, Data* pData, int tag, Int2Type<true>, int count = 1) {
		int size = pData->getSize() * count;
		void* buffer = malloc(size + 10);

		if(buffer == NULL) {
			std::cout << "OUT OF MEMORY ERROR in MSL_Send: malloc returns NULL" << std::endl;
		}

		pData->reduce(buffer, size);
		MPI_Send(buffer, size, MPI_BYTE, destination, tag, MPI_COMM_WORLD);
		free(buffer);
	}

	/* Enhanced send and receive functions which are additionally able to send and
	   receive and arbitrary number of user-defined objects, not only a single one.
	   However, all functions assume that each user-defined object has the same size.
	*/

	/* Version of send for data which need not be serialized, e.g. integrated
	   data types such as int or double and structures.
	*/
	template<class T>
	inline void send(ProcessorNo dst, T* data, int tag, Int2Type<false>, int count = 1) {
		MPI_Send(data, sizeof(T) * count, MPI_BYTE, dst, tag, MPI_COMM_WORLD);
	}

	/* Version of send for data which need to be serialized such as user-defined
	   objects.
	*/
	template<class T>
	inline void send(ProcessorNo dst, T* data, int tag, Int2Type<true>, int count = 1) {
		// determine size of the message
		int size = data[0].getSize();
		// allocate buffer; sizeof(char) = 1 (byte)
		char* buffer = new char[size * count];

		// iterate over all objects to reduce
		for(int i = 0; i < count; i++) {
			data[i].reduce(&buffer[i * size], size);
		}
		
		// send buffer
		MPI_Send(buffer, size * count, MPI_BYTE, dst, tag, MPI_COMM_WORLD);
		// free buffer
		free(buffer);
	}

	/* General send function to be used. The compiler decides at compile-time which of the
	   two versions of the send function to call: Either the one for data which need not be
	   serialized or the one for objects which need serialization.
	*/
	template<class T>
	inline void send(ProcessorNo dst, T* data, int tag = MSLT_MYTAG, int count = 1) {
		if(dst == MSL_UNDEFINED) {
			throws(UndefinedDestinationException());
		}

		send(dst, data, tag, Int2Type<MSL_IS_SUPERCLASS(Serializable, T)>(), count);
	}

	/* Version of receive for data which need to be serialized such as user-defined
	   objects.
	*/
	template<class T>
	inline void receive(ProcessorNo src, T* data, int tag, MPI_Status* status, Int2Type<true>,
		int count = 1) {
		// ???
		MPI_Probe(src, tag, MPI_COMM_WORLD, status);
		// size of each element
		int size = 0;
		// determine number of bytes to receive
		MPI_Get_count(status, MPI_BYTE, &size);
		// calculate size of each element
		size = (int)(size / count);
		// allocate buffer space
		char* buffer = new char[size * count];

		// receive message
		MPI_Recv(buffer, size * count, MPI_BYTE, src, tag, MPI_COMM_WORLD, status);

		// iterate over all elements to receive
		for(int i = 0; i < count; i++) {
			// restore received element
			data[i].expand(&(buffer[i * size]), size);
		}

		// free temporary buffer
		free(buffer);
	}

	/* Version of receive for data which need not be serialized, e.g. integrated
	   data types such as int or double and structures.
	*/
	template<class T>
	inline void receive(ProcessorNo src, T* data, int tag, MPI_Status* status, Int2Type<false>,
		int count = 1) {
		MPI_Recv(data, sizeof(T) * count, MPI_BYTE, src, tag, MPI_COMM_WORLD, status);
	}

	/* General receive function to be used. The compiler decides at compile-time which of
	   the two versions of the send function to call: Either the one for data which need
	   not be serialized or the one for objects which need serialization.
	*/
	template<class T>
	inline void receive(ProcessorNo src, T* data, int tag, MPI_Status* status, int count = 1) {
		if(src == MSL_UNDEFINED) {
			throws(UndefinedDestinationException());
		}

		receive(src, data, tag, status, Int2Type<MSL_IS_SUPERCLASS(Serializable, T)>(), count);
	}

	// Implementierung von MSL_Send fuer bereits serialisierte Objekte
	template<class Data>
	inline void MSL_Send(ProcessorNo destination, Data* pData, int tag, Int2Type<false>, int count = 1) {
		MPI_Send(pData, sizeof(Data) * count, MPI_BYTE, destination, tag, MPI_COMM_WORLD);
	}

	/* Allg. Send fuer serialisierte und nicht serlialisierte Objekte. Diese Methode kann innerhalb
	   der Skelette aufgerufen werden. Der Compiler sucht sich beim Compilieren automatisch die zu
	   Data passende MSL_Send-Implementierung raus.
	*/
	template<class Data>
	inline void MSL_Send(ProcessorNo destination, Data* pData, int tag = MSLT_MYTAG, int count = 1) {
		if(destination == MSL_UNDEFINED) {
			throws(UndefinedDestinationException());
		}

		MSL_Send(destination, pData, tag, Int2Type<MSL_IS_SUPERCLASS(Serializable, Data)>(), count);
	}

	// COMMUNICATION FUNCTIONS / MSL_Receive

 template<class Data>
    inline void MSL_ISend(ProcessorNo destination, Data* pData, int tag, Int2Type < false >, MPI_Request* reqs, MPI_Status* stats, int count = 1) {
        MPI_Isend(pData, sizeof (Data) * count, MPI_BYTE, destination, tag, MPI_COMM_WORLD, reqs);
    }

/* Allg. nichtblockierendes Send fuer serialisierte und nicht serlialisierte Objekte.
     * Diese Methode kann innerhalb der Skelette aufgerufen werden. Der Compiler sucht
     * sich beim Compilieren automatisch die zu Data passende MSL_Send-Implementierung raus.
     */
    template<class Data>
    inline void MSL_ISend(ProcessorNo destination, Data* pData, MPI_Request* reqs, MPI_Status* stats, int tag = MSLT_MYTAG, int count = 1) {
        if (destination == MSL_UNDEFINED) {
            throws(UndefinedDestinationException());
        }

        MSL_ISend(destination, pData, tag, Int2Type < MSL_IS_SUPERCLASS(Serializable, Data)>(), reqs, stats, count);
    }

	// Implementierung von MSL_Receive fuer zu serialisierende Objekte
	template<class Data>
	inline void MSL_Receive(ProcessorNo source, Data* pData, int tag, MPI_Status* pStatus,
		Int2Type<true>, int count = 1) {
		MPI_Probe(source, tag, MPI_COMM_WORLD, pStatus);
		int size = 1;
		MPI_Get_count(pStatus, MPI_BYTE, &size);
		void* buffer = malloc(size * count);
		
		if(buffer == NULL) {
			std::cout << "OUT OF MEMORY ERROR in MSL_Receive: malloc returns NULL" << std::endl;
		}

		MPI_Recv(buffer, size * count, MPI_BYTE, source, tag, MPI_COMM_WORLD, pStatus);
		pData->expand(buffer, size * count);
		free(buffer);
	}

	// Implementierung von MSL_Receive fuer bereits serialisierte Objekte
	template<class Data>
	inline void MSL_Receive(ProcessorNo source, Data* pData, int tag, MPI_Status* pStatus,
		Int2Type<false>, int count = 1) {
		MPI_Recv(pData, sizeof(Data) * count, MPI_BYTE, source, tag, MPI_COMM_WORLD, pStatus);
	}

	/* Allg. Send fuer serialisierte und nicht serlialisierte Objekte. Diese Methode kann innerhalb
	   der Skelette aufgerufen werden. Der Compiler sucht sich beim Compilieren automatisch die zu
	   Data passende MSL_Receive-Implementierung raus.
	*/
	template<class Data>
	inline void MSL_Receive(ProcessorNo source, Data* pData, int tag, MPI_Status* pStatus, int count = 1) {
		if(source == MSL_UNDEFINED) {
			throws(UndefinedDestinationException());
		}

		MSL_Receive(source, pData, tag, pStatus, Int2Type<MSL_IS_SUPERCLASS(Serializable, Data)>(), count);
	}
	
	/* Eigentlich veraltet.
	*/
	template<class C>
	inline void MSL_receive(ProcessorNo source, C* valref, int size, MPI_Status* stat) {
		if(source == MSL_UNDEFINED)
			throws(UndefinedSourceException());

		MPI_Recv(valref, size, MPI_BYTE, source, MPI_ANY_TAG, MPI_COMM_WORLD, stat);
	}

	// COMMUNICATION FUNCTIONS / VARIOUS

	// Verschickt Nachrichten ohne Inhalt, die mit einem bestimmten Tag versehen sind. (Protokoll)
	inline void MSL_SendTag(ProcessorNo destination, int tag) {
	   if(destination == MSL_UNDEFINED)
		   throws(UndefinedDestinationException());

	   int dummy;
	   MPI_Send(&dummy, sizeof(dummy), MPI_BYTE, destination, tag, MPI_COMM_WORLD);
	}

	// Empfaengt Nachrichten ohne Inhalt, die mit einem bestimmten Tag versehen sind. (Protokoll)
	inline void MSL_ReceiveTag(ProcessorNo source, int tag) {
	   if(source == MSL_UNDEFINED)
		   throws(UndefinedDestinationException());

	   int dummy;
	   MPI_Status status;
	   MPI_Recv(&dummy, sizeof(int), MPI_BYTE, source, tag, MPI_COMM_WORLD, &status);
	}

	template<typename C>
	inline void MSL_SendReceive(ProcessorNo dest, C* sendbuf, C* recvbuf, int count = 1) {
		MPI_Status status;

		if(dest > Muesli::MSL_myId) {
			MSL_Send(dest, sendbuf, MSLT_MYTAG, count);
			MSL_Receive(dest, recvbuf, MSLT_MYTAG, &status, count);
		}
		else {
			MSL_Receive(dest, recvbuf, MSLT_MYTAG, &status, count);
			MSL_Send(dest, sendbuf, MSLT_MYTAG, count);
		}
	}

        // blockierendes, synchrones Send
        template<class C>
        inline void syncSend(ProcessorNo destination, C* valref, int size) {
                if(destination == MSL_UNDEFINED)
                   throws(UndefinedDestinationException());

                MPI_Ssend(valref, size, MPI_BYTE, destination, MSLT_MYTAG, MPI_COMM_WORLD);
        }

	// blockierendes synchrones Send mit blockierendem asynchronen recieve
	template<class C>
	inline void sendReceive(ProcessorNo destination, C* valref1, C* valref2, int size) {
		if(destination == MSL_UNDEFINED)
		   throws(UndefinedDestinationException());

		MPI_Status stat;

		if(destination > Muesli::MSL_myId) {
		   syncSend(destination, valref1, size);
		   MSL_receive(destination, valref2, size, &stat);
		}
		else {
		   MSL_receive(destination, valref2, size, &stat);
		   syncSend(destination, valref1, size);
		}
	}

	// COMMUNICATION FUNCTIONS / GLOBAL

	// The following communication functions implement various MPI group
	// communication functions such as MPI_Bcast, MPI_Allgather, and
	// MPI_Allreduce. The re-implementation is necessary, since the MPI
	// communicator MPI_COMM_WORLD should not be used. This is due to the
	// fact, that this data structure can in principal be used inside a
	// task parallel skeleton, such that not all MPI processes participate
	// in the communication. Furthermore, the MPI routines are not able to
	// send or receive serialized objects, while our MSL counterparts can
	// do so by using special send and receive routines defined in Muesli.h.

	// Currently, each communication function is wrapped by a more simple
	// variant which expects less parameters. This is more convenient, as
	// you normally only want to specify what data to send and perhaps
	// where to store the result (when broadcasting a message both are
	// identical). However, the more complex functions always expect an
	// array of MPI ids and its lenght in addition. For now, this is not
	// necessary, since this array is always the 'ranks' array (cf. line
	// 47). In short, the enhanced variants are not always needed, but
	// provide a more general function signature for the future.

	// Note that none of the methods uses the const modifier to guarantee
	// that for example the sendbuf is not altered. This is due to the fact
	// that MPI_Send and MPI_Recv do not guarantee this behaviour and that
	// this functions are used to send and receive data.

	/* Versendet die Nachricht 'message' mit der Größe 'sizeInBytes' per Broadcast
	   an alle beteiligten Prozesse. Als Wurzelprozess agiert dabei der Prozess mit
	   der ID 'idSource', d.h. er stellt die zu versendende Nachricht zur Verfügung
	   und verteilt sie an alle Prozesse.
	*/
	template<class C>
	inline void broadcast(C* message, int sizeInBytes, int idSource) {
		MPI_Bcast(message, sizeInBytes, MPI_BYTE, idSource, MPI_COMM_WORLD);
	}

	/* Benutzt die MPI-Funktion MPI_Allgather() zum Aufsammeln von Elementen aller
	   beteiligten Prozesse. Der übergebene Array 'source' enthält dabei die zu
	   verschickenden Elemente, die anschließend im Array 'destination' gespeichert
	   werden. Hierfür muss der Benutzer ausreichend Platz zur Verfügung stellen.
	*/
	template<class C>
	inline void gatherAll(C* source, C* destination, int size) {
		// alle Prozesse sammeln alle Elemente auf
		MPI_Allgather(source, size, MPI_BYTE, destination, size, MPI_BYTE, MPI_COMM_WORLD);
	}

	/* Benutzt die MPI-Funktion MPI_Gather() zum Aufsammeln von 'count' Elementen
	   aller beteiligten Prozesse. Der übergebene Array 'source' enthält dabei die zu
	   verschickenden Elemente, die anschließend im Array 'destination' gespeichert werden.
	   Hierfür muss der Benutzer ausreichend Platz zur Verfügung stellen.
	*/
	template<class C>
	inline void gather(C source[], C destination[], int size, int root) {
		// der Wurzelprozess sammelt alle Elemente auf
		MPI_Gather(source, size, MPI_BYTE, destination, size, MPI_BYTE, root, MPI_COMM_WORLD);
	}

	/* Computes the number of loop passes which are required to broadcast
	   a message by using a pyramid. Basically, this function computes the
	   ceiling of the logarithm to the basis of two of the given argument
	   'count'.
	*/
	static int getLoopPasses(int count) {
		return (int)(ceil(log((double)count) / log(2.)));
	}

	/* This function implements the MPI routine MPI_Bcast and can be used to
	   broadcast a single element or an array of elements to all other processes,
	   respectively. The broadcast is implemented by using a pyramid. This should
	   be much faster than sequentially sending the messages one after another.
	   
	   The number of communication steps is calculated as follows. In general,
	   a total of (np - 1) messages must be send, since every process must
	   receive the message to broadcast. Using a sequential approach, this
	   takes (np - 1) communication steps. However, using a pyramid reduces
	   this number:

	   - Case 1 (np = 2^x): In total, log_2(np) communication steps are used.
	   - Case 2 (np != 2^x): In total, floor(log_2(np)) + 1 or ceil(log_2(np))
		 communication steps are necessary, respectively. The additional
		 communication step is necessary, since there are more than 2^x but
		 less than 2^(x + 1) processes to send messages to. If there would be
		 2^x processes, log_2(np) steps would suffice but since there are more,
		 an additional step is required.

	   The arguments are:

	   - T2* sendbuf: A pointer to a single element or an array of elements of
		 type T2 to send to all other processes identified by the 'ids' array.
		 The type T2 may differ from the type T used for the matrix elements,
		 since some auxiliary functions such as getElementCount need to broadcast
		 integers.
	   - int* const ids: An array containing the MPI ids of the processes which
		 participate in the broadcast operation. If 'ids' is larger than 'np',
		 none of the surplus processes will send of receive any messages. If
		 'ids' is smaller, the program will exit or starve. Neither of these
		 options is very desirable.
	   - int np: The lenght of the 'ids' array and therefore the number of
		 processes participating in the broadcast operation.
	   - int idRoot: The MPI id of the process which is the root process of the
		 broadcast operation. This id must be contained in the given 'ids' array.
		 Otherwise, the program will exit or starve. However, the position of the
		 given id inside the 'ids' array is irrelevant.
	   - int count: The number of elements the sendbuffer contains.

	   NOTE: The implementation of the method is optimal in the way, that it uses
	   a minimum number of communication steps.

	   NOTE 2: In contrast to the other communication methods (MSL_Allgather and
	   MSL_Allreduce) the given ids array cannot be declared constant since the
	   method needs to switch the position of the root process in case it is not
	   stored in the very first position of the array.

	   NOTE 3: The ids array is declared constant!?

	   MPI_Bcast(void* message, int count, MPI_Datatype type, int root, MPI_Comm comm)
	*/
	template<typename T>
	static void broadcast(T* buf, int* const ids, int np, int idRoot, int count = 1) {
		// number of iterations; height of the pyramid
		int passes = getLoopPasses(np);
		// own position in the given ids array
		int pos = -1;
		// position of the root process
		int posRoot = -1;
		// number of steps where the id of the sending/receiving process is located
		int step;
		// MPI status for receiving the message
		MPI_Status status;

		// number of processes is greater than one
		if(np > 1) {
			// determine own position in ids array
			for(int i = 0; i < np; i++) {
				// own id matches current id
				if(ids[i] == Muesli::MSL_myId) {
					// store position
					pos = i;
				}

				// current id matches root id
				if(ids[i] == idRoot) {
					// store position
					posRoot = i;
				}
			}

			// id of the root process is not the first entry in the ids array -> swap it
			if(posRoot != 0) {
				// process is the root process of the broadcast operation
				if(Muesli::MSL_myId == idRoot) {
					// process must be the first process
					pos = 0;
				}

				// process' id is the first in the ids array
				if(Muesli::MSL_myId == ids[0]) {
					// adjust position
					pos = posRoot;
				}

				// write id of the first process to correct position
				ids[posRoot] = ids[0];
				// write root id to first position in array
				ids[0] = idRoot;
			}

			// iterate over the tree depth
			for(int i = 1; i <= passes; i++) {
				// calculate steps
				step = (int)pow(2., i - 1);

				// process is sender
				if(pos < step) {
					// receiving process exists
					if(pos + step < np) {
						// send message
						send(ids[pos + step], buf, MSLT_BROADCAST, count);
					}
				}

				// process is receiver
				if(pos >= step && pos < 2 * step) {
					// receive message
					receive(ids[pos - step], buf, MSLT_BROADCAST, &status, count);
				}
			}

			// id of the root process was not the first entry in the ids array -> re-swap it
			if(posRoot != 0) {
				// write formerly first entry back
				ids[0] = ids[posRoot];
				// write id of the root process back
				ids[posRoot] = idRoot;
			}
		}
	}

	/* The function implements the MPI routine MPI_Allgather. The function
	   expects a single element or an array of type T which is send to all
	   other processes. Since this is done by every process, each process
	   stores the values of each other process after completing the function.
	   Thus, a buffer is needed to store all values, namely the recvbuf. For
	   the function to complete successfully, he ids array must be of lenght
	   np and the recvbuf must be of length np * count. Otherwise, the
	   function will crash. After completing the function, each process
	   stores the same values at the same location in the receive buffer.

	   In order to exchange messages the processes are organized in a hyper-
	   cube. To avoid deadlocks, processes with an even position in the ids
	   array are allowed to send their message first while processes with an
	   odd position in the ids array must receive a message. Then, this order
	   is reversed. This mechanism guarantees, that no deadlocks can ever
	   occur. Note that this approach is feasible for an arbitrary number
	   of processes, i.e. the function will also complete successfully for
	   an odd number of processes.

	   To allgather the elements, the algorithm executes in two phase:

	   - Phase 1: Reduce the hypercube. In each communication step, the
		 dimension of the hypercube is virtually reduced by 1. By doing so,
		 process ids[0] has gathered all data after ceil(log_2(np)) rounds.
	   - Phase 2: Broadcast the gathered data. Conversely, the dimension of
		 the hypercube is virtually increased by 1 in each communication
		 step (cf. broadcast). By doing so, all processes have gathered
		 all data after ceil(log_2(np)) rounds.

	   In total, the algorithm needs 2 * ceil(log_2(np)) communication steps
	   to allgather the data. Although a lot of processes are idle during the
	   reduce and broadcast operation, this method of allgathering the elements
	   seems to be the fastest way possible. This is on the one hand puzzling,
	   on the other interesting.

	   The implemented algorithm has the useful property, that the gathered
	   elements are stored in the order in which the ids of the processes are
	   stored in the ids array. This property is guaranteed by the exchange
	   mechanism and works as follows: Let ids = [3 1 4 0 2], sendbuf = [id],
	   i.e. each process sends its own id to all other processes. Each process
	   stores a newly received message after all other received messages in
	   the recvbuf, i.e. newly received messages are always appended to the
	   recvbuf:

	   - Step 1: Process 1 sends its id to process 3 and process 0 sends its
		 id to process 4. By doing so, the recvbuf of process 3 contains the
		 elements [3 1] and the recvbuf of process 4 contains the elements
		 [4 0].
	   - Step 2: Process 4 sends its recvbuf to process 3. By doing so, the
		 recvbuf of process 3 contains the elements [3 1 4 0].
	   - Step 3: Process 2 sends its id to process 3. By doing so, the recvbuf
		 of process 3 contains the elements [3 1 4 0 2]. This is the correct
		 sequence (see above).

	   - T* sendbuf: The pointer to the element or array to send. Is not of
		 type T, since when counting elements, the data type is int while this
		 must not be true for the type of the elements stored by the matrix.
	   - T* recvbuf: The array used to store the gathered data. This array
		 must be of size np * count. If it is larger, surplus elements are
		 left unchanged. If it is smaller, the program will exit.
	   - int* ids: An array which contains the MPI ids of the processes which
		 are expected to exchange their data. It must be of size np. If it is
		 larger, surplus processes will not send or receive any messages. If
		 it is smaller, the program will exit.
	   - np: The number of processes which are exchanging their data and
		 at the same time the size of the ids array.
	   - count: The number of elements the sendbuffer contains.

	   NOTE: The implementation of the routine is optimal in the way, that it
	   uses a minimum number of communication steps to exchange the sendbuf.

	   MPI_Allgather(void* sendbuf, int sendcnt, MPI_Datatype type,
		 void* recvbuf, int recvcnt, MPI_Datatype type, MPI_Comm comm);
	*/
	template<typename T>
	static void allgather(T* sendbuf, T* recvbuf, int* const ids, int np, int count) {
		// current offset between two sending processes
		int offset;
		// own position/index in the given ids array
		int posSelf;
		// position of the first process to send in the current round
		int posStart;
		// number of communication rounds
		int rounds;
		// MPI status
		MPI_Status status;
		
		// copy send buffer to receive buffer; no communication necessary
		memcpy(recvbuf, sendbuf, sizeof(T) * count);

		// number of processes is greater than one
		if(np > 1) {
			// iterate over all elements of the ids array
			for(int i = 0; i < np; i++) {
				// current id matches own id
				if(ids[i] == Muesli::MSL_myId) {
					// store current position
					posSelf = i;
				}
			}

			// determine number of rounds
			rounds = getLoopPasses(np);

			// iterate over all rounds to gather sendbuf
			for(int i = 1; i <= rounds; i++) {
				// calculate current offset; offset = 2^i
				offset = 1 << i;
				// calculate starting position; posStart = 2^(i-1)
				posStart = 1 << (i - 1);

				// process has to send a message
				if((posSelf - posStart) % offset == 0) {
					// send message
					send(ids[posSelf - posStart], recvbuf, MSLT_ALLGATHER, posStart * count);
				}

				// process has to receive a message
				if(posSelf % offset == 0 && posSelf + posStart < np) {
					// receive message
					receive(ids[posSelf + posStart], &recvbuf[posStart * count],
						MSLT_ALLGATHER, &status, posStart * count);
				}
			}


			// broadcast recvbuf
			broadcast(recvbuf, ids, np, ids[0], np * count);
		}
	}

	/* The function implements the MPI routine MPI_Allreduce
	   by arranging all processes as nodes of a hypercube and
	   successively reducing its dimension. First of all, the
	   method reduces all elements in such a way that process
	   0 has the reduced result. This needs O(log_2(np)) steps.
	   Then, the reduced result is broadcasted to all other
	   processes. This also takes O(log_2(np)) steps. Thus,
	   the methods need O(2*log_2(np)) steps to allreduce the
	   given vector.

	   The method is able to allreduce single elements as well
	   as whole vectors. For this reason, the parameter 'count'
	   is used to indicate the number of elements the parameter
	   'sendbuf' contains.

	   NOTE: The implementation of the method is optimal in the
	   way, that it uses minimum memory and a minimum number of
	   communication steps.

	   NOTE: Although a lot of processes are idle during the
	   reduce and fold operation, this method of allreducing
	   the elements seems to be the fastest way possible. This
	   is on the one hand puzzling, on the other interesting.

	   MPI_Allreduce(void* sendbuf, void* recvbuf, int sendcnt,
		 MPI_Datatype type, MPI_Op operator, MPI_COMM comm);
	*/
	template<typename T, typename F>
	static void allreduce(T* sendbuf, T* recvbuf, int* const ids, int np, Fct2<T, T, T, F> f, int count) {
		// current offset between two sending processes
		int offset;
		// own position/index in the given ids array
		int posSelf;
		// position of the first process to send in the current round
		int posStart;
		// number of communication rounds
		int rounds;
		// MPI status
		MPI_Status status;
		// temporary buffer
		T* tempbuf;
		
		// number of processes is one
		if(np == 1) {
			// copy send buffer to receive buffer; no communication necessary
			memcpy(recvbuf, sendbuf, sizeof(T) * count);
		}
		// number of processes is greater than one
		else {
			// determine number of rounds
			rounds = getLoopPasses(np);
			// allocate space for tempbuf
			tempbuf = new T[count];
			// copy sendbuf to recvbuf
			memcpy(recvbuf, sendbuf, sizeof(T) * count);

			// iterate over all elements of the ids array
			for(int i = 0; i < np; i++) {
				// current id matches own id
				if(ids[i] == Muesli::MSL_myId) {
					// store current position
					posSelf = i;
				}
			}

			// iterate over all rounds to reduce sendbuf
			for(int i = 1; i <= rounds; i++) {
				// calculate current offset; offset = 2^i
				offset = 1 << i;
				// calculate starting position; posStart = 2^(i-1)
				posStart = 1 << (i - 1);

				// process has to send a message
				if((posSelf - posStart) % offset == 0) {
					// send message
					send(ids[posSelf - posStart], recvbuf, MSLT_ALLREDUCE, count);
				}

				// process has to receive a message
				if(posSelf % offset == 0 && posSelf + posStart < np) {
					// receive message
					receive(ids[posSelf + posStart], tempbuf, MSLT_ALLREDUCE, &status, count);

					// iterate over all element of the received message
					for(int j = 0; j < count; j++) {
						// fold current element and store it in recvbuf
						recvbuf[j] = f(recvbuf[j], tempbuf[j]);
					}
				}
			}

			// broadcast reduced recvbuf
			broadcast(recvbuf, ids, np, ids[0], count);
			// delete tempbuf
			delete [] tempbuf;
		}
	}

	/* Just a wrapper for non-curried funtions.
	*/
	template<typename T>
	static void allreduce(T* sendbuf, T* recvbuf, int* const ids, int np, T (*f)(T, T), int count) {
		allreduce(sendbuf, recvbuf, ids, np, curry(f), count);
	}

	/* This method expects a function of type T2 (*f)(T2, T2, int, int),
	   i.e. it passes the indexes of the current element to the function.
	*/
	template<typename T2, typename F>
	static void MSL_AllreduceIndex(T2* sendbuf, T2* recvbuf, int* const ids, int np,
		Fct4<T2, T2, int, int, T2, F>f, int count) {
		// current offset between two sending processes
		int offset;
		// own position/index in the given ids array
		int posSelf;
		// position of the first process to send in the current round
		int posStart;
		// number of communication rounds
		int rounds;
		// MPI status
		MPI_Status status;
		// temporary buffer
		T2* tempbuf;
		
		// number of processes is one
		if(np == 1) {
			// copy send buffer to receive buffer; no communication necessary
			memcpy(recvbuf, sendbuf, sizeof(T2) * count);
		}
		// number of processes is greater than one
		else {
			// determine number of rounds
			rounds = getLoopPasses(np);
			// allocate space for tempbuf
			tempbuf = new T2[count];
			// copy sendbuf to recvbuf
			memcpy(recvbuf, sendbuf, sizeof(T2) * count);

			// iterate over all elements of the ids array
			for(int i = 0; i < np; i++) {
				// current id matches own id
				if(ids[i] == Muesli::MSL_myId) {
					// store current position
					posSelf = i;
				}
			}

			// iterate over all rounds to reduce sendbuf
			for(int i = 1; i <= rounds; i++) {
				// calculate current offset; offset = 2^i
				offset = 1 << i;
				// calculate starting position; posStart = 2^(i-1)
				posStart = 1 << (i - 1);

				// process has to send a message
				if((posSelf - posStart) % offset == 0) {
					// send message
					MSL_Send(ids[posSelf - posStart], recvbuf, MSLT_ALLREDUCE, count);
				}

				// process has to receive a message
				if(posSelf % offset == 0 && posSelf + posStart < np) {
					// receive message
					MSL_Receive(ids[posSelf + posStart], tempbuf, MSLT_ALLREDUCE, &status, count);

					// iterate over all element of the received message
					for(int j = 0; j < count; j++) {
						// fold current element and store it in recvbuf;
						// I do not know if it is useful to pass the
						// indexes in the way I do it now, but HS may
						// require this in his applications
						recvbuf[j] = f(recvbuf[j], tempbuf[j], j, -1);
					}
				}
			}

			// broadcast reduced recvbuf
			broadcast(recvbuf, ids, np, ids[0], count);
			// delete tempbuf
			delete [] tempbuf;
		}
	}

	/* Just a wrapper for non-curried functions.
	*/
	template<typename T2>
	static void MSL_AllreduceIndex(T2* sendbuf, T2* recvbuf, int* const ids, int np,
		T2 (*f)(T2, T2, int, int), int count) {
		MSL_AllreduceIndex(sendbuf, recvbuf, ids, np, curry(f), count);
	}

	/* The following methods have been written by Holger Sunke. They
	   are very efficient, however, they are not very elegant since
	   they use break statements inside the for loops. I hope that I
	   can implement a more stylish version :-)
	*/

	template<typename T2>
	void MSL_AllreduceVector(T2* sendbuf, T2* recvbuf, int* ranks, int np, T2 (*f)(T2, T2), int count = 1) {
		if(np == 1) {
			memcpy(recvbuf, sendbuf, sizeof(T2) * count);
		}
		else{
			MSL_AllreduceVector(sendbuf, recvbuf, ranks, np, f, 1, count);
		}
	}

	template<typename T2>
	void MSL_AllreduceVector(T2* sendbuf, T2* recvbuf, int* ids, int np, T2 (*f)(T2, T2), int dummy, int count = 1) {
		// current position in the ids array to receive a message from
		int posRecv;
		// current position in the ids array to send a message to
		int posSend;
		// number of communication rounds
		int rounds = getLoopPasses(np);
		// MPI status
		MPI_Status status;
		// temporary array to store reduced messages
		T2* tempbuf = new T2[count];

		// iterate over all collaborating processes
		for(int pos = 0; pos < np; pos++) {
			// own id matches id in the ids array
			if(Muesli::MSL_myId == ids[pos]) {
				// iterate over all communication rounds
				for(int round = 1; round <= rounds; round++) {
					// process has to send a message
					if(pos % (1 << round) > 0) {
						// calculate position to send a message to
						posSend = pos - (1 << (round - 1));
						// send message
						MSL_Send(ids[posSend], sendbuf, MSLT_ALLREDUCE, count);
						// leave current round and enter new position
						break;
					}
					// process has to receive a message
					else {
						// calculate position to receive a message from
						posRecv = pos + (1 << (round - 1));
						
						// receiver is not out of bound; this is
						// necessary in case that log_2(np) != 0
						if(posRecv < np) {
							// receive message and store it in temporary buffer
							MSL_Receive(ids[posRecv], tempbuf, MSLT_ALLREDUCE, &status, count);

							// iterate over all elements to receive
							#pragma omp parallel for
							for(int i = 0; i < count; i++) {
								// fold/reduce the received elements and store them in the recvbuf
								recvbuf[i] = f(recvbuf[i], tempbuf[i]);
							}
						}
					}
				}

				// broadcast the recvbuf to all collaborating processes
				broadcast(recvbuf, ids[0], count);
				// leave current position -> finished
				break;
			}
		}

		// delete temporary buffer
		delete [] tempbuf;
	}

	// **************************************************
	// * auxiliary functions for object serialization
	// **************************************************

	// **** Senden und Empfangen von gepackten Daten

	// Pack-Methoden zur Serialisierung
	inline void MSL_Pack(signed short int* val, void* pBuffer, int bufferSize, int* position) {
		MPI_Pack(val, 1, MPI_SHORT, pBuffer, bufferSize, position, MPI_COMM_WORLD);
	}

	inline void MSL_Pack(signed int* val, void* pBuffer, int bufferSize, int* position) {
		MPI_Pack(val, 1, MPI_INT, pBuffer, bufferSize, position, MPI_COMM_WORLD);
	}

	inline void MSL_Pack(signed char* val, void* pBuffer, int bufferSize, int* position) {
		MPI_Pack(val, 1, MPI_CHAR, pBuffer, bufferSize, position, MPI_COMM_WORLD);
	}

	inline void MSL_Pack(signed long int* val, void* pBuffer, int bufferSize, int* position) {
		MPI_Pack(val, 1, MPI_LONG, pBuffer, bufferSize, position, MPI_COMM_WORLD);
	}

	inline void MSL_Pack(unsigned char* val, void* pBuffer, int bufferSize, int* position) {
		MPI_Pack(val, 1, MPI_UNSIGNED_CHAR, pBuffer, bufferSize, position, MPI_COMM_WORLD);
	}

	inline void MSL_Pack(unsigned short* val, void* pBuffer, int bufferSize, int* position) {
		MPI_Pack(val, 1, MPI_UNSIGNED_SHORT, pBuffer, bufferSize, position, MPI_COMM_WORLD);
	}

	inline void MSL_Pack(unsigned int* val, void* pBuffer, int bufferSize, int* position) {
		MPI_Pack(val, 1, MPI_UNSIGNED, pBuffer, bufferSize, position, MPI_COMM_WORLD);
	}

	inline void MSL_Pack(unsigned long int* val, void* pBuffer, int bufferSize, int* position) {
		MPI_Pack(val, 1, MPI_UNSIGNED_LONG, pBuffer, bufferSize, position, MPI_COMM_WORLD);
	}

	inline void MSL_Pack(float* val, void* pBuffer, int bufferSize, int* position) {
		MPI_Pack(val, 1, MPI_FLOAT, pBuffer, bufferSize, position, MPI_COMM_WORLD);
	}

	inline void MSL_Pack(double* val, void* pBuffer, int bufferSize, int* position) {
		MPI_Pack(val, 1, MPI_DOUBLE, pBuffer, bufferSize, position, MPI_COMM_WORLD);
	}

	inline void MSL_Pack(long double* value, void* pBuffer, int bufferSize, int* position) {
		MPI_Pack(value, 1, MPI_LONG_DOUBLE, pBuffer, bufferSize, position, MPI_COMM_WORLD);
	}

	// Unpack-Methoden zur Serialisierung

	inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, signed short int* val) {
		MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_SHORT, MPI_COMM_WORLD);
	}

	inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, signed int* val) {
		MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_INT, MPI_COMM_WORLD);
	}

	inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, signed char* val) {
		MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_CHAR, MPI_COMM_WORLD);
	}

	inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, signed long int* val) {
		MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_LONG, MPI_COMM_WORLD);
	}

	inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, unsigned char* val) {
		MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_UNSIGNED_CHAR, MPI_COMM_WORLD);
	}

	inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, unsigned short* val) {
		MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_UNSIGNED_SHORT, MPI_COMM_WORLD);
	}

	inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, unsigned int* val) {
		MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_UNSIGNED, MPI_COMM_WORLD);
	}

	inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, unsigned long int* val) {
		MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_UNSIGNED_LONG, MPI_COMM_WORLD);
	}

	inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, float* val) {
		MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_FLOAT, MPI_COMM_WORLD);
	}

	inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, double* val) {
		MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_DOUBLE, MPI_COMM_WORLD);
	}

	inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, long double* val) {
		MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_LONG_DOUBLE, MPI_COMM_WORLD);
	}
}


#endif
