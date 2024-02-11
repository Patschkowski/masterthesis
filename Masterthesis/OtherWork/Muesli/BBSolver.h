// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef BB_SOLVER_H
#define BB_SOLVER_H

#include "BBFrameWorkpool.h"
#include "BBProblemTracker.h"
#include "Curry.h"
#include "Muesli.h"
#include "Process.h"

namespace msl {

	/* Die Klasse repraesentiert einen am Branch-and-Bound Prozess beteiligten
	 * Solver. Instanzen werden durch die Klasse BranchAndBound erzeugt.
	 */
	template<class Problem>
	class BBSolver: public Process {

	private:

		// Variablen zur Steuerung des Ablaufs in start()

		// Variablen fuer Kommunikation

		// Eingaenge aller Solver (inkl. this): entranceOfSolver[0] ist Master
		ProcessorNo* entranceOfSolver;
		// Ausgaenge aller Solver (inkl. this): exitOfSolver[0] ist Master
		ProcessorNo* exitOfSolver;
		// Zur Definition der Topologie; zwar kennt bereits jeder jeden (fuer Incumbents und STOPS)
		ProcessorNo* entranceOfWorkmate;
		// aber zur Lastverteiltung besteht eine bestimmte Topologie
		ProcessorNo* exitOfWorkmate;
		// Mastersolver; entspricht entranceOFSolver[0]
		ProcessorNo masterSolver;
		// Vorgaenger merken zum Verschicken des Tokens fuer Termination Detection
		ProcessorNo tokenPredecessor;
		// Nachfolger merken zum Verschicken des Tokens fuer Termination Detection
		ProcessorNo tokenSuccessor;
		// Anzahl der Solver, die zur Verfuegung stehen
		int numOfSolvers;
		// Anzahl an Prozessen
		int noprocs;
		// Anzahl der Partner, mit denen die Lastverteilung durchgefuehrt wird
		int numOfWorkmates;

		// Variablen fuer Problembearbeitung

		// Zeiger auf aktuell beste (globale) Loesung
		Problem* incumbent;
		// lokaler Speicher fuer zu bearbeitende Probleme
		// Speichert Frames (Frames fuer Termination Detection)
		BBFrameWorkpool<Problem>* workpool;
		BBProblemTracker<Problem>* problemTracker;
		// Maximale Anzahl generierter Teilprobleme
		int numOfMaxSubProblems;

		// Benutzerdefinierte Funktionen

		// branch(Fct2<Problem*,int*,Problem**, Problem** (*)(Problem*,int*)>(br)),
		DFct2<Problem*,int*,Problem**> branch;
		// bound(Fct1<Problem*, void, void (*)(Problem*)>(bnd)),
		DFct1<Problem*,void> bound;
		// betterThan(Fct2<Problem*,Problem*,bool, bool (*)(Problem*,Problem*)>(lth)),
		DFct2<Problem*,Problem*,bool> betterThan;
		// isSolution(Fct1<Problem*, bool, bool (*)(Problem*)>(isSol)),
		DFct1<Problem*,bool> isSolution;
		// getLowerBound(Fct1<Problem*, int, int (*)(Problem*)>(getLB)),
		DFct1<Problem*,int> getLowerBound;

	public:

		/* Standardkonstruktor
		 */
		BBSolver(Problem** (*br)(Problem*, int*), void (*bnd)(Problem*), bool (*lth)(Problem*,Problem*),
			bool (*isSol)(Problem*), int (*getLB)(Problem*), int numSub, int n):
		branch(Fct2<Problem*, int*, Problem**, Problem** (*)(Problem*, int*)>(br)),
		bound(Fct1<Problem*, void, void (*)(Problem*)>(bnd)),
		betterThan(Fct2<Problem*, Problem*, bool, bool (*)(Problem*, Problem*)>(lth)),
		isSolution(Fct1<Problem*, bool, bool (*)(Problem*)>(isSol)),
		getLowerBound(Fct1<Problem*, int, int (*)(Problem*)>(getLB)),
		numOfMaxSubProblems(numSub), noprocs(n), Process() {
			// workpool anlegen; muss die betterThan Funktion kennen
			workpool = new BBFrameWorkpool<Problem>(lth);
			numOfEntrances = 1;
			numOfExits = 1;
			problemTracker = new BBProblemTracker<Problem>(numOfMaxSubProblems);
			entrances = new ProcessorNo[numOfEntrances];
			exits = new ProcessorNo[numOfExits];
			// Eingang des Solvers ist der erste Prozessor
			entrances[0] = Muesli::MSL_runningProcessorNo;
			// ebenso der Ausgang
			exits[0] = entrances[0];
			// Ein Solver arbeitet auf n Prozessoren  TODO: nur bei Datenparallel noetig -> Streichen
			Muesli::MSL_runningProcessorNo += n;
			// Default-Receiver ist 0. Den gibt's immer
			setNextReceiver(0);
			processSendTime = 0;
			processRecvTime = 0;
			incumbent = NULL;
		}

		BBSolver(const DFct2<Problem*, int*,Problem**>& br, const DFct1<Problem*, void>& bnd,
		const DFct2<Problem*, Problem*,bool>& lth, const DFct1<Problem*, bool>& isSol,
		const DFct1<Problem*, int>& getLB, int numSub, int n):
		branch(br), bound(bnd), betterThan(lth), isSolution(isSol), getLowerBound(getLB),
		numOfMaxSubProblems(numSub), noprocs(n), Process() {
      		// workpool anlegen; muss die betterThan Funktion kennen
			workpool = new BBFrameWorkpool<Problem>(lth);
			numOfEntrances = 1;
			numOfExits = 1;
			problemTracker = new BBProblemTracker<Problem>(numOfMaxSubProblems);
			entrances = new ProcessorNo[numOfEntrances];
			exits = new ProcessorNo[numOfExits];
			// Eingang des Solvers ist der erste Prozessor
			entrances[0] = Muesli::MSL_runningProcessorNo;
			// ebenso der Ausgang
			exits[0] = entrances[0];
			// Ein Solver arbeitet auf n Prozessoren  TODO: nur bei Datenparallel noetig -> Streichen
			Muesli::MSL_runningProcessorNo += n;
			// Default-Receiver ist 0. Den gibt's immer
			setNextReceiver(0);
			processSendTime = 0;
			processRecvTime = 0;
			incumbent = NULL;
		}

		~BBSolver() {
			delete workpool;
			delete problemTracker;
			delete[] entrances;
			delete[] exits;

			// sind bei ALL-TO-ALL gleich
			if(entranceOfSolver != entranceOfWorkmate)
				delete[] entranceOfWorkmate;

			if(exitOfSolver != exitOfWorkmate)
				delete[] exitOfWorkmate;

			delete[] entranceOfSolver;
			delete[] exitOfSolver;
		}

		/* Startet den Berechnungsprozess des Solvers.
		*/
		void start() {
			finished = ((Muesli::MSL_myId < entrances[0]) ||
				(Muesli::MSL_myId >= entrances[0] + noprocs));
			
			if(finished)
				return;

			Muesli::MSL_numOfLocalProcs = noprocs;
			ProcessorNo masterSolver = entranceOfSolver[0];
			Muesli::MSL_myEntrance = entrances[0];
			Muesli::MSL_myExit = exits[0];

			// Statistikvariablen initialisieren
			statistics stat;

			stat.statNumMsgProblemSolvedSent = 0;
			stat.statNumMsgProblemSolvedReceived = 0;
			stat.statNumMsgBoundInfoSent = 0;
			stat.statNumMsgBoundInfoReceived = 0;
			stat.statNumMsgWorkPoolEmptySent = 0;
			stat.statNumMsgWorkPoolEmptyRejectionReceived = 0;
			stat.statNumMsgBoundRejectionSent = 0;
			stat.statNumMsgBoundRejectionReceived = 0;
			stat.statNumProblemsSent = 0;
			stat.statNumProblemsReceived = 0;
			stat.statNumProblemsSolved = 0;
			stat.statNumIncumbentReceivedAccepted = 0;
			stat.statNumIncumbentReceivedDiscarded = 0;
			stat.statNumIncumbentSent = 0;
			stat.statNumProblemsBranched = 0;
			stat.statNumProblemsBounded = 0;
			stat.statNumSolutionsFound = 0;
			stat.statNumProblemsTrackedTotal = 0;
			stat.statNumProblemsKilled = 0;
			stat.statTimeProblemProcessing = 0;
			stat.statTimeCommunication = 0;
			stat.statTimeIncumbentHandling = 0;
			stat.statTimeLoadBalancing = 0;
			stat.statTimeTrackerSolvedProblemsReceived = 0;
			stat.statTimeTrackerSolvedProblemsSent = 0;
			stat.statTimeCleanWorkpool = 0;
			stat.statTimeSubProblemSolvedInsert = 0;
			stat.statTimeIdle = 0;
			stat.statTimeInitialIdle = 0;
			stat.statTimeTotal = 0;
			stat.statTimeSinceWorkpoolClean = 0;
			stat.problemTrackerMaxLength = 0;
			stat.problemTrackerAverageLength = 0;
			stat.solvedProblemsQueueMaxLength = 0;
			stat.polvedProblemsQueueAverageLength = 0;
			stat.workpoolMaxLength = 0;
			stat.workpoolAverageLength = 0;

			double statStart = 0;
			double statStartTimeIdle = 0;
			double statStartTimeCommunication = 0;
			double statStartTimeProcessing = 0;

			// Variablen zur Steuerung der Debug Ausgaben
			bool debugMaster = false;
			bool debugCommunication = false;
			bool debugComputation = false;
			bool debugLoadBalancing = false;
			bool debugTermination = false;

			bool analysis = false;

			// Variablen zur Steuerung des Ablaufs

			// falls STOP oder Problem eingeht wird Blockade aufgehoben
			bool masterBlocked = false;
			// Zeigt an, ob bereits eine gueltige Loesung ermittelt wurde
			bool solutionFound = false;
			// damit ein Workrequest bei leerem workpool nur einmal rausgeht
			bool sentBoundInfo = false;
			// Wird auf true gesetzt, wenn eine Sendeanfrage fuer ein Problem
			// (fuer Lastverteilung) gesendet wurde
			bool sentProblemSendRequest = false;
			// wird auf true gesetzt, wenn eine Sendeanfrage fuer ein Incumbent gesendet wurde
			bool sentIncumbentSendRequest = false;
			// wird auf true gesetzt, wenn ein neues Incumbent gefunden wurde
			bool newIncumbentFound = false;
			// Anzahl der Incumbent Nachrichten an die Partner, die gesendet wurden
			int numOfIncumbentMessagesSent(0);
			finished = false;
			masterBlocked = false;

			// Variablen zur Problemverwaltung

			// Zeiger auf aktuell bearbeitetes Problem
			Problem* problem = NULL;
			// Zeiger auf auf das Frame, mit dem ein Problem verschickt wird
			BBFrame<Problem>* problemFrame = NULL;
			BBFrame<Problem>* loadBalanceProblemFrame = NULL;
			// Array ID des Senders der Lastverteilungsanfrage
			int senderOfLoadBalance = 0;
			// Array ID des Empfaengers der letzten Lastverteilungsanfrage
			int receiverOfBoundInfo = 0;

			// Variablen zur MPI Steuerung

			// Flag zum Testen auf MPI Nachrichteneingang
			int messageWaiting = 0;
			// Status der MPI Nachricht bei Empfang einer Nachricht
			MPI_Status messageStatus;
			// zaehlt Anzahl Empfaenger STOP Nachrichten von Vorgaengern
			int receivedStops = 0;
			// Zaehler ueber das predecessors-Array
			int predecessorIndex = 0;

			// Statistik
			stat.statTimeTotal = MPI_Wtime();
			stat.statTimeInitialIdle = MPI_Wtime();

			// Hauptschleife des Solvers. Laeuft solange, bis eine STOP Nachricht empfangen wird
			while(!finished) {
				// Aufgaben des Master Solvers erledigen
				if(Muesli::MSL_myId == masterSolver) {
					if(debugMaster)
						std::cout << Muesli::MSL_myId <<
						": Mastersolver betritt die Buehne" << std::endl;
					
					// Masterempfang regeln (Eintritt in Problemverarbeitung)
					while(!masterBlocked && !finished) {
						// warte auf Nachricht von Vorgaengern -> blockierend!
						if(debugMaster)
							std::cout << Muesli::MSL_myId <<
							": Mastersolver wartet auf neues Problem" << std::endl;
						
						messageWaiting = 0;

						while(!messageWaiting) {
							MPI_Iprobe(predecessors[predecessorIndex], MPI_ANY_TAG, MPI_COMM_WORLD,
								&messageWaiting, &messageStatus);
							predecessorIndex = (predecessorIndex + 1) % numOfPredecessors;
						}

						// Verarbeite Nachricht (Tag oder Problem)
						// TERMINATION_TEST
						ProcessorNo source = messageStatus.MPI_SOURCE;

						if(messageStatus.MPI_TAG == MSLT_TERMINATION_TEST) {
							MSL_ReceiveTag(source, MSLT_TERMINATION_TEST);
						}
						// STOP Tag
						else if(messageStatus.MPI_TAG == MSLT_STOP) {
							// entgegennehmen
							MSL_ReceiveTag(source, MSLT_STOP);

							if(debugMaster)
								std::cout << Muesli::MSL_myId <<
								": Mastersolver empfaengt STOP" << std::endl;
							
							// muss von allen Vorgaengern kommen
							receivedStops++;
							
							if(receivedStops == numOfPredecessors) {
								// benachrichtige Solverkollegen
								if(numOfSolvers > 1) {
									for(int i = 0; i < numOfSolvers; i++) {
										// sich selbst schicken wuerde Absturz bewirken
										if(entranceOfSolver[i] != Muesli::MSL_myId) {
											MSL_SendTag(entranceOfSolver[i], MSLT_STOP);

											if(debugMaster)
												std::cout << Muesli::MSL_myId <<
												": Mastersolver sendet STOP an Solver " <<
												entranceOfSolver[i] << std::endl;
										}
									}
								}

								// benachrichtige Nachfolger
								for(int i = 0; i < numOfSuccessors; i++) {
									if(debugMaster)
										std::cout << Muesli::MSL_myId <<
										": Mastersolver sendet STOP an Nachfolger " <<
										successors[i] << std::endl;
									
									MSL_SendTag(successors[i], MSLT_STOP);
								}

								// zuruecksetzen
								receivedStops = 0;
								// nichts mehr annehmen
								masterBlocked = true;
								finished = true;
							}
						}

						// jetzt kann hoechstens noch ein Problem angekommen sein
						else {
							// Speicher reservieren
							problem = new Problem();
							MSL_Receive(source, problem, MSLT_MYTAG, &messageStatus);
							
							if(debugMaster)
								std::cout << Muesli::MSL_myId <<
								": Mastersolver hat ein neues Problem empfangen" << std::endl;

							stat.statNumProblemsReceived++;
							stat.statTimeInitialIdle = MPI_Wtime() - stat.statTimeInitialIdle;
							// Problem abschaetzen
							bound(problem);

							// Pruefe, ob Problem direkt loesbar ist
							if(!isSolution(problem)) {
								// nichts neues mehr annehmen
								masterBlocked = true;
								problemFrame = new BBFrame<Problem>(0, NULL, masterSolver, -1, problem);
								// ab in den Workpool
								workpool->insert(problemFrame);
								
								if(debugMaster)
									std::cout << Muesli::MSL_myId <<
									": Mastersolver hat ein neues Problem in den Workpool gelegt" <<
									std::endl;
							}
							// schon fertig -> sende an Nachfolger
							else {
								MSL_Send(getReceiver(), problem);
								delete problem;
								problem = NULL;
							}
						}
					}
					// Ende der Empfangsschleife des Masters
				}

				// Solver Kommunikation
				if((numOfSolvers > 1) && !finished && (Muesli::MSL_myId == Muesli::MSL_myEntrance)) {
					if(debugCommunication)
						std::cout << Muesli::MSL_myId <<
						": Solver beginnt Kommunikationsphase" << std::endl;
					
					statStartTimeCommunication = MPI_Wtime();
					// 1. Neues Incumbent zu versenden?
					statStart = MPI_Wtime();

					if(newIncumbentFound && !sentIncumbentSendRequest) {
						if(debugCommunication)
							std::cout << Muesli::MSL_myId <<
							": Solver beginnt mit Versand des neuen Incumbent" << std::endl;
						
						numOfIncumbentMessagesSent = 0;

						for(int i = 0; i < numOfSolvers; i++) {
							if(Muesli::MSL_myId == entranceOfSolver[i])
								continue;
							else if(Muesli::MSL_myId < entranceOfSolver[i]) {
								MSL_Send(entranceOfSolver[i], incumbent, MSLT_BB_INCUMBENT);
								numOfIncumbentMessagesSent++;

								if(debugCommunication)
									std::cout << Muesli::MSL_myId << ": Incumbent an Prozessor " <<
									entranceOfSolver[i] <<"versendet" << std::endl;
							}
							else {
								MSL_SendTag(entranceOfSolver[i], MSLT_BB_INCUMBENT_SENDREQUEST);

								if(debugCommunication)
									std::cout << Muesli::MSL_myId <<
									": Incumbent SendRequest an Prozessor " <<
									entranceOfSolver[i] <<"versendet" << std::endl;

								sentIncumbentSendRequest = true;
							}
						}

						// jetzt zuruecknehmen, weiteres ueber sentIncumbentSendRequest
						newIncumbentFound = false;
						stat.statNumIncumbentSent++;
					}
					// nun sind alle versandt oder SendRequests wurden verschickt
					
					// Pruefen, ob SendRequest beantwortet wurde
					if(sentIncumbentSendRequest) {
						if(debugCommunication)
							std::cout << Muesli::MSL_myId <<
							": Solver prueft, ob IncumbentSendRequest beantwortet wurde" << std::endl;
						
						for(int i = 0; i < numOfSolvers; i++) {
							MPI_Iprobe(exitOfSolver[i], MSLT_BB_INCUMBENT_READYSIGNAL,
								MPI_COMM_WORLD, &messageWaiting, &messageStatus);
							
							if(messageWaiting) {
								if(debugCommunication)
									std::cout << Muesli::MSL_myId <<
									": IncumbentReadySignal von Prozessor " <<
									exitOfSolver[i] << "empfangen" << std::endl;

								MSL_ReceiveTag(exitOfSolver[i], MSLT_BB_INCUMBENT_READYSIGNAL);
								MSL_Send(entranceOfSolver[i], incumbent, MSLT_BB_INCUMBENT);
								
								if(debugCommunication)
									std::cout << Muesli::MSL_myId << ": Incumbent an Prozessor " <<
									entranceOfSolver[i] <<"versendet" << std::endl;
								
								numOfIncumbentMessagesSent++;
							}
						}

						// nicht an sich selbst geschickt -> 1 weniger
						if(numOfIncumbentMessagesSent >= numOfSolvers - 1) {
							sentIncumbentSendRequest = false;
						}
					}
					// 1. Ende: Neues Incumbent versenden

					// 2. Neue Incumbents angekommen?
					if(debugCommunication)
						std::cout << Muesli::MSL_myId <<
						": Solver prueft, ob neues Incumbent eingegangen ist" << std::endl;

					for(int i = 0; i < numOfSolvers; i++) {
						bool incumbentReceived = false;
						// Solver mit kleinerer ProzessorID duerfen
						// direkt senden -> pruefen, ob eingegangen
						MPI_Iprobe(exitOfSolver[i], MSLT_BB_INCUMBENT, MPI_COMM_WORLD,
							&messageWaiting, &messageStatus);
						
						// Nachricht eingegangen
						if(messageWaiting) {
							problem = new Problem();
							MSL_Receive(exitOfSolver[i], problem, MSLT_BB_INCUMBENT, &messageStatus);
							
							if(debugCommunication)
								std::cout << Muesli::MSL_myId << ": Neues Incumbent von Prozessor " <<
								exitOfSolver[i] <<"empfangen. LB: "<< getLowerBound(problem) <<
								std::endl;

							incumbentReceived = true;
						}

						// Solver mit groeßerer ID senden erst Sendeanfrage!
						MPI_Iprobe(exitOfSolver[i], MSLT_BB_INCUMBENT_SENDREQUEST, MPI_COMM_WORLD,
							&messageWaiting, &messageStatus);
						
						if(messageWaiting) {
							if(debugCommunication)
								std::cout << Muesli::MSL_myId <<
								": IncumbentSendrequest von Prozessor " <<
								exitOfSolver[i] <<"empfangen" << std::endl;
							
							problem = new Problem();
							// Tag holen
							MSL_ReceiveTag(exitOfSolver[i], MSLT_BB_INCUMBENT_SENDREQUEST);
							
							if(debugCommunication)
								std::cout << Muesli::MSL_myId <<
								": IncumbentSendrequest von Prozessor " <<
								exitOfSolver[i] <<" abgeholt" << std::endl;

							// Gegenueber darf jetzt senden
							MSL_SendTag(entranceOfSolver[i], MSLT_BB_INCUMBENT_READYSIGNAL);
							
							if(debugCommunication)
								std::cout << Muesli::MSL_myId << ": IncumbentReadySignal an " <<
								exitOfSolver[i] <<" gesendet " << std::endl;

							MSL_Receive(exitOfSolver[i], problem, MSLT_BB_INCUMBENT, &messageStatus);
							
							if(debugCommunication) std::cout << Muesli::MSL_myId <<
								": Neues Incumbent von Prozessor " << exitOfSolver[i] <<
								"empfangen. LB: "<< getLowerBound(problem) << std::endl;

							incumbentReceived = true;
						}

						// Empfangenes Incumbent verarbeiten
						if(incumbentReceived) {
							// Empfangene Loesung besser als bisherige?
							if(!solutionFound || betterThan(problem, incumbent)) {
								if(incumbent != NULL)
									delete incumbent;

								// pool beschneiden spaeter
								incumbent = problem;
								stat.statNumIncumbentReceivedAccepted++;
								solutionFound = true;

								if(debugCommunication)
									std::cout << Muesli::MSL_myId <<
									": Neues Incumbent gesetzt" << std::endl;
							}
							else {
								if(debugCommunication)
									std::cout << Muesli::MSL_myId <<
									": Empfangenes Incumbent von Prozessor " <<
									exitOfSolver[i] <<"schlechter als aktuelles mit LB: "<<
									getLowerBound(incumbent) << std::endl;

								delete problem;
								stat.statNumIncumbentReceivedDiscarded++;
							}

							incumbentReceived = false;
						}
					}
					// Ende 2. Incumbent eingegangen

					stat.statTimeIncumbentHandling += MPI_Wtime() - statStart;
					statStart = MPI_Wtime();

					// 3. Nachrichten ueber geloeste Probleme eingegangen?
					// muss nicht ueber Handshake erfolgen, da nur kleine
					// Daten versandt werden (24 Byte)
					for(int i = 0; i < numOfSolvers; i++) {
						if(debugTermination)
							std::cout << Muesli::MSL_myId <<
							": Pruefe, ob Problemloesungsnachricht eingegangen ist" << std::endl;

						MPI_Iprobe(exitOfSolver[i], MSLT_BB_PROBLEM_SOLVED, MPI_COMM_WORLD,
							&messageWaiting, &messageStatus);
						
						if(messageWaiting) {
							problemFrame = new BBFrame<Problem>();
							MSL_Receive(exitOfSolver[i], problemFrame, MSLT_BB_PROBLEM_SOLVED,
								&messageStatus);
							
							if(debugTermination)
								std::cout << Muesli::MSL_myId << ": Geloestes Problem (ID: " <<
								problemFrame->getID() << ") von Prozessor " << exitOfSolver[i] <<
								" empfangen" << std::endl;
							
							double startTime = MPI_Wtime();
							problemTracker->problemSolved(problemFrame);

							stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
							stat.statNumMsgProblemSolvedReceived++;
							stat.statNumProblemsSolved++;
						}
					}
					// Ende 3. Nachrichten ueber geloeste Probleme eingegangen
					
					stat.statTimeTrackerSolvedProblemsReceived += MPI_Wtime() - statStart;

					// 4. Geloeste Probleme verschicken
					statStart = MPI_Wtime();
					while(!problemTracker->isSolvedQueueEmpty()) {
						problemFrame = problemTracker->readFromSolvedQueue();
						MSL_Send(problemFrame->getOriginator(), problemFrame, MSLT_BB_PROBLEM_SOLVED);
						
						if(debugTermination)
							std::cout << Muesli::MSL_myId << ": Geloestes Problem (ID: " <<
							problemFrame->getID() << ") an Prozessor " <<
							problemFrame->getOriginator() <<" gesendet" << std::endl;

						stat.statNumMsgProblemSolvedSent++;
						problemTracker->removeFromSolvedQueue();
					}
					// 4. Ende Geloeste Probleme verschicken
					
					stat.statTimeTrackerSolvedProblemsSent += MPI_Wtime() - statStart;

					// 5. Lastverteilungsanfragen senden
					statStart = MPI_Wtime();

					// immer nur eine Anfrage auf einmal raus, um das Netz nicht zu
					// ueberfluten, diese wird entweder beantwortet oder abgelehnt
					if(!sentBoundInfo)	{
						// erst Incumbent raus
						if(!sentIncumbentSendRequest) {
							if(debugLoadBalancing)
								std::cout << Muesli::MSL_myId <<
								": BoundInfo senden beginnen" << std::endl;

							// leer, sende, falls noch keine Anfrage raus ist
							if(workpool->isEmpty())	{
								int bound = 2147483647;

								receiverOfBoundInfo = rand() % numOfWorkmates;

								while(entranceOfWorkmate[receiverOfBoundInfo] == Muesli::MSL_myId)
									receiverOfBoundInfo = rand() % numOfWorkmates;

								if(debugLoadBalancing)
									std::cout << Muesli::MSL_myId <<
									": Workpool leer, sende kleinsmoegliche BoundInfo an " <<
									entranceOfWorkmate[receiverOfBoundInfo] << std::endl;

								MSL_Send(entranceOfWorkmate[receiverOfBoundInfo], &bound,
									MSLT_BB_LOADBALANCE);
								// vorerst keine weitere Anfrage
								sentBoundInfo = true;
								statStartTimeIdle = MPI_Wtime();
								stat.statNumMsgWorkPoolEmptySent++;
							}
							// Pool nicht (mehr) leer -> sende trotzdem mit gewisser WS
							// eine Bound; dies fuehrt ggf. zu einem work request
							else {
								// erzeugt Zufallszahl zwischen 1 und 100.
								int random = rand() % 100 + 1;

								// verrate LB nur mit einer bestimmten WS,
								// diese per Kommandozeile uebergeben
								if(random <= Muesli::MSL_ARG2) {
									int bound = getLowerBound(workpool->top()->getData());
									receiverOfBoundInfo = rand() % numOfWorkmates;

									while(entranceOfWorkmate[receiverOfBoundInfo] == Muesli::MSL_myId)
										receiverOfBoundInfo = rand() % numOfWorkmates;

									// wrkpool->top() ist bestes Teilproblem
									MSL_Send(entranceOfWorkmate[receiverOfBoundInfo], &bound,
										MSLT_BB_LOADBALANCE);
									stat.statNumMsgBoundInfoSent++;
									sentBoundInfo = true;

									if(debugLoadBalancing)
										std::cout << Muesli::MSL_myId <<
										": Loadbalance, sende lowerbound " <<
										bound << " an Prozessor " <<
										entranceOfWorkmate[receiverOfBoundInfo]  << std::endl;
								}
							}
						}
					}
					// Anfrage ist schon raus, pruefe ob Antwort vorliegt
					else {
						bool loadReceived = false;

						if(debugLoadBalancing)
							std::cout << Muesli::MSL_myId <<
							": Loadbalance: Pruefe, ob Antwort von Prozessor " <<
							exitOfWorkmate[receiverOfBoundInfo] << " vorliegt"  << std::endl;

						MPI_Iprobe(exitOfWorkmate[receiverOfBoundInfo], MSLT_BB_LOADBALANCE_REJECTION,
							MPI_COMM_WORLD, &messageWaiting, &messageStatus);
						
						// wurde nicht angenommen, weil a) Pool leer b) bessere untere Schranke
						if(messageWaiting) {

							sentBoundInfo = false;
							MSL_ReceiveTag(exitOfWorkmate[receiverOfBoundInfo],
								MSLT_BB_LOADBALANCE_REJECTION);

							if(debugLoadBalancing)
								std::cout << Muesli::MSL_myId <<
								": Loadbalance: Absage von Prozessor " <<
								exitOfWorkmate[receiverOfBoundInfo] << " empfangen"  << std::endl;

							if(workpool->isEmpty()) {
								stat.statTimeIdle += MPI_Wtime() - statStartTimeIdle;
								stat.statNumMsgWorkPoolEmptyRejectionReceived++;
							}
							else
								stat.statNumMsgBoundRejectionReceived++;
						}

						// Solver mit kleinerer ID senden direkt
						MPI_Iprobe(exitOfWorkmate[receiverOfBoundInfo], MSLT_BB_PROBLEM,
							MPI_COMM_WORLD, &messageWaiting, &messageStatus);

						if(messageWaiting) {
							problemFrame = new BBFrame<Problem>();
							MSL_Receive(exitOfWorkmate[receiverOfBoundInfo], problemFrame,
								MSLT_BB_PROBLEM, &messageStatus);
							
							if(debugLoadBalancing)
								std::cout << Muesli::MSL_myId <<
								": Loadbalance: Problem " << problemFrame->getID() <<
								" von Prozessor " << exitOfWorkmate[receiverOfBoundInfo] <<
								" empfangen" << std::endl;

							loadReceived = true;
						}

						// Solver mit groeßerer ID senden erst eine Sendeanfrage
						MPI_Iprobe(exitOfWorkmate[receiverOfBoundInfo], MSLT_BB_PROBLEM_SENDREQUEST,
							MPI_COMM_WORLD, &messageWaiting, &messageStatus);

						if(messageWaiting) {
							problemFrame = new BBFrame<Problem>();

							if(debugLoadBalancing)
								std::cout << Muesli::MSL_myId <<
								": Loadbalance: ProblemSendRequest von Prozessor " <<
								exitOfWorkmate[receiverOfBoundInfo] << " empfangen"  << std::endl;

							MSL_ReceiveTag(exitOfWorkmate[receiverOfBoundInfo],
								MSLT_BB_PROBLEM_SENDREQUEST);
							MSL_SendTag(entranceOfWorkmate[receiverOfBoundInfo],
								MSLT_BB_PROBLEM_READYSIGNAL);
							
							if(debugLoadBalancing)
								std::cout << Muesli::MSL_myId <<
								": Loadbalance: ProblemReadySignal an " <<
								entranceOfWorkmate[receiverOfBoundInfo] << " geschickt"  << std::endl;

							MSL_Receive(exitOfWorkmate[receiverOfBoundInfo], problemFrame,
								MSLT_BB_PROBLEM, &messageStatus);
							
							if(debugLoadBalancing)
								std::cout << Muesli::MSL_myId << ": Loadbalance: Problem " <<
								problemFrame->getID() << " von Prozessor " <<
								exitOfWorkmate[receiverOfBoundInfo] << " empfangen"  << std::endl;

							loadReceived = true;
						}

						// Wenn Nachricht empfangen, diese Verarbeiten
						if(loadReceived) {
							if(stat.statNumProblemsReceived==0)
								stat.statTimeInitialIdle = MPI_Wtime() - stat.statTimeInitialIdle;
							else
								if(workpool->isEmpty())
									stat.statTimeIdle += MPI_Wtime() - statStartTimeIdle;
							
							// nur akzeptieren, wenn theoretisch besser als
							// Incumbent --> (falls zwischendurch eines empfangen wurde)
							problem = problemFrame->getData();
							
							if(!solutionFound || betterThan(problem, incumbent)) {
								workpool->insert(problemFrame);
								
								if(debugLoadBalancing)
									std::cout << Muesli::MSL_myId <<
									": Loadbalance: Problem in Workpool eingefuegt" << std::endl;
							}
							// Problem verwerfen -> also geloest!
							else {
								double startTime = MPI_Wtime();
								// Tracker kuemmert sich darum, dass es in die SendQueue kommt
								problemTracker->problemSolved(problemFrame);
								stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
								
								if(debugLoadBalancing)
									std::cout << Muesli::MSL_myId <<
									": Loadbalance: Problem verworfen. Absender " <<
									exitOfWorkmate[receiverOfBoundInfo] <<
									" benachrichtigt" << std::endl;

								// Tracker loescht Frame
								delete problem;
							}

							sentBoundInfo = false;
							loadReceived = false;
							stat.statNumProblemsReceived++;
						}
					}
					// Ende 5. Lastverteilungsanfragen senden

					// 6. Lastverteilungsanfragen empfangen Teil 1: Anwort
					// bei leerem Workpool; wird sonst unten uebersprungen!
					if(workpool->isEmpty()) {
						if(debugLoadBalancing)
							std::cout << Muesli::MSL_myId <<
							": Workpool leer; prueft, ob LoadBalanceanfrage vorliegen, " \
							"die abgesagt werden muessen" << std::endl;

						for(int i = 0; i < numOfWorkmates; i++) {
							MPI_Iprobe(exitOfWorkmate[i], MSLT_BB_LOADBALANCE, MPI_COMM_WORLD,
								&messageWaiting, &messageStatus);
							
							if(debugLoadBalancing)
								std::cout << Muesli::MSL_myId <<
								": Pruefung, ob Loadbalanceanfrage von Prozessor " <<
								entranceOfSolver[i] << " vorliegt durchgefuehrt: " <<
								messageWaiting << std::endl;

							if(messageWaiting) {
								int loadInfo;

								if(debugLoadBalancing)
									std::cout << Muesli::MSL_myId <<
									": Loadbalance Anfrage von Prozessor" << exitOfWorkmate[i] <<
									" liegt vor"  << std::endl;

								MSL_Receive(exitOfWorkmate[i], &loadInfo, MSLT_BB_LOADBALANCE,
									&messageStatus);
								stat.statNumMsgBoundInfoReceived++;

								if(debugLoadBalancing)
									std::cout << Muesli::MSL_myId <<
									": LoadInfo von Prozessor " << exitOfWorkmate[i] <<
									" angenommen: loadInfo " << loadInfo << std::endl;

								MSL_SendTag(entranceOfWorkmate[i], MSLT_BB_LOADBALANCE_REJECTION);
								stat.statNumMsgBoundRejectionSent++;
								
								if(debugLoadBalancing)
									std::cout << Muesli::MSL_myId <<
									": Workpool leer: LoadBalanceRejection an " <<
									entranceOfWorkmate[i] << " schicken" << std::endl;
							}
						}
					}

					// muss auf jeden Fall verschickt werden, damit Termination detection funktioniert
					if(sentProblemSendRequest) {
						if(debugLoadBalancing)
							std::cout << Muesli::MSL_myId <<
							": prueft, ob readysignal vorliegt" << std::endl;

						MPI_Iprobe(entranceOfWorkmate[senderOfLoadBalance], MSLT_BB_PROBLEM_READYSIGNAL,
							MPI_COMM_WORLD, &messageWaiting, &messageStatus);

						if(messageWaiting) {
							MSL_ReceiveTag(entranceOfWorkmate[senderOfLoadBalance],
								MSLT_BB_PROBLEM_READYSIGNAL);
							
							if(debugLoadBalancing)
								std::cout << Muesli::MSL_myId <<
								": LoadInfo ProblemReadySignal angenommen: Problem verschickt an " <<
								entranceOfSolver[senderOfLoadBalance] << std::endl;

							MSL_Send(entranceOfWorkmate[senderOfLoadBalance],
								loadBalanceProblemFrame, MSLT_BB_PROBLEM);
							stat.statNumProblemsSent++;
							sentProblemSendRequest = false;

							delete loadBalanceProblemFrame->getData();
							delete loadBalanceProblemFrame;
						}
					}
					// Ende 6. Lastverteilungsanfragen empfangen
					
					stat.statTimeLoadBalancing += MPI_Wtime() - statStart;

					// 6. STOP Nachrichten empfangen
					MPI_Iprobe(masterSolver, MSLT_STOP, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
					
					if(messageWaiting) {
						MSL_ReceiveTag(masterSolver, MSLT_STOP);
						finished = true;
					
						if(debugCommunication)
							std::cout << Muesli::MSL_myId << ": STOP: empfangen " << std::endl;
					}
					// Ende 6. STOP Nachrichten empfangen
					
					stat.statTimeCommunication += MPI_Wtime() - statStartTimeCommunication;
				}
				// Ende Solverinterne Kommunikation

				// Beginn Problemverarbeitung / Lastverteilungsanfragenbearbeitung
				if(!finished && !workpool->isEmpty()) {
					if(debugComputation)
						std::cout << Muesli::MSL_myId << ": beginnt Verarbeitungsphase" << std::endl;

					BBFrame<Problem>* workingProblemFrame = workpool->get();
					Problem* workingProblem = workingProblemFrame->getData();
					
					if(debugComputation)
						std::cout << Muesli::MSL_myId <<
						": Problem aus dem Workpool entnommen" << std::endl;

					// eingehende Workrequests bearbeiten. wenn andere Solver vorhanden und der Pool
					// nicht leer ist, dann kann Lastverteilung durchgefuehrt werden und auch erst,
					// wenn kein Incumbent unterwegs ist
					statStart = MPI_Wtime();
					statStartTimeCommunication = MPI_Wtime();

					if(numOfSolvers > 1 && !sentProblemSendRequest && !sentIncumbentSendRequest) {
						if(debugLoadBalancing)
							std::cout << Muesli::MSL_myId <<
							": geht in Loadbalanceantwortphase" << std::endl;

						if(debugLoadBalancing)
							std::cout << Muesli::MSL_myId <<
							": prueft, ob LoadBalanceanfrage vorliegt" << std::endl;

						int loadInfo;
						messageWaiting = 0;
						// maximal ein kompletter Durchlauf, sonst dauert
						// es zu lang, falls die Austausch-WS zu gering ist
						int check = senderOfLoadBalance;
						
						do {
							// senderOfLoadBalance wurde zuletzt geprueft,
							// fange also mit dem naechsten an!
							senderOfLoadBalance = (senderOfLoadBalance + 1) % numOfWorkmates;
							MPI_Iprobe(exitOfWorkmate[senderOfLoadBalance], MSLT_BB_LOADBALANCE,
								MPI_COMM_WORLD, &messageWaiting, &messageStatus);
						}
						while(!messageWaiting && senderOfLoadBalance!=check);

						if(messageWaiting) {
							MSL_Receive(exitOfWorkmate[senderOfLoadBalance], &loadInfo,
								MSLT_BB_LOADBALANCE, &messageStatus);
							stat.statNumMsgBoundInfoReceived++;

							if(debugLoadBalancing)
								std::cout << Muesli::MSL_myId <<
								": LoadInfo von Prozessor" <<
								entranceOfWorkmate[senderOfLoadBalance] <<
								"ist " << loadInfo << std::endl;

							// nichts zu verschicken, sende Rejection Message
							if(workpool->isEmpty()) {
								MSL_SendTag(entranceOfWorkmate[senderOfLoadBalance],
									MSLT_BB_LOADBALANCE_REJECTION);
								stat.statNumMsgBoundRejectionSent++;

								if(debugLoadBalancing)
									std::cout << Muesli::MSL_myId <<
									": Workpool leer: LoadBalanceRejection an " <<
									exitOfWorkmate[senderOfLoadBalance] << " schicken" << std::endl;
							}
							// Mit Problem antworten
							else {
								// Dann vergleichen mit aktuell zweitbester
								int currentBestBound = getLowerBound(workpool->top()->getData());
								
								// dann zweitbestes Problem verschicken ->
								// unterstuetzt schnelles finden guter Loesungen
								if(currentBestBound < loadInfo) {
									// alternativ: loesung in einen eigenen Puffer
									// wegschreiben, der in der Kommunikationsphase geleert wird!
									// muss nun auf JEDEN FALL verschickt oder "geloest" werden
									loadBalanceProblemFrame = workpool->get();
									Problem* load = loadBalanceProblemFrame->getData();

									// nur dann ueberhaupt verschicken, wenn
									// moeglicherweise besser als das Incumbent
									if(!solutionFound || betterThan(load, incumbent)) {
										if(debugLoadBalancing)
											std::cout << Muesli::MSL_myId <<
											": Load wird an Prozessor" <<
											entranceOfWorkmate[senderOfLoadBalance] <<
											"gesendet " << std::endl;
										
										// Wenn ID kleiner als die des Empfaengers,
										// dann direkt verschicken
										if(Muesli::MSL_myId < entranceOfWorkmate[senderOfLoadBalance]) {
											if(debugLoadBalancing)
												std::cout << Muesli::MSL_myId <<
												": Load direkt an Prozessor " <<
												entranceOfWorkmate[senderOfLoadBalance] <<
												" senden" << std::endl;

											MSL_Send(entranceOfWorkmate[senderOfLoadBalance],
												loadBalanceProblemFrame, MSLT_BB_PROBLEM);
											stat.statNumProblemsSent++;

											if(debugLoadBalancing)
												std::cout << Muesli::MSL_myId <<
												": LoadInfo angenommen: Problem verschickt an " <<
												entranceOfWorkmate[senderOfLoadBalance] << std::endl;

											delete load;
											delete loadBalanceProblemFrame;
											loadBalanceProblemFrame = NULL;
										}
										else {
											MSL_SendTag(entranceOfWorkmate[senderOfLoadBalance],
												MSLT_BB_PROBLEM_SENDREQUEST);
											
											if(debugLoadBalancing)
												std::cout << Muesli::MSL_myId <<
												": LoadInfo angenommen: ProblemSendRequest " \
												"verschickt an " <<
												entranceOfWorkmate[senderOfLoadBalance] << std::endl;

											sentProblemSendRequest = true;
										}
									}
									// Absage verschicken, weil problem gar nicht verschickt werden
									// muss, da schlechter als Incumbent (nur wenn zwischenzeitlich
									// incumbent eingegangen und pool nicht bereinigt)
									else {
										if(debugLoadBalancing)
											std::cout << Muesli::MSL_myId <<
											": Load schlechter als Incumbent!: " \
											"LoadBalanceRejection an " <<
											entranceOfWorkmate[senderOfLoadBalance] <<
											" schicken" << std::endl;

										MSL_SendTag(entranceOfWorkmate[senderOfLoadBalance],
											MSLT_BB_LOADBALANCE_REJECTION);
										stat.statNumMsgBoundRejectionSent++;
										// Tracker loescht Frame
										delete load;
										double startTime = MPI_Wtime();
										problemTracker->problemSolved(loadBalanceProblemFrame);
										stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
									}
								}
								else {
									if(debugLoadBalancing)
										std::cout << Muesli::MSL_myId <<
										": LoadInfo schlechter als zweitbestes Problem im Workpool!" \
										" LoadBalanceRejection an " <<
										entranceOfSolver[senderOfLoadBalance] <<
										" schicken" << std::endl;

									MSL_SendTag(entranceOfWorkmate[senderOfLoadBalance],
										MSLT_BB_LOADBALANCE_REJECTION);
									stat.statNumMsgBoundRejectionSent++;
								}
							}
						}
					}
					// Ende Lastverteilung

					stat.statTimeLoadBalancing += MPI_Wtime() - statStart;
					stat.statTimeCommunication += MPI_Wtime() - statStartTimeCommunication;

					if(debugComputation)
						std::cout << Muesli::MSL_myId <<
						": Beginne aktuelles Problem zu pruefen" << std::endl;

					// test if the best local problem may lead to a new incumbent.
					if(solutionFound && betterThan(incumbent, workingProblem)) {
						// No local problem can lead to a new incumbent ->
						// discard all local problems from the work pool
						statStart = MPI_Wtime();

						if(debugTermination)
							std::cout << Muesli::MSL_myId <<
							": Incumbent besser als alle Subprobleme!" << std::endl;

						double startTime;
						
						if(!workpool->isEmpty()) {
							if(debugTermination)
								std::cout << Muesli::MSL_myId <<
								": Pool leeren, da Incumbent besser als alle Subprobleme!"<< std::endl;

							// alle Probleme im Workpool werden geloescht und sind damit "geloest"
							// daher muessen fuer diese Probleme die entsprechenden Vaterprobleme
							// benachrichtigt werden moeglich, da zu diesem Zeitpunkt alle Probleme
							// im Workpool ungeloeste Subprobleme sind
							while(!workpool->isEmpty()) {
								problemFrame = workpool->get();
								// Tracker loescht Frame
								delete problemFrame->getData();
								startTime = MPI_Wtime();
								problemTracker->problemSolved(problemFrame);
								stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
								stat.statNumProblemsSolved++;
							}
							workpool->reset();
						}
						// aktuelles Problem ist ebenfalls geloest!
						startTime = MPI_Wtime();
						problemTracker->problemSolved(workingProblemFrame);
						stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
						delete workingProblem;

						stat.statTimeCleanWorkpool += MPI_Wtime() - statStart;
						stat.statTimeSinceWorkpoolClean = MPI_Wtime();
						stat.statNumProblemsSolved++;
					}
					// Yes, the workingProblem may lead to a new incumbent.
					// sonst zerlegen
					else {
						statStartTimeProcessing = MPI_Wtime();
						// Vaterproblem ID und Ursprung merken und anschließend zerlegen
						long parentID = workingProblemFrame->getID();
						ProcessorNo originator = workingProblemFrame->getOriginator();
						
						if(debugComputation)
							std::cout << Muesli::MSL_myId << ": Problem ID: " << parentID <<
							" originator: " << originator << " ist zu zerlegen" << std::endl;

						// branch the problem
						int numOfGeneratedSubproblems = 0;
						Problem** subProblems = branch(workingProblem, &numOfGeneratedSubproblems);
						stat.statNumProblemsBranched++;


						if(debugComputation)
							std::cout << Muesli::MSL_myId << ": Problem (ID " << parentID <<
							") in" << numOfGeneratedSubproblems << " zerlegt!" << std::endl;

						workingProblemFrame->setNumOfSubProblems(numOfGeneratedSubproblems);
						// sicherheitshalber
						workingProblemFrame->setNumOfSolvedSubProblems(0);

						if(numOfGeneratedSubproblems>0) {
							problemTracker->addProblem(workingProblemFrame);
							
							if(debugComputation)
								std::cout << Muesli::MSL_myId << ": Problem ID: " << parentID <<
								" originator: " << originator << " in den Tracker eingefuegt" <<
								std::endl;
						}
						// illegale Loesung -> als geloest behandeln
						else {
							if(debugTermination)
								std::cout << Muesli::MSL_myId << ": Problem ID: " << parentID <<
								" originator: " << originator <<
								" hat keine Teilprobleme erzeugt: als geloest markieren" << std::endl;

							delete workingProblem;
							double startTime = MPI_Wtime();
							problemTracker->problemSolved(workingProblemFrame);
							stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
							stat.statNumProblemsKilled++;
						}
						// hoechste KindID
						long subProblemID = parentID * numOfMaxSubProblems + numOfMaxSubProblems;

						// analyze subproblems
						for(int i = 0; i < numOfGeneratedSubproblems; i++) {
							// create a problem Frame
							problemFrame = new BBFrame<Problem>(subProblemID--, workingProblemFrame,
								Muesli::MSL_myId, 0, subProblems[i]);

							if(debugComputation)
								std::cout << Muesli::MSL_myId << ": Betrachte Teilproblem: " <<
								i << std::endl;

							// if a solution is found
							if(isSolution(subProblems[i])) {
								if(debugTermination)
									std::cout << Muesli::MSL_myId << ": Problem (ID " <<
									subProblemID << ") geloest!" << std::endl;

								// test if it is a new incumbent
								if(!solutionFound || betterThan(subProblems[i], incumbent)) {
									if(debugComputation)
										std::cout << Muesli::MSL_myId << ": Problem (ID " <<
										subProblemID << ") ist neues Incumbent!" << std::endl;

									// yes! New incumbent is found
									newIncumbentFound = true;
									
									// free memory for old incumbent
									if(incumbent != NULL)
										delete incumbent;
									
									// store new incumbent
									incumbent = subProblems[i];
									solutionFound = true;
								}
								// free memory for worse solutions
								else
									delete subProblems[i];

								double startTime = MPI_Wtime();

								problemTracker->problemSolved(problemFrame);
								stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
								stat.statNumProblemsSolved++;
								stat.statNumSolutionsFound++;
							}
							// subproblem is not solved yet
							else  {
								if(debugComputation)
									std::cout << Muesli::MSL_myId << ": Problem (ID " <<
									subProblemID << ") ist keine Loesung, zunaechst abschaetzen" <<
									std::endl;

								// estimate subproblem
								bound(subProblems[i]);
								stat.statNumProblemsBounded++;

								// check if the problem has been solved by applying bound
								// koennte Incumbent sein, falls es jetzt eine Loesung ist
								if(isSolution(subProblems[i])) {
									if(debugTermination)
										std::cout << Muesli::MSL_myId << ": Problem (ID " <<
										subProblemID << ") geloest!" << std::endl;

									// test if it is a new incumbent
									if(!solutionFound || betterThan(subProblems[i], incumbent)) {
										if(debugComputation)
											std::cout << Muesli::MSL_myId << ": Problem (ID " <<
											subProblemID << ") ist neues Incumbent!" << std::endl;

										// yes! New incumbent is found
										newIncumbentFound = true;
										
										// free memory for old incumbent
										if(incumbent != NULL)
											delete incumbent;

										// store new incumbent
										incumbent = subProblems[i];
										solutionFound = true;
									}
									// free memory for worse solutions
									else
										delete subProblems[i];

									if(debugTermination)
										std::cout << Muesli::MSL_myId << ": Problem (ID " <<
										subProblemID << ") im Tracker als geloest markieren" <<
										std::endl;

									double startTime = MPI_Wtime();
									problemTracker->problemSolved(problemFrame);
									stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
									stat.statNumProblemsSolved++;
									stat.statNumSolutionsFound++;
								}
								// subproblem is not solved yet
								// ist noch ein Problem -> wenn es noch besser
								// werden kann als das Incumbent -> in den Workpool
								else {
									if(debugComputation)
										std::cout << Muesli::MSL_myId << ": Problem (ID " <<
										subProblemID << ") in den Workpool verschoben! LowerBound: " <<
										getLowerBound(subProblems[i]) << std::endl;

									// if the estimation is better than the best solution found
									// so far, solving the subproblem may lead to a new incumbent
									if(!solutionFound || betterThan(subProblems[i], incumbent))
										workpool->insert(problemFrame);
									// oherwise discard this problem
									else {
										double startTime = MPI_Wtime();
										problemTracker->problemSolved(problemFrame);
										stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
										// free memory
										delete subProblems[i];
										stat.statNumProblemsSolved++;
									}
								}
							}
						}

						// free memory for array of subproblems returned by branch
						// ist nur das Array von Zeigern
						delete[] subProblems;
						stat.statTimeProblemProcessing += MPI_Wtime() - statStartTimeProcessing;
					}

				}
				// Ende Problemverarbeitung

				// Termination Detection, Tracker ist leer, falls alle Solver terminiert sind
				if(debugTermination)
					std::cout << Muesli::MSL_myId << ": Teste, ob Tracker leer" << std::endl;

				if(Muesli::MSL_myId == masterSolver && problemTracker->isTrackerEmpty()) {
					if(debugTermination)
						std::cout << Muesli::MSL_myId << ": Optimale Loesung gefunden!" << std::endl;

					MSL_Send(getReceiver(), incumbent);
					masterBlocked = false;
					solutionFound = false;
				}
			}
			// Ende while(!finished)

			// Statistiken berechnen, am MasterSolver sammeln und ausgeben
			stat.statTimeTotal = MPI_Wtime() - stat.statTimeTotal;
			stat.statTimeSinceWorkpoolClean = MPI_Wtime() - stat.statTimeSinceWorkpoolClean;

			stat.problemTrackerMaxLength = problemTracker->getProblemTrackerMaxLength();
			stat.problemTrackerAverageLength = problemTracker->getProblemTrackerAverageLength();
			stat.solvedProblemsQueueMaxLength = problemTracker->getSolvedQueueMaxLength();
			stat.polvedProblemsQueueAverageLength = problemTracker->getSolvedQueueAverageLength();
			stat.workpoolMaxLength = workpool->getMaxLength();
			stat.workpoolAverageLength = workpool->getAverageLength();

			if(analysis) {
				if(Muesli::MSL_myId == masterSolver) {
					std::cout << Muesli::MSL_ARG1 << "; total; runtime; " <<
						MPI_Wtime() - Muesli::MSL_Starttime << std::endl << std::flush;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumMsgProblemSolvedSent; " <<
						stat.statNumMsgProblemSolvedSent << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumMsgProblemSolvedReceived; " <<
						stat.statNumMsgProblemSolvedReceived << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumMsgBoundInfoSent; " <<
						stat.statNumMsgBoundInfoSent << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumMsgBoundInfoReceived; " <<
						stat.statNumMsgBoundInfoReceived << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumMsgWorkPoolEmptySent; " <<
						stat.statNumMsgWorkPoolEmptySent << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumMsgBoundRejectionSent; " <<
						stat.statNumMsgBoundRejectionSent << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumMsgBoundRejectionReceived; " <<
						stat.statNumMsgBoundRejectionReceived << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumProblemsSent; " <<
						stat.statNumProblemsSent << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumProblemsReceived; " <<
						stat.statNumProblemsReceived << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumProblemsSolved; " <<
						stat.statNumProblemsSolved << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumIncumbentReceivedAccepted; " <<
						stat.statNumIncumbentReceivedAccepted << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumIncumbentReceivedDiscarded; " <<
						stat.statNumIncumbentReceivedDiscarded << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumIncumbentSent; " <<
						stat.statNumIncumbentSent << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumProblemsBranched; " <<
						stat.statNumProblemsBranched << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumProblemsBounded; " <<
						stat.statNumProblemsBounded << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumSolutionsFound; " <<
						stat.statNumSolutionsFound << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumProblemsTrackedTotal; " <<
						stat.statNumProblemsTrackedTotal << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statNumProblemsKilled; " <<
						stat.statNumProblemsKilled << std::endl;

					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; ProblemTrackerMaxLength ; " <<
						stat.problemTrackerMaxLength << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; ProblemTrackerAverageLength; " <<
						stat.problemTrackerAverageLength << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; SolvedProblemsQueueMaxLength; " <<
						stat.solvedProblemsQueueMaxLength << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; SolvedProblemsQueueAverageLength; " <<
						stat.polvedProblemsQueueAverageLength << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; WorkpoolMaxLength; " <<
						stat.workpoolMaxLength << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; WorkpoolAverageLength; " <<
						stat.workpoolAverageLength << std::endl;

					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statTimeTotal; " <<
						stat.statTimeTotal << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statTimeProblemProcessing; " <<
						stat.statTimeProblemProcessing << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statTimeCommunication; " <<
						stat.statTimeCommunication << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statTimeIncumbentHandling; " <<
						stat.statTimeIncumbentHandling << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statTimeLoadBalancing; " <<
						stat.statTimeLoadBalancing << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statTimeTrackerSolvedProblemsReceived; " <<
						stat.statTimeTrackerSolvedProblemsReceived << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statTimeTrackerSolvedProblemsSent; " <<
						stat.statTimeTrackerSolvedProblemsSent << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statTimeCleanWorkpool; " <<
						stat.statTimeCleanWorkpool << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statTimeSubProblemSolvedInsert; " <<
						stat.statTimeSubProblemSolvedInsert << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statTimeIdle; " <<
						stat.statTimeIdle << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; statTimeInitialIdle; " <<
						stat.statTimeInitialIdle << std::endl;
					std::cout << Muesli::MSL_ARG1 << "; " << Muesli::MSL_myId <<
						"; timeSinceWorkpoolClean; " <<
						stat.statTimeSinceWorkpoolClean<< std::endl;

					for(int i = 0; i < numOfSolvers; i++) {
						if(exitOfSolver[i] == Muesli::MSL_myId)
							continue;

						statistics* statRemote = new statistics();
						MSL_Receive(exitOfSolver[i], statRemote, MSLT_BB_STATISTICS, &messageStatus);
						
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumMsgProblemSolvedSent; " <<
							statRemote->statNumMsgProblemSolvedSent << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumMsgProblemSolvedReceived; " <<
							statRemote->statNumMsgProblemSolvedReceived << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumMsgBoundInfoSent; " <<
							statRemote->statNumMsgBoundInfoSent << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumMsgBoundInfoReceived; " <<
							statRemote->statNumMsgBoundInfoReceived << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumMsgWorkPoolEmptySent; " <<
							statRemote->statNumMsgWorkPoolEmptySent << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumMsgBoundRejectionSent; " <<
							statRemote->statNumMsgBoundRejectionSent << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumMsgBoundRejectionReceived; " <<
							statRemote->statNumMsgBoundRejectionReceived << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumProblemsSent; " <<
							statRemote->statNumProblemsSent << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumProblemsReceived; " <<
							statRemote->statNumProblemsReceived << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumProblemsSolved; " <<
							statRemote->statNumProblemsSolved << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumIncumbentReceivedAccepted; " <<
							statRemote->statNumIncumbentReceivedAccepted << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumIncumbentReceivedDiscarded; " <<
							statRemote->statNumIncumbentReceivedDiscarded << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumIncumbentSent; " <<
							statRemote->statNumIncumbentSent << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumProblemsBranched; " <<
							statRemote->statNumProblemsBranched << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumProblemsBounded; " <<
							statRemote->statNumProblemsBounded << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumSolutionsFound; " <<
							statRemote->statNumSolutionsFound << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumProblemsTrackedTotal; " <<
							statRemote->statNumProblemsTrackedTotal << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statNumProblemsKilled; " <<
							statRemote->statNumProblemsKilled << std::endl;

						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; ProblemTrackerMaxLength ; " <<
							statRemote->problemTrackerMaxLength << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; ProblemTrackerAverageLength; " <<
							statRemote->problemTrackerAverageLength << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; SolvedProblemsQueueMaxLength; " <<
							statRemote->solvedProblemsQueueMaxLength << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; SolvedProblemsQueueAverageLength; " <<
							statRemote->polvedProblemsQueueAverageLength << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; WorkpoolMaxLength; " <<
							statRemote->workpoolMaxLength << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; WorkpoolAverageLength; " <<
							statRemote->workpoolAverageLength << std::endl;

						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statTimeTotal; " <<
							statRemote->statTimeTotal << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statTimeProblemProcessing; " <<
							statRemote->statTimeProblemProcessing << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statTimeCommunication; " <<
							statRemote->statTimeCommunication << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statTimeIncumbentHandling; " <<
							statRemote->statTimeIncumbentHandling << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statTimeLoadBalancing; " <<
							statRemote->statTimeLoadBalancing << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statTimeTrackerSolvedProblemsReceived; " <<
							statRemote->statTimeTrackerSolvedProblemsReceived << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statTimeTrackerSolvedProblemsSent; " <<
							statRemote->statTimeTrackerSolvedProblemsSent << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statTimeCleanWorkpool; " <<
							statRemote->statTimeCleanWorkpool << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statTimeSubProblemSolvedInsert; " <<
							statRemote->statTimeSubProblemSolvedInsert << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statTimeIdle; " <<
							statRemote->statTimeIdle << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; statTimeInitialIdle; " <<
							statRemote->statTimeInitialIdle << std::endl;
						std::cout << Muesli::MSL_ARG1 << "; " << exitOfSolver[i] <<
							"; timeSinceWorkpoolClean; " <<
							statRemote->statTimeSinceWorkpoolClean<< std::endl;

						delete statRemote;
					}
				} else {
					MSL_Send(masterSolver, &stat, MSLT_BB_STATISTICS);
				}
			}
		}

		void setWorkmates(BBSolver<Problem>** solver, int length, int id,
			int topology = MSL_BB_TOPOLOGY_ALLTOALL) {
			numOfSolvers = length;
			entranceOfSolver = new ProcessorNo[numOfSolvers];
			exitOfSolver = new ProcessorNo[numOfSolvers];

			// Ein- und Ausgaenge aller Solver berechnen (inkl. des eigenen Ein- und Ausgangs)
			ProcessorNo* ext;
			ProcessorNo* entr;

			for(int i = 0; i < length; i++) {
				entr = (*(solver[i])).getEntrances();
				entranceOfSolver[i] = entr[0];
				ext = (*(solver[i])).getExits();
				exitOfSolver[i] = ext[0];
			}

			// erzeuge gewuenschte Topologie
			switch (topology) {
				case MSL_BB_TOPOLOGY_ALLTOALL:
					// einfach ein- und ausgaenge von oben nutzen
					entranceOfWorkmate = entranceOfSolver;
					exitOfWorkmate = exitOfSolver;
					numOfWorkmates = numOfSolvers;
					break;
				case MSL_BB_TOPOLOGY_HYPERCUBE: {
						// generiere Hyperwuerfel. Bestimme Dimension -> 2er
						// logarithmus der Laenge (muss dazu 2er Potenz sein!)
						int dim = 0;
						int tmp = length;

						while(tmp != 1) {
							dim++;
							tmp>>=1;
						}

						entranceOfWorkmate = new ProcessorNo[dim];
						exitOfWorkmate = new ProcessorNo[dim];
						int conNr = 1;
						
						for(int i = 0; i < dim; i++) {
							int mate = id ^ conNr;
							entranceOfWorkmate[i] = entranceOfSolver[mate];
							exitOfWorkmate[i] = exitOfSolver[mate];
							conNr<<=1;
						}

						numOfWorkmates = dim;
					}
					break;
				case MSL_BB_TOPOLOGY_RING: {
					// bidirektionaler ring. nachfolger <-> id <-> vorgaenger. zur Umsetzung
					// eines unidirektionalen Rings muss die Lastverteilung geaendert werden
					// denn dort werden nur die Ausgaenge von Workmates auf Anfragen geprueft
					entranceOfWorkmate = new ProcessorNo[2];
					exitOfWorkmate = new ProcessorNo[2];
					// vorgaenger
					entranceOfWorkmate[0] = entranceOfSolver[(id - 1 + length) % length];
					exitOfWorkmate[0] = exitOfSolver[(id - 1 + length) % length];

					// nachfolger
					entranceOfWorkmate[1] = entranceOfSolver[(id + 1) % length];
					exitOfWorkmate[1] = exitOfSolver[(id + 1) % length];
					numOfWorkmates = 2;
				}
				break;
			}
		}

		Process* copy() {
			return new BBSolver<Problem>(branch, bound, betterThan, isSolution, getLowerBound,
				numOfMaxSubProblems, noprocs);
		}

   		void show() const {
   			if(Muesli::MSL_myId == 0)
				std::cout << Muesli::MSL_myId << " BBSolver " << entranceOfSolver[0] <<
				std::endl << std::flush;
		}

	};

}

#endif
