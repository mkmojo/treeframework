#include "CommLayer.hpp"

template<typename T> class MessageBuffer{
    std::vector<std::vector<Message<T>* > > msgQueues;

    //APMessage checkMessage(int senderID);
    void _checkQueueForSend(int procID, SendMode mode){
        size_t numMsgs = msgQueues[procID].size();
        // check if message should be sent
        if((numMsgs == MAX_MESSAGES || mode == SM_IMMEDIATE) && numMsgs > 0){
            // Calculate the total size of the message
            size_t totalSize = 0;
            for(auto i = 0; i < numMsgs; i++) totalSize += msgQueues[procID][i]->getNetworkSize();

            //Generate a buffer for all themessages;
            char* buffer = new char[totalSize];

            //Copy the messages into the buffer
            size_t offset = 0;
            for(auto i = 0; i < numMsgs; i++) offset += msgQueues[procID][i]->serialize(buffer + offset);

            assert(offset == totalSize);
            msgBufferLayer.sendBufferedMessage(procID, buffer, totalSize);

            delete [] buffer;
            _clearQueue(procID);

            msgBufferLayer.txPackets++;
            msgBufferLayer.txMessages += numMsgs;
            msgBufferLayer.txBytes += totalSize;
        }
    }

    void _clearQueue(int procID){
        size_t numMsgs = msgQueues[procID].size();
        for(auto i = 0; i < numMsgs; i++){
            // Delete the messages
            delete msgQueues[procID][i];
            msgQueues[procID][i] = 0;
        }
        msgQueues[procID].clear();
    }

public:
    CommLayer<T> msgBufferLayer;
    MessageBuffer() : msgQueues(numProc){ 
        for (auto i = 0; i < msgQueues.size(); i++)
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

    void queueMessage(int procID, Message<T>* pMessage, SendMode mode = SM_BUFFERED){
        msgQueues[procID].push_back(pMessage);
        _checkQueueForSend(procID, mode);
    }

    bool transmitBufferEmpty() const{
        bool isEmpty = true;
        for (auto it = msgQueues.begin(); it != msgQueues.end(); ++it) {
            if (!it->empty()) {
                std::cerr << procRank << ": error: tx buffer should be empty: "
                    << it->size() << " messages from "
                    << procRank << " to " << it - msgQueues.begin()
                    << '\n';
                isEmpty = false;
            }
        }
        return isEmpty;
    }

    void flush(){
        for(auto id=0; id < msgQueues.size(); id++){
            // force the queue to send any pending messages
            _checkQueueForSend(id, SM_IMMEDIATE);
        }
    }

};
