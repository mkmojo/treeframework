#ifndef COMMLAYER_HPP
#define COMMLAYER_HPP
#include "Message.hpp"
#include "CommUtils.hpp"
#include <mpi.h>
#include <cassert>
#include <cstring>
#include <vector>

class CommLayer{
    uint8_t* m_rxBuffer;
    MPI_Request m_request;

    bool _request_get_status(const MPI_Request& req, MPI_Status& status){
        int flag;
        MPI_Request_get_status(req, &flag, &status);
        //Work around a bug prestn in Open MPI 1.3.3
        //and eailer.
        //MPI_Request_get_status may return false on
        //the first call even though a message
        //is waiting. The second call should work.
        if(!flag)
            MPI_Request_get_status(req, &flag, &status);
        return flag;
    }

public:
    CommLayer() : m_rxBuffer(new uint8_t[RX_BUFSIZE]), m_request(MPI_REQUEST_NULL){
        MPI_Comm_size(MPI_COMM_WORLD, &numProc);
    	MPI_Comm_rank(MPI_COMM_WORLD, &procRank);
        MPI_Irecv(m_rxBuffer, RX_BUFSIZE, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &m_request);
    }

    ~CommLayer(){
        MPI_Cancel(&m_request);
        delete[] m_rxBuffer;
    }

    APMessage checkMessage(int &senderID){
        MPI_Status status;
        bool flag = _request_get_status(m_request, status);
        if (flag) senderID = status.MPI_SOURCE;
        return flag ? (APMessage)status.MPI_TAG : APM_NONE;
    }

    inline void barrier(){ MPI_Barrier(MPI_COMM_WORLD); }

    inline bool isMaster(){
    	return procRank == 0;
    }

    inline bool isLastProc(){
		return procRank == (numProc-1);
	}

    // Send a buffered message
    // Need to be Isend later to avoid deadlock 
    inline void sendBufferedMessage(int destID, char* msg, size_t size){ 
        MPI_Send(msg, size, MPI_BYTE, destID, APM_BUFFERED, MPI_COMM_WORLD); 
    }

    //TODO Add Wrapper for MPI_Wait

    void getRecvBufferAddress(char** poutaddress, size_t *poutlength){
        int flag;
        MPI_Status status;
        MPI_Test(&m_request, &flag, &status);
        assert(flag);
        assert((APMessage)status.MPI_TAG == APM_BUFFERED);

        *poutaddress = (char*)m_rxBuffer;

        int size;
        MPI_Get_count(&status, MPI_BYTE, &size);
        *poutlength = size;
    }

    void Irecv(){
        assert(m_request == MPI_REQUEST_NULL);
        MPI_Irecv(m_rxBuffer, RX_BUFSIZE, 
                MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, 
                &m_request);
    }

    double reduce( double count, REDUCE_OP op) {
        //logger(4) << "entering reduce\n";
        double res;
        if(op == SUM)
            MPI_Allreduce(&count, &res, 1, MPI_DOUBLE, MPI_SUM,
                    MPI_COMM_WORLD);

        if(op == MIN)
            MPI_Allreduce(&count, &res, 1, MPI_DOUBLE, MPI_MIN,
                    MPI_COMM_WORLD);

        if(op == MAX)
            MPI_Allreduce(&count, &res, 1, MPI_DOUBLE, MPI_MAX,
                    MPI_COMM_WORLD);

        //logger(4) << "left reduce\n";
        return res;
    }

    void gather(long* send, int sendSize, long* recv, int recvSize) {
        //logger(4) << "entering gather\n";
        MPI_Gather(send, sendSize, MPI_LONG, recv, recvSize,
                MPI_LONG, 0, MPI_COMM_WORLD);
        //logger(4) << "left gather\n";
    }

    int gatherAll(long* send, int sendSize, long* &recv) {
        int size = sendSize * numProc;
        recv = (long*) malloc(size * sizeof(long));
        //logger(4) << "entering gather\n";
        MPI_Allgather(send, sendSize, MPI_LONG, recv, sendSize, MPI_LONG, MPI_COMM_WORLD);
        return size;
    }

    int gatherAll(int* send, int sendSize, int* &recv) {
        int size = sendSize * numProc;
        recv = (int*) malloc(size * sizeof(int));
        //logger(4) << "entering gather\n";
        MPI_Allgather(send, sendSize, MPI_INT, recv, sendSize, MPI_INT, MPI_COMM_WORLD);
        return size;
    }

    void receiveGather(long* send, int sendSize, long* recv, int recvSize ) {
        MPI_Gather(send, sendSize, MPI_LONG, recv, recvSize,
                MPI_LONG, 0, MPI_COMM_WORLD);
    }

    void bcast(long* send, int sendSize) {
        MPI_Bcast(send, sendSize, MPI_LONG, 0, MPI_COMM_WORLD);
    }

    void bcast(int* send, int sendSize){
        MPI_Bcast(send, sendSize, MPI_INT, 0, MPI_COMM_WORLD);
    }

    void receiveBcast(long* recv, int recvSize) {
        MPI_Bcast(recv, recvSize, MPI_LONG, 0, MPI_COMM_WORLD);
    }
};
#endif
