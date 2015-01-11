#ifndef MESSAGEBUFFER_H
#define MESSAGEBUFFER_H 1

#include "CommLayer.h"
#include "Messages.h"
#include "Common/Options.h"
#include "treedef.hpp"
#include <iostream>
#include <assert.h>
#include <vector>

using namespace std;
using treedef::TreeNode;
using treedef::NodeFlag;

namespace CommLayer{
    template <typename U, typename V> class MessageBuffer;
    template<typename U, typename V> using MsgBuffer = std::vector<Message<U, V>*> ;
    template<typename U, typename V> using MessageQueues = std::vector<MsgBuffer<U, V> > ;

    enum SendMode
    {
        SM_BUFFERED,
        SM_IMMEDIATE
    };

    /** A buffer of Message. */
    template<typename U, typename V> 
        class MessageBuffer : public CommLayer <U, V>
    {
        public:
            MessageBuffer<U, V>();

            void sendCheckPointMessage(int argument = 0)
            {
                assert(transmitBufferEmpty());
                CommLayer<U, V>::sendCheckPointMessage(argument);
            }

            void sendControlMessage(APControl command, int argument = 0)
            {
                assert(transmitBufferEmpty());
                CommLayer<U, V>::sendControlMessage(command, argument);
            }

            void sendControlMessageToNode(int dest,
                    APControl command, int argument = 0)
            {
                assert(transmitBufferEmpty());
                CommLayer<U, V>::sendControlMessageToNode(dest,
                        command, argument);
            }

            void sendBinAddMessage(int nodeID, const TreeNode<U, V>& x);
            void sendBinRemoveMessage(int nodeID, const TreeNode<U, V>& x);
            void sendSetFlagMessage(int nodeID,
                    const TreeNode<U, V>& x, NodeFlag flag);
            void sendBinDataRequest(int nodeID,
                    IDType group, IDType id, const TreeNode<U, V>& x);
            void sendBinDataResponse(int nodeID,
                    IDType group, IDType id, const TreeNode<U, V>& x, int multiplicity);

            void flush();
            void queueMessage (int nodeID, 
                    Message<U, V>* message, SendMode mode);

            // clear out a queue
            void clearQueue(int nodeID);
            bool transmitBufferEmpty() const;

            // check if a queue is full, if so, send the messages if the
            // immediate mode flag is set, send even if the queue is not
            // full.
            void checkQueueForSend(int nodeID, SendMode mode);

        private:
            static const size_t MAX_MESSAGES = 100;
            MessageQueues<U, V> m_msgQueues;
    };

    /************************************Implementation**********************************/


    //allocate space for message queues
    template <typename U, typename V>
        MessageBuffer<U, V>::MessageBuffer()
        : m_msgQueues(opt::numProc)
        {
            for (unsigned i = 0; i < m_msgQueues.size(); i++)
                m_msgQueues[i].reserve(MAX_MESSAGES);
        }

    template <typename U, typename V> 
        void MessageBuffer<U, V>::sendBinAddMessage(int nodeID, const TreeNode<U, V>& x)
        {
            queueMessage(nodeID, new BinAddMessage<U, V>(x), SM_BUFFERED);
        }

    template <typename U, typename V>
        void MessageBuffer<U, V>::sendBinRemoveMessage(int nodeID, const TreeNode<U, V>& x)
        {
            queueMessage(nodeID, new BinRemoveMessage<U, V>(x), SM_BUFFERED);
        }

    // Send a set flag message
    template <typename U, typename V>
        void MessageBuffer<U,V>::sendSetFlagMessage(int nodeID,
                const TreeNode<U, V>& x, NodeFlag flag)
        {
            queueMessage(nodeID, new SetFlagMessage<U, V>(x, flag), SM_BUFFERED);
        }

    // Send a binary data request
    template <typename U, typename V>
        void MessageBuffer<U, V>::sendBinDataRequest(int nodeID,
                IDType group, IDType id, const TreeNode<U, V>& x)
        {
            queueMessage(nodeID, new BinDataRequest<U, V>(x, group, id), SM_IMMEDIATE);
        }

    // Send a binary data response
    template <typename U, typename V>
        void MessageBuffer<U, V>::sendBinDataResponse(int nodeID,
                IDType group, IDType id, const TreeNode<U, V>& x, int multiplicity)
        {
            queueMessage(nodeID, new BinDataResponse<U, V>(x, group, id, multiplicity),
                    SM_IMMEDIATE);
        }

    template <typename U, typename V>
        void MessageBuffer<U, V>::queueMessage(
                int nodeID, Message<U, V>* message, SendMode mode)
        {
            if (opt::verbose >= 9)
                cout << opt::rank << " to " << nodeID << ": " << *message;
            m_msgQueues[nodeID].push_back(message);
            checkQueueForSend(nodeID, mode);
        }

    template <typename U, typename V>
        void MessageBuffer<U, V>::checkQueueForSend(int nodeID, SendMode mode)
        {
            size_t numMsgs = m_msgQueues[nodeID].size();
            // check if the messages should be sent
            if ((numMsgs == MAX_MESSAGES || mode == SM_IMMEDIATE)
                    && numMsgs > 0) {
                // Calculate the total size of the message
                size_t totalSize = 0;
                for(size_t i = 0; i < numMsgs; i++)
                {
                    totalSize += m_msgQueues[nodeID][i]->getNetworkSize();
                }

                // Generate a buffer for all the messages
                char* buffer = new char[totalSize];

                // Copy the messages into the buffer
                size_t offset = 0;
                for(size_t i = 0; i < numMsgs; i++)
                    offset += m_msgQueues[nodeID][i]->serialize(
                            buffer + offset);

                assert(offset == totalSize);
                this->sendBufferedMessage(nodeID, buffer, totalSize);

                delete [] buffer;
                clearQueue(nodeID);

                this->m_txPackets++;
                this->m_txMessages += numMsgs;
                this->m_txBytes += totalSize;
            }
        }

    // Clear a queue of messages
    template <typename U, typename V>
        void MessageBuffer<U,V>::clearQueue(int nodeID)
        {
            size_t numMsgs = m_msgQueues[nodeID].size();
            for(size_t i = 0; i < numMsgs; i++)
            {
                // Delete the messages
                delete m_msgQueues[nodeID][i];
                m_msgQueues[nodeID][i] = 0;
            }
            m_msgQueues[nodeID].clear();
        }

    // Flush the message buffer by sending all messages that are queued
    template <typename U, typename V>
        void MessageBuffer<U, V>::flush()
        {
            // Send all messages in all queues
            for(size_t id = 0; id < m_msgQueues.size(); ++id)
            {
                // force the queue to send any pending messages
                checkQueueForSend(id, SM_IMMEDIATE);
            }
        }

    // Check if all the queues are empty
    template <typename U, typename V>
        bool MessageBuffer<U, V>::transmitBufferEmpty() const
        {
            bool isEmpty = true;
            for (auto it = m_msgQueues.begin();
                    it != m_msgQueues.end(); ++it) {
                if (!it->empty()) {
                    cerr
                        << opt::rank << ": error: tx buffer should be empty: "
                        << it->size() << " messages from "
                        << opt::rank << " to " << it - m_msgQueues.begin()
                        << '\n';
                    for (auto j = it->begin();
                            j != it->end(); ++j)
                        cerr << **j << '\n';
                    isEmpty = false;
                }
            }
            return isEmpty;
        }
}// namespace CommLayer
#endif
