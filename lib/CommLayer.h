#ifndef COMMLAYER_H
#define COMMLAYER_H 1

#include "Messages.h"
#include <mpi.h>
#include <vector>

enum APMessage
{
    APM_NONE,
    APM_CONTROL,
    APM_BUFFERED,
};

enum APControl
{
    APC_SET_STATE,
    APC_CHECKPOINT,
    APC_WAIT,
    APC_BARRIER,
};

//For reduce operation type
enum MPI_OP_T{
    SUM, MIN, MAX,
};

typedef std::vector<Message*> MessagePtrVector;

struct ControlMessage
{
    int64_t id;
    APControl msgType;
    int argument;
};

class CommLayer
{
    public:
        CommLayer();
        ~CommLayer();

        APMessage checkMessage(int &senderID);

        std::vector<long unsigned> reduce(
                const std::vector<long unsigned>& v);

        double reduce( double count, MPI_OP_T op=SUM);
        long reduce( long count, MPI_OP_T op=SUM);
        void reduce(long *dest, long *recv, long size);
        void gather(const long *send, int sendSize, long *recv, int recvSize);
        void receiveGather(const long *send, int sendSize, long *recv, int recvSize);
        void bcast(long* send, int sendSize); 
        void receiveBcast(long* recv, int recvSize);
        void barrier();

        // Send a control message
        void sendControlMessage(APControl m, int argument = 0 );

        uint64_t sendControlMessageToNode(int nodeID,
                APControl command, int argument = 0);
        // Receive a control message
        ControlMessage receiveControlMessage();

        // Send a message that the checkpoint has been reached
        uint64_t sendCheckPointMessage(int argument = 0);

        // Send a buffered message
        void sendBufferedMessage(int destID, char* msg, size_t size);

        void receiveBufferedMessage(MessagePtrVector& outmessages);


    private:
        uint64_t m_msgID;
        uint8_t* m_rxBuffer;
        MPI_Request m_request;

    protected:
        // Counters
        uint64_t m_rxPackets;
        uint64_t m_rxMessages;
        uint64_t m_rxBytes;
        uint64_t m_txPackets;
        uint64_t m_txMessages;
        uint64_t m_txBytes;
};

#endif
