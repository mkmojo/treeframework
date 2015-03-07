#ifndef COMMON_OPTIONS_H
#define COMMON_OPTIONS_H 1
#include <string>
/**
 * Global variables that are mostly constant for the duration of the
 * execution of the program.
 */
namespace opt {
	extern int numProc; 
    extern int level; // level of octTree, should be less than 21
    extern int domain;
	extern int rank;
	extern int verbose;
    extern std::string inFile;
}

#endif
