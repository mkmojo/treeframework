#include "MessageBuffer.hpp"

template<typename T> class Messager {
protected:
    //messages stored in a list of queues
    MessageBuffer<T> msgBuffer;
    
    unsigned numReachedCheckpoint, checkpointSum; 
    NetworkActionState state;

    inline bool _checkpointReached() const{ return numReachedCheckpoint == numProc; }

    virtual void _parseControlMessage(int source){
        ControlMessage controlMsg = msgBuffer.msgBufferLayer.receiveControlMessage();
        switch(controlMsg.msgType){
            case APC_SET_STATE:
                _setState(NetworkActionState(controlMsg.argument));
                break;
            case APC_CHECKPOINT:
                numReachedCheckpoint++;
                checkpointSum += controlMsg.argument;
                break;
            case APC_WAIT:
                _setState(NAS_WAITING);
                msgBuffer.msgBufferLayer.barrier();
                break;
            case APC_BARRIER:
                assert(state == NAS_WAITING);
                msgBuffer.msgBufferLayer.barrier();
                break;
            //sl15: need default case
        }
    }

    virtual size_t _pumpNetwork(){
        for( size_t count = 0; ;count++){
            int senderID;
            APMessage msg = msgBuffer.msgBufferLayer.checkMessage(senderID);
            switch(msg){
                case APM_CONTROL:{
                    _parseControlMessage(senderID);
                    //deal with control message before 
                    //any other type of message
                    return ++count;
                }
                case APM_BUFFERED:{
                    //Deal all message here until done
                    std::vector<Message<T>*> msgs;
                    msgBuffer.msgBufferLayer.receiveBufferedMessage(msgs);
                    for(auto iter = msgs.begin(); iter != msgs.end(); iter++){
                        //handle message based on its type
                        (*iter)->HandleMessage(senderID, this);
                        delete (*iter); //sl15: bad coding. this is trying to use vector as a deque. changing to use a queue should solve this
                        *iter = 0;
                    }
                    break;
                }
                case APM_NONE:
                    return count;
            }
        }
    }

    virtual void _setState(NetworkActionState newState){
        assert(msgBuffer.transmitBufferEmpty());
        state = newState;

        // Reset the checkpoint counter
        numReachedCheckpoint = 0;
        checkpointSum = 0;
    }

    inline virtual void _endState(){ msgBuffer.flush(); }


public:
    Messager() : numReachedCheckpoint(0), checkpointSum(0), state(NAS_WAITING){ };

    virtual void run(){ 
        _setState(NAS_LOADING);
        while(state != NAS_DONE){
            switch (state){ 
                //sl15: the calls commented out in this block needs a different interface
                case NAS_LOADING:
                    //loadPoints();
                    _endState();
                    _setState(NAS_WAITING);
                    msgBuffer.sendCheckPointMessage();
                    break;
                case NAS_LOAD_COMPLETE:
                    msgBuffer.msgBufferLayer.barrier();
                    _pumpNetwork();
                    _setState(NAS_SORT);
                    break;
                case NAS_SORT:
                    msgBuffer.msgBufferLayer.barrier();
                    //setUpGlobalMinMax(); //sl15
                    //setUpPointIds(); //sl15
                    //sortLocalPoints();
                    //getLocalSample();
                    //setGlobalPivot();
                    msgBuffer.msgBufferLayer.barrier();
                    //distributePoints();
                    _setState(NAS_DONE);
                    break;
                case NAS_WAITING:
                    _pumpNetwork();
                    break;
                case NAS_DONE:
                    break;
            }
        }

    }

    virtual void runControl(){
        _setState(NAS_LOADING);
        while(state != NAS_DONE){
            switch(state){
                //sl15: the calls commented out in this block needs a different interface
                case NAS_LOADING:
                    //loadPoints();
                    _endState();
                    numReachedCheckpoint++;

                    while(!_checkpointReached())
                        _pumpNetwork();

                    //Load complete
                    _setState(NAS_LOAD_COMPLETE);
                    msgBuffer.sendControlMessage(APC_SET_STATE, NAS_LOAD_COMPLETE);
                    msgBuffer.msgBufferLayer.barrier();
                    //printPoints();
                    _pumpNetwork();
                    _setState(NAS_SORT);
                    break;
                case NAS_SORT:
                    msgBuffer.sendControlMessage(APC_SET_STATE, NAS_SORT);
                    msgBuffer.msgBufferLayer.barrier();
                    //setUpGlobalMinMax();
                    //setUpPointIds();
                    //sortLocalPoints();
                    //getLocalSample();
                    //setGlobalPivot();
                    msgBuffer.msgBufferLayer.barrier();
                    //distributePoints();
                    _setState(NAS_DONE);
                    break;
                case NAS_DONE:
                    break;
            }
        }
    }

    //sl15: this method needs to be overriden by the subclasses
    //virtual int computeProcID() = 0;

    //sl15: the way procID is computed should be attached to the user defined data structure
    virtual void addMessage(Message<T>* newmessage){
        //msgBuffer.queueMessage(computeProcID(), newmessage);
        msgBuffer.queueMessage(0, newmessage);
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
