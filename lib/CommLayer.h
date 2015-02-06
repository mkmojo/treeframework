#ifndef COMMLAYER_H
#define COMMLAYER_H 1
#include <mpi.h>
#include <vector>
#include "Messages.h"

enum APMessage
{
    APM_NONE,
    APM_CONTROL,
    APM_BUFFERED
};

enum APControl
{
    APC_SET_STATE,
    APC_CHECKPOINT,
    APC_WAIT,
    APC_BARRIER,
};

using MessagePtrVector = std::vector<Message*>;

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

        void barrier();
        // Send a control message
        void sendControlMessage(APControl m, int argument = 0 );

        // Receive a control message
        ControlMessage receiveControlMessage();

        // Send a message that the checkpoint has been reached
        uint64_t sendCheckPointMessage(int argument = 0);

        // Send a buffered message
        void sendBufferedMessage(int destID, char* msg, size_t size);


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
