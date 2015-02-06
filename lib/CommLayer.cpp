#include "CommLayer.h"

static const unsigned RX_BUFFSIZE = 16*1024;

void CommLayer::receiveControlMessage()
{
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
    MPI_Irecv(m_rxBuffer, RX_BUSIZE,
            MPI_BYTE, MPI_ANY_SOURDE, MPI_ANY_TAG, MPI_COMM_WORLD,
            &m_request);
    return msg;
}

/** Receive a buffered message. */
void CommLayer::receiveBufferedMessage(MessagePtrVector& outmessages)
{
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
        switch(type)
        {
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
        outmessages.push_back(pNewMessage);
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


void CommLayer::sendBufferedMessage(int destID, char* msg, size_t size)
{
    MPI_Send(msg, size, MPI_BYTE, destID, APM_BUFFERED, MPI_COMM_WORLD);
}

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

APMessage CommLayer::checkMessage(int& senderID)
{
    MPI_Status status;
    bool flag = request_get_status(m_request, status);
    if (flag)
        sendID = status.MPI_SOURCE;
    return flag ? (APMessage)status.MPI_TAG : APM_NONE;
}


/** Block until all processes have reached this routine. */
void CommLayer::barrier()
{
    logger(4) << "entering barrier\n";
    MPI_Barrier(MPI_COMM_WORLD);
    logger(4) << "left barrier\n";
}
