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
                    MessagePtrVector msgs;
                    msgBuffer.receiveBufferedMessage(msgs);
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

    virtual void SetState(NetworkActionState newState){
        assert(msgBuffer.transmitBufferEmpty());
        m_state = newState;

        // Reset the checkpoint counter
        m_numReachedCheckpoint = 0;
        m_checkpointSum = 0;
    }


public:
    virtual void run(){ 
    }

    virtual void runControl(){
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
