#include "CommLayer.hpp"

template<typename T> class MessageBuffer{
    std::vector<std::vector<Message<T>* > > msgQueues;

    //APMessage checkMessage(int senderID);
    void _checkQueueForSend(int procID, SendMode mode){
        size_t numMsgs = msgQueues[procID].size();
        // check if message should be sent
        if((numMsgs == MAX_MESSAGES || mode == SM_IMMEDIATE) && numMsgs > 0){
            // Calculate the total size of the message
            size_t s=msgQueues[procID][0]->getNetworkSize();
            size_t totalSize = s*numMsgs;
            //for(auto i = 0; i < numMsgs; i++) totalSize += msgQueues[procID][i]->getNetworkSize();

            //Generate a buffer for all themessages;
            char* buffer = new char[totalSize];

            //Copy the messages into the buffer
            for(auto i = 0; i < numMsgs; i++) msgQueues[procID][i]->serialize(buffer + (i*s));

            msgBufferLayer.sendBufferedMessage(procID, buffer, totalSize);

            delete [] buffer;
            _clearQueue(procID);

            msgBufferLayer.txPackets++;
            msgBufferLayer.txMessages += numMsgs;
            msgBufferLayer.txBytes += totalSize;
        }
    }

    void queueMessage(int procID, Message<T>* pMessage, SendMode mode = SM_BUFFERED){
        msgQueues[procID].push_back(pMessage);
        _checkQueueForSend(procID, mode);
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
    CommLayer msgBufferLayer;
    MessageBuffer() : msgQueues(numProc){ 
        for (auto i = 0; i < msgQueues.size(); i++)
            msgQueues[i].reserve(MAX_MESSAGES);
    }


    //sl15: this will be called by the load point method
    void addMessage(int procID, const T& data_in){
        queueMessage(procID, new Message<T>(data_in)); //sl15: we use default sm_buffered, but is there any other case?
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
