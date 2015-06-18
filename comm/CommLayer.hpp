#include "Message.hpp"
#include "CommUtils.hpp"
#include <mpi.h>

//sl15: needs to modify reduce, gather, etc to have template parameter
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

    uint64_t _sendControlMessageToNode(int nodeID, APControl command, 
            int argument = 0){

        assert(opt::rank == 0);
        ControlMessage msg;
        msg.id = m_msgID++;
        msg.msgType = m;
        msg.argument = argument;

        MPI_Send(&msg, sizeof msg,
                MPI_BYTE, nodeID, APM_CONTROL, MPI_COMM_WORLD);
        return msg.id;
    }

public:
    CommLayer() : m_msgID(0){m_rxBuffer(new uint8_t[RX_BUFSIZE]), 
        m_request(MPI_REQUEST_NULL), m_rxPackets(0), m_rxMessages(0), 
        m_rxBytes(0), m_txPackets(0), m_txMessages(0), m_txBytes(0){

        assert(m_request == MPI_REQUEST_NULL);
        MPI_Irecv(m_rxBuffer, RX_BUFSIZE, MPI_BYTE, MPI_ANY_SOURCE, 
                MPI_ANY_TAG, MPI_COMM_WORLD, &m_request);
    }

    ~CommLayer(){
        MPI_Cancel(&m_request);
        delete[] m_rxBuffer;
    }

    APMessage checkMessage(int &senderID){
        MPI_Status status;
        bool flag = request_get_status(m_request, status);
        if (flag) senderID = status.MPI_SOURCE;
        return flag ? (APMessage)status.MPI_TAG : APM_NONE;
    }

    //sl15: Qiyuan: please modify these functions such that the template parameter is used
    double reduce( double count, MPI_OP_T op=SUM);
    long reduce( long count, MPI_OP_T op=SUM);

    //sl15: these need to be looked at
    void gather(long *send, int sendSize, long *recv, int recvSize);
    void receiveGather(long *send, int sendSize, long *recv, int recvSize);
    void bcast(long* send, int sendSize); 
    void receiveBcast(long* recv, int recvSize);

    void barrier();


    // Send a control message
    void sendControlMessage(APControl m, int argument = 0 ){
        for (int i = 0; i < opt::numProc; i++)
            if (i != opt::rank) // Don't send the message to myself.
                _sendControlMessageToNode(i, m, argument);
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
        memcpy(&msg, m_rxBuffer, sizeof msg);
        assert(m_request == MPI_REQUEST_NULL);
        MPI_Irecv(m_rxBuffer, RX_BUFSIZE, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, 
                MPI_COMM_WORLD, &m_request);
        return msg;
    }


    // Send a message that the checkpoint has been reached
    uint64_t sendCheckPointMessage(int argument = 0){
        assert(opt::rank != 0);
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

    void receiveBufferedMessage(MessagePtrQueue& outmessages){
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
            MessageType type = Message::readMessageType(
                    (char*)m_rxBuffer + offset);

            Message* pNewMessage;
            switch(type){
                case MT_ADD:
                    pNewMessage = new SeqAddMessage();
                    break;
                default:
                    assert(false);
                    break;
            }

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
};
