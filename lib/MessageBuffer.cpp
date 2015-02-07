#include "MessageBuffer.h"
#include "Common/Options.h"
#include <iostream>
using namespace std;

void MessageBuffer::checkQueueForSend(int procID, SendMode mode)
{
    size_t numMsgs = m_msgQueues[procID].size();
    // check if message should be sent
    if((numMsgs == MAX_MESSAGES || mode == SM_IMMEDIATE) 
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
            offset += m_msgQueues[procID][i]->serialize(
                    buffer + offset);

        assert(offset == totalSize);
        sendBufferedMessage(procID, buffer, totalSize);

        delete [] buffer;
        clearQueue(procID);

        m_txPackets++;
        m_txMessages += numMsgs;
        m_txBytes += totalSize;
    }
}

void MessageBuffer::queueMessage(int procID, Message* message, SendMode mode)
{
    m_msgQueues[procID].push_back(message);
    checkQueueForSend(procID, mode);
}

void MessageBuffer::sendSeqAddMessage(int procID, const Point& p)
{
    queueMessage(procID, new SeqAddMessage(p), SM_BUFFERED);
}

void MessageBuffer::clearQueue(int procID)
{
    size_t numMsgs = m_msgQueues[procID].size();
    for(size_t i = 0; i < numMsgs; i++)
    {
        // Delete the messages
        delete m_msgQueues[procID][i];
        m_msgQueues[procID][i] = 0;
    }
    m_msgQueues[procID].clear();
}

MessageBuffer::MessageBuffer() : m_msgQueues(opt::numProc)
{
    for (unsigned i = 0; i < m_msgQueues.size(); i++)
        m_msgQueues[i].reserve(MAX_MESSAGES);
}

void MessageBuffer::flush()
{
    for(size_t id=0; id < m_msgQueues.size(); id++)
    {
        // force the queue to send any pending messages
        checkQueueForSend(id, SM_IMMEDIATE);
    }
}

bool MessageBuffer::transmitBufferEmpty() const
{
    bool isEmpty = true;
    for (MessageQueues::const_iterator it = m_msgQueues.begin();
            it != m_msgQueues.end(); ++it) {
        if (!it->empty()) {
            cerr << opt::rank << ": error: tx buffer should be empty: "
                << it->size() << " messages from "
                << opt::rank << " to " << it - m_msgQueues.begin()
                << '\n';
            //for (MsgBuffer::const_iterator j = it->begin();
            //j != it->end(); ++j)
            //cerr << **j << '\n';
            isEmpty = false;
        }
    }
    return isEmpty;
}
