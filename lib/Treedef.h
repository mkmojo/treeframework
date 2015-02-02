#ifndef TREEDEF_H
#define TREEDEF_H 1
#include "Log.h"
#include "MessageBuffer.h"

using namespace std;

enum NetworkActionState
{
    NAS_LOADING, // loading points
    NAS_LOAD_COMPLETE, // loading is complete
    NAS_WAITING, // non-control process is waiting
    NAS_DONE // finished, clean up and exit
};


/* -------------------------- BaseTree --------------------------- */
class BaseTree
{
    public:
        virtual ~BaseTree() {}
        virtual void add() = 0;
        virtual size_t pumpNetwork() = 0;
};


/* -------------------------- OctTree ---------------------------- */
class LocalOctTree : public LocalBaseTree
{
    public:
        LocalOctTree()
            :m_state(NAS_WAITING){};
        void runControl();
        void run();
        void SetState(NetworkActionState newState);
        void EndState();
        
        //Recieve and dispatch packets
        size_t pumpNetwork();
    private:
        NetworkActionState m_state;
        MessageBuffer m_comm;
        unsigned m_numReachedCheckpoint;

        unsigned m_checkpointSum;
}

void LocalOctTree::parseControlMessage(int source)
{
    ControlMessage controlMsg = m_comm.receiveControlMessage();
    switch(controlMsg.msgType)
    {
        case APC_SET_STATE:
            SetState(NetworkActionState(controlMsg.argument));
            break;
        case APC_CHECKPOINT:
            logger(4) << "checkpoint from " << source << ": "
                << controlMsg.argument << '\n';
            m_numReachedCheckpoint++;
            m_checkpointSum += controlMsg.argument;
            break;
        case APC_WAIT:
            SetState(NAS_WAITING);
            m_comm.barrier();
            break;
        case APC_BARRIER:
            assert(m_state == NAS_WAITING);
            m_comm.barrier();
            break;
    }
}


size_t LocalOctTree::pumpNetwork()
{
    for( size_t count = 0; ;count++){
        int senderID;
        APMessage msg = m_comm.checkMessage(senderID);
        switch(msg)
        {
            case APM_CONTROL:
                {
                    parseControlMessage(senderID);
                    //deal with control message before 
                    //any other type of message
                    return ++count;
                }
            case APM_BUFFERED:
                {
                    //Deal all message here until done
                    MessagePtrVector msgs;
                    m_comm.receiveBufferedMessage(msgs);
                    for(MessagePtrVector::iterator 
                            iter = msgs.begin();
                            iter != msgs.end(); iter++){
                        //handle message based on its type
                        (*iter)->handle(senderID, *this);
                        delete (*iter);
                        *iter = 0;
                    }
                    break;
                }
            case APM_NONE:
                return count;
        }
    }
}



void LocalOctTree::EndState()
{
    m_comm.flush();
}

//
// Set the action state 
//
void LocalOctTree::SetState(
        NetworkActionState newState)
{
    logger(2) << "SetState " << newState
        << " (was " << m_state << ")\n";

    // Ensure there are no pending messages
    assert(m_comm.transmitBufferEmpty());

    m_state = newState;

    // Reset the checkpoint counter
    m_numReachedCheckpoint = 0;
    m_checkpointSum = 0;
}

//
// Run master process state machine
//
void LocalOctTree::runControl()
{
    SetState(NAS_LOADING);
    while(m_state != NAS_DONE){
        swicth(m_state){
            case NAS_LOADING:
                {
                    loadPoints();
                    EndState();

                    m_numReachedCheckpoint++;
                    while(!checkpointReached())
                        pumpNetwork();

                    SetState(NAS_LOAD_COMPLETE);
                    m_comm.sendControlMessage(APC_SET_STATE,
                            NAS_LOAD_COMPLETE);

                    m_comm.barrier();
                    pumpNetwork();

                    SetState(NAS_DONE);
                    break;
                }
            case NAS_DONE:
                break;
        }
    }
}

//
// Run worker state machine
//
void LocalOctTree::run()
{
    SetState(NAS_LOADING);
    while(m_state != NAS_DONE){
        switch (m_state){
            case NAS_LOADING:
                {
                    loadPoints();
                    EndState();
                    SetState(NAS_WAITING);
                    m_comm.sendCheckPointMessage();
                    break;
                }
            case NAS_LOAD_COMPLETE:
                {
                    m_comm.barrier();
                    pumpNetwork();
                    SetState(NAS_WAITING);
                    break;
                }
            case NAS_WAITING:
                pumpNetwork();
                break;
            case NAS_DONE:
                break;
        }
    }
}

LocalOctTree::flush()
{
    m_comm.flush();
}



#endif
