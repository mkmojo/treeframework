#include "LocalOctTree.h"
#include "Common/Options.h"
#include "Algorithm.h"
#include "Point.h"
#include "Cell.h"
#include <algorithm>
#include <time.h> 
#include <iostream>
#include <set>
#include <math.h>
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

void LocalOctTree::handle(int, const SeqSortMessage& message)
{
    m_sort_buffer.push_back(message.m_point);
}

bool LocalOctTree::isLocal(const Point& p) const
{
    return computeProcID(p) == opt::rank;
}

void LocalOctTree::add(const Point &p)
{
    if(isLocal(p)){
        //cout << p.x << " "  << p.y << " "  << p.z << " " 
        //<< p.mass << " local --> " <<  computeProcID(p) << endl;
        m_data.push_back(p);
    } else{
        //cout << p.x << " "  << p.y << " "  << p.z << " " 
        //<<  p.mass << " not local --> " << computeProcID(p) << endl;
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
    //TODO: what do we do with a processor that does not 
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


template<typename T>
static void printVector(const vector<T> vec, string info){
    cout << "DEBUG " << opt::rank << ": ";
    cout << info << endl;
    for(size_t i=0;i<vec.size();i++){
        cout << vec[i] << " ";
    }
    cout << endl;
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
                    //cout << "DEBUG " << opt::rank << ": m_point ";
                    //p.print_m_point();
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

//TODO:
//Probably this should not be a member function
//Pass min&max as parameter might be a better option
long LocalOctTree::setCellId(Point &p)
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

    return p.setCellId(computeKey(xKey, yKey, zKey));
}

void LocalOctTree::setUpPointIds()
{
    for(unsigned i=0; i < m_data.size(); i++) {
        //TODO:
        //is it better to have setCellId as a member
        //function of Point class?
        setCellId(m_data[i]);
    }
}

bool cmpPoint(const Point &p1, const Point &p2)
{
    return  p1.getCellId()< p2.getCellId();
}

void LocalOctTree::sortLocalPoints()
{
    long numTotal = 0;
    numTotal = m_comm.reduce((long)m_data.size());
    if(opt::rank == 0) 
        cout<< "DEBUG: " << numTotal<<" int total."  <<endl;

    //TODO: extend this to user defined data type
    //So customized iterator/cmp function etc.
    sort(m_data.begin(), m_data.end(), cmpPoint);
}

//void LocalOctTree::printCellIds(string sectionName)
//{
//    cout <<  "DEBUG " << opt::rank<< ": "<< sectionName << endl;
//    for(unsigned int i=0; i<m_cells.size();i++){
//        cout << m_cells[i] << " ";
//    }
//    cout << endl;
//}

void LocalOctTree::getLocalSample()
{
    //sample local data points

	double step = 1.0*m_data.size()/opt::numProc;
	int start = round((step-1) * (1.0 * opt::rank / (opt::numProc-1)));
//	cout<<"step="<<step<<" data size = "<<m_data.size()<<" start="<<start<<endl;
    for(unsigned int i=0;i<opt::numProc; i++)
    {
		int index = int(start + i * step);
        long id = m_data[index].getCellId();
        m_cbuffer.push_back(id);
    }
//    printVector(m_cbuffer, "local samples");
    // cout<< "DEBUG " << opt::rank << ": " << m_cbuffer.size() <<endl; 
    // for(unsigned int i=0;i<m_cbuffer.size();i++)
    // {
    //     cout << m_cbuffer[i] << " ";
    // }
    // cout <<endl;
}

template<typename T>
void removeDuplicates(vector<T>& nodeIds){
	set<int> s( nodeIds.begin(), nodeIds.end() );
	nodeIds.assign(s.begin(), s.end());
}

void LocalOctTree::setGlobalPivot()
{
    unsigned int numGlobalSample= opt::numProc * opt::numProc;
    m_pivotbuffer.resize(numGlobalSample);
    if(opt::rank == 0){
        m_comm.gather(&m_cbuffer[0], m_cbuffer.size(), 
                &m_pivotbuffer[0], opt::numProc);
        removeDuplicates(m_pivotbuffer);
        sort(m_pivotbuffer.begin(), m_pivotbuffer.end());

        //DEBUG
        //cout << "DEBUG " << opt::rank << ": " << m_pivotbuffer.size()<<endl;
        //for(unsigned int i=0; i<m_pivotbuffer.size(); i++) {
        //    cout << m_pivotbuffer[i] << " ";
        //}
        //cout << endl;
    }
    else{
        m_comm.receiveGather(&m_cbuffer[0], m_cbuffer.size(), 
                &m_pivotbuffer[0], opt::numProc);
    }

    //Sample global pivots
    if(opt::rank == 0){
//    	printVector(m_pivotbuffer, "Sorted samples");
    	double step = 1.0*m_pivotbuffer.size()/opt::numProc;
        for(int i=0;i<opt::numProc; i++) {
            long id = m_pivotbuffer[int(i*step)];
            m_cbuffer[i] = id;
        }
//        printVector(m_cbuffer, "boundaries");
    }

    //Bcast pivots to all procs
    if(opt::rank == 0) {
        m_comm.bcast(&m_cbuffer[0], opt::numProc);
        m_cbuffer.resize(opt::numProc);
//        cout << "DEBUG " << opt::rank << ": " << m_cbuffer.size()<<endl;
//        for(unsigned int i=0; i<m_cbuffer.size(); i++) {
//            cout << m_cbuffer[i] << " ";
//        }
//        cout << endl;
    }
    else{
        m_comm.receiveBcast(&m_cbuffer[0], opt::numProc);
        m_cbuffer.resize(opt::numProc);
//        cout << "DEBUG " << opt::rank << ": " << m_cbuffer.size()<<endl;
//        for(unsigned int i=0; i<m_cbuffer.size(); i++) {
//            cout << m_cbuffer[i] << " ";
//        }
//        cout << endl;
    }
}

//lengths will be modified to contain number of points at each splitter
static void setUpLengthsArray(vector<int> &lengths, 
        const vector<Point> &data, const vector<long> &splitter)
{
    size_t i = 0;
    for(size_t j=0;j<splitter.size()-1;j++){
        long left = splitter[j];
        long right = splitter[j+1];

        //cout << "DEBUG " << opt::rank <<": " << left << " " <<right<<endl;
        while(true){
            long cntId = data[i].getCellId();
            if(cntId < right && cntId >= left){
                //TODO do I need to create a new message 
                //for different buffer?
                //cout <<"sent "<<data[i].getCellId() << endl;
                //m_comm.sendSeqSortMessage(j, data[i]);
                i++;
                lengths[j]++;
            }else {
                break;
            }
        }
    }

    //send the rest to the very last proce
    //cout << "DEBUG " << opt::rank <<": " << data.size() <<endl;
    while(i<data.size()){
        //m_comm.sendSeqSortMessage(opt::numProc - 1, data[i]);
        i++;
        lengths[opt::numProc - 1]++;
    }
}

void LocalOctTree::print_m_alllengths() const
{
    assert(m_alllengths.size() == 
            (unsigned int)opt::numProc *(unsigned int) opt::numProc);

    cout << "DEBUG "<< opt::rank << ": size: "  << m_alllengths.size() <<endl;
    for(size_t i = 0; i < m_alllengths.size(); i++){
        cout << m_alllengths[i] << " ";
    }
    cout << endl;
}

void LocalOctTree::print_m_allstarts() const
{
    assert(m_allstarts.size() == 
            (unsigned int)opt::numProc *(unsigned int) opt::numProc);

    cout << "DEBUG "<< opt::rank << ": size: "  << m_allstarts.size() <<endl;
    for(size_t i = 0; i < m_allstarts.size(); i++){
        cout << m_allstarts[i] << " ";
    }
    cout << endl;
}

static void setUpStartsArray(vector<int> &starts, const vector<int> &lengths)
{
   for(size_t i=1;i<lengths.size();i++) {
       starts[i] = starts[i-1] + lengths[i-1];
   }
}

static void setUpRecvLengths(const vector<int> &allLengths, vector<int> & rLengths)
{
    int step = opt::numProc;
    for(int i=0; i<opt::numProc; i++){
        rLengths[i] = allLengths[step*i + opt::rank];
    }
}

static void setUpRecvStarts(const vector<int> &rLengths, vector<int> & rStarts)
{
    for(size_t i=1;i<rLengths.size();i++){
       rStarts[i] = rStarts[i-1] + rLengths[i-1];
    }
}

template <typename T>
static void scale(vector<T>& vec)
{
    for(size_t i=0;i<vec.size();i++) {
        vec[i] *= sizeof(Point);
    }
}

template <typename T>
static long totalLength(const vector<T> lengths) {

    long total = 0;
    for(size_t i=0; i<lengths.size(); i++){
        total += lengths[i];
    }

    return total;
}

void LocalOctTree::distributePoints()
{
    vector<int> lengths(opt::numProc, 0);
    vector<int> starts(opt::numProc, 0);
    vector<int> rLengths(opt::numProc, 0);
    vector<int> rStarts(opt::numProc, 0);

    setUpLengthsArray(lengths, m_data, m_cbuffer);
    setUpStartsArray(starts, lengths);

    //all_gather lengths 
    //recved lengths will be placed in rank order
    m_alllengths.resize(opt::numProc * opt::numProc);
    MPI_Allgather(&lengths[0], lengths.size(), MPI_INT, 
            &m_alllengths[0], lengths.size(), MPI_INT, 
            MPI_COMM_WORLD);
    //print_m_lengthbuffer();

    m_allstarts.resize(opt::numProc * opt::numProc);
    MPI_Allgather(&starts[0], starts.size(), MPI_INT, 
            &m_allstarts[0], starts.size(), MPI_INT, 
            MPI_COMM_WORLD);
    //print_m_allstarts();

    setUpRecvLengths(m_alllengths, rLengths);
    setUpRecvStarts(rLengths, rStarts);

//    printVector(rLengths, "rLengths");
//    printVector(rStarts, "rStarts");
//    printVector(lengths, "lengths");
//    printVector(starts,"starts");

    m_sort_buffer.resize(totalLength(rLengths));

    //getting the right number of bytes
    scale(lengths); //<--length for local
    scale(starts);  //<--starts for local
    scale(rLengths);//<--from all proc
    scale(rStarts); //<--from all proc

    printVector(m_data, "m_data");

    MPI_Alltoallv(&m_data[0], &lengths[0], &starts[0], MPI_BYTE, &m_sort_buffer[0], &rLengths[0], &rStarts[0], MPI_BYTE, MPI_COMM_WORLD);
//    for(int i=0;i<opt::numProc;i++) {
//        MPI_Scatterv(&m_data[0], &lengths[0], &starts[0], MPI_BYTE,
//                &(m_sort_buffer[0 + rStarts[i]]), rLengths[i], MPI_BYTE,
//                i, MPI_COMM_WORLD);
//    }
    sort(m_sort_buffer.begin(), m_sort_buffer.end(), cmpPoint);
    printVector(m_sort_buffer, "m_sort_buffer");
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
                    //Load complete
                    SetState(NAS_LOAD_COMPLETE);
                    m_comm.sendControlMessage(APC_SET_STATE,
                            NAS_LOAD_COMPLETE);
                    m_comm.barrier();
                    //printPoints();

                    pumpNetwork();
                    SetState(NAS_SORT);
                    break;
                }
            case NAS_SORT:
                {
                    m_comm.sendControlMessage(APC_SET_STATE,
                            NAS_SORT);
                    m_comm.barrier();
                    setUpGlobalMinMax();
                    setUpPointIds();
                    sortLocalPoints();
                    getLocalSample();
                    setGlobalPivot();
                    m_comm.barrier();
                    distributePoints();

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
                    //printPoints();

                    pumpNetwork();
                    SetState(NAS_SORT);
                    break;
                }
            case NAS_SORT:
                {
                    m_comm.barrier();
                    setUpGlobalMinMax();
                    setUpPointIds();
                    sortLocalPoints();
                    getLocalSample();
                    setGlobalPivot();
                    m_comm.barrier();
                    distributePoints();

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
