#ifndef MESSAGEBUFFER_HPP
#define MESSAGEBUFFER_HPP

#include "CommLayer.hpp"
#include <iostream>

class MessageBuffer{
    std::vector<std::vector<Message* > > msgQueues;

    //APMessage checkMessage(int senderID);
    void _checkQueueForSend(int procID, SendMode mode){
        size_t numMsgs = msgQueues[procID].size();
        // check if message should be sent
        size_t offset = 0;
        size_t totalSize = 0;
        if((numMsgs == MAX_MESSAGES || mode == SM_IMMEDIATE) && numMsgs > 0){
            // Calculate the total size of the message
            size_t s=msgQueues[procID][0]->getNetworkSize();
            for(auto i = 0; i < numMsgs; i++)
                totalSize += msgQueues[procID][i]->getNetworkSize();

            //Generate a buffer for all the messages;
            char* buffer = new char[totalSize];

            //Copy the messages into the buffer
            for(auto i = 0; i < numMsgs; i++) {
                msgQueues[procID][i]->serialize(buffer + offset);
                offset += msgQueues[procID][i]->getNetworkSize();
            }

            msgBufferLayer.sendBufferedMessage(procID, buffer, totalSize);
            std::cout << "DEBUG: " + std::to_string(procRank) +  " called Message sent\n";

            delete [] buffer;
            msgQueues[procID].clear();
            //TODO MPI_Wait to complete send/recv 
        }
    }

    public:
    CommLayer msgBufferLayer;
    MessageBuffer() : msgQueues(numProc){ 
        for (auto i = 0; i < msgQueues.size(); i++)
            msgQueues[i].reserve(MAX_MESSAGES);
    }

    void addMessage(int procID, Message* pMessage, SendMode mode = SM_BUFFERED){
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
#endif
