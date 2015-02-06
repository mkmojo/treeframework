#ifndef LOCALOCTTREE_H
#define LOCALOCTTREE_H 1
#include "BaseTree.h"

/* -------------------------- OctTree ---------------------------- */
class LocalOctTree : public LocalBaseTree
{
    public:
        LocalOctTree()
            :m_state(NAS_WAITING){};
        void runControl();
        void run();
        void SetState(NetworkActionState newState);
        void EndState();
        void handle(int, const SeqAddMessage& Message);
        void add(Point& p);
        
        //Recieve and dispatch packets
        size_t pumpNetwork();
    private:
        NetworkActionState m_state;
        MessageBuffer m_comm;
        unsigned m_numReachedCheckpoint;

        unsigned m_checkpointSum;
        Vector<DataPoint> m_data;
}
#endif
