#include "MessageBuffer.hpp"

template<typename T> class Messager {
protected:
    //messages stored in a list of queues
    MessageBuffer<T> msgBuffer;
    std::vector<T> localBuffer;
    
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

    //sl15: currently the return value is not used, but might be later.
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
                    std::queue<Message<T>*> msgs;
                    msgBuffer.msgBufferLayer.receiveBufferedMessage(msgs);
                    while(!msgs.empty()){
                        Message<T>* p = msgs.front();
                        msgs.pop();
                        //sl15: need to assert that the point is local
                        localBuffer.push_back(p->Data); //push into the local buffer
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

    virtual void _sort() = 0;

    virtual void _load() = 0;

public:
    Messager() : numReachedCheckpoint(0), checkpointSum(0), state(NAS_WAITING){ };

    void add(const T& data){
        int data_id = computeProcID(data); //sl15: computeProcID should be implemented by the user
        if(data_id == procRank) localBuffer.push_back(data);
        else msgBuffer.addMessage(data_id, data);
    }

    virtual void run(){ 
        _setState(NAS_LOADING);
        while(state != NAS_DONE){
            switch (state){ 
                //sl15: the calls commented out in this block needs a different interface
                case NAS_LOADING:
                    _endState();
                    _setState(NAS_WAITING);
                    msgBuffer.sendCheckPointMessage();
                    break;
                case NAS_LOAD_COMPLETE:
                    msgBuffer.msgBufferLayer.barrier();
                    _pumpNetwork(); //sl15: double check, this may not be needed
                    _setState(NAS_SORT);
                    break;
                case NAS_SORT:
                    msgBuffer.msgBufferLayer.barrier();
                    _sort(); //sl15: should be overriden by the subclasses
                    //setUpGlobalMinMax(); //sl15: should be in subclass _sort() such as octtree.hpp etc
                    //setUpPointIds(); //sl15: should be in subclass _sort() such as octtree.hpp
                    //sortLocalPoints(); //sl15: should bring back
                    //getLocalSample(); //sl15: should bring back
                    //setGlobalPivot(); //sl15: should bring back
                    //distributePoints(); //sl15: should bring back
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
                    _load();
                    _endState();
                    numReachedCheckpoint++;

                    while(!_checkpointReached()) _pumpNetwork();

                    //Load complete
                    _setState(NAS_LOAD_COMPLETE);
                    msgBuffer.sendControlMessage(APC_SET_STATE, NAS_LOAD_COMPLETE);
                    msgBuffer.msgBufferLayer.barrier();

                    //printPoints();
                    _pumpNetwork(); //sl15: this might not be necessary
                    _setState(NAS_SORT);
                    break;
                case NAS_SORT:
                    msgBuffer.sendControlMessage(APC_SET_STATE, NAS_SORT);
                    msgBuffer.msgBufferLayer.barrier();
                    _sort();
                    //setUpGlobalMinMax();
                    //setUpPointIds();
                    //sortLocalPoints();
                    //getLocalSample();
                    //setGlobalPivot();
                    //distributePoints();
                    _setState(NAS_DONE);
                    break;
                case NAS_DONE:
                    break;
            }
        }
    }

    //sl15: this method needs to be overriden by the subclasses
    virtual int computeProcID() {
        return 0;
    }
};
