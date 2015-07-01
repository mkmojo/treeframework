#include "Message.hpp"
#include "CommUtils.hpp"
#include <mpi.h>
#include <cassert>
#include <cstring>
#include <vector>

template<typename T> class CommLayer{
    uint64_t m_msgID;
    uint8_t* m_rxBuffer;
    MPI_Request m_request;

    // Counters
    uint64_t m_rxPackets;
    uint64_t m_rxMessages;
    uint64_t m_rxBytes;
    uint64_t m_txPackets;
    uint64_t m_txMessages;
    uint64_t m_txBytes;

    uint64_t _sendControlMessageToNode(int nodeID, APControl command, int argument = 0){
        assert(procRank == 0);
        ControlMessage msg;
        msg.id = m_msgID++;
        msg.msgType = command;
        msg.argument = argument;

        MPI_Send(&msg, sizeof msg, MPI_BYTE, nodeID, APM_CONTROL, MPI_COMM_WORLD);
        return msg.id;
    }

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
    uint64_t txPackets, txMessages, txBytes;
    CommLayer() : m_msgID(0), m_rxBuffer(new uint8_t[RX_BUFSIZE]), m_request(MPI_REQUEST_NULL), m_rxPackets(0), m_rxMessages(0), m_rxBytes(0), m_txPackets(0), m_txMessages(0), m_txBytes(0){
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

    // Send a control message
    void sendControlMessage(APControl m, int argument = 0 ){
        for (int i = 0; i < numProc; i++)
            if (i != procRank) // Don't send the message to myself.
                _sendControlMessageToNode(i, m, argument);
    }

    bool isMaster(){
    	return procRank == 0;
    }
    // Receive a control message
    ControlMessage receiveControlMessage(){
        int flag;
        MPI_Status status;
        MPI_Test(&m_request, &flag, &status);
        assert(flag);
        assert((APMessage)status.MPI_TAG == APM_CONTROL);

        int count;
        MPI_Get_count(&status, MPI_BYTE, &count);
        ControlMessage msg;
        assert(count == sizeof msg);
        std::memcpy(&msg, m_rxBuffer, sizeof msg);
        assert(m_request == MPI_REQUEST_NULL);
        MPI_Irecv(m_rxBuffer, RX_BUFSIZE, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, 
                MPI_COMM_WORLD, &m_request);
        return msg;
    }


    // Send a message that the checkpoint has been reached
    uint64_t sendCheckPointMessage(int argument = 0){
        assert(procRank != 0);
        ControlMessage msg;
        msg.id = m_msgID++;
        msg.msgType = APC_CHECKPOINT;
        msg.argument = argument;

        MPI_Send(&msg, sizeof msg, MPI_BYTE, 0, APM_CONTROL, MPI_COMM_WORLD);
        return msg.id;
    }


    // Send a buffered message
    inline void sendBufferedMessage(int destID, char* msg, size_t size){ 
        MPI_Send(msg, size, MPI_BYTE, destID, APM_BUFFERED, MPI_COMM_WORLD); 
    }

    void receiveBufferedMessage(std::queue<Message<T>*>& outmessages){
        assert(outmessages.empty());
        int flag;
        MPI_Status status;
        MPI_Test(&m_request, &flag, &status);
        assert(flag);
        assert((APMessage)status.MPI_TAG == APM_BUFFERED);

        int size;
        MPI_Get_count(&status, MPI_BYTE, &size);

        int offset = 0;
        while(offset < size){
            //create empty new message
            Message<T>* pNewMessage = new Message<T>();

            // Unserialize the new message from the buffer
            offset += pNewMessage->unserialize((char*)m_rxBuffer + offset);

            // Construted message will be deleted in the 
            // LocalOctTree calling function.
            outmessages.push(pNewMessage);
        }
        assert(offset == size);

        assert(m_request == MPI_REQUEST_NULL);
        MPI_Irecv(m_rxBuffer, RX_BUFSIZE, 
                MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, 
                &m_request);

        m_rxPackets++;
        m_rxMessages += outmessages.size();
        m_rxBytes += size;
    }



    std::vector<long unsigned> reduce(
            const std::vector<long unsigned>& v)
    {
        //logger(4) << "entering reduce\n";
        std::vector<long unsigned> sum(v.size());
        MPI_Allreduce(const_cast<long unsigned*>(&v[0]),
                &sum[0], v.size(), MPI_UNSIGNED_LONG, MPI_SUM,
                MPI_COMM_WORLD);
        //logger(4) << "left reduce\n";
        return sum;
    }

    void reduce(long *dest, long* recv, long size)
    {
        //logger(4) << "entering reduce\n";
        MPI_Allreduce(dest, recv, size, MPI_LONG, MPI_SUM,
                MPI_COMM_WORLD);
        //logger(4) << "left reduce\n";
    }

    long reduce( long count, REDUCE_OP op)
    {
        //logger(4) << "entering reduce\n";
        long res;
        if(op == SUM)
            MPI_Allreduce(&count, &res, 1, MPI_LONG, MPI_SUM,
                    MPI_COMM_WORLD);

        if(op == MIN)
            MPI_Allreduce(&count, &res, 1, MPI_LONG, MPI_MIN,
                    MPI_COMM_WORLD);

        if(op == MAX)
            MPI_Allreduce(&count, &res, 1, MPI_LONG, MPI_MAX,
                    MPI_COMM_WORLD);

        //logger(4) << "left reduce\n";
        return res;
    }


    double reduce( double count, REDUCE_OP op)
    {
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

    void gather(long* send, int sendSize,
            long* recv, int recvSize)
    {
        //logger(4) << "entering gather\n";
        MPI_Gather(send, sendSize, MPI_LONG, recv, recvSize,
                MPI_LONG, 0, MPI_COMM_WORLD);
        //logger(4) << "left gather\n";
    }

    void gatherAll(long* send, int sendSize, long* &recv) {
    	recv = (long*)malloc(sendSize*numProc*sizeof(long));
        //logger(4) << "entering gather\n";
        MPI_Allgather(send, sendSize, MPI_LONG, recv, sendSize,
                MPI_LONG, MPI_COMM_WORLD);
    }

    void gatherAll(int* send, int sendSize, int* &recv) {
    	recv = (int*)malloc(sendSize*numProc*sizeof(int));
        //logger(4) << "entering gather\n";
        MPI_Allgather(send, sendSize, MPI_INT, recv, sendSize,
                MPI_INT, MPI_COMM_WORLD);
    }

    void receiveGather(long* send, int sendSize,
            long* recv, int recvSize )
    {
        //logger(4) << "entering receiveGather\n";
        MPI_Gather(send, sendSize, MPI_LONG, recv, recvSize,
                MPI_LONG, 0, MPI_COMM_WORLD);
        //logger(4) << "left receiveGather\n";
    }

    void bcast(long* send, int sendSize)
    {
        //logger(4) << "entering broadcast\n";
        MPI_Bcast(send, sendSize, MPI_LONG, 0, MPI_COMM_WORLD);
        //logger(4) << "left broadcast\n";
    }

    void receiveBcast(long* recv, int recvSize)
    {
        //logger(4) << "entering receiveBroadcast\n";
        MPI_Bcast(recv, recvSize, MPI_LONG, 0, MPI_COMM_WORLD);
        //logger(4) << "left receiveBroadcast\n";
    }

    //void allToAll(){}










};
