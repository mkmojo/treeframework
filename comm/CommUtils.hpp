#include <vector>
#include <iostream>

extern const unsigned RX_BUFSIZE = 16*1024;
extern const unsigned MAX_MESSAGES = 1024;
extern const unsigned numProc = 64; //sl15: this can be parsed from the command line
extern int procRank = 0; //sl15: this is determined by calling mpi_rank

enum APControl{
    APC_SET_STATE,
    APC_CHECKPOINT,
    APC_WAIT,
    APC_BARRIER,
};


struct ControlMessage{
    int64_t id;
    APControl msgType;
    int argument;
};

enum NetworkActionState{
    NAS_LOADING, // loading points
    NAS_LOAD_COMPLETE, // loading is complete
    NAS_WAITING, // non-control process is waiting
    NAS_SORT,
    NAS_DONE // finished, clean up and exit
};

enum APMessage{
    APM_NONE,
    APM_CONTROL,
    APM_BUFFERED,
};

//For reduce operation type
enum MPI_OP_T{
    SUM, MIN, MAX,
};

enum SendMode{
    SM_BUFFERED, SM_IMMEDIATE
};
