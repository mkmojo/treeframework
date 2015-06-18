#include "MessageBuffer.hpp"
#include "Message.hpp"

template<typename T> class Messager {
protected:
    //messages stored in a list of queues
    MessageBuffer msgBuffer;

    inline bool checkpointReached() const{ return m_numReachedCheckpoint == (unsigned) opt::numProc; }

    virtual void parseControl(int source){
        //sl15: control message type
        ControlMessage controlMsg = msgBuffer.receiveControlMessage();
        switch(controlMsg.msgType){
            case APC_SET_STATE:
                SetState(NetworkActionState(controlMsg.argument));
                break;
            case APC_CHECKPOINT:
                m_numReachedCheckpoint++;
                m_checkpointSum += controlMsg.argument;
                break;
            case APC_WAIT:
                SetState(NAS_WAITING);
                msgBuffer.barrier();
                break;
            case APC_BARRIER:
                assert(m_state == NAS_WAITING);
                msgBuffer.barrier();
                break;
            //sl15: need default case
        }
    }

    virtual size_t pumpNetwork(){
        for( size_t count = 0; ;count++){
            int senderID;
            APMessage msg = msgBuffer.checkMessage(senderID);
            switch(msg){
                case APM_CONTROL:{
                    parseControlMessage(senderID);
                    //deal with control message before 
                    //any other type of message
                    return ++count;
                }
                case APM_BUFFERED:{
                    //Deal all message here until done
                    MessagePtrQueue msgs;
                    msgBuffer.receiveBufferedMessage(msgs);
                    while(!msgs.empty()){
                        Message* p = msgs.top();
                        msgs.pop();
                        p->handle(senderID, *this);
                    }
                    break;
                }
                case APM_NONE:
                    return count;
            }
        }
    }

    virtual void SetState(NetworkActionState newState){
        assert(msgBuffer.transmitBufferEmpty());
        m_state = newState;

        // Reset the checkpoint counter
        m_numReachedCheckpoint = 0;
        m_checkpointSum = 0;
    }


public:
    virtual void run(){
        SetState(NAS_LOADING);
        while(m_state != NAS_DONE){
            switch (m_state){
                case NAS_LOADING:{
                    loadPoints();
                    EndState();
                    SetState(NAS_WAITING);
                    msgBuffer.sendCheckPointMessage();
                    break;
                }
                case NAS_LOAD_COMPLETE:{
                    msgBuffer.barrier();
                    pumpNetwork();
                    SetState(NAS_SORT);
                    break;
                }
                case NAS_SORT:{
                    msgBuffer.barrier();
                    setUpGlobalMinMax();
                    setUpPointIds();
                    sortLocalPoints();
                    getLocalSample();
                    setGlobalPivot();
                    msgBuffer.barrier();
                    distributePoints();

                    SetState(NAS_DONE);
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

    virtual void runControl(){
        SetState(NAS_LOADING);
        while(m_state != NAS_DONE){
            switch(m_state){
                case NAS_LOADING:{
                    loadPoints();
                    EndState();
                    m_numReachedCheckpoint++;
                    while(!checkpointReached())
                        pumpNetwork();
                    //Load complete
                    SetState(NAS_LOAD_COMPLETE);
                    msgBuffer.sendControlMessage(APC_SET_STATE,
                            NAS_LOAD_COMPLETE);
                    msgBuffer.barrier();
                    //printPoints();

                    pumpNetwork();
                    SetState(NAS_SORT);
                    break;
                }
                case NAS_SORT:{
                    msgBuffer.sendControlMessage(APC_SET_STATE, NAS_SORT);
                    msgBuffer.barrier();
                    setUpGlobalMinMax();
                    setUpPointIds();
                    sortLocalPoints();
                    getLocalSample();
                    setGlobalPivot();
                    msgBuffer.barrier();
                    distributePoints();

                    SetState(NAS_DONE);
                    break;
                }
                case NAS_DONE:
                    break;
            }
        }
    }

    //sl15: this method needs to be overriden by the subclasses
    virtual int computeProcID() = 0;

    //sl15: this queue needs to be modified to use MessageBuffer instead
    //sl15: the way procID is computed should be attached to the user defined data structure
    virtual void addMessage(int Message<T>* newmessage){
        msgBuffer.queueMessage(computeProcID(), newmessage);
    }

    //sl15: this queue needs to be modified to use MessageBuffer instead
    void processMessages(){
        //pop the queue, and handle the message
        while(!messagequeue.empty()){
            Message<T>* pMessage = messagequeue.front();
            messagequeue.pop();
            pMessage->HandleMessage(this); //handle message should call te correct subclasses of Handler
            //cout << " Now the tree looks like: " << endl;
            //printData();
            //cout << endl;
        }
    }
 
};
