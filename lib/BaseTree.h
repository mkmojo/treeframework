#ifndef BASETREE_H
#define BASETREE_H 1
#include "Common/Log.h"
#include "MessageBuffer.h"

using namespace std;

enum NetworkActionState
{
    NAS_LOADING, // loading points
    NAS_LOAD_COMPLETE, // loading is complete
    NAS_WAITING, // non-control process is waiting
    NAS_DONE // finished, clean up and exit
};


/* -------------------------- BaseTree --------------------------- */
class BaseTree
{
    public:
        virtual ~BaseTree() {}
        virtual void add() = 0;
        virtual size_t pumpNetwork() = 0;
};
#endif
