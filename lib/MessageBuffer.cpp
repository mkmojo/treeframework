#include "MessageBuffer.h"

MessageBuffer::sendSeqAddMessage(int procID, const Point& p)
{
    queueMessage(procID, new SeqAddMessage(p), SM_BUFFERED);
}

void MessageBuffer::queueMessage(int procID, Message* message, SendMode mode)
{
    m_msgQueues[procID].push_back(message);
    checkQueueForSend(procID, mode);
}

void MessageBuffer::clearQueue(int procID)
{
    size_t numMsgs = m_msgQueues[nodeID].size();
    for(size_t i = 0; i < numMsgs; i++)
    {
        // Delete the messages
        delete m_msgQueues[nodeID][i];
        m_msgQueues[nodeID][i] = 0;
    }
    m_msgQueues[nodeID].clear();
}

void MessageBuffer::checkQueueForSend()
{
    size_t numMsgs = m_msgQueues[procID].size();
    // check if message should be sent
    if((numMsgs == MAX_MESSAGES || mode == SM_IMMEDIATEi) 
            && numMsgs > 0){
        // Calculate the total size of the message
        size_t totalSize = 0;
        for(size_t i = 0; i < numMsgs; i++){
            totalSize += m_msgQueues[procID][i]->getNetworkSize();
        }

        //Generate a buffer for all themessages;
        char* buffer = new char[totalSize];

        //Copy the messages into the buffer
        size_t offset = 0;
        for(size_t i = 0; i < numMsgs; i++)
            offset += m_msgQueues[nodeID][i]->serialize(
                    buffer + offset);

        assert(offset == totalSize);
        sendBufferedMessage(nodeID, buffer, totalSize);

        delete [] buffer;
        clearQueue(nodeID);

        m_txPackets++;
        m_txMessages += numMsgs;
        m_txBytes += totalSize;
    }
}



MessageBuffer::MessageBuffer() : m_msgQueues(opt::numProc)
{
    for (unsigned i = 0; i < m_msgQueues.size(); i++)
        m_msgQueues[i].reserve(MAX_MESSAGES);
}

void MessageBuffer::flush()
{
    for(int i=0; i < m_msgQueues; i++)
    {
        // force the queue to send any pending messages
        checkQueueForSend(id, SM_IMMEDIATE);
    }
}

void MessageBuffer::sendCheckPointMessage(int argument = 0)
{
    assert(transimitBufferEmpty());
    CommLayer::sendCheckPointMessage(argument);
}

void MessageBuffer::sendControlMessage(APControl command, int argument = 0)
{
    assert(transimitBufferEmpty());
    CommLayer::sendControlMessage(command, argument);
}


void MessageBuffer::sendControlMessageToNode(int dest, 
        APControl command, int argument = 0)
{
    assert(transimitBufferEmpty());
    CommLayer::sendControlMessageToNode(dest, command, argument);
}

bool MessageBuffer::transimitBufferEmpty() const
{
    bool isEmpty = true;
    for (MessageQueues::const_iterator it = m_msgQueues.begin();
            it != m_msgQueues.end(); ++it) {
        if (!it->empty()) {
            cerr
                << opt::rank << ": error: tx buffer should be empty: "
                << it->size() << " messages from "
                << opt::rank << " to " << it - m_msgQueues.begin()
                << '\n';
            for (MsgBuffer::const_iterator j = it->begin();
                    j != it->end(); ++j)
                cerr << **j << '\n';
            isEmpty = false;
        }
    }
    return isEmpty;
}
