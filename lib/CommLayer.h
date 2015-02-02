#ifndef COMMLAYER_H
#define COMMLAYER_H 1
#include <mpi.h>

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
        // Send a message that the checkpoint has been reached
        uint64_t sendCheckPointMessage(int argument = 0);

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

/** Send a control message to every other process. */
void CommLayer::sendControlMessage(APControl m, int argument)
{
    for (int i = 0; i < opt::numProc; i++)
        if (i != opt::rank) // Don't send the message to myself.
            sendControlMessageToNode(i, m, argument);
}



/** Send a control message to a specific node. */
uint64_t CommLayer::sendControlMessageToNode(int nodeID,
        APControl m, int argument)
{
    assert(opt::rank == 0);
    ControlMessage msg;
    msg.id = m_msgID++;
    msg.msgType = m;
    msg.argument = argument;

    MPI_Send(&msg, sizeof msg,
            MPI_BYTE, nodeID, APM_CONTROL, MPI_COMM_WORLD);
    return msg.id;
}



uint64_t CommLayer::sendCheckPointMessage(int argument)
{
    logger(4) << "checkpoint: " << argument << '\n';
    assert(opt::rank != 0);
    ControlMessage msg;
    msg.id = m_msgID++;
    msg.msgType = APC_CHECKPOINT;
    msg.argument = argument;

    MPI_Send(&msg, sizeof msg,
            MPI_BYTE, 0, APM_CONTROL, MPI_COMM_WORLD);
    return msg.id;
}

CommLayer::CommLayer() : m_msgID(0),
    m_rxBuffer(new uint8_t[RX_BUFSIZE]),
    m_request(MPI_REQUEST_NULL),
    m_rxPackets(0), m_rxMessages(0), m_rxBytes(0),
    m_txPackets(0), m_txMessages(0), m_txBytes(0)
{
    assert(m_request == MPI_REQUEST_NULL);
    MPI_Irecv(m_rxBuffer, RX_BUFSIZE,
            MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
            &m_request);
}


CommLayer::~CommLayer()
{
    MPI_Cancel(&m_request);
    delete[] m_rxBuffer;
    logger(1) << "Sent " << m_msgID << " control, "
        << m_txPackets << " packets, "
        << m_txMessages << " messages, "
        << m_txBytes << " bytes. "
        << "Received " << m_rxPackets << " packets, "
        << m_rxMessages << " messages, "
        << m_rxBytes << " bytes.\n";
}



#endif
