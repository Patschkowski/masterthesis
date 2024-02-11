############################################################################################################################################################
# Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen                        
#         		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de             
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to   # deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/    # or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN # THE SOFTWARE.																																			   ############################################################################################################################################################

##############################################################################
#                                                                            #
#              Makefile for the MUESLI test case suite                       #
#                                                                            #
#                                                                            #
# Usage:                                                                     #
#                                                                            #
#       'make' builds all test cases                		                 #
#		'make testruns' builds all test cases and executes them              #
#       'make clean' removes all files created by the build/run process      #
#       'make new' is combination of 'make clean' and 'make'                 #
#                                                                            #
##############################################################################


SHELL      = /bin/sh

#
#  Adjust these vars to match your MPI C++ compiler and job launcher:
#

CXX      =  mpicxx
MPIEXEC  =  mpirun


#
#  Append your compiler options to CXXFLAGS:
#

CXXFLAGS   +=  -Wall        
CXXFLAGS   +=  -O2          
CXXFLAGS   +=  -fopenmp     
CXXFLAGS   +=  -DMPICH_IGNORE_CXX_SEEK       # see http://www.spec.org/mpi2007/flags/MPI2007_flags.20070717.02.html#Section_portability


#
#  Enable some implicit rules:
#

.SUFFIXES:                # This disables all the default rules.
.SUFFIXES: .cpp .o .h     # This enables only the ones we use here.



# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- #


#
#  List of unit tests.
#
#  We could avoid this by pattern-matching Test*.cpp and then removing
#  the extension, but I like being able to comment stuff out here...
#

TESTS = TestGauss \
        TestMatMult \
        TestShortest\
		TestVarious \
        TestAtomic \
        TestInitial \
        TestFinal \
        TestProcess \
        TestPipe \
        TestFarm \
        TestFrame \
        TestWorkpool \
        TestWorkpoolManager \
        TestWorkpoolManager2 \
        TestDistributedArray \
        TestDistributedMatrix \
        TestDistributedSparseMatrix \
        TestSamplesort \
        TestMisc \
        FrobeniusNorm\
        TestBitonic \
        TestBranchAndBound \
        #TestDivideAndConquer \ #produces errors, please refer to the file 'TestDivideAndConquer.cpp'


#
#  Now the list of test *runs*: same names, only with `.run' appended.
#

TESTRUNS = $(TESTS:=.run)

#
# Build all.
#

all: $(TESTS)


#
#  Run all tests.
#

testruns: $(TESTRUNS)


#
#  Run a single test.
#

Test%.run: Test%
	@echo "\n=========================================================================================="
	@echo "  `date`  `uname -srp`    $@" | tee $@
	@echo "  `whoami`$$ mpirun -np $(NP) $<" | tee -a $@
	@echo "==========================================================================================\n"
	@mpirun -np $(NP) $< | tee -a $@
	@echo "\n"

FrobeniusNorm.run: FrobeniusNorm
	@echo "\n=========================================================================================="
	@echo "  `date`  `uname -srp`    $@" | tee $@
	@echo "  `whoami`$$ mpirun -np $(NP) $<" | tee -a $@
	@echo "==========================================================================================\n"
	@mpirun -np $(NP) $< | tee -a $@
	@echo "\n"
#
#  Delete non-essential files, i.e. by-products of last build:
#

cleanobjs:
	@rm -f *.o
	@echo "Removed object code files"
cleantests:
	@rm -f $(TESTS)
	@echo "Removed test case binaries"
cleanruns:
	@rm -f $(TESTRUNS)
	@echo "Removed test case run logs"
cleandeps:
	@rm -f deps
	@echo "Removed cached deps file"


#
#  Delete all of the above except for the deps file.
#

clean: cleantests cleanobjs cleanruns


#
#  Rebuild everything from scratch.
#

new: clean all

#
#  Common object code. Every test in the suite requires linkage of these:
#

$(TESTS): Muesli.o Utility.o


#
#  For the remaining tests, minimal link requirements are as follows:
#

TestProcess TestFrame TestBranchAndBound TestWorkpool \
TestWorkpoolManager TestWorkpoolManager2: Double.o

TestDistributedArray: DistributedDataStructure.o

TestDistributedSparseMatrix TestMisc: DistributedDataStructure.o \
  MatrixIndex.o BsrIndex.o OAL.o Distribution.o \
  RoundRobinDistribution.o  BlockDistribution.o 

TestSamplesort TestFft TestBitonic TestMatMult TestGauss: \
  DistributedDataStructure.o OAL.o

FrobeniusNorm: BsrIndex.o Distribution.o OAL.o RoundRobinDistribution.o

#
#  The number of processes required by each test.
#

$(TESTRUNS):                        NP = 1   # default #
TestFarm.run:                       override   NP = 3
TestPipe.run:                       override   NP = 3
TestDistributedArray.run:           override   NP = 2
TestDistributedMatrix.run:          override   NP = 4
TestDistributedSparseMatrix.run:    override   NP = 4
TestMatMult.run:                    override   NP = 4
TestGauss.run:                      override   NP = 2
TestMisc.run:                       override   NP = 4
TestFft.run:                        override   NP = 2
TestBranchAndBound.run:             override   NP = 2
TestDivideAndConquer.run:           override   NP = 3
TestVarious.run:					override   NP = 4


#
#  Compute all dependencies by source code inspection, and save them
#  (as a set of Makefile-compatible rules) to a `deps' text file...
#

deps: 
	@echo "Checking dependencies ..."
	@$(CXX) -MM -E *.cpp > deps


#
#  ...then -include it here, extending this Makefile with up-to-date rules:
#

-include deps


#
#  Pseudo-targets are useful to model unconditional chores and/or shortcuts:
#

.PHONY: all new tests testruns clean cleanobjs cleantests cleanruns cleandeps
