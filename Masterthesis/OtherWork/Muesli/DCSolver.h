// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef DC_SOLVER_H
#define DC_SOLVER_H

#include "Frame.h"
#include "mpi.h"
#include "Muesli.h"
#include "Process.h"
#include "SolutionpoolManager.h"
#include "WorkpoolManager.h"

namespace msl {

	/* Dezentrales Divide and Conquer Skelett, angelehnt an BranchAndBoundOld
	*/
	template<class Problem, class Solution>
	class DCSolver: public Process {

	private:

		// state of Mastersolver
		bool blocked;
		// indicates if incumbent found
		bool solutionFound;
		// Eingaenge aller Solver (inkl. this): entranceOfSolver[0] ist Master
		ProcessorNo* entranceOfSolver;
		// Ausgaenge aller Solver (inkl. this): exitOfSolver[0] ist Master
		ProcessorNo* exitOfSolver;
		// Ringstruktur: Solver empfaengt Token von left
		ProcessorNo left;
		// Ringstruktur: Solver sendet Token an right
		ProcessorNo right;
		// Anzahl der Solver, die durchs BnB-Skelett gruppiert werden
		int numOfSolvers;
		// assumption: uses contiguous block of processors
		int noprocs;
		//
		int D;
		// Zeiger auf benutzerdefinierte Funktion
		bool (*isSimple)(Problem*);
		// Zeiger auf benutzerdefinierte Funktion
		Solution* (*solve)(Problem*);
		// Zeiger auf bound-Methode. Schaetzt Subproblem ab.
		Problem** (*divide)(Problem*);
		// Zeiger auf branch-Methode. Liefert Zeiger auf Array mit Zeigern auf erzeugte Subprobleme
		Solution* (*combine)(Solution**);
		//
		WorkpoolManager<Problem>* workpool;
		//
		SolutionpoolManager<Solution>* solutionpool;

	public:

		DCSolver(Problem** (*div)(Problem*), Solution* (*comb)(Solution**), Solution* (*solv)(Problem*),
			bool (*smpl)(Problem*), int d, int n): D(d), noprocs(n), isSimple(smpl), solve(solv),
			divide(div), combine(comb), Process() {
			// Workpool und Solutionpool erzeugen
			workpool = new WorkpoolManager<Problem>();
			solutionpool = new SolutionpoolManager<Solution>(comb,d);
      		solutionFound = false;
			numOfEntrances = 1;
			numOfExits = 1;
			entrances = new ProcessorNo[numOfEntrances];
			exits = new ProcessorNo[numOfExits];
			entrances[0] = Muesli::MSL_runningProcessorNo;
			exits[0] = entrances[0];
			Muesli::MSL_runningProcessorNo += n;
			// Default-Receiver ist 0. Den gibt's immer
			setNextReceiver(0);
			// zufaellig gestellte Workrequests sicherstellen
			srand(time(NULL));
		}

		~DCSolver() {
			delete[] entrances; 	 entrances = NULL;
			delete[] exits; 		 exits = NULL;
			delete workpool; 		 workpool = NULL;
			delete solutionpool; 	 solutionpool = NULL;
			delete entranceOfSolver; entranceOfSolver = NULL;
			delete exitOfSolver;	 exitOfSolver = NULL;
		}

		void start() {
			// am Prozess beteiligte Prozessoren merken sich entrance und exit des Solvers
			finished = ((Muesli::MSL_myId < entrances[0]) ||
				(Muesli::MSL_myId >= entrances[0] + noprocs));

			if(finished)
				return;

			bool deepCombineEnabled = true;
			bool analyse = false;

			//debugmessages
			bool debugCommunication = false;
			bool debugLoadBalancing = false;
			bool debugTerminationDetection = false;
			bool debugComputation = false;
			bool debugFreeMem = false;
			bool logIDs = false;

			// fuer die Statistik
			std::vector<unsigned long> v;
			int numOfProblemsProcessed = 0;
			int numOfSolutionsSent = 0;
			int numOfSolutionsReceived = 0;
			int numOfSubproblemsSent = 0;
			int numOfSubproblemsReceived = 0;
			int numOfWorkRequestsSent = 0;
			int numOfWorkRequestsReceived = 0;
			int numOfRejectionsSent = 0;
			int numOfRejectionsReceived = 0;
			int numOfSimpleProblemsSolved = 0;
			double time_solve = 0;
			double time_divide = 0;
			double time_combine = 0;
			double time_start = 0;
			double time_new = 0;
			double time_workpool = 0;
			double time_solutionpool = 0;
			double time_solver = 0;

			// define internal tolology
			int ALL_TO_ALL = 1;
			int topology = ALL_TO_ALL;

			// alle anderen merken sich die Anzahl der am Skelett
			// beteiligten Prozessoren und den masterSolver-Eingang
			Muesli::MSL_numOfLocalProcs = noprocs;
			ProcessorNo masterSolver = entranceOfSolver[0];
			// Eingang des DCSolvers merken
			Muesli::MSL_myEntrance = entrances[0];
			// Ausgang des DCSolvers merken
			Muesli::MSL_myExit = exits[0];

			Problem* problem = NULL;
			Solution* solution = NULL;
			Frame<Problem>* problemFrame = NULL;
			Frame<Problem>* subproblemFrame = NULL;
			Frame<Solution>* solutionFrame = NULL;

			MPI_Status status;
			// Anzahl bereits eingegangener Stop-Tags
			int receivedStops = 0;
			// Anzahl bereits eingegangener TerminationTest-Tags
			int receivedTT = 0;
			// (*) speichert TAGs
			int dummy;
			bool workRequestSent = false;
			bool sendRequestSent = false;
			bool deepCombineIsNecessary = false;
			int flag = 0;
			long originator = -1;
			// nicht ueberschreiben, sonst geht die Lastverteilung kaputt!!!
			int receiverOfWorkRequest;
			int predecessorIndex = 0;
			void* p1;
			void* p2;

			// solange Arbeit noch nicht beendet ist
			blocked = false;

			while(!finished) {
				// PROBLEMANNAHME. Nur der Master-Solver kann im Zustand FREE
				// externe Nachrichten (Probleme und TAGs) annehmen
				if(Muesli::MSL_myId == masterSolver && !blocked) {
					// Verbleibe in dieser Schleife, bis das naechste
					// komplexe Problem ankommt oder Feierabend ist
					while(!blocked && !finished) {
						// blockiere bis Nachricht vom Vorgaenger
						// eingetroffen ist (Problem oder STOP moeglich)
						flag = 0;

						if(debugCommunication)
							std::cout << Muesli::MSL_myId <<
							": Mastersolver wartet auf neues Problem" << std::endl;
						
						while(!flag) {
							MPI_Iprobe(predecessors[predecessorIndex], MPI_ANY_TAG, MPI_COMM_WORLD,
								&flag, &status);
							predecessorIndex = (predecessorIndex + 1) % numOfPredecessors;
						}

						if(debugCommunication)
							std::cout << Muesli::MSL_myId <<
							": Mastersolver empfaengt Daten von " << status.MPI_SOURCE <<
							" mit Tag = " << status.MPI_TAG << std::endl;
						
						// TAGS verarbeiten (optimierbar, wenn man
						// Reihenfolge aendert: 1. Problem, 2. TAG)
						ProcessorNo source = status.MPI_SOURCE;
						
						if(status.MPI_TAG == MSLT_TERMINATION_TEST) {
							// Nimm TT-Nachricht entgegen
							MSL_ReceiveTag(source, MSLT_TERMINATION_TEST);
						}
						else if(status.MPI_TAG == MSLT_STOP) {
							// Nimm STOP-Nachricht entgegen
							MSL_ReceiveTag(source, MSLT_STOP);
							
							if(debugCommunication)
								std::cout << Muesli::MSL_myId <<
								":(DCSolver::start) Mastersolver hat STOP empfangen" << std::endl;
							
							// Tags sammeln
							receivedStops++;
							
							// wenn genug STOPs gesammelt
							if(receivedStops == numOfPredecessors) {
								if(debugCommunication)
									std::cout << Muesli::MSL_myId <<
									":(DCSolver::start) Mastersolver hat #numOfPredecessors STOPS " \
									"empfangen -> Terminiere" << std::endl;
								
								// alle solverinternen Prozessoren benachrichtigen
								// (falls Solver datenparallel arbeitet)
								for(int i = Muesli::MSL_myEntrance + 1;
									i < Muesli::MSL_myEntrance + noprocs; i++)
									MSL_SendTag(i, MSLT_STOP);
								
								// alle Solver (auﬂer sich selbst) benachrichtigen
								if(numOfSolvers > 1) {
									for(int i = 0; i < numOfSolvers; i++) {
										if(entranceOfSolver[i] != Muesli::MSL_myId) {
											if(debugCommunication)
												std::cout << Muesli::MSL_myId <<
												":(DCSolver::start) Mastersolver sendet STOP " \
												"intern an "<< entranceOfSolver[i] << std::endl;
											
											MSL_SendTag(entranceOfSolver[i], MSLT_STOP);
										}
									}
								}
								// alle Nachfolger benachrichtigen
								for(int i = 0; i < numOfSuccessors; i++) {
									if(debugCommunication)
										std::cout << Muesli::MSL_myId <<
										":(DCSolver::start) Mastersolver schickt STOP an Nachfolger " <<
										successors[i] << std::endl;
									
									MSL_SendTag(successors[i], MSLT_STOP);
								}

								receivedStops = 0;
								// Feierabend! DCSolver terminiert...
								finished = true;
								
								if(debugCommunication)
									std::cout << Muesli::MSL_myId <<
									":(DCSolver::start) MasterSolver " << Muesli::MSL_myId <<
									" terminiert"<< std::endl;
							}
						}
						// wenn's kein TAG war, war's ein neues DC-Problem
						else {
							if(debugCommunication)
								std::cout << Muesli::MSL_myId <<
								": Mastersolver bereitet Empfang eines DC-Problems von " <<
								source << " mit Tag = " << MSLT_MYTAG << " vor" << std::endl;
							
							problem = new Problem();
							Muesli::numP++;
							MSL_Receive(source, problem, MSLT_MYTAG, &status);
							// statistics
							numOfSubproblemsReceived++;
							
							if(debugCommunication)
								std::cout << Muesli::MSL_myId <<
								":(DCSolver::start) Mastersolver empfaengt DC-Problem" << std::endl;
							
							// wenn das Problem einfach genug ist, loese es
							// direkt, und verschicke das Ergebnis an den Nachfolger
							if(isSimple(problem)) {
								if(debugComputation)
									std::cout << Muesli::MSL_myId <<
									":(DCSolver::start) Problem wird direkt geloest"<< std::endl;
								
								solution = solve(problem);
								Muesli::numS++;
								p1 = (void*)problem;
								p2 = (void*)solution;
								
								if(p1 != p2) {
									delete problem;
									problem = NULL;
									Muesli::numP--;
								}
								
								if(debugComputation)
									std::cout << Muesli::MSL_myId <<
									":(DCSolver::start) Problem geloest"<< std::endl;
								
								MSL_Send(getReceiver(), solution);
								
								if(debugCommunication)
									std::cout << Muesli::MSL_myId <<
									":(DCSolver::start) Loesung verschickt"<< std::endl;
								
								delete solution;
								solution = NULL;
								Muesli::numS--;
								// statistics
								numOfSolutionsSent++;
							}
							// sonst initiiere die parallele Loesung des Problems
							else {
								// Blockiere Solver bis Problem geloest ist
								// Abbruch der inneren while-Schleife
								blocked = true;
								
								if(debugComputation)
									std::cout << Muesli::MSL_myId <<
									":(DCSolver::start) Problem ist zu komplex und wird " \
									"parallel geloest"<< std::endl;

								// Root-Initialization: KindID = VaterID * D + 1,
								// VaterID = (KindID - 1) / D, WurzelID = 0
								time_start = MPI_Wtime();
								// Pair: (id, Problem*)
								problemFrame = new Frame<Problem>(0, -1, -1, masterSolver, problem);
								time_new += MPI_Wtime() - time_start;
								Muesli::numPF++;
								// Frame in den Workpool schreiben
								time_start = MPI_Wtime();
								workpool->insert(problemFrame);
								time_workpool += MPI_Wtime() - time_start;
								
								if(debugComputation) {
									workpool->show(); solutionpool->show();
								}
							}
						}
					}
					// end of inner while
					
					// hier angekommen, wurde entweder ein komplexes DC-Problem
					// empfangen (Workpool nicht leer) oder es ist Feierabend
				}
				// ende Problemannahme

				// INTERNER NACHRICHTENVERKEHR zwischen den Solvern, falls es mehrere gibt:
				// Kommunikation und Problemverarbeitung muss verschachtelt werden, damit
				// eine schnelle Reaktion auf Arbeitsanfragen garantiert werden kann.
				if((numOfSolvers>1) && (!finished) && (Muesli::MSL_myId == Muesli::MSL_myEntrance)) {
					// 1. verarbeite alle empfangenen Teilloesungen
					// Ankommende, geloeste Teilprobleme werden einfach in den SolutionPool
					// geschrieben und dort genau so weiterverarbeitet, als waere die Loesung
					// lokal berechnet worden. Wenn sich im Rahmen des Combine eine neue
					// VaterID ergibt, zu der es in der RootList ein Paar (VaterID, Sender)
					// gibt, so ist das Teilproblem geloest und kann an den Sender zurueck-
					// geschickt werden. Das Paar (VaterID, Sender) wird aus der RootList
					// entfernt.
					for(int i = 0; i < numOfSolvers; i++) {
						// alle Solver mit kleinerer ProzessID duerfen immer senden
						MPI_Iprobe(exitOfSolver[i], MSLT_SOLUTION, MPI_COMM_WORLD, &flag, &status);
						
						if(flag) {
							solutionFrame = new Frame<Solution>();
							Muesli::numSF++;
							MSL_Receive(exitOfSolver[i], solutionFrame, MSLT_SOLUTION, &status);
							// statistics
							numOfSolutionsReceived++;
							
							if(debugLoadBalancing) {
								workpool->show();
								solutionpool->show();
								std::cout << Muesli::MSL_myId <<
									": empfange Teilloesung " << solutionFrame->getID() << std::endl;
							}

							solutionpool->insert(solutionFrame);
							time_start = MPI_Wtime();
							
							if(solutionpool->combineIsPossible())
								solutionpool->combine();

							// deepCombine anstoﬂen
							deepCombineIsNecessary = true;
							time_combine += MPI_Wtime() - time_start;
							
							if(debugLoadBalancing) {
								solutionpool->show();
							}
						}

						// Solver mit groeﬂerer ID schicken Sendeanfragen. Bei Eingang
						// einer Sendeanfrage geht der Solver direkt auf Empfang
						MPI_Iprobe(exitOfSolver[i], MSLT_SENDREQUEST, MPI_COMM_WORLD, &flag, &status);
						
						if(flag) {
							if(debugLoadBalancing)
								std::cout << Muesli::MSL_myId <<
								": SENDREQUEST von " << exitOfSolver[i] <<
								" eingegangen. Sende READYSIGNAL und gehe auf Empfang" << std::endl;

							MSL_ReceiveTag(exitOfSolver[i], MSLT_SENDREQUEST);
							MSL_SendTag(entranceOfSolver[i], MSLT_READYSIGNAL);
							solutionFrame = new Frame<Solution>();
							Muesli::numSF++;
							MSL_Receive(exitOfSolver[i], solutionFrame, MSLT_SOLUTION, &status);
							// statistics
							numOfSolutionsReceived++;
							
							if(debugLoadBalancing) {
								workpool->show();
								solutionpool->show();
								std::cout << Muesli::MSL_myId <<
									": empfange Teilloesung " << solutionFrame->getID() << std::endl;
							}

							solutionpool->insert(solutionFrame);
							time_start = MPI_Wtime();
							
							if(solutionpool->combineIsPossible())
								solutionpool->combine();

							time_combine += MPI_Wtime() - time_start;
							
							if(debugLoadBalancing) {
								solutionpool->show();
							}
						}
					}

					// 2. Nach dem Empfang einer Teilloesung liegen ggf. kombinierbare Teilloesungen
					// tief im Stack. Solange versunkene kombinierbare Teilloesungen vorhanden sind,
					// wird ein deepCombine durchgefuehrt. Der Methodenaufruf wird umgangen, wenn
					// keine kombinierbaren Teilprobleme im Stack liegen.
					if(deepCombineEnabled && deepCombineIsNecessary)
						deepCombineIsNecessary = solutionpool->deepCombine();

					// 3. kontrolliertes Verschicken von Nachrichten in der SendQueue
					// nur eine Verschicken, um Deadlocks vorzubeugen??? Idee: Das Verschicken einer
					// Loesung wird uebersprungen, solange noch potentiell groﬂe Teilprobleme oder
					// Loesungen vom Empaenger entgegen genommen werden muessen. Auf diese Weise
					// werden Deadlocks hoffentlich vermieden. Eine Loesung darf ich nur verschicken,
					// wenn ein ggf. aktiver Lastverteilungsprozess abgeschlossen ist
					if(!solutionpool->sendQueueIsEmpty() && !workRequestSent) {
						solutionFrame = solutionpool->readElementFromSendQueue();
						originator = solutionFrame->getOriginator();
						
						// Senden ist erlaubt, wenn SenderID < EmpfaengerID und vorher kein WorkRequest
						// verschickt wurde. Dies koennte sonst zu einem zyklischen Deadlock fuehren:
						// Situation: A: WorkReq an C, danach Loesung an B; B schickt Loesung an C; C
						// antwortet A mit Problem. -> Deadlock, wenn alle Nachrichten Handshake
						// erfordern! A wartet auf B, B wartet auf C und C wartet auf A
						if(Muesli::MSL_myId < originator) {
							// direktes Verschicken von Loesungen ist verboten, wenn vorher ein
							// Workrequest an den Empfaenger gesendet wurde, denn dann kann ggf. als
							// Antwort darauf ein Teilproblem unterwegs sein, das ebenfalls ein
							// Handshake erfordert. Das Ergebnis waere ein Deadlock, da Sender und
							// Empfaenger gegenseitig auf die Abnahme der Nachricht warten.
							if(debugLoadBalancing) {
								std::cout << Muesli::MSL_myId <<
									": sende Teilloesung " << solutionFrame->getID() <<
									" direkt an " << originator << std::endl;
							}
							
							// kann blockieren!
							MSL_Send(originator, solutionFrame, MSLT_SOLUTION);
							solutionpool->removeElementFromSendQueue();
							solutionFrame = NULL;
						}
						// sonst muss zunaechst ein SendRequest an den
						// Empfaenger der Nachricht geschickt werden
						else if(!sendRequestSent) {
							if(debugLoadBalancing) {
								solutionpool->showSendQueue();
								std::cout << Muesli::MSL_myId <<
									": versuche Frame aus Sendqueue zu senden: (" <<
									solutionFrame->getID() << "," << solutionFrame->getRootNodeID() <<
									"," << originator << ")" << std::endl;
								std::cout << Muesli::MSL_myId <<
									": sende SENDREQUEST an " << originator << std::endl;
							}

							MSL_SendTag(originator, MSLT_SENDREQUEST);
							sendRequestSent = true;
						}
						// ist das bereits geschehen, darf die Nachricht erst
						// nach Eingang des ReadySignals verschickt werden
						else {
							// warte auf readySignal
							MPI_Iprobe(originator, MSLT_READYSIGNAL, MPI_COMM_WORLD, &flag, &status);

							if(flag) {
								// Empfaenger wartet auf den Eingang des SolutionFrames
								if(debugLoadBalancing) {
									std::cout << Muesli::MSL_myId <<
										": READYSIGNAL eingegangen" << std::endl;
								}

								MSL_ReceiveTag(originator, MSLT_READYSIGNAL);
								
								if(debugLoadBalancing) {
									std::cout << Muesli::MSL_myId <<
										": sende Teilloesung " << solutionFrame->getID() <<
										" an " << originator << std::endl;
								}
								
								MSL_Send(originator, solutionFrame, MSLT_SOLUTION);
								// Antwort erhalten
								sendRequestSent = false;
								solutionpool->removeElementFromSendQueue();
								solutionFrame = NULL;
								
								if(debugLoadBalancing) {
									solutionpool->showSendQueue();
								}
							}
						}
					}

					//* 4. eingehende WorkRequests verarbeiten. eingehende Workrequests von P werden
					// erst dann verarbeitet, wenn die ‹bertragung von Teilloesungen an P abgeschlossen
					// ist. Andernfalls erwartet P nach einem Readysignal eine Solution, aufgrund des
					// Workrequests wird aber ein Subproblem geschickt. Dies wuerde zum Deadlock fuehren,
					// wenn beide Nachrichten per Handshake verschickt werden muessen.
					originator = -1;

					if(!solutionpool->sendQueueIsEmpty())
						originator = solutionpool->readElementFromSendQueue()->getOriginator();

					for(int i = 0; i < numOfSolvers; i++) {
						MPI_Iprobe(exitOfSolver[i], MSLT_WORKREQUEST, MPI_COMM_WORLD, &flag, &status);

						if(flag && !sendRequestSent) {
							// receive message
							MSL_ReceiveTag(exitOfSolver[i], MSLT_WORKREQUEST);
							// statistics
							numOfWorkRequestsReceived++;

							// inform sender about an empty workpool
							if(!workpool->hasLoad()) {
								MSL_SendTag(entranceOfSolver[i], MSLT_REJECTION);
								
								if(debugLoadBalancing)
									std::cout << Muesli::MSL_myId <<
									": REJECTION an " << entranceOfSolver[i] << std::endl;
								
								// statistics
								numOfRejectionsSent++;
							}
							// distribute load (min 2 subproblems are stored in the workpool)
							else {
								if(debugCommunication) {
									workpool->show();
									solutionpool->show();
								}

								// get the lastElement stored in the workpools' linked list
								problemFrame = workpool->getLoad();
								
								if(debugCommunication)
									std::cout << Muesli::MSL_myId <<
									": Abgabe von Problem " << problemFrame->getID() <<
									" an " << entranceOfSolver[i] << std::endl;

								// TODO Deadlockgefahr!? Groﬂe Probleme werden
								// ohne Sendrequest verschickt! TODO
								MSL_Send(entranceOfSolver[i], problemFrame, MSLT_SUBPROBLEM);
								delete problemFrame->getData();
								delete problemFrame;
								Muesli::numP--;
								Muesli::numPF--;
								problemFrame = NULL;
								// statistics
								numOfSubproblemsSent++;
							}
						}
					}

					//* 5. Lastverteilung
					if(workpool->isEmpty() && !sendRequestSent) {
						// Wenn noch kein Work Request verschickt wurde
						if(!workRequestSent) {
							// sende Work Request und merke den Index des Empfaenger
							do {
								receiverOfWorkRequest = entranceOfSolver[rand()%numOfSolvers];
							}
							while(receiverOfWorkRequest == Muesli::MSL_myId);

							MSL_SendTag(receiverOfWorkRequest, MSLT_WORKREQUEST);
							
							if(debugLoadBalancing) {
								std::cout << Muesli::MSL_myId <<
									": WORKREQUEST an " << receiverOfWorkRequest <<
									std::endl; solutionpool->show();
							}

							// fuer die Statistik
							numOfWorkRequestsSent++;
							// avoid flooding the network with work requests
							workRequestSent = true;
						}
						// sonst warte auf Antwort
						else {
							// Wenn eine Absage angekommt
							MPI_Iprobe(receiverOfWorkRequest, MSLT_REJECTION, MPI_COMM_WORLD,
								&flag, &status);
							
							if(flag) {
								// Absage entgegennehmen und naechsten Work Request verschicken
								MSL_ReceiveTag(receiverOfWorkRequest, MSLT_REJECTION);
								
								if(debugLoadBalancing)
									std::cout << Muesli::MSL_myId << ": REJECTION von " <<
									receiverOfWorkRequest << std::endl;
								
								// statistics
								numOfRejectionsReceived++;
								// Antwort erhalten
								workRequestSent = false;
							}
							// sonst ist vielleicht Arbeit angekommen
							else {
								// TODO: Nach einer Arbeitsanfrage wird ein neues Teilproblem
								// empfangen. Die Loesung dieses Problems muss an den Sender
								// zurueckgeschickt werden. Dazu erstelle ein Paar (ID, Sender)
								// und schreibe es in die RootList des SolutionPoolManagers.
								// Das empfange Problem wird im Workpool abgespeichert. Es ist
								// darauf zu achten, dass dieses Problem im Rahmen der
								// Lastverteilung nicht direkt wieder abgegeben wird (sonst waere
								// der Workpool wieder leer). Dies ist implizit sichergestellt,
								// da die Problemverarbeitungsphase vor der naechsten
								// Lastverteilungsphase durchlaufen wird.
								MPI_Iprobe(receiverOfWorkRequest, MSLT_SUBPROBLEM, MPI_COMM_WORLD,
									&flag, &status);
								
								if(flag) {
									// Konstruktor?
									problemFrame = new Frame<Problem>();
									Muesli::numPF++;
									MSL_Receive(receiverOfWorkRequest, problemFrame, MSLT_SUBPROBLEM,
										&status);
									// statistics
									numOfSubproblemsReceived++;
									// Den Absender und die RootID in den Frame schreiben
									// an diese Adr. wird die Loesung geschickt
									problemFrame->setOriginator(receiverOfWorkRequest);
									problemFrame->setRootNodeID(problemFrame->getID());
									
									// Problem einfach in den Workpool schreiben,
									// der sich um alles Weitere kuemmert
									if(debugCommunication)
										std::cout << Muesli::MSL_myId <<
										": empfange Problem " << problemFrame->getID() <<
										" von " << receiverOfWorkRequest << std::endl;
									
									workpool->insert(problemFrame);
									// Antwort erhalten
									workRequestSent = false;
								}
							}
						}
					}

					// 6. Pruefe auf STOPS (Mastersolver empfaengt nie STOP als interne Nachricht)
					// Anmerkung 05.03.08: Warum prueft sich der MS denn selbst, wenn er nie interne
					// STOPs bekommen kann!? Vermutlich ein Copy-Paste-Fehler, weil aus BnB
					// uebernommen!? STOPs werden nur vom Master versendet, da dieser die Termination
					// Detection uebernimmt.
					MPI_Iprobe(masterSolver, MSLT_STOP, MPI_COMM_WORLD, &flag, &status);
					
					if(flag == true) {
						flag = false;
						MSL_ReceiveTag(masterSolver, MSLT_STOP);
						
						if(debugCommunication)
							std::cout << "(DCSolver::start): DCSolver " << Muesli::MSL_myId <<
							" hat STOP empfangen" << std::endl;

						// alle solverinternen Prozessoren benachrichtigen
						// (falls Solver datenparallel ist)
						for(int i = Muesli::MSL_myEntrance + 1; i < Muesli::MSL_myEntrance + noprocs; i++)
							MSL_SendTag(i, MSLT_STOP);

						// 05.03.08: HARMLOSER BUG: Diese Nachrichten wird von niemandem
						// empfangen, oder!? ist successors[i] ueberhaupt fuer DCSolver definiert???
						for(int i = 0; i < numOfSuccessors; i++)
							MSL_SendTag(successors[i], MSLT_STOP);

						receivedStops = 0;
						// Feierabend
						finished = true;
						
						if(debugCommunication)
							std::cout << "DCSolver " << Muesli::MSL_myId << " terminiert"<< std::endl;
					}
				}
				// interner Nachrichtenverkehr

				// 7. PROBLEMVERARBEITUNG: (fertig), Workpool speichert auch einfache Probleme.
				// - simple Probleme koennen als Last abgegeben werden, obwohl lokale Bearbeitung
				//   schneller waere
				// - einfache Probleme werden nicht direkt geloest, sondern vorher als Frame wieder
				//   in den Workpool geschrieben (Overhead)
				// - erhoehter Speicherplatzbedarf, da einfache Probleme ebenfalls im Workpool liegen
				// - pro geloestes Problem ist 1 combinationIsPossible-Pruefung notwendig (Overhead)
				// + geringer Rechnenaufwand pro Iteration -> schnelles Erreichen und haeufiges
				//   Durchlaufen der Kommunikationsphase
				if(!finished && (!workpool->isEmpty())) {
					time_start = MPI_Wtime();
					problemFrame = workpool->get();
					time_workpool += MPI_Wtime() - time_start;
					problem = problemFrame->getData();
					numOfProblemsProcessed++;
					
					if(debugComputation) {
						std::cout << Muesli::MSL_myId << ": get Problem" << std::endl;
						workpool->show();
						solutionpool->show();
					}

					long currentID = problemFrame->getID();
					long rootNodeID = problemFrame->getRootNodeID();
					originator = problemFrame->getOriginator();
					long poolID = problemFrame->getPoolID();
					delete problemFrame;
					problemFrame = NULL;
					Muesli::numPF--;
					
					// Wenn das Problem zu komplex ist, zerlegen und in Workpool schreiben
					if(!isSimple(problem)) {
						time_start = MPI_Wtime();
						// ### USER DEFINED DIVIDE() ###
						Problem** subproblem = divide(problem);
						time_divide += MPI_Wtime() - time_start;
						Muesli::numP += D;
						bool toDelete = true;
						
						for(int i = 0; i < D; i++) {
							// falls das an divide uebergebene Problem vom user wiederverwendet
							// wurde und Teil des subproblem-Arrays ist, darf es nicht geloescht
							// werden!
							if(subproblem[i] == problem)
								toDelete = false;

							break;
						}
						
						if(toDelete) {
							delete problem;
							problem = NULL;
							Muesli::numP--;
						}

						// KindID = VaterID * D + 1; VaterID = (KindID - 1) / D; WurzelID = 0
						// ID des letzten Kindknotens
						long subproblemID = currentID * D + 1 + (D - 1);
						
						// erzeuge pro Subproblem einen neuen Frame (ID,
						// Problem*) und speichere ihn im Workpool
						for(int i = D - 1; i >= 0; i--) {
							// Original-ProblemID, Owner und Absender werden
							// lokal nie geaendert, sondern einfach kopiert
							time_start = MPI_Wtime();
							problemFrame = new Frame<Problem>(subproblemID--,
								rootNodeID, originator, poolID, subproblem[i]);
							time_new += MPI_Wtime() - time_start;
							Muesli::numPF++;
							time_start = MPI_Wtime();
							workpool->insert(problemFrame);
							time_workpool += MPI_Wtime() - time_start;
							
							if(debugComputation) {
								std::cout << Muesli::MSL_myId << ": insert Problem" << std::endl;
								workpool->show();
								solutionpool->show();
							}
						}

						// Hinweis in der Dokumentation, dass Speicher intern freigegeben wird!
						delete[] subproblem;
					}
					// Wenn das Problem einfach genug ist, loesen, unter
					// gleicher ID im Solutionpool ablegen und ggf. kombinieren
					else {
						time_start = MPI_Wtime();
						// ### USER DEFINED SOLVE() ###
						solution = solve(problem);
						time_solve += MPI_Wtime() - time_start;
						Muesli::numS++;
						numOfSimpleProblemsSolved++;
						p1 = (void*)problem;
						p2 = (void*)solution;
						
						if(p1 != p2) {
							delete problem;
							problem = NULL;
							Muesli::numP--;
						}

						// Original-ProblemID und Absender werden lokal
						// nie geaendert, sondern einfach kopiert
						time_start = MPI_Wtime();
						solutionFrame = new Frame<Solution>(currentID,
							rootNodeID, originator, poolID, solution);
						time_new += MPI_Wtime() - time_start;
						Muesli::numSF++;

						if(currentID == rootNodeID) {
							// Loesung in SendQueue schreiben
							time_start = MPI_Wtime();
							solutionpool->writeElementToSendQueue(solutionFrame);
							time_solutionpool += MPI_Wtime() - time_start;
							solutionFrame = NULL;
						}
						else {
							if(debugComputation)
								std::cout << Muesli::MSL_myId << ": Problem (" <<
								currentID << "," << rootNodeID << "," << originator <<
								") geloest. Schreibe in Solutionpool." << std::endl;

							time_start = MPI_Wtime();
							solutionpool->insert(solutionFrame);
							time_solutionpool += MPI_Wtime() - time_start;
							time_start = MPI_Wtime();

							if(solutionpool->combineIsPossible())
								solutionpool->combine();

							time_combine += MPI_Wtime() - time_start;

							if(debugComputation) {
								std::cout << Muesli::MSL_myId <<
									": combine/insert solution" << std::endl;
								workpool->show();
								solutionpool->show();
							}
						}
					}
				}
				// Ende Problemverarbeitung
				
				// weitere Variante von 1: hasLoad() = true, wenn
				// !isSimple(potentiellAbzugebendesProblem), dann kann der Alg. 1 so bleiben und
				// hat nicht die Nachteile von Alg. 2

				// PROBLEMVERARBEITUNG: (fertig)
				// Workpool speichert nur komplexe Probleme
				// + einfache Probleme werden stets lokal verarbeitet und nie verschickt
				// + im Rahmen der Lastverteilung werden nur komplexe Probleme abgegeben
				// + weniger Speicherplatzbedarf, da einfache Probleme direkt geloest und nicht
				//   im Workpool abgelegt werden
				// + Probleme werden schneller kombiniert, da simple Probleme direkt geloest werden
				//   und keine combinationIsPossible-Pruefung notwendig ist
				// + minimaler combine-Aufwand (pro 1 divide() max. 1 combine())
				// - hoher Rechenaufwand pro Iteration -> Kommunikationsphase wird weniger oft
				//   durchlaufen: Auswirkung auf IDLE-Zeiten?
				if(!finished && (!(*workpool).isEmpty())) {
					problemFrame = workpool->get();
					problem = problemFrame->getData();
					long currentID = problemFrame->getID();
					// Da der Workpool nur komplexe Probleme speichert, ist ein divide() immer moeglich
					Problem** subproblem = divide(problem);
					// ID des ersten Kindknotens
					long subproblemID = currentID * D + 1;

					for(int i = 0; i < D; i++) {
						if(isSimple(subproblem[i])) {
							solution = solve(subproblem[i]);
							// ATTENTION: THE FOLLOWING CALL IS ERRENOUS, SINCE NO OVERLOADED
							// CONSTRUCTOR OF FRAME TAKES TWO ARGUMENTS! HOWEVER, I DO NOT KNOW
							// WHICH PARAMETERS TO PASS TO THE CONSTRUCTOR, SUCH THAT THE CLASS
							// DCSOLVER IS NOT INCLUDED IN THE FINAL MUESLI PACKAGE!
							solutionFrame = new Frame<Solution>(subproblemID++, solution);
							solutionpool->insert(solutionFrame);
						}
						else {
							// Pair: (id, Problem*)
							// ATTENTION: SEE ABOVE
							Frame<Problem>* subproblemFrame = new Frame<Problem>(subproblemID++,
								subproblem[i]);
							workpool->insert(subproblemFrame);
						}
					}

					if(solutionpool->combineIsPossible())
						solutionpool->combine();

					// Hinweis in der Dokumentation, dass Speicher intern freigegeben wird!
					delete[] subproblem;
				}
				// Ende Problemverarbeitung
				
				// TERMINATION DETECTION (durch Master)
				if(Muesli::MSL_myId == masterSolver && (!finished)) {
					// fertig, wenn einzige Loesung im Solutionpool die ID "0" traegt
					if(solutionpool->hasSolution()) {
						solution = solutionpool->top()->getData();
						int receiver = getReceiver();
						
						if(debugCommunication)
							std::cout << Muesli::MSL_myId <<
							": DCSolver sendet Loesung an " << receiver << std::endl;

						MSL_Send(receiver, solution);
						solutionpool->pop();
						solution = NULL;
						// statistics
						numOfSolutionsSent++;
						// loese Blockade
						blocked = false;
					}
				}
			}
			// ende while

			time_solver = MPI_Wtime() - time_solver;

			if(analyse) {
				std::cout << Muesli::MSL_myId <<
					"start" << std::endl;
				std::cout << Muesli::MSL_myId <<
					": processed subproblems: " << numOfProblemsProcessed << std::endl;
				std::cout << Muesli::MSL_myId <<
					": simple subproblems: " << numOfSimpleProblemsSolved << std::endl;
				std::cout << Muesli::MSL_myId <<
					": shared subproblems: " << numOfSubproblemsSent << std::endl;
				std::cout << Muesli::MSL_myId <<
					": received subproblems: " << numOfSubproblemsReceived << std::endl;
				std::cout << Muesli::MSL_myId <<
					": work requests: " << numOfWorkRequestsSent << std::endl;
				std::cout << Muesli::MSL_myId <<
					": time_solve: " << time_solve << std::endl;
				std::cout << Muesli::MSL_myId <<
					": time_combine: " << time_combine << std::endl;
				std::cout << Muesli::MSL_myId <<
					": time_divide: " << time_divide << std::endl;
				std::cout << Muesli::MSL_myId <<
					"end" << std::endl;
			}

			if(debugCommunication)
				std::cout << Muesli::MSL_myId << ": DCSolver terminiert." << std::endl;
		}
		// end of start()

		// Berechnet die Ein- und Ausgaenge aller Solverkollegen.
		// @param solver	Zeiger auf das komplette Solverarray (dort ist man selbst auch eingetragen)
		// @param length	Laenge des Solverarrays
		// @param id		Index des Zeigers im Solverarray, der auf diesen Solver (this) zeigt
		//
		void setWorkmates(DCSolver<Problem, Solution>** solver, int length, int id) {
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

			// Sender und Empfaenger fuer das Token berechnen
			left = exitOfSolver[(id - 1 + length) % length];
			right = entranceOfSolver[(id + 1) % length];
		}


		Process* copy() {
			return new DCSolver<Problem, Solution>(divide, combine, solve, isSimple, D, noprocs);
		}

   		void show() const {
			if(Muesli::MSL_myId == 0)
				std::cout << "DCSolver (PID = " << entrances[0] << ")" << std::endl << std::flush;
		}

	};

}

#endif
