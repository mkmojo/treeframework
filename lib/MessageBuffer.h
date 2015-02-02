#ifndef MESSAGE_BUFFER_H
#define MESSAGE_BUFFER_H 1
class MessageBuffer;

#include "CommLayer.h"
#include <vector>

using MsgBuffer = std::vector<Message*>;
using MessageQueues = std::vector<MsgBuffer>;

enum SendMode
{
    SM_BUFFERED,
    SM_IMMEDIATE
};

class MessageBuffer : public CommLayer
{
    public:
        MessageBuffer();

        void sendCheckPointMessage(int argument = 0);
        {
            assert(transmitBufferEmpty());
            CommLayer::sendCheckPointeMessage(argument);
        }

        void sendControlMessage(APControl command, int argument = 0)
        {
            assert(transmitBufferEmpty());
            CommLayer::sendControlMessage(command, argument);
        }

        void sendControlMessageToNode(int dest,
                APControl command, int argument = 0)
        {
            assert(transmitBufferEmpty());
            CommLayer::sendControlMessageToNode(dest,
                    command, argument);
        }

        void flush();
    private:
        static const size_t MAX_MESSAGES = 100;
        MessageQueues m_msgQueues;

};


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
#endif
