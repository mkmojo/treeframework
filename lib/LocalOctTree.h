#ifndef LOCALOCTTREE_H
#define LOCALOCTTREE_H 1
#include "BaseTree.h"
#include "Point.h"
#include <vector>

enum NetworkActionState
{
    NAS_LOADING, // loading points
    NAS_LOAD_COMPLETE, // loading is complete
    NAS_SORT,
    NAS_WAITING, // non-control process is waiting
    NAS_DONE // finished, clean up and exit
};

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
        void setUpGlobalMinMax();
        //return local

        //DEBUG
        NetworkActionState getState(){
            return m_state;
        }
        
        //Recieve and dispatch packets
        size_t pumpNetwork();
    private:
        void loadPoints();
        void printPoints();
        NetworkActionState m_state;
        MessageBuffer m_comm;
        unsigned m_numReachedCheckpoint;
        void parseControlMessage(int source);

        unsigned m_checkpointSum;
        std::vector<Point> m_data;
        //TODO
        //delete all these stand alone variable
        //and use a container for them
        double localMinX, localMaxX;
        double localMinY, localMaxY;
        double localMinZ, localMaxZ;
        double globalMinX, globalMaxX;
        double globalMinY, globalMaxY;
        double globalMinZ, globalMaxZ;
};
#endif
