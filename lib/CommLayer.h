#ifndef COMMLAYER_H
#define COMMLAYER_H 1

#include "config.h"
#include "CommLayer.h"
#include "Common/Options.h"
#include "Common/Log.h"
#include "MessageBuffer.h"
#include "Messages.h"
#include <cstring>
#include <vector>
#include <iostream>
#include <mpi.h>
#include <vector>
#include <assert.h>
namespace CommLayer {
    enum APMessage
    {
        APM_NONE,
        APM_CONTROL,
        APM_BUFFERED
    };

    enum APControl
    {
        APC_SET_STATE,
        APC_GLOBAL_SORT,
        APC_CHECKPOINT,
        APC_WAIT,
        APC_BARRIER,
    };

    template<typename U, typename V>
        using MessagePtrVector = std::vector<Message<U, V>* > ;

    struct ControlMessage
    {
        int64_t id;
        APControl msgType;
        int argument;
    };

    /** Interprocess communication and synchronization primitives. */
    template < typename U, typename V>
        class CommLayer
        {
            public:
                CommLayer();
                ~CommLayer();

                // Check if a message exists, if it does return the type
                APMessage checkMessage(int &sendID);

                // Return whether a message has been received.
                bool receiveEmpty();

                // Block until all processes have reached this routine.
                void barrier();

                void broadcast(int message);
                int receiveBroadcast();

                // Block until all processes have reached this routine.
                long long unsigned reduce(long long unsigned count);
                std::vector<unsigned> reduce(const std::vector<unsigned>& v);
                std::vector<long unsigned> reduce(
                        const std::vector<long unsigned>& v);

                // Send a control message
                void sendControlMessage(APControl m, int argument = 0);

                // Send a control message to a specific node
                uint64_t sendControlMessageToNode(int nodeID,
                        APControl command, int argument = 0);

                // Receive a control message
                ControlMessage receiveControlMessage();

                // Send a message that the checkpoint has been reached
                uint64_t sendCheckPointMessage(int argument = 0);

                // Send a buffered message
                void sendBufferedMessage(int destID, char* msg, size_t size);

                // Receive a buffered sequence of messages
                void receiveBufferedMessage(MessagePtrVector<U, V>& outmessages);

                uint64_t reduceInflight()
                {
                    return reduce(m_txPackets - m_rxPackets);
                }

            private:
                uint64_t m_msgID;
                uint8_t* m_rxBuffer;
                MPI_Request m_request;

            protected:
                // Counters
                uint64_t m_rxPackets;
                uint64_t m_rxMessages;
                uint64_t m_rxBytes;
                uint64_t m_txPackets;
                uint64_t m_txMessages;
                uint64_t m_txBytes;
        };
    /*********************Implementation*********************/

    using namespace std;

    static const unsigned RX_BUFSIZE = 16*1024;

    template <typename U, typename V>
    CommLayer<U, V>::CommLayer()
        : m_msgID(0),
        m_rxBuffer(new uint8_t[RX_BUFSIZE]),
        m_request(MPI_REQUEST_NULL),
        m_rxPackets(0), m_rxMessages(0), m_rxBytes(0),
        m_txPackets(0), m_txMessages(0), m_txBytes(0)
    {
        assert(m_request == MPI_REQUEST_NULL);
        int dummy = sizeof(U); // < DEBUG
        MPI_Irecv(m_rxBuffer, RX_BUFSIZE,
                MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                &m_request);
    }

    template <typename U, typename V>
    CommLayer<U, V>::~CommLayer()
    {
        int dummy = sizeof(U); // < DEBUG 
        MPI_Cancel(&m_request);
        delete[] m_rxBuffer;
       // logger(1) << "Sent " << m_msgID << " control, "
       //     << m_txPackets << " packets, "
       //     << m_txMessages << " messages, "
       //     << m_txBytes << " bytes. "
       //     << "Received " << m_rxPackets << " packets, "
       //     << m_rxMessages << " messages, "
       //     << m_rxBytes << " bytes.\n";
    }

    /** Return the status of an MPI request.
     * Wraps MPI_Request_get_status.
     */
    static bool request_get_status(const MPI_Request& req,
            MPI_Status& status)
    {
        int flag;
        MPI_Request_get_status(req, &flag, &status);
        // Work around a bug present in Open MPI 1.3.3 and earlier.
        // MPI_Request_get_status may return false on the first call even
        // though a message is waiting. The second call should work.
        if (!flag)
            MPI_Request_get_status(req, &flag, &status);
        return flag;
    }

    /** Return the tag of the received message or APM_NONE if no message
     * has been received. If a message has been received, this call should
     * be followed by a call to either ReceiveControlMessage or
     * ReceiveBufferedMessage.
     */
    template <typename U, typename V>
    APMessage CommLayer<U, V>::checkMessage(int& sendID)
    {
        MPI_Status status;
        bool flag = request_get_status(m_request, status);
        if (flag)
            sendID = status.MPI_SOURCE;
        return flag ? (APMessage)status.MPI_TAG : APM_NONE;
    }

    /** Return true if no message has been received. */
    template <typename U, typename V>
    bool CommLayer<U, V>::receiveEmpty()
    {
        MPI_Status status;
        return !request_get_status(m_request, status);
    }

    /** Block until all processes have reached this routine. */
    template <typename U, typename V>
    void CommLayer<U, V>::barrier()
    {
        //logger(4) << "entering barrier\n";
        MPI_Barrier(MPI_COMM_WORLD);
        //logger(4) << "left barrier\n";
    }

    /** Broadcast a message. */
    template <typename U, typename V>
    void CommLayer<U, V>::broadcast(int message)
    {
        assert(opt::rank == 0);
        MPI_Bcast(&message, 1, MPI_INT, 0, MPI_COMM_WORLD);
        barrier();
    }

    /** Receive a broadcast message. */
    template <typename U, typename V>
    int CommLayer<U, V>::receiveBroadcast()
    {
        assert(opt::rank != 0);
        int message;
        MPI_Bcast(&message, 1, MPI_INT, 0, MPI_COMM_WORLD);
        barrier();
        return message;
    }

    /** Block until all processes have reached this routine.
     * @return the sum of count from all processors
     */
    template <typename U, typename V>
    long long unsigned CommLayer<U, V>::reduce(long long unsigned count)
    {
        //logger(4) << "entering reduce: " << count << '\n';
        long long unsigned sum;
        MPI_Allreduce(&count, &sum, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM,
                MPI_COMM_WORLD);
        //logger(4) << "left reduce: " << sum << '\n';
        return sum;
    }

    /** Reduce the specified vector. */
    template <typename U, typename V>
    vector<unsigned> CommLayer<U, V>::reduce(const vector<unsigned>& v)
    {
        //logger(4) << "entering reduce\n";
        vector<unsigned> sum(v.size());
        MPI_Allreduce(const_cast<unsigned*>(&v[0]),
                &sum[0], v.size(), MPI_UNSIGNED, MPI_SUM,
                MPI_COMM_WORLD);
        //logger(4) << "left reduce\n";
        return sum;
    }

    /** Reduce the specified vector. */
    template <typename U, typename V>
    vector<long unsigned> CommLayer<U, V>::reduce(
            const vector<long unsigned>& v)
    {
        //logger(4) << "entering reduce\n";
        vector<long unsigned> sum(v.size());
        MPI_Allreduce(const_cast<long unsigned*>(&v[0]),
                &sum[0], v.size(), MPI_UNSIGNED_LONG, MPI_SUM,
                MPI_COMM_WORLD);
        //logger(4) << "left reduce\n";
        return sum;
    }

    template <typename U, typename V>
    uint64_t CommLayer<U, V>::sendCheckPointMessage(int argument)
    {
        //logger(4) << "checkpoint: " << argument << '\n';
        assert(opt::rank != 0);
        ControlMessage msg;
        msg.id = m_msgID++;
        msg.msgType = APC_CHECKPOINT;
        msg.argument = argument;

        MPI_Send(&msg, sizeof msg,
                MPI_BYTE, 0, APM_CONTROL, MPI_COMM_WORLD);
        return msg.id;
    }

    /** Send a control message to all other processes. */
    template <typename U, typename V>
    void CommLayer<U, V>::sendControlMessage(APControl m, int argument)
    {
        for (int i = 0; i < opt::numProc; i++)
            if (i != opt::rank) // Don't send the message to myself.
                sendControlMessageToNode(i, m, argument);
    }

    /** Send a control message to a specific node. */
    template <typename U, typename V>
    uint64_t CommLayer<U, V>::sendControlMessageToNode(int nodeID,
            APControl m, int argument)
    {
        assert(opt::rank == 0);
        ControlMessage msg;
        msg.id = m_msgID++;
        msg.msgType = m;
        msg.argument = argument;

        MPI_Send(&msg, sizeof msg,
                MPI_BYTE, nodeID, APM_CONTROL, MPI_COMM_WORLD);
        return msg.id;
    }

    /** Receive a control message. */
    template <typename U, typename V>
    ControlMessage CommLayer<U, V>::receiveControlMessage()
    {
        int flag;
        MPI_Status status;
        MPI_Test(&m_request, &flag, &status);
        assert(flag);
        assert((APMessage)status.MPI_TAG == APM_CONTROL);

        int count;
        MPI_Get_count(&status, MPI_BYTE, &count);
        ControlMessage msg;
        assert(count == sizeof msg);
        memcpy(&msg, m_rxBuffer, sizeof msg);
        assert(m_request == MPI_REQUEST_NULL);
        MPI_Irecv(m_rxBuffer, RX_BUFSIZE,
                MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                &m_request);
        return msg;
    }

    /** Send a buffered collection of messages. */
    template <typename U, typename V>
    void CommLayer<U, V>::sendBufferedMessage(int destID,
            char* msg, size_t size)
    {
        int t = sizeof(U); // < DEBUG
        MPI_Send(msg, size, MPI_BYTE, destID, APM_BUFFERED,
                MPI_COMM_WORLD);
    }

    /** Receive a buffered message. */
    template <typename U, typename V>
    void CommLayer<U, V>::receiveBufferedMessage(MessagePtrVector<U, V>& outmessages)
    {
        assert(outmessages.empty());
        int flag;
        MPI_Status status;
        MPI_Test(&m_request, &flag, &status);
        assert(flag);
        assert((APMessage)status.MPI_TAG == APM_BUFFERED);

        int size;
        MPI_Get_count(&status, MPI_BYTE, &size);

        int offset = 0;
        while (offset < size) {
            MessageType type = Message<U, V>::readMessageType(
                    (char*)m_rxBuffer + offset);

            Message<U, V>* pNewMessage;
            switch(type)
            {
                case MT_ADD:
                    pNewMessage = new BinAddMessage<U, V>();
                    break;
                case MT_REMOVE:
                    pNewMessage = new BinRemoveMessage<U, V>();
                    break;
                case MT_SET_FLAG:
                    pNewMessage = new SetFlagMessage<U, V>();
                    break;
                case MT_BIN_DATA_REQUEST:
                    pNewMessage = new BinDataRequest<U, V>();
                    break;
                case MT_BIN_DATA_RESPONSE:
                    pNewMessage = new BinDataResponse<U, V>();
                    break;
                default:
                    assert(false);
                    break;
            }

            // Unserialize the new message from the buffer
            offset += pNewMessage->unserialize(
                    (char*)m_rxBuffer + offset);

            // Constructed message will be deleted in the
            // NetworkSequenceCollection calling function.
            outmessages.push_back(pNewMessage);
        }
        assert(offset == size);

        assert(m_request == MPI_REQUEST_NULL);
        MPI_Irecv(m_rxBuffer, RX_BUFSIZE,
                MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                &m_request);

        m_rxPackets++;
        m_rxMessages += outmessages.size();
        m_rxBytes += size;
    }
}// namespace CommLayer 
#include "MessageBuffer.h"
#endif
