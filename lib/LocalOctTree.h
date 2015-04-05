#ifndef LOCALOCTTREE_H
#define LOCALOCTTREE_H 1
#include "BaseTree.h"
#include "Point.h"
#include "Cell.h"
#include <vector>
#include <iostream>

enum NetworkActionState
{
    NAS_LOADING, // loading points
    NAS_LOAD_COMPLETE, // loading is complete
    NAS_WAITING, // non-control process is waiting
    NAS_SORT,
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
        void handle(int senderID, const SeqSortMessage& message);
        void setUpGlobalMinMax();
        void printGlobalMinMax();
        void sortLocalPoints();
        long setCellId(Point &p);
        void setUpPointIds();
        void getLocalSample();
        void setGlobalPivot();
        void distributePoints();

        //DEBUG
        NetworkActionState getState(){
            return m_state;
        }
        void printCellIds(std::string sectionName);

        //Recieve and dispatch packets
        size_t pumpNetwork();
    private:
        void loadPoints();
        void printPoints();
        long getCellId(const Point& p);
        NetworkActionState m_state;
        MessageBuffer m_comm;
        unsigned m_numReachedCheckpoint;
        void parseControlMessage(int source);

        unsigned m_checkpointSum;
        std::vector<Point> m_data;
        std::vector<Point> m_sort_buffer;
        std::vector<long> m_cbuffer;
        std::vector<long> m_pivotbuffer;
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
