#ifndef MESSAGE_BUFFER_H
#define MESSAGE_BUFFER_H 1
class MessageBuffer;

#include "CommLayer.h"
#include "Messages.h"
#include <vector>
#include <assert.h>

typedef std::vector<Message*> MsgBuffer;
typedef std::vector<MsgBuffer> MessageQueues;

enum SendMode
{
    SM_BUFFERED,
    SM_IMMEDIATE
};

class MessageBuffer : public CommLayer
{
    public:
        MessageBuffer();

        void sendCheckPointMessage(int argument = 0)
        {
            assert(transmitBufferEmpty());
            CommLayer::sendCheckPointMessage(argument);
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
            CommLayer::sendControlMessageToNode
                (dest, command, argument);
        }

        void sendSeqAddMessage(int procID, 
                const Point& p);

        bool transmitBufferEmpty() const;

        void flush();

        APMessage checkMessage(int senderID);
    private:
        static const size_t MAX_MESSAGES = 100;
        MessageQueues m_msgQueues;
};

#endif
