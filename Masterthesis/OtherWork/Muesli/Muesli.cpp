// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#include <cstdio>
#include <sstream>
#include <iostream>

#include "mpi.h"
#include "Process.h"
#include "Muesli.h"


namespace msl {

	// **************************************************
	// * initialize static class variables
	// **************************************************

	bool Muesli::MSL_COMMUNICATION = MSL_NOT_SERIALIZED;

	int Muesli::MSL_ARG1 = 0;
	int Muesli::MSL_ARG2 = 0;
	int Muesli::MSL_ARG3 = 0;
	int Muesli::MSL_ARG4 = 0;
	int Muesli::MSL_DISTRIBUTION_MODE;
	int Muesli::MSL_numOfTotalProcs;
	int Muesli::MSL_numOfLocalProcs = 1;
	int Muesli::numP  = 0;
	int Muesli::numS  = 0;
	int Muesli::numPF = 0;
	int Muesli::numSF = 0;

	char* Muesli::MSL_ProgrammName;

	double Muesli::MSL_Starttime;

	ProcessorNo Muesli::MSL_myEntrance = MSL_UNDEFINED;
	ProcessorNo Muesli::MSL_myExit = MSL_UNDEFINED;
	ProcessorNo Muesli::MSL_myId = MSL_UNDEFINED;
	ProcessorNo Muesli::MSL_runningProcessorNo = 0;
        
        Process* Muesli::MSL_myProcess = nullptr;

	// **************************************************
	// * auxiliary functions
	// **************************************************

	void throws(const Exception& e) {
		std::cout << Muesli::MSL_myId << ": " << e << std::flush;
	}

}

// **************************************************
// * initialize and terminate skeleton library
// **************************************************

/* Needs to be called before a skeleton is used
   MSL_ARG2 = 1: MSL_RANDOM_DISTRIBUTION
   MSL_ARG2 = 2: cyclic
*/
void msl::InitSkeletons(int argc, char** argv, bool serialization) {
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &Muesli::MSL_numOfTotalProcs);
	MPI_Comm_rank(MPI_COMM_WORLD, &Muesli::MSL_myId);

	// Parameter von der Konsole einlesen und verarbeiten
	if(1 <= argc) {
		Muesli::MSL_ProgrammName = argv[0];
	}

	if(2 <= argc) {
		Muesli::MSL_ARG1 = (int)strtol(argv[1], NULL,10);
	}

	if(3 <= argc) {
		Muesli::MSL_ARG2 = (int)strtol(argv[2], NULL,10);
	}

	if(4 <= argc) {
		Muesli::MSL_ARG3 = (int)strtol(argv[3], NULL,10);
	}

	if(5 <= argc) {
		Muesli::MSL_ARG4 = (int)strtol(argv[4], NULL,10);
	}

	// Kommunikationsmodi festlegen
	Muesli::MSL_DISTRIBUTION_MODE = msl::MSL_CYCLIC_DISTRIBUTION;
	// Serialisierung aktivieren / deaktivieren (per default aus)
	Muesli::MSL_COMMUNICATION = serialization;

	Muesli::MSL_myEntrance = 0;
	Muesli::MSL_myExit = 0;
	// ??? hier wird globales MSL_numOfLocalProcs = 1 doch direkt ueberschrieben? vgl. Atomic!
	Muesli::MSL_numOfLocalProcs = Muesli::MSL_numOfTotalProcs;
	// Zeitmessung: vgl. MPI_Wtick(void)
	Muesli::MSL_Starttime = MPI_Wtime();
}

/* Needs to be called after the skeletal computation is finished.
*/
void msl::TerminateSkeletons() {
	std::ostringstream s;

	MPI_Barrier(MPI_COMM_WORLD);

	if(Muesli::MSL_myId == 0) {
		s << "vers: " << MSL_VERSION_MAJOR << "." << MSL_VERSION_MINOR << std::endl;
		s << "name: " << Muesli::MSL_ProgrammName << std::endl;
		s << "comm: ";
		
		if(Muesli::MSL_DISTRIBUTION_MODE == msl::MSL_RANDOM_DISTRIBUTION) {
			s << "random, ";
		}
		else {
			s << "cyclic, ";
		}

		if(MSL_isSerialized()) {
			s << "serialized";
		}
		else {
			s << "not serialized";
		}

		s << std::endl << "proc: " << Muesli::MSL_numOfTotalProcs << std::endl;
		s << "size: " << Muesli::MSL_ARG1 << std::endl;
		s << "time: " << MPI_Wtime() - Muesli::MSL_Starttime << std::endl << std::flush;

		printf("%s", s.str().c_str());
	}

	// MPI beenden
	MPI_Finalize();
	// Anzahl arbeitender Prozessoren auf Null setzen
	Muesli::MSL_runningProcessorNo = 0;
}
