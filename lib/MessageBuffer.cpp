#include "MessageBuffer.h"

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
