#include "Options.h"
#include <iostream>

namespace opt {

	/** MPI rank */
	int rank = 0;

	/** Number of MPI processes */
	int numProc = 1;

	/** Verbose output */
	int verbose;

    /** OctTree height **/
    int treeHeight;

    char inFile = "file.txt";
}
