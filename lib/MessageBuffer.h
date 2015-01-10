#ifndef MESSAGE_BUFFER_H
#define MESSAGE_BUFFER_H 1

template <typename U, typename V> class MessageBuffer;

#include "CommLayer.h"
#include "Messages.h"
#include <assert.h>
#include <vector>

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

#endif
