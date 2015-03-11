#include "Options.h"
#include <iostream>
#include <string>

namespace opt {

    /** MPI rank */
    int rank = 0;

    /** Number of MPI processes */
    int numProc = 1;

    /** Verbose output */
    int verbose;


    /** OctTree height **/
    int level;

    /** Input filename **/
    std::string inFile;
};
