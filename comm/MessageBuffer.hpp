#include "CommLayer.hpp"
#include <queue>

template<typename T> class MessageBuffer{
    CommLayer<T> msgBufferLayer;
    vector<queue<Message<T>* > > msgQueues;

public:
    MessageBuffer() : msgQueues(opt::numProc){ 
        for (unsigned i = 0; i < msgQueues.size(); i++)
            msgQueues[i].reserve(MAX_MESSAGES);
    }

    void sendCheckPointMessage(int argument = 0){
        assert(transmitBufferEmpty());
        msgBufferLayer.sendCheckPointMessage(argument);
    }

    void sendControlMessage(APControl command, int argument = 0){
        assert(transmitBufferEmpty());
        msgBufferLayer.sendControlMessage(command, argument);
    }

    void sendMessage(int procID, Message<T>* pMessage){
        msgQueues[procID].push_back(message);
        checkQueueForSend(procID, mode);
    }

    bool transmitBufferEmpty() const{
        bool isEmpty = true;
        for (MessageQueues::const_iterator it = msgQueues.begin();
                it != msgQueues.end(); ++it) {
            if (!it->empty()) {
                cerr << opt::rank << ": error: tx buffer should be empty: "
                    << it->size() << " messages from "
                    << opt::rank << " to " << it - msgQueues.begin()
                    << '\n';
                isEmpty = false;
            }
        }
        return isEmpty;
    }

    void flush(){
        for(size_t id=0; id < msgQueues.size(); id++){
            // force the queue to send any pending messages
            checkQueueForSend(id, SIMMEDIATE);
        }
    }

    void clearQueue(int procID){
        size_t numMsgs = msgQueues[procID].size();
        for(size_t i = 0; i < numMsgs; i++){
            // Delete the messages
            delete msgQueues[procID][i];
            msgQueues[procID][i] = 0;
        }
        msgQueues[procID].clear();
    }

    //APMessage checkMessage(int senderID);
    void checkQueueForSend(int procID, SendMode mode){
        size_t numMsgs = msgQueues[procID].size();
        // check if message should be sent
        if((numMsgs == MAX_MESSAGES || mode == SIMMEDIATE) && numMsgs > 0){
            // Calculate the total size of the message
            size_t totalSize = 0;
            for(size_t i = 0; i < numMsgs; i++){
                totalSize += msgQueues[procID][i]->getNetworkSize();
            }

            //Generate a buffer for all themessages;
            char* buffer = new char[totalSize];

            //Copy the messages into the buffer
            size_t offset = 0;
            for(size_t i = 0; i < numMsgs; i++)
                offset += msgQueues[procID][i]->serialize(buffer + offset);

            assert(offset == totalSize);
            msgBufferLayer.sendBufferedMessage(procID, buffer, totalSize);

            delete [] buffer;
            clearQueue(procID);

            txPackets++;
            txMessages += numMsgs;
            txBytes += totalSize;
        }
    }

    void clearQueue(int procID){
        size_t numMsgs = msgQueues[procID].size();
        for(size_t i = 0; i < numMsgs; i++){
            // Delete the messages
            delete msgQueues[procID][i];
            msgQueues[procID][i] = 0;
        }
        msgQueues[procID].clear();
    }
};
