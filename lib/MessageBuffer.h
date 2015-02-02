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

        bool transmitBufferEmpty() const;

        void flush();

        APMessage checkMessage(int senderID);
    private:
        static const size_t MAX_MESSAGES = 100;
        MessageQueues m_msgQueues;
};

#endif
