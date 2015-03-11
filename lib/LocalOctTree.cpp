#include "LocalOctTree.h"
#include "Common/Options.h"
#include "Algorithm.h"
#include <algorithm>
#include <time.h> 
#include <iostream>
#include "Point.h"
using namespace std;

const long NOT_INIT = -1;

int LocalOctTree::computeProcID(const Point& p) const
{
    return p.getCode() % (unsigned) opt::numProc;
}

void LocalOctTree::handle(int, const SeqAddMessage& message)
{
    assert(isLocal(message.m_point));
    //need to use the local structure to put content in message to local m_data
    m_data.push_back(message.m_point);
}

bool LocalOctTree::isLocal(const Point& p) const
{
    return computeProcID(p) == opt::rank;
}

void LocalOctTree::add(const Point &p)
{
    if(isLocal(p)){
        cout << p.x << " "  << p.y << " "  << p.z << " " 
            << p.mass << " local --> " <<  computeProcID(p) << endl;
        m_data.push_back(p);
    } else{
        cout << p.x << " "  << p.y << " "  << p.z << " " 
            <<  p.mass << " not local --> " << computeProcID(p) << endl;
        m_comm.sendSeqAddMessage(computeProcID(p), p);
    }
}

void LocalOctTree::parseControlMessage(int source)
{
    ControlMessage controlMsg = m_comm.receiveControlMessage();
    switch(controlMsg.msgType)
    {
        case APC_SET_STATE:
            SetState(NetworkActionState(controlMsg.argument));
            break;
        case APC_CHECKPOINT:
            //logger(4) << "checkpoint from " << source << ": "
            //<< controlMsg.argument << '\n';
            m_numReachedCheckpoint++;
            m_checkpointSum += controlMsg.argument;
            break;
        case APC_WAIT:
            SetState(NAS_WAITING);
            m_comm.barrier();
            break;
        case APC_BARRIER:
            assert(m_state == NAS_WAITING);
            m_comm.barrier();
            break;
    }
}


size_t LocalOctTree::pumpNetwork()
{
    for( size_t count = 0; ;count++){
        int senderID;
        APMessage msg = m_comm.checkMessage(senderID);
        switch(msg)
        {
            case APM_CONTROL:
                {
                    parseControlMessage(senderID);
                    //deal with control message before 
                    //any other type of message
                    return ++count;
                }
            case APM_BUFFERED:
                {
                    //Deal all message here until done
                    MessagePtrVector msgs;
                    m_comm.receiveBufferedMessage(msgs);
                    for(MessagePtrVector::iterator 
                            iter = msgs.begin();
                            iter != msgs.end(); iter++){
                        //handle message based on its type
                        (*iter)->handle(senderID, *this);
                        delete (*iter);
                        *iter = 0;
                    }
                    break;
                }
            case APM_NONE:
                return count;
        }
    }
}
void LocalOctTree::EndState()
{
    m_comm.flush();
}

//
// Set the action state 
//
void LocalOctTree::SetState(
        NetworkActionState newState)
{
    //logger(2) << "SetState " << newState
    //<< " (was " << m_state << ")\n";

    // Ensure there are no pending messages
    assert(m_comm.transmitBufferEmpty());

    m_state = newState;

    // Reset the checkpoint counter
    m_numReachedCheckpoint = 0;
    m_checkpointSum = 0;
}

void LocalOctTree::loadPoints()
{
    //Timer timer("LoadSequences");
    if(opt::rank == 0)
        FMMAlgorithms::loadPoints(this, opt::inFile); 
}


//Find local min and max for each dimension
//Find global min and max through MPI ALLreduce
//distribute the result
void LocalOctTree::setUpGlobalMinMax()
{
    //what do we do with a processor that does not 
    //have any data?
    assert(m_data.size() > 0);
    Point cntPoint = m_data[0];
    double minX(cntPoint.x), maxX(cntPoint.x);
    double minY(cntPoint.y), maxY(cntPoint.y);
    double minZ(cntPoint.z), maxZ(cntPoint.z);

    for(unsigned i=1;i<m_data.size();i++){
        Point cntPoint = m_data[i];
        //For current smallest cord
        if(cntPoint.x < minX){
            minX = cntPoint.x;
        }
        if(cntPoint.y < minY){
            minY = cntPoint.y;
        }
        if(cntPoint.z < minZ){
            minZ = cntPoint.z;
        }

        //For current lareges cord
        if(cntPoint.x > maxX){
            maxX = cntPoint.x;
        }
        if(cntPoint.y > maxY){
            maxY = cntPoint.y;
        }
        if(cntPoint.z > maxZ){
            maxZ = cntPoint.z;
        }
    }
    
    localMinX = minX;
    localMinY = minY;
    localMinZ = minZ;
    localMaxX = maxX;
    localMaxY = maxY;
    localMaxZ = maxZ;

    //set globla dimension
    globalMinX = m_comm.reduce(minX, MIN);
    globalMinY = m_comm.reduce(minY, MIN);
    globalMinZ = m_comm.reduce(minZ, MIN);
    //set global dimension
    globalMaxX = m_comm.reduce(maxX, MAX);
    globalMaxY = m_comm.reduce(maxY, MAX);
    globalMaxZ = m_comm.reduce(maxZ, MAX);
}

void LocalOctTree::printGlobalMinMax()
{
    cout << "DEBUG: globalMinX " << globalMinX <<endl;
    cout << "DEBUG: globalMinY " << globalMinY <<endl;
    cout << "DEBUG: globalMinZ " << globalMinZ <<endl;
    cout << "DEBUG: globalMaxX " << globalMaxX <<endl;
    cout << "DEBUG: globalMaxY " << globalMaxY <<endl;
    cout << "DEBUG: globalMaxZ " << globalMaxZ <<endl;
}


void LocalOctTree::setUpGlobalIndices()
{
    //global min and max;
    long min=0, max=0;

    long localMin = NOT_INIT;
    long localMax = NOT_INIT;

    for(unsigned i=0; i<m_cell_id.size();i++){
        long id = m_cell_id[i];
        if( id > localMax){
            localMax = id;
        }
        if( localMin == NOT_INIT || id < localMin){
            localMin = id;
        }
    }

    //cout << "DEBUG before: mix - min: " << max <<" - "<< min << endl;

    min = m_comm.reduce(localMin, MIN);
    max = m_comm.reduce(localMax, MAX);

    long k = max - min + 1;
    //cout << "DEBUG after: mix - min: " << max <<" - "<< min << endl;
    assert(k >= 0);

    long nodeCounter[k];
    long localNodeCounter[k];

    //init things
    for(unsigned i=0;i<k;i++) {
        localNodeCounter[i] = 0;
        nodeCounter[i] = 0;
    }

    //keep track of which node ids are avaiable locally
    for(unsigned i=0;i<m_cell_id.size();i++){
        localNodeCounter[m_cell_id[i] - min]++;
    }
    
    m_comm.reduce(localNodeCounter, nodeCounter, k);

    long globalIndices[k];
    long index = 0;
    for(int i=0; i<k; i++){
        if(nodeCounter[i] > 0){
            globalIndices[i] = index++;
        }else{
            globalIndices[i] = NOT_INIT;
        }
    }

    for(unsigned i=0; i<m_cell_id.size(); i++){
        long globalIndicesArrayIndex=m_cell_id[i] - min;
        m_cell_id[i]=globalIndices[globalIndicesArrayIndex];
    }
    
}

void LocalOctTree::printPoints()
{
    //simple MPI critical section
    int rank = 0;
    while(rank < opt::numProc){
        if(opt::rank == rank){
            if(m_data.size() == 0) {
                cout << "DEBUG " << opt::rank << ": empty" <<endl;
            } else{
                for(unsigned i=0; i<m_data.size();i++) {
                    Point p = m_data[i];
                    cout << "DEBUG " << opt::rank << ": cord ";
                    p.print_cord();
                    cout << "DEBUG " << opt::rank << ": m_point ";
                    p.print_m_point();
                }
            }
        }
        rank++;
        m_comm.barrier();
    }
}

bool LocalOctTree::checkpointReached() const
{
    return m_numReachedCheckpoint == (unsigned) opt::numProc;
}

long computeKey( unsigned int x, unsigned int y, unsigned z)
{
    // number of bits required for each x, y and z = height of the 
    // tree = level_ - 1
    // therefore, height of more than 21 is not supported
    unsigned int height = opt::level - 1;

    int key = 1;
    long x_64 = (long) x << (64 - height);
    long y_64 = (long) y << (64 - height);
    long z_64 = (long) z << (64 - height);
    long mask = 1;
    mask = mask << 63; // leftmost bit is 1, rest 0

    for(unsigned int i = 0; i < height; ++i) {
        key = key << 1;
        if(x_64 & mask) key = key | 1;
        x_64 = x_64 << 1;

        key = key << 1;
        if(y_64 & mask) key = key | 1;
        y_64 = y_64 << 1;

        key = key << 1;
        if(z_64 & mask) key = key | 1;
        z_64 = z_64 << 1;
    } // for

    return key;

}

long LocalOctTree::getCellId(const Point& p)
{
    //return the cell id for a point; 
    //r stands for range 
    double x = p.x - localMinX;
    double y = p.y - localMinY;
    double z = p.z - localMinZ;
    double rx = localMaxX - localMinX;
    double ry = localMaxY - localMinY;
    double rz = localMaxZ - localMinZ;

    double domain = (rx > ry && rx > rz) ?  
        rx :
        ((ry > rz) ? ry : rz);

    unsigned int numCells = 1 << (opt::level - 1);
    double cellSize = domain / numCells;
    int xKey = (unsigned int) (x / cellSize);
    int yKey = (unsigned int) (y / cellSize);
    int zKey = (unsigned int) (z / cellSize);

    return computeKey(xKey, yKey, zKey);
}

void LocalOctTree::setUpCellIds()
{
    for(unsigned i=0; i < m_data.size(); i++) {
        int cell_id = getCellId(m_data[i]);
        vector<long>::const_iterator it;
        it = find(m_cell_id.begin(), m_cell_id.end(), cell_id);
        if( it == m_cell_id.end()){
            m_cell_id.push_back(cell_id);
        }
    }
}


void LocalOctTree::printCellIds()
{
    cout <<  "DEBUG: " << "rank: " << opt::rank << endl;
    for(unsigned int i=0; i<m_cell_id.size();i++){
        cout << m_cell_id[i]<<" ";
    }
    cout << endl;
}

//
// Run master process state machine
//
void LocalOctTree::runControl()
{
    SetState(NAS_LOADING);
    while(m_state != NAS_DONE){
        switch(m_state){
            case NAS_LOADING:
                {
                    loadPoints();
                    EndState();

                    m_numReachedCheckpoint++;
                    while(!checkpointReached())
                        pumpNetwork();

                    SetState(NAS_LOAD_COMPLETE);
                    m_comm.sendControlMessage(APC_SET_STATE,
                            NAS_LOAD_COMPLETE);

                    m_comm.barrier();
                    pumpNetwork();

                    SetState(NAS_SETUP_GLOBAL_MIN_MAX);
                    break;
                }
            case NAS_SETUP_GLOBAL_MIN_MAX:
                {
                    //TODO
                    //figure out global minimum and maximum
                    m_comm.sendControlMessage(APC_SET_STATE,
                            NAS_SETUP_GLOBAL_MIN_MAX);
                    m_comm.barrier();
                    setUpGlobalMinMax();
                    EndState();
                    //DEBUG
                    //printGlobalMinMax();

                    SetState(NAS_SETUP_NODEID);
                    break;
                }
            case NAS_SETUP_NODEID:
                {
                    m_comm.sendControlMessage(APC_SET_STATE,
                            NAS_SETUP_NODEID);
                    m_comm.barrier();
                    setUpCellIds();
                    EndState();
                    //printCellIds();
                    SetState(NAS_SETUP_GLOBAL_INDECIES);
                    break;
                }
            case NAS_SETUP_GLOBAL_INDECIES:
                {
                    m_comm.sendControlMessage(APC_SET_STATE,
                            NAS_SETUP_GLOBAL_INDECIES);
                    m_comm.barrier();
                    setUpGlobalIndices();
                    m_comm.barrier();
                    printCellIds();
                    SetState(NAS_DONE);
                    break;
                }
            case NAS_DONE:
                break;
        }
    }
}

//
// Run worker state machine
//
void LocalOctTree::run()
{
    SetState(NAS_LOADING);
    while(m_state != NAS_DONE){
        switch (m_state){
            case NAS_LOADING:
                {
                    loadPoints();
                    EndState();
                    SetState(NAS_WAITING);
                    m_comm.sendCheckPointMessage();
                    break;
                }
            case NAS_LOAD_COMPLETE:
                {
                    m_comm.barrier();
                    pumpNetwork();

                    SetState(NAS_WAITING);
                    break;
                }
            case NAS_SETUP_GLOBAL_MIN_MAX:
                {
                    //TODO
                    //figure out global minimum and maximum
                    //and save them 
                    m_comm.barrier();
                    setUpGlobalMinMax();
                    EndState();
                    SetState(NAS_WAITING);
                    break;
                }
            case NAS_SETUP_NODEID:
                {
                    m_comm.barrier();
                    setUpCellIds();
                    EndState();
                    //printCellIds();
                    SetState(NAS_WAITING);
                    break;
                }
            case NAS_SETUP_GLOBAL_INDECIES:
                {
                    m_comm.barrier();
                    setUpGlobalIndices();
                    m_comm.barrier();
                    printCellIds();
                    SetState(NAS_DONE);
                    break;
                }
            case NAS_WAITING:
                pumpNetwork();
                break;
            case NAS_DONE:
                break;
        }
    }
}

//should be modified to return a difference
void LocalOctTree::pumpFlush()
{
    pumpNetwork();
    m_comm.flush();
}
