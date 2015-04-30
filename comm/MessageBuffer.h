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
    vector<queue<Message<T>* > > m_msgQueues;
public:
    MessageBuffer() : m_msgQueues(opt::numProc){ }

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

    void sendMessage(int procID, Message<T>* pMessage){
        m_msgQueues[procID].push_back(message);
        checkQueueForSend(procID, mode);
    }

    bool transmitBufferEmpty() const;

    void flush(){
        for(auto&& it : m_msgQueues)
            checkQueueForSend(it, SM_IMMEDIATE);
    }

    void clearQueue(int procID);

    //APMessage checkMessage(int senderID);
    void checkQueueForSend(int procID, SendMode mode){
        size_t numMsgs = m_msgQueues[procID].size();
        // check if message should be sent
        if((numMsgs == MAX_MESSAGES || mode == SM_IMMEDIATE) && numMsgs > 0){
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
};

#endif
