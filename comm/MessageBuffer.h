#ifndef MESSAGE_BUFFER_H
#define MESSAGE_BUFFER_H 1
class MessageBuffer;

#include "CommLayer.h"
#include "Messages.h"
#include <vector>
#include <assert.h>

enum SendMode
{
    SM_BUFFERED,
    SM_IMMEDIATE
};

template<typename T> class MessageBuffer : public CommLayer{
    static const size_t MAX_MESSAGES = 100;
    vector<queue<Message<T>* > > m_msgQueues;
public:
    MessageBuffer();

    void sendCheckPointMessage(int argument = 0){
        assert(transmitBufferEmpty());
        CommLayer::sendCheckPointMessage(argument);
    }

    void sendControlMessage(APControl command, int argument = 0){
        assert(transmitBufferEmpty());
        CommLayer::sendControlMessage(command, argument);
    }

    void sendControlMessageToNode(int dest, APControl command, int argument = 0){
        assert(transmitBufferEmpty());
        CommLayer::sendControlMessageToNode
            (dest, command, argument);
    }

    void sendSeqAddMessage(int procID, const Point& p);

    void sendSeqSortMessage(int procID, const Point& P);

    bool transmitBufferEmpty() const;

    void flush();
    void clearQueue(int procID);
    void queueMessage(int nodeID, Message<T>* message, SendMode mode){
        for (unsigned i = 0; i < m_msgQueues.size(); i++)
            m_msgQueues[i].reserve(MAX_MESSAGES);
    }

    //APMessage checkMessage(int senderID);
    void checkQueueForSend(int procID, SendMode mode);
};

#endif
