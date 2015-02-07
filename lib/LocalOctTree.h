#ifndef LOCALOCTTREE_H
#define LOCALOCTTREE_H 1
#include "BaseTree.h"
#include "Point.h"
#include <vector>

/* -------------------------- OctTree ---------------------------- */
class LocalOctTree : public BaseTree
{
    public:
        LocalOctTree()
            :m_state(NAS_WAITING){};
        void runControl();
        void run();
        void SetState(NetworkActionState newState);
        void EndState();
        void add(const Point& p);
        void pumpFlush();
        bool checkpointReached() const;
        bool isLocal(const Point& p) const;
        int computeProcID(const Point& p) const;
        void handle(int senderID, const SeqAddMessage& message);
        
        //Recieve and dispatch packets
        size_t pumpNetwork();
    private:
        void loadPoints();
        NetworkActionState m_state;
        MessageBuffer m_comm;
        unsigned m_numReachedCheckpoint;
        void parseControlMessage(int source);

        unsigned m_checkpointSum;
        std::vector<Point> m_data;
};
#endif
