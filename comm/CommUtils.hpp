#include <vector>
#include <iostream>

extern const unsigned RX_BUFSIZE = 16*1024;

struct ControlMessage{
    int64_t id;
    APControl msgType;
    int argument;
};

typedef std::vector<Message*> MessagePtrVector;

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

enum APControl{
    APC_SET_STATE,
    APC_CHECKPOINT,
    APC_WAIT,
    APC_BARRIER,
};

//For reduce operation type
enum MPI_OP_T{
    SUM, MIN, MAX,
};

enum SendMode{
    SM_BUFFERED, SM_IMMEDIATE
};