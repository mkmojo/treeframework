#include "Message.hpp"
#include "CommUtils.hpp"
#include <mpi.h>
#include <cassert>
#include <cstring>

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
            //sl15 this needs a rework as add, sort should not go into commlayer
            //sl15: this should be moved to somewhere else
            /*
            MessageType type = Message::readMessageType((char*)m_rxBuffer + offset);

            Message<T>* pNewMessage;
            switch(type){
                case MT_ADD:
                    pNewMessage = new SeqAddMessage();
                    break;
                default:
                    assert(false); //sl15: print a message and exit
                    break;
            }

            // Unserialize the new message from the buffer
            offset += pNewMessage->unserialize((char*)m_rxBuffer + offset);

            // Construted message will be deleted in the 
            // LocalOctTree calling function.
            outmessages.push(pNewMessage);
            */
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
