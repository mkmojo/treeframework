#include <mpi.h>
#include <climits>
#include <iostream>
#include <unistd.h>
#include "Common/Options.h"
#include "LocalOctTree.h"

using namespace std;

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &opt::rank);
    MPI_Comm_size(MPI_COMM_WORLD, &opt::numProc);
    //opt::inFile = "file.txt";

    MPI_Barrier(MPI_COMM_WORLD);
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, sizeof hostname);
    MPI_Barrier(MPI_COMM_WORLD);

    if(opt::rank == 0 ){
        LocalOctTree localTree;
        localTree.runControl();
    }else{
        LocalOctTree localTree;
        localTree.run();
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    
    if(opt::rank == 0){
        cout << "Done." <<endl;
    }

    return 0;
}
